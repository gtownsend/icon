/*
 * twindows.c - main program for translator and linker under Windows.
 *
 * THIS FILE IS UNTESTED.
 *
 * It is file is based on the old tmain.c from Icon 9.3.2, but it
 * HAS NOT BEEN TESTED and probably needs some additional work.
 */

#include "../h/gsupport.h"
#include "tproto.h"

#if SCCX_MX
   #include "../h/filepat.h"
   /* This sets the stack size so it runs in a Windows DOS box */
   unsigned _stack = 100000;
#endif      /* SCCX_MX */

#ifdef MSWindows
   #ifdef NTConsole
      #define int_PASCAL int PASCAL
      #define LRESULT_CALLBACK LRESULT CALLBACK
      #include <windows.h>
      #include "../h/filepat.h"
   #endif				/* NTConsole */
#endif					/* MSWindows */

#if WildCards
   #ifndef ConsoleWindow
      #include "../h/filepat.h"
   #endif				/* ConsoleWindow */
#endif					/* WildCards */

/*
 * Prototypes.
 */

static	void	execute	(char *ofile,char *efile,char * *args);
static	void	rmfiles (char **p);
static	void	usage (void);

#if MSDOS
   char pathToIconDOS[129];
#endif					/* MSDOS */

/*
 *  Define global variables.
 */

#define Global
#define Init(v) = v
#include "tglobals.h"

char *ofile = NULL;			/* linker output file name */

char patchpath[MaxPath+18] = "%PatchStringHere->";

char *refpath = RefPath;

/*
 * getopt() variables
 */
extern int optind;		/* index into parent argv vector */
extern int optopt;		/* character checked for validity */
extern char *optarg;		/* argument associated with option */

#ifdef ConsoleWindow
   int ConsolePause = 1;
#endif					/* ConsoleWindow */

#ifdef ConsoleWindow
/*
 * expand Icon project (.icp) files
 */
void expand_proj(int *argc, char ***argv)
{
   int ac = *argc, i, j, k;
   char **av = *argv, buf[1024];
   FILE *f;
   for (i=1; i<ac; i++) {
      if (strstr(av[i], ".ICP") || strstr(av[i], ".icp")) break;
      }
   if (i == ac) return;
   if ((f = fopen(av[i], "r")) == NULL) {
      fprintf(stderr, "icont: can't open %s\n", av[i]);
      fflush(stderr);
      return;
      }
   if ((*argv = malloc(ac * sizeof (char *))) == NULL) {
      fprintf(stderr, "icont: can't malloc for %s\n", av[i]);
      fflush(stderr);
      return;
      }
   for(j=0; j<i; j++) (*argv)[j] = av[j];
   k = j++;
   if(fgets(buf, 1023, f) != NULL) {
      if (strchr(buf, '\n') != NULL)
         buf[strlen(buf)-1] = '\0';
      (*argv)[k++] = salloc(buf);
      while(fgets(buf, 1023, f) != NULL) {
         if (strchr(buf, '\n') != NULL)
            buf[strlen(buf)-1] = '\0';
         (*argc)++;
         if ((*argv = realloc(*argv, *argc * sizeof (char *))) == NULL) {
            fprintf(stderr, "icont: can't malloc for %s\n", av[i]);
            fflush(stderr);
            return;
            }
         (*argv)[k++] = salloc(buf);
         }
      }
   fclose(f);
   for( ; j < ac; j++, k++) (*argv)[k] = av[j];
}
#endif					/* ConsoleWindow */

#ifndef NTConsole
#ifdef MSWindows
void icont(int argc, char **argv);
#define int_PASCAL int PASCAL
#define LRESULT_CALLBACK LRESULT CALLBACK
#undef lstrlen
#include <windows.h>
#define lstrlen longstrlen
#include "../wincap/dibutil.h"

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

   while (*t2) {
      while (*t2 && isspace(*t2)) t2++;
      switch (*t2) {
	 case '\0': break;
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

LRESULT_CALLBACK WndProc	(HWND, UINT, WPARAM, LPARAM);
char *lognam;
char tmplognam[128];
extern FILE *flog;

void MSStartup(int argc, char **argv, HINSTANCE hInstance, HINSTANCE hPrevInstance)
   {
   WNDCLASS wc;

   /*
    * Select log file name.  Might make this a command-line option.
    * Default to "WICON.LOG".  The log file is used by Wi to report
    * translation errors and jump to the offending source code line.
    */
   if ((lognam = getenv("WICONLOG")) == NULL)
      lognam = "WICON.LOG";
   if (flog = fopen(lognam, "r")) {
      fclose(flog);
      flog = NULL;
      remove(lognam);
      }
   lognam = strdup(lognam);
   flog = fopen(tmpnam(tmplognam), "w");
   if (flog == NULL) {
      fprintf(stderr, "unable to open logfile");
      exit(EXIT_FAILURE);
      }

   if (!hPrevInstance) {
#if NT
      wc.style = CS_HREDRAW | CS_VREDRAW;
#else					/* NT */
      wc.style = 0;
#endif					/* NT */
#ifdef NTConsole
      wc.lpfnWndProc = DefWindowProc;
#else					/* NTConsole */
      wc.lpfnWndProc = WndProc;
#endif					/* NTConsole */
      wc.cbClsExtra = 0;
      wc.cbWndExtra = 0;
      wc.hInstance  = hInstance;
      wc.hIcon      = NULL;
      wc.hCursor    = LoadCursor(NULL, IDC_ARROW);
      wc.hbrBackground = GetStockObject(WHITE_BRUSH);
      wc.lpszMenuName = NULL;
      wc.lpszClassName = "iconx";
      RegisterClass(&wc);
      }
   }

HANDLE mswinInstance;
int ncmdShow;

jmp_buf mark_sj;

int_PASCAL WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
                   LPSTR lpszCmdParam, int nCmdShow)
   {
   int argc;
   char **argv;

   mswinInstance = hInstance;
   ncmdShow = nCmdShow;
   argc = CmdParamToArgv(GetCommandLine(), &argv);
   MSStartup(argc, argv, hInstance, hPrevInstance);
#if BORLAND_286
   _InitEasyWin();
#endif					/* BORLAND_286 */
   if (setjmp(mark_sj) == 0)
      icont(argc,argv);
   while (--argc>=0)
      free(argv[argc]);
   free(argv);
   wfreersc();
   return 0;
}

#define main icont
#endif					/* MSWindows */
#endif					/* NTConsole */

/*
 *  main program
 */
int main(argc, argv)
int argc;
char **argv;
   {
   int nolink = 0;			/* suppress linking? */
   int errors = 0;			/* translator and linker errors */
   char **tfiles, **tptr;		/* list of files to translate */
   char **lfiles, **lptr;		/* list of files to link */
   char **rfiles, **rptr;		/* list of files to remove */
   char *efile = NULL;			/* stderr file */
   char buf[MaxFileName];		/* file name construction buffer */
   int c, n;
   char ch;
   struct fileparts *fp;

#if WildCards
   FINDDATA_T fd;
   int j;
#endif					/* WildCards */

   if ((int)strlen(patchpath) > 18)
      iconxloc = patchpath+18;
   else {
      iconxloc = (char *)alloc((unsigned)strlen(refpath) + 8);
      strcpy(iconxloc, refpath);
#if NT
#ifdef MSWindows
      strcat(iconxloc, "w");		/* wiconx */
#else					/* MSWindows */
      strcat(iconxloc, "nt");		/* nticonx */
#endif					/* MSWindows */
#endif					/* NT */
      strcat(iconxloc, "iconx");
      }

#ifdef ConsoleWindow
   if ((int)strlen(patchpath) <= 18) {
      free(iconxloc);
      iconxloc = (char *)alloc((unsigned)strlen(refpath) + 7);
      strcpy(iconxloc, refpath);
      strcat(iconxloc, "wiconx");
      }
   expand_proj(&argc, &argv);
#endif					/* ConsoleWindow */

   /*
    * Process options.
    */
   while ((c = getopt(argc,argv,IconOptions)) != EOF)
      switch (c) {
         case 'C':			/* Ignore: compiler only */
            break;
         case 'E':			/* -E: preprocess only */
	    pponly = 1;
	    nolink = 1;
            break;

         case 'L':			/* -L: enable linker debugging */
#ifdef DeBugLinker
            Dflag = 1;
#endif					/* DeBugLinker */
            break;

         case 'S':			/* -S */
            fprintf(stderr, "Warning, -S option is obsolete\n");
            break;

#if MSDOS
         case 'X':			/* -X */
#if ZTC_386
            fprintf(stderr, "Warning: -X option is not available\n");
#else					/* ZTC_386 */
            makeExe = 1;
#endif					/* ZTC_386 */
            break;
         case 'I':			/* -C */
            makeExe = 0;
            break;
#ifdef SCCX_MX
         case 'A':
            makeExe = 2;
            break;
#endif                  /* SCCX_MX */
#endif					/* MSDOS */

         case 'c':			/* -c: compile only (no linking) */
            nolink = 1;
            break;
         case 'e':			/* -e file: redirect stderr */
            efile = optarg;
            break;
         case 'f':			/* -f features: enable features */
            if (strchr(optarg, 's') || strchr(optarg, 'a'))
               strinv = 1;		/* this is the only icont feature */
            break;
         case 'n':			/* Ignore: compiler only */
            break;
         case 'o':			/* -o file: name output file */
            ofile = optarg;
            break;

#ifdef ConsoleWindow
	 case 'q':
 	    ConsolePause = 0;
	    break;
#endif					/* ConsoleWindow */

         case 'r':			/* Ignore: compiler only */
            break;
         case 's':			/* -s: suppress informative messages */
            silent = 1;
            verbose = 0;
            break;
         case 't':			/* -t: turn on procedure tracing */
            trace = -1;
            break;
         case 'u':			/* -u: warn about undeclared ids */
            uwarn = 1;
            break;
         case 'v':			/* -v n: set verbosity level */
            if (sscanf(optarg, "%d%c", &verbose, &ch) != 1)
               quitf("bad operand to -v option: %s",optarg);
            if (verbose == 0)
               silent = 1;
            break;
         default:
         case 'x':			/* -x illegal until after file list */
            usage();
         }

#if MSDOS && !NT
      /*
       * Define pathToIconDOS as a global accessible from inside
       * separately-compiled compilation units.
       */
      if( makeExe ){
         char *pathCursor;

         strcpy (pathToIconDOS, argv[0]);
         pathCursor = (char *)strrchr (pathToIconDOS, '\\');
         if (!pathCursor) {
            fprintf (stderr,
               "Can't understand what directory icont was run from.\n");
            exit(EXIT_FAILURE);
            }
            strcpy( ++pathCursor, (makeExe==1) ?  "ixhdr.exe" : "iconx.exe");
         }
#endif                                  /* MSDOS && !NT */

   /*
    * Allocate space for lists of file names.
    */
   n = argc - optind + 1;

#if WildCards
   {
   for(j=optind; j < argc; j++) {
      if (FINDFIRST(argv[j], &fd))
         while(FINDNEXT(&fd)) n++;
      FINDCLOSE(&fd);
      }
   }
#endif					/* WildCards */

   tptr = tfiles = (char **)alloc((unsigned int)(n * sizeof(char *)));
   lptr = lfiles = (char **)alloc((unsigned int)(n * sizeof(char *)));
   rptr = rfiles = (char **)alloc((unsigned int)(2 * n * sizeof(char *)));

   /*
    * Scan file name arguments.
    */
   while (optind < argc)  {
      if (strcmp(argv[optind],"-x") == 0)	/* stop at -x */
         break;
      else if (strcmp(argv[optind],"-") == 0) {
         *tptr++ = "-";				/* "-" means standard input */
         *lptr++ = *rptr++ = "stdin.u1";
         *rptr++ = "stdin.u2";
         }
      else {
#if WildCards
	 char tmp[MaxPath], dir[MaxPath];
         int matches = 0;

	 fp = fparse(argv[optind]);
	 /* Save because *fp will get overwritten frequently */
	 strcpy(dir, fp->dir);
	 if (*fp->ext == '\0')
	    makename(tmp,NULL,argv[optind], SourceSuffix);
	 else
	    strcpy(tmp, argv[optind]);

	 if (strchr(tmp, '*') || strchr(tmp, '?')) {
	    if (!FINDFIRST(tmp, &fd)) {
	       fprintf(stderr, "File %s: no match\n", tmp);
	       fflush(stderr);
	       }
	    else matches = 1;
	    }
         do {
	 if (matches) {
	    makename(tmp,dir,FILENAME(&fd),NULL);
	    argv[optind] = tmp;
	    }
#endif					/* WildCards */
         fp = fparse(argv[optind]);		/* parse file name */
         if (*fp->ext == '\0' || smatch(fp->ext, SourceSuffix)) {
            makename(buf,SourceDir,argv[optind], SourceSuffix);
            *tptr++ = salloc(buf);		/* translate the .icn file */
            makename(buf,TargetDir,argv[optind],U1Suffix);
            *lptr++ = *rptr++ = salloc(buf);	/* link & remove .u1 */
            makename(buf,TargetDir,argv[optind],U2Suffix);
            *rptr++ = salloc(buf);		/* also remove .u2 */
            }
         else if (smatch(fp->ext,U1Suffix) || smatch(fp->ext,U2Suffix)
               || smatch(fp->ext,USuffix)) {
            makename(buf,TargetDir,argv[optind],U1Suffix);
            *lptr++ = salloc(buf);
            }
         else
            quitf("bad argument %s",argv[optind]);
#if WildCards
	 if (!matches)
	    break;
         } while (FINDNEXT(&fd));
	 if (matches)
	 FINDCLOSE(&fd);
#endif					/* WildCards */
         }
      optind++;
      }

   *tptr = *lptr = *rptr = NULL;	/* terminate filename lists */
   if (lptr == lfiles)
      usage();				/* error -- no files named */

   /*
    * Initialize globals.
    */
   initglob();

   /*
    * Translate .icn files to make .u1 and .u2 files.
    */
   if (tptr > tfiles) {
      if (!pponly)
         report("Translating");
      errors = trans(tfiles);
      if (errors > 0) {			/* exit if errors seen */
         exit(EXIT_FAILURE);
	 }
      }

   /*
    * Link .u1 and .u2 files to make an executable.
    */
   if (nolink) {			/* exit if no linking wanted */
      exit(EXIT_SUCCESS);
      }

#if MSDOS

#if NT
   {
   if (ofile == NULL)  {                /* if no -o file, synthesize a name */
      ofile = salloc(makename(buf,TargetDir,lfiles[0],
                              makeExe ? ".exe" : IcodeSuffix));
      }
   else {                             /* add extension if necessary */
      fp = fparse(ofile);
      if (*fp->ext == '\0' && *IcodeSuffix != '\0') /* if no ext given */
         ofile = salloc(makename(buf,NULL,ofile,
                                 makeExe ? ".exe" : IcodeSuffix));
      }
   }
#else					/* NT */
   if (ofile == NULL)  {                /* if no -o file, synthesize a name */
      ofile = salloc(makename(buf,TargetDir,lfiles[0],
                              makeExe ? ".Exe" : IcodeSuffix));
      }
   else {                             /* add extension if necessary */
      fp = fparse(ofile);
      if (*fp->ext == '\0' && *IcodeSuffix != '\0') /* if no ext given */
         ofile = salloc(makename(buf,NULL,ofile,
                                 makeExe ? ".Exe" : IcodeSuffix));
   }
#endif					/* NT */

#else                                   /* MSDOS */

   if (ofile == NULL)  {		/* if no -o file, synthesize a name */
      ofile = salloc(makename(buf,TargetDir,lfiles[0],IcodeSuffix));
   } else {				/* add extension in necessary */
      fp = fparse(ofile);
      if (*fp->ext == '\0' && *IcodeSuffix != '\0') /* if no ext given */
         ofile = salloc(makename(buf,NULL,ofile,IcodeSuffix));
   }

#endif					/* MSDOS */

   report("Linking");
   errors = ilink(lfiles,ofile);	/* link .u files to make icode file */

#if NT
   if (!stricmp(".exe", ofile+strlen(ofile)-4)) {
      FILE *f, *f2;
      char tmp[MaxPath], tmp2[MaxPath];
      /* if we generated a .exe, we need to rename it and then prepend wiconx
       */
      strcpy(tmp, ofile);
      strcpy(tmp+strlen(tmp)-4, ".bat");
      rename(ofile, tmp);
#ifdef MSWindows
      if ((f = pathOpen("wiconx.exe", ReadBinary)) == NULL) {
	 report("Tried to open wiconx to build .exe, but couldn't\n");
         errors++;
         }
#else					/* MS Windows */
      if ((f = pathOpen("nticonx.exe", ReadBinary)) == NULL) {
	 report("Tried to open wiconx to build .exe, but couldn't\n");
	 errors++;
         }
#endif					/* MS Windows */
      else {
         f2 = fopen(ofile, WriteBinary);
	 while ((c = fgetc(f)) != EOF) {
	    fputc(c, f2);
	    }
	 fclose(f);
	 if ((f = fopen(tmp, ReadBinary)) == NULL) {
	    report("tried to read .bat to append to .exe, but couldn't\n");
	    errors++;
	    }
	 else {
	    while ((c = fgetc(f)) != EOF) {
	       fputc(c, f2);
	       }
	    fclose(f);
	    }
	 fclose(f2);
	 unlink(tmp);
         }
      }
#endif					/* NT */

   /*
    * Finish by removing intermediate files.
    *  Execute the linked program if so requested and if there were no errors.
    */
   rmfiles(rfiles);			/* remove intermediate files */
   if (errors > 0) {			/* exit if linker errors seen */
      remove(ofile);
      exit(EXIT_FAILURE);
      }
#ifdef ConsoleWindow
   else
      report("No errors\n");
#endif					/* ConsoleWindow */

   if (optind < argc)  {
      report("Executing");
      execute (ofile, efile, argv+optind+1);
      }

   free(tfiles);
   free(lfiles);
   free(rfiles);

   exit(EXIT_SUCCESS);
   return 0;
   }

/*
 * execute - execute iconx to run the icon program
 */
static void execute(ofile,efile,args)
char *ofile, *efile, **args;
   {

   int n;
   char **argv, **p;

   for (n = 0; args[n] != NULL; n++)	/* count arguments */
      ;
   p = argv = (char **)alloc((unsigned int)((n + 5) * sizeof(char *)));

   *p++ = iconxloc;			/* set iconx pathname */
   if (efile != NULL) {			/* if -e given, copy it */
      *p++ = "-e";
      *p++ = efile;
      }
   *p++ = ofile;			/* pass icode file name */

#ifdef MSWindows
#ifndef NTConsole
   {
      char cmdline[256], *tmp;

      strcpy(cmdline, "wiconx ");
      if (efile != NULL) {
         strcat(cmdline, "-e ");
         strcat(cmdline, efile);
         strcat(cmdline, " ");
      }
   strcat(cmdline, ofile);
   strcat(cmdline, " ");
   while ((tmp = *args++) != NULL) {	/* copy args into argument vector */
      strcat(cmdline, tmp);
      strcat(cmdline, " ");
   }

   WinExec(cmdline, SW_SHOW);
   return;
   }
#endif					/* NTConsole */
#endif					/* MSWindows */

   while ((*p++ = *args++) != 0)      /* copy args into argument vector */
      ;
   *p = NULL;

   /*
    * Call iconx on the way out.
    */

#if MSDOS
      /* No special handling is needed for an .exe files, since iconx
       * recognizes it from the extension andfrom internal .exe data.
       */
#if MICROSOFT || TURBO || BORLAND_286 || BORLAND_386
      execvp(iconxloc,argv);	/* execute with path search */
#endif					/* MICROSOFT || ... */

#if INTEL_386 || ZTC_386 || HIGHC_386 || WATCOM || SCCX_MX
      fprintf(stderr,"-x not supported\n");
      fflush(stderr);
#endif					/* INTEL_386 || ... */
#endif					/* MSDOS */

   quitf("could not run %s",iconxloc);
   }

void report(s)
char *s;
   {
   char *c = (strchr(s, '\n') ? "" : ":\n") ;
   if (!silent)
      fprintf(stderr,"%s%s",s,c);
#ifdef ConsoleWindow
   else if (flog != NULL)
      fprintf(flog, "%s%s",s,c);
#endif					/* ConsoleWindow */
   }

/*
 * rmfiles - remove a list of files
 */

static void rmfiles(p)
char **p;
   {
   for (; *p; p++) {
      remove(*p);
      }
   }

/*
 * Print an error message if called incorrectly.  The message depends
 *  on the legal options for this system.
 */
static void usage()
   {
   fprintf(stderr,"usage: %s %s file ... [-x args]\n", progname, TUsage);
   exit(EXIT_FAILURE);
   }
