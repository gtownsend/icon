#if !COMPILER
/*
 * File: imain.r
 * Interpreter main program, argument handling, and such.
 * Contents: main, icon_call, icon_setup, resolve, xmfree
 */

#include "../h/version.h"
#include "../h/header.h"
#include "../h/opdefs.h"

void	icon_setup	(int argc, char **argv, int *ip);

extern int set_up;

word istart[4];
int mterm = Op_Quit;

#ifndef MultiThread
   int n_globals = 0;			/* number of globals */
   int n_statics = 0;			/* number of statics */
#endif					/* MultiThread */


/*
 * Initial icode sequence. This is used to invoke the main procedure with one
 *  argument.  If main returns, the Op_Quit is executed.
 */


#ifdef MSWindows

FILE *finredir, *fouredir, *ferredir;

void detectRedirection()
{
   struct stat sb;
   /*
    * Look at the standard file handles and attempt to detect
    * redirection.
    */
   if (fstat(stdin->_file, &sb) == 0) {
      if (sb.st_mode & S_IFCHR) {		/* stdin is a device */
	 }
      if (sb.st_mode & S_IFREG) {		/* stdin is a regular file */
	 }
      /* stdin is of size sb.st_size */
      if (sb.st_size > 0) {
         ConsoleFlags |= StdInRedirect;
	 }
      }
   else {					/* unable to identify stdin */
      }

   if (fstat(stdout->_file, &sb) == 0) {
      if (sb.st_mode & S_IFCHR) {		/* stdout is a device */
	 }
      if (sb.st_mode & S_IFREG) {		/* stdout is a regular file */
	 }
      /* stdout is of size sb.st_size */
      if (sb.st_size == 0)
         ConsoleFlags |= StdOutRedirect;
      }
   else {					/* unable to identify stdout */
     }
}

int CmdParamToArgv(char *s, char ***avp)
   {
   char tmp[MaxPath], dir[MaxPath];
   char *t, *t2;
   int rv=0, i;
   FILE *f;

   t = salloc(s);
   t2 = t;

   *avp = malloc(2 * sizeof(char *));
   (*avp)[rv] = NULL;

   detectRedirection();

   while (*t2) {
      while (*t2 && isspace(*t2)) t2++;
      switch (*t2) {
	 case '\0': break;
	 case '<': case '>': {
	    /*
	     * perform file redirection; this is for Windows 3.1
	     * and other situations where Wiconx is launched from
	     * a shell that does not process < and > characters.
	     */
	    char c = *t2++, buf[128], *t3;
	    FILE *f;
	    while (*t2 && isspace(*t2)) t2++;
	    t3 = buf;
	    while (*t2 && !isspace(*t2)) *t3++ = *t2++;
	    *t3 = '\0';
	    if (c == '<')
	       f = fopen(buf, "r");
	    else
	       f = fopen(buf, "w");
	    if (f == NULL) {
	       MessageBox(0, "unable to redirect i/o", "system error",
			  MB_ICONHAND);
	       c_exit(-1);
	       }
	    if (c == '<') {
	       finredir = f;
	       ConsoleFlags |= StdInRedirect;
	       }
	    else {
	       fouredir = f;
	       ConsoleFlags |= StdOutRedirect;
	       }
	    break;
	    }
	 case '"': {
	    char *t3 = ++t2;			/* skip " */

            while (*t2 && (*t2 != '"')) t2++;
            if (*t2)
	       *t2++ = '\0';
	    *avp = realloc(*avp, (rv + 2) * sizeof (char *));
	    (*avp)[rv++] = salloc(t3);
            (*avp)[rv] = NULL;

	    break;
	    }
         default: {
            FINDDATA_T fd;
	    char *t3 = t2;
            while (*t2 && !isspace(*t2)) t2++;
	    if (*t2)
	       *t2++ = '\0';
            strcpy(tmp, t3);
	    if (!FINDFIRST(tmp, &fd)) {
	       *avp = realloc(*avp, (rv + 2) * sizeof (char *));
	       (*avp)[rv++] = salloc(t3);
               (*avp)[rv] = NULL;
               }
	    else {
               int end;
               strcpy(dir, t3);
	       do {
	          end = strlen(dir)-1;
	          while (end >= 0 && dir[end] != '\\' && dir[end] != '/' &&
			dir[end] != ':') {
                     dir[end] = '\0';
		     end--;
	             }
		  strcat(dir, FILENAME(&fd));
	          *avp = realloc(*avp, (rv + 2) * sizeof (char *));
	          (*avp)[rv++] = salloc(dir);
                  (*avp)[rv] = NULL;
	          } while (FINDNEXT(&fd));
	       FINDCLOSE(&fd);
	       }
            break;
	    }
         }
      }

   free(t);
   return rv;
   }

char *lognam;
char tmplognam[128];

void MSStartup(HINSTANCE hInstance, HINSTANCE hPrevInstance)
   {
   WNDCLASS wc;
   #ifdef ConsoleWindow
      extern FILE *flog;
   
      /*
       * Select log file name.  Might make this a command-line option.
       * Default to "WICON.LOG".  The log file is used by Wi to report
       * translation errors and jump to the offending source code line.
       */
      if ((lognam = getenv("WICONLOG")) == NULL)
         lognam = "WICON.LOG";
      remove(lognam);
      lognam = strdup(lognam);
      flog = fopen(tmpnam(tmplognam), "w");
   
      if (flog == NULL) {
         syserr("unable to open logfile");
         }
   #endif				/* ConsoleWindow */
   if (!hPrevInstance) {
      #if NT
         wc.style = CS_HREDRAW | CS_VREDRAW;
      #else				/* NT */
         wc.style = 0;
      #endif				/* NT */
      wc.lpfnWndProc = WndProc;
      wc.cbClsExtra = 0;
      wc.cbWndExtra = 0;
      wc.hInstance  = hInstance;
      wc.hIcon      = NULL;
      wc.hCursor    = NULL;
      wc.hbrBackground = GetStockObject(WHITE_BRUSH);
      wc.lpszMenuName = NULL;
      wc.lpszClassName = "iconx";
      RegisterClass(&wc);
      }
   }

void iconx(int argc, char **argv);

jmp_buf mark_sj;

int_PASCAL WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
                   LPSTR lpszCmdLine, int nCmdShow)
   {
   int argc;
   char **argv;

   mswinInstance = hInstance;
   ncmdShow = nCmdShow;

   argc = CmdParamToArgv(GetCommandLine(), &argv);
   MSStartup(hInstance, hPrevInstance);
   if (setjmp(mark_sj) == 0)
      iconx(argc,argv);
   while (--argc>=0)
      free(argv[argc]);
   free(argv);
   wfreersc();
   xmfree();
   return 0;
}
#define main iconx
#endif					/* MSWindows */

int main(argc, argv)
int argc;
char **argv;
   {
   int i, slen;

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
      new_argv = (char **)malloc((argc + j) * sizeof(char *));
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
#endif					/* MultiThread */

   ipc.opnd = NULL;

   /*
    * Setup Icon interface.  It's done this way to avoid duplication
    *  of code, since the same thing has to be done if calling Icon
    *  is enabled.
    */

   icon_setup(argc, argv, &i);

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

   set_up = 1;			/* post fact that iconx is initialized */

   /*
    * Start things rolling by calling interp.  This call to interp
    *  returns only if an Op_Quit is executed.	If this happens,
    *  c_exit() is called to wrap things up.
    */

   #ifdef CoProcesses
      codisp();    /* start up co-expr dispatcher, which will call interp */
   #else				/* CoProcesses */
      interp(0,(dptr)NULL);                        /*      [[I?]] */
   #endif				/* CoProcesses */

   c_exit(EXIT_SUCCESS);
   return 0;
}

/*
 * icon_setup - handle interpreter command line options.
 */
void icon_setup(argc,argv,ip)
int argc;
char **argv;
int *ip;
   {

   #ifdef TallyOpt
      extern int tallyopt;
   #endif				/* TallyOpt */

   *ip = 0;			/* number of arguments processed */

   #ifdef ExecImages
      if (dumped) {
         /*
          * This is a restart of a dumped interpreter.  Normally, argv[0] is
          *  iconx, argv[1] is the icode file, and argv[2:(argc-1)] are the
          *  arguments to pass as a list to main().  For a dumped interpreter
          *  however, argv[0] is the executable binary, and the first argument
          *  for main() is argv[1].  The simplest way to handle this is to
          *  back up argv to point at argv[-1] and increment argc, giving the
          *  illusion of an additional argument at the head of the list.  Note
          *  that this argument is never referenced.
          */
         argv--;
         argc++;
         (*ip)--;
         }
   #endif				/* ExecImages */

   #if NT
      /*
       * if we didn't start with nticonx.exe or wiconx.exe, backup one
       * so that our icode filename is argv[1].
       */
      {
      char tmp[256], *t2;
      int len = 0;
      strcpy(tmp, argv[0]);
      t2 = tmp;
      while (*t2) {
         *t2 = tolower(*t2);
         t2++;
         len++;
         }
   
      /*
       * if argv[0] is not a reference to our interpreter, take it as the
       * name of the icode file, and back up for it.
       */
      #ifdef MSWindows
         if (!((len == 6 && !strcmp(tmp+len-6, "wiconx")) ||
               (len > 6 && !strcmp(tmp+len-7, "\\wiconx")) ||
               (len == 10 && !strcmp(tmp+len-10, "wiconx.exe")) ||
               (len > 10 && !strcmp(tmp+len-11, "\\wiconx.exe")))) {
      #else				/* MSWindows */
         if (!((len == 7 && !strcmp(tmp+len-7, "nticonx")) ||
               (len > 7 && !strcmp(tmp+len-8, "\\nticonx")) ||
               (len == 11 && !strcmp(tmp+len-11, "nticonx.exe")) ||
               (len > 11 && !strcmp(tmp+len-12, "\\nticonx.exe")))) {
      #endif				/* MSWindows */
         argv--;
         argc++;
         (*ip)--;
         }
      }
   #endif				/* NT */

   #ifdef MaxLevel
      maxilevel = 0;
      maxplevel = 0;
      maxsp = 0;
   #endif				/* MaxLevel */

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
       * Set stderr to new file if -e option is given.
       */
	 case 'e': {
	    char *p;
	    if ( *(argv[1]+2) != '\0' )
	       p = argv[1]+2;
	    else {
	       argv++;
	       argc--;
               (*ip)++;
	       p = argv[1];
	       if ( !p )
		  error(NULL, "no file name given for redirection of &errout");
	       }
            if (!redirerr(p))
               syserr("Unable to redirect &errout\n");
	    break;
	    }
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
   extern Omkrec();
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


/*
 * Free malloc-ed memory; the main regions then co-expressions.  Note:
 *  this is only correct if all allocation is done by routines that are
 *  compatible with free() -- which may not be the case if Allocreg()
 *  in rmemfix.c is defined to be other than malloc().
 */

void xmfree()
   {
   register struct b_coexpr **ep, *xep;
   register struct astkblk *abp, *xabp;

   if (mainhead != (struct b_coexpr *)NULL)
      free((pointer)mainhead->es_actstk);	/* activation block for &main */
   free((pointer)code);			/* icode */
   code = NULL;
   free((pointer)stack);		/* interpreter stack */
   stack = NULL;
   /*
    * more is needed to free chains of heaps, also a multithread version
    * of this function may be needed someday.
    */
   if (strbase)
      free((pointer)strbase);		/* allocated string region */
   strbase = NULL;
   if (blkbase)
      free((pointer)blkbase);		/* allocated block region */
   blkbase = NULL;
   if (curstring != &rootstring)
      free((pointer)curstring);		/* string region */
   curstring = NULL;
   if (curblock != &rootblock)
      free((pointer)curblock);		/* allocated block region */
   curblock = NULL;
   if (quallist)
      free((pointer)quallist);		/* qualifier list */
   quallist = NULL;

   /*
    * The co-expression blocks are linked together through their
    *  nextstk fields, with stklist pointing to the head of the list.
    *  The list is traversed and each stack is freeing.
    */
   ep = &stklist;
   while (*ep != NULL) {
      xep = *ep;
      *ep = (*ep)->nextstk;
       /*
        * Free the astkblks.  There should always be one and it seems that
        *  it's not possible to have more than one, but nonetheless, the
        *  code provides for more than one.
        */
 	 for (abp = xep->es_actstk; abp; ) {
            xabp = abp;
            abp = abp->astk_nxt;
            free((pointer)xabp);
            }

         #ifdef CoProcesses
            coswitch(BlkLoc(k_current)->coexpr.cstate, xep->cstate, -1);
                /* terminate coproc for coexpression first */
         #endif				/* CoProcesses */

      free((pointer)xep);
      stklist = NULL;
      }

   }
#endif					/* !COMPILER */
