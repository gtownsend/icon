/*
 * pthreads.c -- Icon context switch code using POSIX threads and semaphores
 *
 * This code implements co-expression context switching on any system that
 * provides POSIX threads and semaphores.  It has these drawbacks compared
 * to architecture-specific code:
 *
 *    It is typically much slower.
 *
 *    It requires Icon 9.4.1 built with "#define CoClean" in order to free
 *    threads and semaphores when co-expressions ARE collected.
 *
 *    The C stack is created at the system default size, ignoring any
 *    COEXPSIZE environment variable.
 */

#include <pthread.h>
#include <semaphore.h>
#include <stdio.h>

extern void new_context(int, void *);
extern void syserr(char *msg);
extern void *alloc(unsigned int n);
static void *nctramp(void *arg);

static int inited = 0;		/* has first-time initialization been done? */

/*
 * Define a "context" struct to hold the thread information we need.
 */
typedef struct {
   pthread_t thread;	/* thread ID (thread handle) */
   sem_t sema;		/* synchronization semaphore */
   int alive;		/* set zero when thread is to die */
   } context;

/*
 * Treat an Icon "cstate" array as an array of context pointers.
 * cstate[0] is used by Icon code that thinks it's setting a stack pointer.
 * We use cstate[1] to point to the actual context struct.
 * (Both of these are initialized to NULL by Icon 9.4.1.)
 */
typedef context **cstate;

/*
 * coswitch(old, new, first) -- switch contexts.
 */
int coswitch(void *o, void *n, int first) {

   cstate ocs = o;			/* old cstate pointer */
   cstate ncs = n;			/* new cstate pointer */
   context *old, *new;			/* old and new context pointers */

   if (inited)				/* if not first call */
      old = ocs[1];			/* load current context pointer */
   else {
      /*
       * This is the first coswitch() call.
       * Allocate and initialize the context struct for &main.
       */
      old = ocs[1] = alloc(sizeof(context));
      if (sem_init(&old->sema, 0, 0) == -1)
         syserr("cannot init semaphore");
      old->thread = pthread_self();
      old->alive = 1;
      inited = 1;
      }

   if (first != 0)			/* if not first call for this cstate */
      new = ncs[1];			/* load new context pointer */
   else {
      /*
       * This is a newly allocated cstate array.
       * Allocate and initialize a context struct.
       */
      new = ncs[1] = alloc(sizeof(context));
      if (sem_init(&new->sema, 0, 0) == -1)
         syserr("cannot init semaphore");
      if (pthread_create(&new->thread, NULL, nctramp, new) != 0) 
         syserr("cannot create thread");
      new->alive = 1;
      }

   sem_post(&new->sema);		/* unblock the new thread */
   sem_wait(&old->sema);		/* block this thread */

   if (!old->alive)		
      pthread_exit(NULL);		/* if unblocked because unwanted */
   return 0;				/* else return to continue running */
   }

/*
 * coclean(old) -- clean up co-expression state before freeing.
 */
void coclean(void *o) {
   cstate ocs = o;			/* old cstate pointer */
   context *old = ocs[1];		/* old context pointer */
   if (old == NULL)			/* if never initialized, do nothing */
      return;
   old->alive = 0;			/* signal thread to exit */
   sem_post(&old->sema);		/* unblock it */
   pthread_join(old->thread, NULL);	/* wait for thread to exit */
   sem_destroy(&old->sema);		/* destroy associated semaphore */
   free(old);				/* free context block */
   }

/*
 * nctramp() -- trampoline for calling new_context(0,0).
 */
static void *nctramp(void *arg) {
   context *new = arg;			/* new context pointer */
   sem_wait(&new->sema);		/* wait for signal */
   new_context(0, 0);			/* call new_context; will not return */
   syserr("new_context returned to nctramp");
   return NULL;
   }
