/*
 * cmain.c - main program icon compiler.
 */
#include "../h/gsupport.h"
#include "ctrans.h"
#include "ctree.h"
#include "ccode.h"
#include "csym.h"
#include "cproto.h"

/*
 * Prototypes.
 */
static void execute (char *ofile, char *efile, char **args);
static FILE  *open_out (char *fname);
static void rmfile  (char *fname);
static void report  (char *s);
static void usage   (void);

#ifdef strlen
#undef strlen				/* pre-defined in some contexts */
#endif					/* strlen */

#ifdef ExpTools
char *toolstr = "${TOOLS}";
#endif					/* ExpTools */

char *refpath;

/*
 *  Define global variables.
 */

#define Global
#define Init(v) = v
#include "cglobals.h"

/*
 * getopt() variables
 */
extern int optind;		/* index into parent argv vector */
extern int optopt;		/* character checked for validity */
extern char *optarg;		/* argument associated with option */

/*
 *  main program
 */
int main(argc,argv)
int argc;
char **argv;
   {
   int no_c_comp = 0;			/* suppress C compile and link? */
   int errors = 0;			/* compilation errors */
   char *cfile = NULL;			/* name of C file - primary */
   char *hfile = NULL;			/* name of C file - include */
   char *ofile = NULL;			/* name of executable result */
   char *efile = NULL;			/* stderr file */

   char *db_name = "rt.db";		/* data base name */
   char *incl_file = "rt.h";		/* header file name */

   char *db_path;			/* path to data base */
   char *db_lst;			/* list of private data bases */
   char *incl_path;			/* path to header file */
   char *s, c1;
   char buf[MaxFileName];		/* file name construction buffer */

#ifdef ExpTools
   char Buf[MaxFileName];
   char *tools;				/* patch and TOOLS string buffer */
#endif					/* ExpTools */

   int c;
   int ret_code;
   struct fileparts *fp;

#ifdef ExpTools
   /* needs rewrite */
   if (strlen(patchpath)>18) {
      refpath = patchpath+18;
      if(!strncmp(refpath,toolstr,strlen(toolstr))) {	/* Is it TOOLS   */
         refpath = refpath+strlen(toolstr);		/* skip TOOLS    */
         if ((tools = getenv("TOOLS")) == NULL) {
            fprintf(stderr,
              "patchstr begins with \"${TOOLS}\" but ${TOOLS} has no value\n");
            fprintf(stderr, "patchstr=%s\ncompilation aborted\n", refpath);
            exit(EXIT_FAILURE);
           } else strcpy(Buf,tools);
         strcat(Buf,refpath);			/* append name   */
	 if (Buf[strlen(Buf)-1] != '/') strcat(Buf,"/");
	 refpath = Buf;				/* use refpath   */
  	 }
      }
   fprintf(stderr,"iconc library files found in %s\n",refpath);
#else					/* ExpTools */
   refpath = relfile(argv[0], "/../");
#endif					/* ExpTools */

   /*
    * Process options.
    */
   while ((c = getopt(argc,argv,IconOptions)) != EOF)
      switch (c) {
         case 'C':			/* -C C-comp: C compiler*/
            c_comp = optarg;
            break;
         case 'E':			/* -E: preprocess only */
            pponly = 1;
            no_c_comp = 1;
            break;
         case 'L':			/* Ignore: interpreter only */
            break;
         case 'S':			/* Ignore: interpreter only */
            break;
	 case 'T':
	    just_type_trace = 1;
	    break;
         case 'c':			/* -c: produce C file only */
            no_c_comp = 1;
            break;
         case 'e':			/* -e file: redirect stderr */
            efile = optarg;
            break;
         case 'f':			/* -f: enable features */
            for (s = optarg; *s != '\0'; ++s) {
               switch (*s) {
                  case 'a':             /* -fa: enable all features */
                     line_info = 1;
                     debug_info = 1;
                     err_conv = 1;
                     largeints = 1;
                     str_inv = 1;
                     break;
                  case 'd':             /* -fd: enable debugging features */
                     line_info = 1;
                     debug_info = 1;
                     break;
                  case 'e':             /* -fe: enable error conversion */
                     err_conv = 1;
                     break;
                  case 'l':             /* -fl: support large integers */
                     largeints = 1;
                     break;
                  case 'n':             /* -fn: enable line numbers */
                     line_info = 1;
                     break;
                  case 's':		/* -fs: enable full string invocation */
                     str_inv = 1;
                     break;
                  default:
                     quitf("-f option must be a, d, e, l, n, or s. found: %s",
                        optarg);
                  }
               }
            break;
         case 'm':			/* -m: preprocess using m4(1) [UNIX] */
            m4pre = 1;
            break;
         case 'n':			/* -n: disable optimizations */
            for (s = optarg; *s != '\0'; ++s) {
               switch (*s) {
                  case 'a':		/* -na: disable all optimizations */
                     opt_cntrl = 0;
                     allow_inline = 0;
                     opt_sgnl = 0;
                     do_typinfer = 0;
                     break;
                  case 'c':		/* -nc: disable control flow opts */
                     opt_cntrl = 0;
                     break;
                  case 'e':		/* -ne: disable expanding in-line */
                     allow_inline = 0;
                     break;
                  case 's':		/* -ns: disable switch optimizations */
                     opt_sgnl = 0;
                     break;
                  case 't':		/* -nt: disable type inference */
                     do_typinfer = 0;
                     break;
                  default:
                     usage();
                  }
               }
            break;
         case 'o':			/* -o file: name output file */
            ofile = optarg;
            break;
         case 'p':			/* -p C-opts: options for C comp */
            if (*optarg == '\0')	/* if empty string, clear options */
               c_opts = optarg;
            else {			/* else append to current set */
               s = (char *)alloc(strlen(c_opts) + 1 + strlen(optarg) + 1);
               sprintf(s, "%s %s", c_opts, optarg);
               c_opts = s;
               }
            break;
         case 'r':			/* -r path: primary runtime system */
            refpath = optarg;
            break;
         case 's':			/* -s: suppress informative messages */
            verbose = 0;
            break;
         case 't':                      /* -t: &trace = -1 */
            line_info = 1;
            debug_info = 1;
            trace = 1;
            break;
         case 'u':			/* -u: warn about undeclared ids */
            uwarn = 1;
            break;
         case 'v':			/* -v: set level of verbosity */
            if (sscanf(optarg, "%d%c", &verbose, &c1) != 1)
               quitf("bad operand to -v option: %s",optarg);
            break;
         default:
         case 'x':                      /* -x illegal until after file list */
            usage();
         }

   init();			/* initialize memory for translation */

   /*
    * Load the data bases of information about run-time routines and
    *  determine what libraries are needed for linking (these libraries
    *  go before any specified on the command line).
    */
   db_lst = getenv("DBLIST");
   if (db_lst != NULL)
      db_lst = salloc(db_lst);
   s = db_lst;
   while (s != NULL) {
      db_lst = s;
      while (isspace(*db_lst))
         ++db_lst;
      if (*db_lst == '\0')
         break;
      for (s = db_lst; !isspace(*s) && *s != '\0'; ++s)
         ;
      if (*s == '\0')
         s = NULL;
      else
         *s++ = '\0';
      readdb(db_lst);
      addlib(salloc(makename(buf,SourceDir, db_lst, LibSuffix)));
      }
   db_path = (char *)alloc((unsigned int)strlen(refpath) + strlen(db_name) + 1);
   strcpy(db_path, refpath);
   strcat(db_path, db_name);
   readdb(db_path);
   addlib(salloc(makename(buf,SourceDir, db_path, LibSuffix)));

   /*
    * Scan the rest of the command line for file name arguments.
    */
   while (optind < argc)  {
      if (strcmp(argv[optind],"-x") == 0)	/* stop at -x */
         break;
      else if (strcmp(argv[optind],"-") == 0)
         src_file("-");				/* "-" means standard input */

/*
 * The following code is operating-system dependent [@tmain.02].  Check for
 *  C linker options on the command line.
 */

#if PORT
   /* something is needed */
Deliberate Syntax Error
#endif					/* PORT */

#if UNIX
      else if (argv[optind][0] == '-')
         addlib(argv[optind]);		/* assume linker option */
#endif					/* UNIX ... */

#if AMIGA || ATARI_ST || MACINTOSH || MSDOS || MVS || OS2 || VM || VMS
      /*
       * Linker options on command line not supported.
       */
#endif					/* AMIGA || ATARI_ST || ... */

/*
 * End of operating-system specific code.
 */

      else {
         fp = fparse(argv[optind]);		/* parse file name */
         if (*fp->ext == '\0' || smatch(fp->ext, SourceSuffix)) {
            makename(buf,SourceDir,argv[optind], SourceSuffix);
#if VMS
	    strcat(buf, fp->version);
#endif					/* VMS */
            src_file(buf);
            }
         else

/*
 * The following code is operating-system dependent [@tmain.03].  Pass
 *  appropriate files on to linker.
 */

#if PORT
   /* something is needed */
Deliberate Syntax Error
#endif					/* PORT */

#if UNIX
            /*
             * Assume all files that are not Icon source go to linker.
             */
            addlib(argv[optind]);
#endif					/* UNIX ... */

#if AMIGA || ATARI_ST || MACINTOSH || MSDOS || MVS || OS2 || VM || VMS
            /*
             * Pass no files to the linker.
             */
            quitf("bad argument %s",argv[optind]);
#endif					/* AMIGA || ATARI_ST || ... */

/*
 * End of operating-system specific code.
 */

         }
      optind++;
      }
 
   if (srclst == NULL)
      usage();				/* error -- no files named */

   if (pponly) {
      if (trans() == 0)
	 exit (EXIT_FAILURE);
      else
	 exit (EXIT_SUCCESS);
      }

   if (ofile == NULL)  {		/* if no -o file, synthesize a name */
      if (strcmp(srclst->name,"-") == 0)
         ofile = salloc(makename(buf,TargetDir,"stdin",ExecSuffix));
      else
         ofile = salloc(makename(buf,TargetDir,srclst->name,ExecSuffix));
   } else {				/* add extension if necessary */
      fp = fparse(ofile);
      if (*fp->ext == '\0' && *ExecSuffix != '\0')
         ofile = salloc(makename(buf,NULL,ofile,ExecSuffix));
   }

   /*
    * Make name of intermediate C files.
    */
   cfile = salloc(makename(buf,TargetDir,ofile,CSuffix));
   hfile = salloc(makename(buf,TargetDir,ofile,HSuffix));

   codefile = open_out(cfile);
   fprintf(codefile, "#include \"%s\"\n", hfile);

   inclfile = open_out(hfile);
   fprintf(inclfile, "#define COMPILER 1\n");

   incl_path = (char *)alloc((unsigned int)(strlen(refpath) +
       strlen(incl_file) + 1));
   strcpy(incl_path, refpath);
   strcat(incl_path, incl_file);
   fprintf(inclfile,"#include \"%s\"\n", incl_path);

   /*
    * Translate .icn files to make C file.
    */
   if ((verbose > 0) && !just_type_trace)
      report("Translating to C");

   errors = trans();
   if ((errors > 0) || just_type_trace) {	/* exit if errors seen */
      rmfile(cfile);
      rmfile(hfile);
      if (errors > 0)
	 exit(EXIT_FAILURE);
      else exit(EXIT_SUCCESS);
      }

   fclose(codefile);
   fclose(inclfile);

   /*
    * Compile and link C file.
    */
   if (no_c_comp)			/* exit if no C compile wanted */
      exit(EXIT_SUCCESS);

#if !MSDOS
   if (verbose > 0)
      report("Compiling and linking C code");
#endif					/* !MSDOS */

   ret_code = ccomp(cfile, ofile);
   if (ret_code == EXIT_FAILURE) {
      fprintf(stderr, "*** C compile and link failed ***\n");
      rmfile(ofile);
      }

   /*
    * Finish by removing C files.
    */
#if !MSDOS
   rmfile(cfile);
   rmfile(hfile);
#endif					/* !MSDOS */
   rmfile(makename(buf,TargetDir,cfile,ObjSuffix));

   if (ret_code == EXIT_SUCCESS && optind < argc)  {
      if (verbose > 0)
         report("Executing");
      execute (ofile, efile, argv+optind+1);
      }

   return ret_code;
   }

/*
 * execute - execute compiled Icon program
 */
static void execute(ofile,efile,args)
char *ofile, *efile, **args;
   {

#if !(MACINTOSH && MPW)
   int n;
   char **argv, **p;

#if UNIX
      char buf[MaxFileName];		/* file name construction buffer */

      ofile = salloc(makename(buf,"./",ofile,ExecSuffix));
#endif					/* UNIX */

   for (n = 0; args[n] != NULL; n++)	/* count arguments */
      ;
   p = argv = (char **)alloc((unsigned int)((n + 2) * sizeof(char *)));

   *p++ = ofile;			/* set executable file */

#if AMIGA && LATTICE
   *p = *args;
   while (*p++) {
      *p = *args;
      args++;
   }
#else					/* AMIGA && LATTICE */
   while (*p++ = *args++)		/* copy args into argument vector */
      ;
#endif					/* AMIGA && LATTICE */

   *p = NULL;

   if (efile != NULL && !redirerr(efile)) {
      fprintf(stderr, "Unable to redirect &errout\n");
      fflush(stderr);
      }

/*
 * The following code is operating-system dependent [@tmain.04].  It calls
 *  the Icon program on the way out.
 */

#if PORT
   /* something is needed */
Deliberate Syntax Error
#endif					/* PORT */

#if AMIGA
#if AZTEC_C
      execvp(ofile,argv);
      return;
#endif					/* AZTEC_C */
#if LATTICE
      {
      struct ProcID procid;
      if (forkv(ofile,argv,NULL,&procid) == 0) { 
         wait(&procid);
         return;
         }
      }
#endif					/* LATTICE */
#endif					/* AMIGA */

#if ATARI_ST || MACINTOSH
      fprintf(stderr,"-x not supported\n"); fflush(stderr);
#endif					/* ATARI_ST || ... */

#if MSDOS
#if MICROSOFT || TURBO || BORLAND_286 || BORLAND_386
      execvp(ofile,argv);
#endif					/* MICROSOFT || ... */
#if HIGHC_386 || INTEL_386 || ZTC_386 || WATCOM
      fprintf(stderr,"-x not supported\n");
      fflush(stderr);
#endif					/* HIGHC_386 || ... */
#endif					/* MSDOS */

#if MVS || VM
#if SASC
	  exit(sysexec(ofile, argv));
#endif					/* SASC */
      fprintf(stderr,"-x not supported\n");
      fflush(stderr);
#endif                                  /* MVS || VM */

#if OS2 || UNIX || VMS
      execvp(ofile,argv);
#endif					/* OS2 || UNIX || VMS */


/*
 * End of operating-system specific code.
 */

   quitf("could not run %s",ofile);

#else					/* !(MACINTOSH && MPW) */
   printf("-x not supported\n");
#endif					/* !(MACINZTOSH && MPW) */

   }

static void report(s)
char *s;
   {

/*
 * The following code is operating-system dependent [@tmain.05].  Report
 *  phase.
 */

#if PORT
   fprintf(stderr,"%s:\n",s);
Deliberate Syntax Error
#endif					/* PORT */

#if AMIGA || ATARI_ST || MSDOS || MVS || OS2 || UNIX || VM || VMS
   fprintf(stderr,"%s:\n",s);
#endif					/* AMIGA || ATARI_ST || ... */

#if MACINTOSH
#if MPW
   printf("Echo '%s:' > Dev:StdErr\n",s);
#endif					/* MPW */
#if LSC
   fprintf(stderr,"%s:\n",s);
#endif					/* LSC */
#endif					/* MACINTOSH */

/*
 * End of operating-system specific code.
 */

   }

/*
 * rmfile - remove a file
 */

static void rmfile(fname)
char *fname;
   {

#if MACINTOSH && MPW
      /*
       * MPW generates commands rather than doing the actions
       *  at this time.
       */
   fprintf(stdout,"Delete %s\n", fname);
#else					/* MACINTOSH && MPW */
   remove(fname);
#endif					/* MACINTOSH && MPW */

   }

/*
 * open_out - open a C output file and write identifying information
 *  to the front.
 */
static FILE *open_out(fname)
char *fname;
   {
   FILE *f;
   static char *ident = "/*ICONC*/";
   int c;
   int i;

   /*
    * If the file already exists, make sure it is old output from iconc
    *   before overwriting it. Note, this test doesn't work if the file
    *   is writable but not readable.
    */
   f = fopen(fname, "r");
   if (f != NULL) {
      for (i = 0; i < (int)strlen(ident); ++i) {
         c = getc(f);
         if (c == EOF)
             break;
         if ((char)c != ident[i])
            quitf("%s not in iconc format; rename or delete, and rerun", fname);
         }
      fclose(f);
      }

   f = fopen(fname, "w");
   if (f == NULL)
      quitf("cannot create %s", fname);
   fprintf(f, "%s\n", ident); /* write "belongs to iconc" comment */
   id_comment(f);             /* write detailed comment for human readers */
   fflush(f);
   return f;
   }

/*
 * Print an error message if called incorrectly.  The message depends
 *  on the legal options for this system.
 */
static void usage()
   {
   fprintf(stderr,"usage: %s %s file ... [-x args]\n", progname, CUsage);
   exit(EXIT_FAILURE);
   }
