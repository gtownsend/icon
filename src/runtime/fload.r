/*
 * File: fload.r
 *  Contents: loadfunc.
 *
 *  This file contains loadfunc(), the dynamic loading function for
 *  Unix systems having the <dlfcn.h> interface.
 *
 *  from Icon:
 *     p := loadfunc(filename, funcname)
 *     p(arg1, arg2, ...)
 *
 *  in C:
 *     int func(int argc, dptr argv)
 *        return -1 for failure, 0 for success, >0 for error
 *        argc is number of true args not including argv[0]
 *        argv[0] is for return value; others are true args
 */

#ifdef LoadFunc

#ifndef RTLD_LAZY	/* normally from <dlfcn.h> */
   #define RTLD_LAZY 1
#endif					/* RTLD_LAZY */

#ifdef FreeBSD
   /*
    * If DL_GETERRNO exists, this is an FreeBSD 1.1.5 or 2.0
    * which lacks dlerror(); supply a substitute.
    */
   #passthru #ifdef DL_GETERRNO
      char *dlerror(void)
      {
         int no;

         if (0 == dlctl(NULL, DL_GETERRNO, &no))
            return(strerror(no));
         else
            return(NULL);
      }
   #passthru #endif
#endif					/* __FreeBSD__ */

int glue();
int makefunc	(dptr d, char *name, int (*func)());

"loadfunc(filename,funcname) - load C function dynamically."

function{0,1} loadfunc(filename,funcname)

   if !cnv:C_string(filename) then
      runerr(103, filename)
   if !cnv:C_string(funcname) then
      runerr(103, funcname)

   abstract {
      return proc
      }
   body
      {
      int (*func)();
      static char *curfile;
      static void *handle;
      char *funcname2;

      /*
       * Get a library handle, reusing it over successive calls.
       */
      if (!handle || !curfile || strcmp(filename, curfile) != 0) {
         if (curfile)
            free((pointer)curfile);	/* free the old file name */
         curfile = salloc(filename);	/* save the new name */
         handle = dlopen(filename, RTLD_LAZY);	/* get the handle */
         }
      /*
       * Load the function.  Diagnose both library and function errors here.
       */
      if (handle) {
         func = (int (*)())dlsym(handle, funcname);
         if (!func) {
            /*
             * If no function, try again by prepending an underscore.
             * (for OpenBSD and similar systems.)
             */
            funcname2 = malloc(strlen(funcname) + 2);
            if (funcname2) {
               *funcname2 = '_';
               strcpy(funcname2 + 1, funcname);
               func = (int (*)())dlsym(handle, funcname2);
               free(funcname2);
               }
            }
         }
      if (!handle || !func) {
         fprintf(stderr, "\nloadfunc(\"%s\",\"%s\"): %s\n",
            filename, funcname, dlerror());
         runerr(216);
         }
      /*
       * Build and return a proc descriptor.
       */
      if (!makefunc(&result, funcname, func))
         runerr(305);
      return result;
      }
end

/*
 * makefunc(d, name, func) -- make function descriptor in d.
 *
 *  Returns 0 if memory could not be allocated.
 */
int makefunc(d, name, func)
dptr d;
char *name;
int (*func)();
   {
   struct b_proc *blk;

   blk = (struct b_proc *)malloc(sizeof(struct b_proc));
   if (!blk)
      return 0;
   blk->title = T_Proc;
   blk->blksize = sizeof(struct b_proc);

#if COMPILER
   blk->ccode = glue;		/* set code addr to glue routine */
#else					/* COMPILER */
   blk->entryp.ccode = glue;	/* set code addr to glue routine */
#endif					/* COMPILER */

   blk->nparam = -1;		/* varargs flag */
   blk->ndynam = -1;		/* treat as built-in function */
   blk->nstatic = 0;
   blk->fstatic = 0;
   blk->pname.dword = strlen(name);
   blk->pname.vword.sptr = salloc(name);
   blk->lnames[0].dword = 0;
   blk->lnames[0].vword.sptr = (char *)func;
				/* save func addr in lnames[0] vword */
   d->dword = D_Proc;		/* build proc descriptor */
   d->vword.bptr = (union block *)blk;
   return 1;
   }

/*
 * This glue routine is called when a loaded function is invoked.
 * It digs the actual C code address out of the proc block, and calls that.
 */

#if COMPILER

int glue(argc, dargv, rslt, succ_cont)
int argc;
dptr dargv;
dptr rslt;
continuation succ_cont;
   {
   int i, status, (*func)();
   struct b_proc *blk;
   struct descrip r;
   tended struct descrip p;

   dargv--;				/* reset pointer to proc entry */
   for (i = 0; i <= argc; i++)
      deref(&dargv[i], &dargv[i]);	/* dereference args including proc */

   blk = (struct b_proc *)dargv[0].vword.bptr;	/* proc block address */
   func = (int (*)())blk->lnames[0].vword.sptr;	/* entry point address */

   p = dargv[0];			/* save proc for traceback */
   dargv[0] = nulldesc;			/* set default return value */
   status = (*func)(argc, dargv);	/* call func */

   if (status == 0) {
      *rslt = dargv[0];
      Return;				/* success */
      }

   if (status < 0)
      Fail;				/* failure */

   r = dargv[0];			/* save result value */
   dargv[0] = p;			/* restore proc for traceback */
   if (is:null(r))
      RunErr(status, NULL);		/* error, no value */
   RunErr(status, &r);			/* error, with value */
   }

#else					/* COMPILER */

int glue(argc, dargv)
int argc;
dptr dargv;
   {
   int status, (*func)();
   struct b_proc *blk;
   struct descrip r;
   tended struct descrip p;

   blk = (struct b_proc *)dargv[0].vword.bptr;	/* proc block address */
   func = (int (*)())blk->lnames[0].vword.sptr;	/* entry point address */

   p = dargv[0];			/* save proc for traceback */
   dargv[0] = nulldesc;			/* set default return value */
   status = (*func)(argc, dargv);	/* call func */

   if (status == 0)
      Return;				/* success */
   if (status < 0)
      Fail;				/* failure */

   r = dargv[0];			/* save result value */
   dargv[0] = p;			/* restore proc for traceback */
   if (is:null(r))
      RunErr(status, NULL);		/* error, no value */
   RunErr(status, &r);			/* error, with value */
   }

#endif					/* COMPILER */

#endif					/* LoadFunc */
