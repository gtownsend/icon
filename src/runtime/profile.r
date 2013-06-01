/*
 * File: profile.r
 *  Contents: initprofile, countline, genprofile
 *
 *  Code associated with execution profiling.
 */

int profiling_active = 0;	/* global flag checked in interp() */

#define PROFBINS 7993		/* number of hash bins to use for profiling */
#define INTERVAL 1000		/* desired profiling interval in microseconds */

struct profbin {		/* struct recording profiling info */
   word p_ticks;		/* number of interrupts seen */
   word p_visits;		/* number of times reached */
   union {
      word *pu_ipc;		/* icode PC value */
      word pu_lnum;		/* later reused to hold line number */
      } pu_loc;
   union {
      struct profbin *pu_next;	/* pointer to next entry in hash chain */
      char * pu_fname;		/* later reused to hold file name */
      } pu_ptr;
   };

#define p_ipc pu_loc.pu_ipc	/* p_ipc:   field used as IPC */
#define p_next pu_ptr.pu_next	/* p_next:  field used as hash bin link */
#define p_lnum pu_loc.pu_lnum	/* p_lnum:  field used as line number */
#define p_fname pu_ptr.pu_fname	/* p_fname: field used as file name */

static struct profbin **pbin;	/* hash bins */
static struct profbin *latest;	/* most recent line seen */
static FILE *outfile;		/* profiling output file */

static void tick(int v);
static int ipccmp(const void *v1, const void *v2);
static int linecmp(const void *v1, const void *v2);

/*
 * initprofile - initialize profiling if enabled by environment variable
 */
void initprofile()
   {
   struct itimerval tv;
   struct sigaction sg;

   char *fname = getenv("ICONPROFILE");	/* get output file name */
   if (fname == NULL || fname[0] == '\0') /* if not profiling, just return */
      return;
   fprintf(stderr, "[profiling to $ICONPROFILE = %s]\n", fname);
   fflush(stderr);

   outfile = fopen(fname, "w");		/* open output now, to catch any errs */
   if (!outfile)
      error(fname, "cannot open for profiler output");

   /*
    * Everything looks good.  Allocate hash table.
    */
   pbin = alloc(PROFBINS * sizeof(struct profbin *));
   memset(pbin, 0, PROFBINS * sizeof(struct profbin *));

   /*
    * Arrange for periodic interrupts to call tick() for time-based profiling.
    */
   memset(&sg, 0, sizeof(struct sigaction));
   sg.sa_handler = tick;
   tv.it_interval.tv_sec = tv.it_value.tv_sec = 0;
   tv.it_interval.tv_usec = tv.it_value.tv_usec = INTERVAL;
   if (sigaction(SIGVTALRM, &sg, NULL) < 0
   || setitimer(ITIMER_VIRTUAL, &tv, NULL) < 0)
      perror("no timer");

   profiling_active = 1;
   }

/*
 * tick - handle timer interrupt
 */
static void tick(int signum)
   {
   if (latest != NULL)
      latest->p_ticks++;		/* credit to most recently seen IPC */
   }

/*
 * countline - note arrival at line for profiling purposes
 */
void countline(word *ipc)
   {
   int b = ((unsigned long)ipc) % PROFBINS;
   struct profbin *p;

   /*
    * Search the hash table for an existing entry.
    */
   for (p = pbin[b]; p != NULL; p = p->p_next) {
      if (p->p_ipc == ipc) {		/* found a match */
         p->p_visits++;			/* count the visit */
         latest = p;			/* note for use in next timer tick */
         return;
         }
      }

   /*
    * Create a new entry the first time a particular IPC is seen.
    */
   p = alloc(sizeof(struct profbin));	/* allocate entry */
   p->p_ipc = ipc;			/* save IPC value */
   p->p_ticks = 0;			/* initialize tick count to 0 */
   p->p_visits = 1;			/* count this first visit */
   p->p_next = pbin[b];			/* link to previous hash bin entry */
   pbin[b] = latest = p;		/* put this at head of hash chain */
   return;
   }

/*
 * genprofile - report profile information
 */
void genprofile(void)
   {
   struct profbin *p, *q, **a, **results;
   int i, j, n;

   if (! profiling_active)		/* don't do anything if not profiling */
      return;

   latest = NULL;			/* don't accumulate any more ticks */

   /*
    * Count the number of distinct locations (IPC values) seen.
    */
   for (i = n = 0; i < PROFBINS; i++)
      for (p = pbin[i]; p != NULL; p = p->p_next)
         n++;

   /*
    * Make a linear list of pointers to the result structs.
    */
   results = alloc(n * sizeof(struct profbin *));
   for (i = 0, a = results; i < PROFBINS; i++)
      for (p = pbin[i]; p != NULL; p = p->p_next)
         *a++ = p;

   /*
    * First sort them by IPC to put everything in linkage order.
    */
   qsort(results, n, sizeof(struct profbin *), ipccmp);

   /*
    * Convert IPC values to file and line number, overwriting IPCs and links.
    *  Within single-file runs, sort again by line number.
    */
   for (i = j = 0; i < n; i++) {
      p = results[i];
      p->p_fname = findfile(p->p_ipc);	/* convert filename first because... */
      p->p_lnum = findline(p->p_ipc);	/* converted line number clobbers IPC */
      if (i > 0 && p->p_fname != results[i - 1]->p_fname) {
         /*
          * This is the start of a new source file.
          *  Sort the previous run by IPC.
          */
         qsort(results + j, i - j, sizeof(struct profbin *), linecmp);
         j = i;
         }
      }
   /*
    * Sort the results from the last file.
    */
   qsort(results + j, i - j, sizeof(struct profbin *), linecmp);

   /*
    * Combine multiple reports covering the same line:
    *  add the timer ticks
    *  use the largest of the visit counts
    */
   for (i = 1; i < n; i++) {
      p = results[i - 1];
      q = results[i];
      if (p->p_lnum == q->p_lnum && p->p_fname == q->p_fname) {
         q->p_ticks += p->p_ticks;	/* add ticks */
         if (q->p_visits < p->p_visits)	/* use larger of visit counts */
            q->p_visits = p->p_visits;
         p->p_visits = 0;		/* mark earlier entry for skipping */
         }
      }

   /*
    * Report the results now that everything has been sorted and cleaned.
    */
   for (i = 0, a = results; i < n; i++) {
      p = *a++;
      if (p->p_visits > 0)		/* if not eliminated as a duplicate */
         fprintf(outfile, "%5ld %10ld  %s %ld\n", (long)p->p_ticks,
            (long)p->p_visits, p->p_fname, (long)p->p_lnum);
      }
   }

/*
 * ipccmp - compare profiling buckets for sorting by IPC.
 */
static int ipccmp(const void *v1, const void *v2)
   {
   struct profbin *p1 = *(struct profbin **)v1;
   struct profbin *p2 = *(struct profbin **)v2;
   return p1->p_ipc - p2->p_ipc;
   }

/*
 * linecmp - compare profiling buckets for sorting by line number.
 */
static int linecmp(const void *v1, const void *v2)
   {
   struct profbin *p1 = *(struct profbin **)v1;
   struct profbin *p2 = *(struct profbin **)v2;
   return p1->p_lnum - p2->p_lnum;
   }
