#if !COMPILER
/*
 * File: imain.r
 * Interpreter main program, argument handling, and such.
 * Contents: main, iconx, ixopts, resolve
 */

#include "../h/version.h"
#include "../h/header.h"
#include "../h/opdefs.h"

static int iconx(int argc, char *argv[]);
static void ixopts(int argc, char *argv[], int *ip);

/*
 * Initial interpreter entry point (for all remaining platforms).
 */
int main(int argc, char *argv[]) {
   return iconx(argc, argv);
}

/*
 * Initial icode sequence. This is used to invoke the main procedure
 *  with one argument.  If main returns, the Op_Quit is executed.
 */
int iconx(int argc, char *argv[]) {
   int i, slen;
   static word istart[4];
   static int mterm = Op_Quit;

   #ifdef MultiThread
      /*
       * Look for MultiThread programming environment in which to execute
       * this program, specified by MTENV environment variable.
       */
      {
      char *p;
      char **new_argv;
      int i, j = 1, k = 1;
      if ((p = getenv("MTENV")) != NULL) {
         for(i=0;p[i];i++)
   	 if (p[i] == ' ')
   	    j++;
         new_argv = malloc((argc + j) * sizeof(char *));
         new_argv[0] = argv[0];
         for (i=0; p[i]; ) {
   	 new_argv[k++] = p+i;
   	 while (p[i] && (p[i] != ' '))
   	    i++;
   	 if (p[i] == ' ')
   	    p[i++] = '\0';
   	 }
         for(i=1;i<argc;i++)
   	 new_argv[k++] = argv[i];
         argc += j;
         argv = new_argv;
         }
      }
   #endif				/* MultiThread */

   ipc.opnd = NULL;

   #ifdef LoadFunc
      /*
       *  Append to FPATH the bin directory from which iconx was executed.
       */
      {
         char *p, *q, buf[1000];
         p = getenv("FPATH");
         q = relfile(argv[0], "/..");
         sprintf(buf, "FPATH=%s %s", (p ? p : "."), (q ? q : "."));
         putenv(buf);
         }
   #endif				/* LoadFunc */

   /*
    * Setup Icon interface.  It's done this way to avoid duplication
    *  of code, since the same thing has to be done if calling Icon
    *  is enabled.
    */

   ixopts(argc, argv, &i);

   if (i < 0) {
      argc++;
      argv--;
      i++;
      }

   while (i--) {			/* skip option arguments */
      argc--;
      argv++;
      }

   if (argc <= 1)
      error(NULL, "no icode file specified");

   /*
    * Call icon_init with the name of the icode file to execute.	[[I?]]
    */
   icon_init(argv[1], &argc, argv);

   /*
    *  Point sp at word after b_coexpr block for &main, point ipc at initial
    *	icode segment, and clear the gfp.
    */

   stackend = stack + mstksize/WordSize;
   sp = stack + Wsizeof(struct b_coexpr);

   ipc.opnd = istart;
   *ipc.op++ = Op_Noop;  /* aligns Invoke's operand */	/*	[[I?]] */
   *ipc.op++ = Op_Invoke;				/*	[[I?]] */
   *ipc.opnd++ = 1;
   *ipc.op = Op_Quit;
   ipc.opnd = istart;

   gfp = 0;

   /*
    * Set up expression frame marker to contain execution of the
    *  main procedure.  If failure occurs in this context, control
    *  is transferred to mterm, the address of an Op_Quit.
    */
   efp = (struct ef_marker *)(sp);
   efp->ef_failure.op = &mterm;
   efp->ef_gfp = 0;
   efp->ef_efp = 0;
   efp->ef_ilevel = 1;
   sp += Wsizeof(*efp) - 1;

   pfp = 0;
   ilevel = 0;

   /*
    * We have already loaded the
    * icode and initialized things, so it's time to just push main(),
    * build an Icon list for the rest of the arguments, and called
    * interp on a "invoke 1" bytecode.
    */

   /*
    * The first global variable holds the value of "main".  If it
    *  is not of type procedure, this is noted as run-time error 117.
    *  Otherwise, this value is pushed on the stack.
    */
   if (globals[0].dword != D_Proc)
      fatalerr(117, NULL);
   PushDesc(globals[0]);
   PushNull;
   glbl_argp = (dptr)(sp - 1);

   /*
    * If main() has a parameter, it is to be invoked with one argument, a list
    *  of the command line arguments.  The command line arguments are pushed
    *  on the stack as a series of descriptors and Ollist is called to create
    *  the list.  The null descriptor first pushed serves as Arg0 for
    *  Ollist and receives the result of the computation.
    */
   if (((struct b_proc *)BlkLoc(globals[0]))->nparam > 0) {
      for (i = 2; i < argc; i++) {
         char *tmp;
         slen = strlen(argv[i]);
         PushVal(slen);
         Protect(tmp=alcstr(argv[i],(word)slen), fatalerr(0,NULL));
         PushAVal(tmp);
         }

      Ollist(argc - 2, glbl_argp);
      }

   sp = (word *)glbl_argp + 1;
   glbl_argp = 0;
   ixinited = 1;		/* post fact that iconx is initialized */

   /*
    * Start things rolling by calling interp.  This call to interp
    *  returns only if an Op_Quit is executed.	If this happens,
    *  c_exit() is called to wrap things up.
    */
   interp(0,(dptr)NULL);
   c_exit(EXIT_SUCCESS);
   return 0;
}

/*
 * ixopts - handle interpreter command line options.
 */
void ixopts(argc,argv,ip)
int argc;
char **argv;
int *ip;
   {

   #ifdef TallyOpt
      extern int tallyopt;
   #endif				/* TallyOpt */

   *ip = 0;			/* number of arguments processed */

   #if MSWIN
      /*
       * if we didn't start with iconx.exe, backup one
       * so that our icode filename is argv[1].
       */
      {
      char tmp[256], *t2, *basename, *ext;
      int len = 0;
      strcpy(tmp, argv[0]);
      for (t2 = tmp; *t2; t2++) {
         switch (*t2) {
	    case ':':
	    case '/':
	    case '\\':
	       basename = t2 + 1;
	       ext = NULL;
	       break;
	    case '.':
	       ext = t2;
	       break;
	    default:
         *t2 = tolower(*t2);
	       break;
	    }
         }
      /* If present, cut the ".exe" extension. */
      if (ext != NULL && !strcmp(ext, ".exe"))
         *ext = 0;

      /*
       * if argv[0] is not a reference to our interpreter, take it as the
       * name of the icode file, and back up for it.
       */
      if (strcmp(basename, "iconx")) {
         argv--;
         argc++;
         (*ip)--;
         }
      }
   #endif				/* MSWIN */

   /*
    * Handle command line options.
    */
   while ( argv[1] != 0 && *argv[1] == '-' ) {

      switch ( *(argv[1]+1) ) {

         #ifdef TallyOpt
         /*
          * Set tallying flag if -T option given
          */
         case 'T':
            tallyopt = 1;
            break;
         #endif				/* TallyOpt */

         /*
          * Announce version on stderr if -V is given.
          */
         case 'V':
            fprintf(stderr, "%s  (%s, %s)\n", Version, Config, __DATE__);
	    if (!argv[2])
	       exit(0);
            break;
   
         }

      argc--;
      (*ip)++;
      argv++;
      }
   }

/*
 * resolve - perform various fix-ups on the data read from the icode
 *  file.
 */
#ifdef MultiThread
   void resolve(pstate)
   struct progstate *pstate;
#else					/* MultiThread */
   void resolve()
#endif					/* MultiThread */

   {
   register word i, j;
   register struct b_proc *pp;
   register dptr dp;
   extern int Omkrec();
   #ifdef MultiThread
      register struct progstate *savedstate;
   #endif				/* MultiThread */

   #ifdef MultiThread
      savedstate = curpstate;
      if (pstate) curpstate = pstate;
   #endif				/* MultiThread */

   /*
    * Relocate the names of the global variables.
    */
   for (dp = gnames; dp < egnames; dp++)
      StrLoc(*dp) = strcons + (uword)StrLoc(*dp);

   /*
    * Scan the global variable array for procedures and fill in appropriate
    *  addresses.
    */
   for (j = 0; j < n_globals; j++) {

      if (globals[j].dword != D_Proc)
         continue;

      /*
       * The second word of the descriptor for procedure variables tells
       *  where the procedure is.  Negative values are used for built-in
       *  procedures and positive values are used for Icon procedures.
       */
      i = IntVal(globals[j]);

      if (i < 0) {
         /*
          * globals[j] points to a built-in function; call (bi_)strprc
	  *  to look it up by name in the interpreter's table of built-in
	  *  functions.
          */
	 if((BlkLoc(globals[j])= (union block *)bi_strprc(gnames+j,0)) == NULL)
            globals[j] = nulldesc;		/* undefined, set to &null */
         }
      else {

         /*
          * globals[j] points to an Icon procedure or a record; i is an offset
          *  to location of the procedure block in the code section.  Point
          *  pp at the block and replace BlkLoc(globals[j]).
          */
         pp = (struct b_proc *)(code + i);
         BlkLoc(globals[j]) = (union block *)pp;

         /*
          * Relocate the address of the name of the procedure.
          */
         StrLoc(pp->pname) = strcons + (uword)StrLoc(pp->pname);

         if (pp->ndynam == -2) {
            /*
             * This procedure is a record constructor.	Make its entry point
             *	be the entry point of Omkrec().
             */
            pp->entryp.ccode = Omkrec;

	    /*
	     * Initialize field names
	     */
            for (i = 0; i < pp->nfields; i++)
               StrLoc(pp->lnames[i]) = strcons + (uword)StrLoc(pp->lnames[i]);

	    }
         else {
            /*
             * This is an Icon procedure.  Relocate the entry point and
             *	the names of the parameters, locals, and static variables.
             */
            pp->entryp.icode = code + pp->entryp.ioff;
            for (i = 0; i < abs((int)pp->nparam)+pp->ndynam+pp->nstatic; i++)
               StrLoc(pp->lnames[i]) = strcons + (uword)StrLoc(pp->lnames[i]);
            }
         }
      }

   /*
    * Relocate the names of the fields.
    */

   for (dp = fnames; dp < efnames; dp++)
      StrLoc(*dp) = strcons + (uword)StrLoc(*dp);

   #ifdef MultiThread
      curpstate = savedstate;
   #endif				/* MultiThread */
   }

#endif					/* !COMPILER */
