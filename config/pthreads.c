/*
 * pthreads.c -- Icon context switch code using POSIX threads and semaphores
 *
 * This code implements co-expression context switching on any system that
 * provides POSIX threads and semaphores.  It requires Icon 9.4.1 or later
 * built with "#define CoClean" in order to free threads and semaphores when
 * co-expressions are collected.  It is typically much slower when called
 * than platform-specific custom code, but of course it is much more portable,
 * and it is typically used infrequently.
 *
 * Unnamed semaphores are used unless NamedSemaphores is defined.
 * (This is for Mac OS 10.3 which does not have unnamed semaphores.)
 */

#include <fcntl.h>
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

static int inited = 0;		/* has first-time initialization been done? */

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
      if (pthread_create(&new->thread, NULL, nctramp, new) != 0) 
         syserr("cannot create thread");
      new->alive = 1;
      }

   sem_post(new->semp);			/* unblock the new thread */
   sem_wait(old->semp);			/* block this thread */

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
         syserr("cannot create semaphore");
      sem_unlink(name);
   #else				/* NamedSemaphores */
      if (sem_init(&ctx->sema, 0, 0) == -1)
         syserr("cannot init semaphore");
      ctx->semp = &ctx->sema;
   #endif				/* NamedSemaphores */
   }

/*
 * nctramp() -- trampoline for calling new_context(0,0).
 */
static void *nctramp(void *arg) {
   context *new = arg;			/* new context pointer */
   sem_wait(new->semp);			/* wait for signal */
   new_context(0, 0);			/* call new_context; will not return */
   syserr("new_context returned to nctramp");
   return NULL;
   }
