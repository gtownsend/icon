/*
 * lmem.c -- memory initialization and allocation; also parses arguments.
 */

#include "link.h"
#include "tproto.h"
#include "tglobals.h"

/*
 * Prototypes.
 */

static struct	lfile *alclfile	(char *name);

void dumplfiles(void);

/*
 * Memory initialization
 */

struct gentry **lghash;		/* hash area for global table */
struct ientry **lihash;		/* hash area for identifier table */
struct fentry **lfhash;		/* hash area for field table */

struct lentry *lltable;		/* local table */
struct centry *lctable;		/* constant table */
struct ipc_fname *fnmtbl;	/* table associating ipc with file name */
struct ipc_line *lntable;	/* table associating ipc with line number */

char *lsspace;			/* string space */
word *labels;			/* label table */
char *codeb;			/* generated code space */

struct ipc_fname *fnmfree;	/* free pointer for ipc/file name table */
struct ipc_line *lnfree;	/* free pointer for ipc/line number table */
word lsfree;			/* free index for string space */
char *codep;			/* free pointer for code space */

struct fentry *lffirst;		/* first field table entry */
struct fentry *lflast;		/* last field table entry */
struct gentry *lgfirst;		/* first global table entry */
struct gentry *lglast;		/* last global table entry */

/*
 * linit - scan the command line arguments and initialize data structures.
 */
void linit()
   {
   struct gentry **gp;
   struct ientry **ip;
   struct fentry **fp;

   llfiles = NULL;		/* Zero queue of files to link. */

   /*
    * Allocate the various data structures that are used by the linker.
    */
   lghash   = (struct gentry **) tcalloc(ghsize, sizeof(struct gentry *));
   lihash   = (struct ientry **) tcalloc(ihsize, sizeof(struct ientry *));
   lfhash   = (struct fentry **) tcalloc(fhsize, sizeof(struct fentry *));

   lltable  = (struct lentry *) tcalloc(lsize, sizeof(struct lentry));
   lctable  = (struct centry *) tcalloc(csize, sizeof(struct centry));

   lnfree = lntable  = (struct ipc_line*)tcalloc(nsize,sizeof(struct ipc_line));

   lsspace = (char *) tcalloc(stsize, sizeof(char));
   lsfree = 0;

   fnmtbl = (struct ipc_fname *) tcalloc(fnmsize, sizeof(struct ipc_fname));
   fnmfree = fnmtbl;

   labels  = (word *) tcalloc(maxlabels, sizeof(word));
   codep = codeb = (char *) tcalloc(maxcode, 1);

   lffirst = NULL;
   lflast = NULL;
   lgfirst = NULL;
   lglast = NULL;

   /*
    * Zero out the hash tables.
    */
   for (gp = lghash; gp < &lghash[ghsize]; gp++)
      *gp = NULL;
   for (ip = lihash; ip < &lihash[ihsize]; ip++)
      *ip = NULL;
   for (fp = lfhash; fp < &lfhash[fhsize]; fp++)
      *fp = NULL;

   /*
    * Install "main" as a global variable in order to insure that it
    *  is the first global variable.  iconx/start.s depends on main
    *  being global number 0.
    */
   putglobal(instid("main"), F_Global, 0, 0);
   }

#ifdef DeBugLinker
   /*
    * dumplfiles - print the list of files to link.  Used for debugging only.
    */
   void dumplfiles()
      {
      struct lfile *p,*lfls;

      fprintf(stderr,"lfiles:\n");
      lfls = llfiles;
      while (p = getlfile(&lfls))
          fprintf(stderr,"'%s'\n",p->lf_name);
      fflush(stderr);
      }
#endif					/* DeBugLinker */

/*
 * alsolink - create an lfile structure for the named file and add it to the
 *  end of the list of files (llfiles) to generate link instructions for.
 */
void alsolink(name)
char *name;
   {
   struct lfile *nlf, *p;
   char file[MaxPath];

   if (!pathfind(file, ipath, name, U1Suffix))
     quitf("cannot resolve reference to file '%s'",name);

   nlf = alclfile(file);
   if (llfiles == NULL) {
      llfiles = nlf;
      }
   else {
      p = llfiles;
      while (p->lf_link != NULL) {
        if (strcmp(p->lf_name,file) == 0)
           return;
        p = p->lf_link;
        }
      if (strcmp(p->lf_name,file) == 0)
        return;
      p->lf_link = nlf;
      }
   }

/*
 * getlfile - return a pointer (p) to the lfile structure pointed at by lptr
 *  and move lptr to the lfile structure that p points at.  That is, getlfile
 *  returns a pointer to the current (wrt. lptr) lfile and advances lptr.
 */
struct lfile *getlfile(lptr)
struct lfile **lptr;
   {
   struct lfile *p;

   if (*lptr == NULL)
      return (struct lfile *)NULL;
   else {
      p = *lptr;
      *lptr = p->lf_link;
      return p;
      }
   }

/*
 * alclfile - allocate an lfile structure for the named file, fill
 *  in the name and return a pointer to it.
 */
static struct lfile *alclfile(name)
char *name;
   {
   struct lfile *p;

   p = (struct lfile *) alloc(sizeof(struct lfile));
   p->lf_link = NULL;
   p->lf_name = salloc(name);
   return p;
   }

/*
 * lmfree - free memory used by the linker
 */
void lmfree()
   {
   struct fentry *fp, *fp1;
   struct gentry *gp, *gp1;
   struct rentry *rp, *rp1;
   struct ientry *ip, *ip1;
   int i;

   for (i = 0; i < ihsize; ++i)
      for (ip = lihash[i]; ip != NULL; ip = ip1) {
           ip1 = ip->i_blink;
           free((char *)ip);
           }

   free((char *) lghash);   lghash = NULL;
   free((char *) lihash);   lihash = NULL;
   free((char *) lfhash);   lfhash = NULL;
   free((char *) lltable);   lltable = NULL;
   free((char *) lctable);   lctable = NULL;
   free((char *) lntable);   lntable = NULL;
   free((char *) lsspace);   lsspace = NULL;
   free((char *) fnmtbl);   fnmtbl = NULL;
   free((char *) labels);   labels = NULL;
   free((char *) codep);   codep = NULL;

   for (fp = lffirst; fp != NULL; fp = fp1) {
      for(rp = fp->f_rlist; rp != NULL; rp = rp1) {
         rp1 = rp->r_link;
         free((char *)rp);
         }
      fp1 = fp->f_nextentry;
      free((char *)fp);
      }
   lffirst = NULL;
   lflast = NULL;

   for (gp = lgfirst; gp != NULL; gp = gp1) {
      gp1 = gp->g_next;
      free((char *)gp);
      }
   lgfirst = NULL;
   lglast = NULL;
   }
