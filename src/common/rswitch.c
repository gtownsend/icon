/*
 * rswitch.c -- context switch code using POSIX threads and semaphores
 *
 * This code implements co-expression context switching on any system that
 * provides POSIX threads and semaphores.
 *
 * Anonymous semaphores are used unless NamedSemaphores is defined.
 * (This is for MacOS which does not have anonymous semaphores.)
 */

#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <pthread.h>
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>

#include "../h/define.h"

extern void new_context(int, void *);
extern void syserr(char *msg);
extern void *alloc(unsigned int n);

extern long stksize;		/* value of COEXPSIZE */

static int inited = 0;		/* has first-time initialization been done? */
static pthread_attr_t attribs;	/* thread creation attributes */

/*
 * Define a "context" struct to hold the thread information we need.
 */
typedef struct {
   pthread_t thread;	/* thread ID (thread handle) */
   sem_t sema;		/* synchronization semaphore (if unnamed) */
   sem_t *semp;		/* pointer to semaphore */
   int alive;		/* set zero when thread is to die */
   } context;

static void makesem(context *ctx);
static void *nctramp(void *arg);
static void uerror(char *msg);

/*
 * Treat an Icon "cstate" array as an array of context pointers.
 * cstate[0] is used by Icon code that thinks it's setting a stack pointer.
 * We use cstate[1] to point to the actual context struct.
 * (Both of these are initialized to NULL by Icon 9.4.1 or later.)
 */
typedef context **cstate;

/*
 * coswitch(old, new, first) -- switch contexts.
 */
int coswitch(void *o, void *n, int first) {

   cstate ocs = o;			/* old cstate pointer */
   cstate ncs = n;			/* new cstate pointer */
   context *old, *new;			/* old and new context pointers */
   size_t newsize;			/* stack size for new thread */
   size_t pagesize;			/* system page size */

   if (inited)				/* if not first call */
      old = ocs[1];			/* load current context pointer */
   else {
      /*
       * This is the first coswitch() call.
       * Allocate and initialize the context struct for &main.
       */
      old = ocs[1] = alloc(sizeof(context));
      makesem(old);
      old->thread = pthread_self();
      old->alive = 1;

      /*
       * Set up thread attributes to honor COEXPSIZE for setting stack size.
       */
      pagesize = sysconf(_SC_PAGESIZE);
      newsize = stksize;
      #ifdef PTHREAD_STACK_MIN
         if (newsize < PTHREAD_STACK_MIN)   /* ensure system minimum is met */
            newsize = PTHREAD_STACK_MIN;
      #endif
      if (pagesize > 0 && (newsize % pagesize) != 0) {
         /* some systems require an exact multiple of the system page size */
         newsize = newsize + pagesize - (newsize % pagesize);
      }
      pthread_attr_init(&attribs);
      if (pthread_attr_setstacksize(&attribs, newsize) != 0) {
         uerror("cannot set stacksize for thread");
      }

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
      makesem(new);
      if (pthread_create(&new->thread, &attribs, nctramp, new) != 0)
         uerror("cannot create thread");
      new->alive = 1;
      }

   sem_post(new->semp);			/* unblock the new thread */
   while (sem_wait(old->semp) < 0)	/* block this thread */
      if (errno != EINTR)
         uerror("sem_wait in coswitch");

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
   sem_post(old->semp);			/* unblock it */
   pthread_join(old->thread, NULL);	/* wait for thread to exit */
   #ifdef NamedSemaphores
      sem_close(old->semp);		/* close associated semaphore */
   #else
      sem_destroy(old->semp);		/* destroy associated semaphore */
   #endif
   free(old);				/* free context block */
   }

/*
 * makesem(ctx) -- initialize semaphore in context struct.
 */
static void makesem(context *ctx) {
   #ifdef NamedSemaphores		/* if cannot use unnamed semaphores */
      char name[50];
      sprintf(name, "i%ld.sem", (long)getpid());
      ctx->semp = sem_open(name, O_CREAT, S_IRUSR | S_IWUSR, 0);
      if (ctx->semp == (sem_t *)SEM_FAILED)
         uerror("cannot create semaphore");
      sem_unlink(name);
   #else				/* NamedSemaphores */
      if (sem_init(&ctx->sema, 0, 0) == -1)
         uerror("cannot init semaphore");
      ctx->semp = &ctx->sema;
   #endif				/* NamedSemaphores */
   }

/*
 * nctramp() -- trampoline for calling new_context(0,0).
 */
static void *nctramp(void *arg) {
   context *new = arg;			/* new context pointer */
   while (sem_wait(new->semp) < 0)	/* wait for signal */
      if (errno != EINTR)
         uerror("sem_wait in nctramp");
   new_context(0, 0);			/* call new_context; will not return */
   syserr("new_context returned to nctramp");
   return NULL;
   }

/*
 * uerror(s) -- abort due to Unix error.
 */
static void uerror(char *msg) {
   perror(msg);
   syserr(NULL);
   }
