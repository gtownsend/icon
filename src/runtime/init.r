/*
 * File: init.r
 * Initialization, termination, and such.
 * Contents: readhdr, init/icon_init, envset, env_err, env_int,
 *  fpe_trap, inttrag, segvtrap, error, syserr, c_exit, err,
 *  fatalerr, pstrnmcmp, datainit, [loadicode, savepstate, loadpstate]
 */

static void	env_err		(char *msg, char *name, char *val);
FILE		*pathOpen       (char *fname, char *mode);

#include "../h/header.h"
static FILE *readhdr(char *name, struct header *hdr);

#passthru #define OpDef(p,n,s,u) int Cat(O,p) (dptr cargp);
#passthru #include "../h/odefs.h"
#passthru #undef OpDef

/*
 * External declarations for operator blocks.
 */

#passthru #define OpDef(f,nargs,sname,underef)\
	{\
	T_Proc,\
	Vsizeof(struct b_proc),\
	Cat(O,f),\
	nargs,\
	-1,\
	underef,\
	0,\
	{{sizeof(sname)-1,sname}}},
#passthru static B_IProc(2) init_op_tbl[] = {
#passthru #include "../h/odefs.h"
#passthru   };
#undef OpDef

#ifdef WinGraphics
   static void MSStartup(HINSTANCE hInstance, HINSTANCE hPrevInstance);
#endif					/* WinGraphics */

/*
 * A number of important variables follow.
 */

char *prog_name;		/* name of icode file */

int line_info;			/* flag: line information is available */
char *file_name = NULL;		/* source file for current execution point */
int line_num = 0;		/* line number for current execution point */
struct b_proc *op_tbl;		/* operators available for string invocation */

extern struct errtab errtab[];	/* error numbers and messages */

word mstksize = MStackSize;	/* initial size of main stack */
word stksize = StackSize;	/* co-expression stack size */

int k_level = 0;		/* &level */
struct descrip k_main;		/* &main */

int ixinited = 0;		/* set-up switch */

char *currend = NULL;		/* current end of memory region */


word qualsize = QualLstSize;	/* size of quallist for fixed regions */

word memcushion = RegionCushion;  /* memory region cushion factor */
word memgrowth = RegionGrowth;	  /* memory region growth factor */

uword stattotal = 0;		/* cumulative total static allocation */
uword strtotal = 0;		/* cumulative total string allocation */
uword blktotal = 0;		/* cumulative total block allocation */

int dodump;			/* if nonzero, core dump on error */
int noerrbuf;			/* if nonzero, do not buffer stderr */

struct descrip maps2;		/* second cached argument of map */
struct descrip maps3;		/* third cached argument of map */

struct descrip k_current;	/* current expression stack pointer */
int k_errornumber = 0;		/* &errornumber */
char *k_errortext = "";		/* &errortext */
struct descrip k_errorvalue;	/* &errorvalue */
int have_errval = 0;		/* &errorvalue has legal value */
int t_errornumber = 0;		/* tentative k_errornumber value */
int t_have_val = 0;		/* tentative have_errval flag */
struct descrip t_errorvalue;	/* tentative k_errorvalue value */

struct b_coexpr *stklist;	/* base of co-expression block list */

struct tend_desc *tend = NULL;  /* chain of tended descriptors */

struct region rootstring, rootblock;

dptr glbl_argp = NULL;		/* argument pointer */
dptr globals, eglobals;		/* pointer to global variables */
dptr gnames, egnames;		/* pointer to global variable names */
dptr estatics;			/* pointer to end of static variables */
struct region *curstring, *curblock;
int n_globals = 0;		/* number of globals */
int n_statics = 0;		/* number of statics */

int debug_info=1;		/* flag: debugging information IS available */
int err_conv=1;			/* flag: error conversion IS supported */

int op_tbl_sz = (sizeof(init_op_tbl) / sizeof(struct b_proc));
struct pf_marker *pfp = NULL;  /* Procedure frame pointer */

   struct b_coexpr *mainhead;  /* &main */

   char *code;			/* interpreter code buffer */
   char *ecode;			/* end of interpreter code buffer */
   word *records;		/* pointer to record procedure blocks */
   int *ftabp;			/* pointer to record/field table */
   dptr fnames, efnames;	/* pointer to field names */
   dptr statics;		/* pointer to static variables */
   char *strcons;		/* pointer to string constant table */
   struct ipc_fname *filenms, *efilenms; /* pointer to ipc/file name table */
   struct ipc_line *ilines, *elines;	/* pointer to ipc/line number table */

word *stack;			/* Interpreter stack */
word *stackend;			/* End of interpreter stack */

/*
 * Open the icode file and read the header.
 * Used by icon_init().
 */
static FILE *readhdr(name,hdr)
char *name;
struct header *hdr;
   {
   FILE *fname = NULL;
   int n;
   struct fileparts fp;

   if (!name)
      error(name, "No interpreter file supplied");

   /*
    * Try adding the suffix if the file name doesn't end in it.
    */
   n = strlen(name);
   fp = *fparse(name);

   if ( IcodeSuffix[0] != '\0' && strcmp(fp.ext,IcodeSuffix) != 0
   && ( IcodeASuffix[0] == '\0' || strcmp(fp.ext,IcodeASuffix) != 0 ) ) {
      char tname[100], ext[50];
      if (n + strlen(IcodeSuffix) + 1 > 100)
	 error(name, "icode file name too long");
      strcpy(ext,fp.ext);
      strcat(ext,IcodeSuffix);
      makename(tname,NULL,name,ext);

      #if MSWIN
	  fname = pathOpen(tname,"rb");	/* try to find path */
      #else				/* MSWIN */
	  fname = fopen(tname, "rb");
      #endif				/* MSWIN */

      }

   if (fname == NULL)			/* try the name as given */
      #if MSWIN
         fname = pathOpen(name, "rb");
      #else				/* MSWIN */
         fname = fopen(name, "rb");
      #endif				/* MSWIN */

   if (fname == NULL)
      return NULL;

   {
   static char errmsg[] = "can't read interpreter file header";

#ifdef BinaryHeader
   if (fseek(fname, (long)MaxHdr, 0) == -1)
      error(name, errmsg);
#else					/* BinaryHeader */
   char buf[200];

   for (;;) {
      if (fgets(buf, sizeof buf-1, fname) == NULL)
	 error(name, errmsg);
         if (strncmp(buf, "[executable Icon binary follows]", 32) == 0)
            break;
      }

   while ((n = getc(fname)) != EOF && n != '\f')	/* read thru \f\n\0 */
      ;
   getc(fname);
   getc(fname);
#endif					/* BinaryHeader */

   if (fread((char *)hdr, sizeof(char), sizeof(*hdr), fname) != sizeof(*hdr))
      error(name, errmsg);
   }

   return fname;
   }

/*
 * init/icon_init - initialize memory and prepare for Icon execution.
 */
struct header hdr;

void icon_init(name, argcp, argv)
char *name;
int *argcp;
char *argv[];
   {
   char *itval;
   int delete_icode = 0;
   FILE *fname = NULL;
   word cbread, longread();

   prog_name = name;			/* Set icode file name */

#ifdef WinGraphics
   {
   STARTUPINFO si;

   /*
    * Initialize windows stuff.
    */
   GetStartupInfo(&si);
   ncmdShow = si.wShowWindow;
   if ( ncmdShow == SW_HIDE )
      /* Started from command line, show normal windows in this case. */
      ncmdShow = SW_SHOWNORMAL;
   mswinInstance = GetModuleHandle( NULL );
   MSStartup( mswinInstance, NULL );
   }
#endif					/* WinGraphics */

   /*
    * Look for environment variable ICODE_TEMP=xxxxx:yyyyy as a message
    * from icont to delete icode file xxxxx and to use yyyyy for &progname.
    * (This is used with Unix "#!" script files written in Icon.)
    */
   itval = getenv("ICODE_TEMP");
   if (itval != NULL && strncmp(name, itval, strlen(name)) == 0) {
      delete_icode = 1;
      prog_name = strchr(itval, ':') + 1;
      prog_name[-1] = '\0';
      }

   curstring = &rootstring;
   curblock  = &rootblock;
   rootstring.size = MaxStrSpace;
   rootblock.size  = MaxAbrSize;
   op_tbl = (struct b_proc*)init_op_tbl;

#ifdef Double
   if (sizeof(struct size_dbl) != sizeof(double))
      syserr("Icon configuration does not handle double alignment");
#endif					/* Double */

   /*
    * Catch floating-point traps, memory faults, and QUIT (^\) signals.
    */
   signal(SIGFPE, fpetrap);
   signal(SIGSEGV, segvtrap);
   signal(SIGQUIT, quittrap);

   /*
    * Initialize data that can't be initialized statically.
    */
   datainit();

   /*
    * Read the header.
    */
   fname = readhdr(name,&hdr);
   if (fname == NULL)
      error(name, "cannot open interpreter file");
   k_trace = hdr.trace;
   if (hdr.magic != IHEADER_MAGIC)	/* if flags word is not valid */
      hdr.flags = 0;			/* clear it out */

   /*
    * Examine the environment and make appropriate settings.    [[I?]]
    */
   envset();

   /*
    * Convert stack sizes from words to bytes.
    */
   stksize *= WordSize;
   mstksize *= WordSize;

   /*
    * Allocate memory for various regions.
    */
   initalloc(hdr.hsize);

   /*
    * Establish pointers to icode data regions.		[[I?]]
    */
   ecode = code + hdr.Records;
   records = (word *)ecode;
   ftabp = (int *)(code + hdr.Ftab);
   fnames = (dptr)(code + hdr.Fnames);
   globals = efnames = (dptr)(code + hdr.Globals);
   gnames = eglobals = (dptr)(code + hdr.Gnames);
   statics = egnames = (dptr)(code + hdr.Statics);
   estatics = (dptr)(code + hdr.Filenms);
   filenms = (struct ipc_fname *)estatics;
   efilenms = (struct ipc_fname *)(code + hdr.linenums);
   ilines = (struct ipc_line *)efilenms;
   elines = (struct ipc_line *)(code + hdr.Strcons);
   strcons = (char *)elines;
   n_globals = eglobals - globals;
   n_statics = estatics - statics;

   /*
    * Allocate stack and initialize &main.
    */
   stack = (word *)malloc(mstksize);
   mainhead = (struct b_coexpr *)stack;
   if (mainhead == NULL)
      fatalerr(303, NULL);

   mainhead->title = T_Coexpr;
   mainhead->id = 1;
   mainhead->size = 1;			/* pretend main() does an activation */
   mainhead->nextstk = NULL;
   mainhead->es_tend = NULL;
   mainhead->freshblk = nulldesc;	/* &main has no refresh block. */
					/*  This really is a bug. */
   Protect(mainhead->es_actstk = alcactiv(), fatalerr(0,NULL));
   pushact(mainhead, mainhead);

   /*
    * Point &main at the co-expression block for the main procedure and set
    *  k_current, the pointer to the current co-expression, to &main.
    */
   k_main.dword = D_Coexpr;
   BlkLoc(k_main) = (union block *) mainhead;
   k_current = k_main;

   /*
    * Read the interpretable code and data into memory.
    */
   if ((cbread = longread(code, sizeof(char), (long)hdr.hsize, fname)) !=
      hdr.hsize) {
      fprintf(stderr,"Tried to read %ld bytes of code, got %ld\n",
	(long)hdr.hsize,(long)cbread);
      error(name, "bad icode file");
      }
   fclose(fname);
   if (delete_icode)		/* delete icode file if flag set earlier */
      remove(itval);

   /*
    * Make sure the version number of the icode matches the interpreter version.
    */
   if (strcmp((char *)hdr.iversion,IVersion)) {
      fprintf(stderr,"icode version mismatch in %s\n", name);
      fprintf(stderr,"\ticode version: %s\n",(char *)hdr.iversion);
      fprintf(stderr,"\texpected version: %s\n",IVersion);
      error(name, "cannot run");
      }

   /*
    * Resolve references from icode to run-time system.
    */
   resolve();

   /*
    * Allocate and assign a buffer to stderr if possible.
    */
   if (noerrbuf)
      setbuf(stderr, NULL);
   else {
      void *buf = malloc(BUFSIZ);
      if (buf == NULL)
         fatalerr(305, NULL);
      setbuf(stderr, buf);
      }

   /*
    * Initialize the profiling system, if configured.
    */
   if (hdr.flags & IHEADER_PROFILING)
      initprofile();

   /*
    * Start timing execution.
    */
   millisec();
   }

/*
 * Service routines related to getting things started.
 */


/*
 * Check for environment variables that Icon uses and set system
 *  values as is appropriate.
 */
void envset()
   {
   register char *p;

   if ((p = getenv("NOERRBUF")) != NULL)
      noerrbuf++;
   env_int("TRACE", &k_trace, 0, (uword)0);
   env_int("COEXPSIZE", &stksize, 1, (uword)MaxUnsigned);
   env_int("STRSIZE", &ssize, 1, (uword)MaxBlock);
   env_int("BLKSIZE", &abrsize, 1, (uword)MaxBlock);
   env_int("MSTKSIZE", &mstksize, 1, (uword)MaxUnsigned);
   env_int("QLSIZE", &qualsize, 1, (uword)MaxBlock);
   env_int("IXCUSHION", &memcushion, 1, (uword)100);	/* max 100 % */
   env_int("IXGROWTH", &memgrowth, 1, (uword)10000);	/* max 100x growth */

   if ((p = getenv("ICONCORE")) != NULL && *p != '\0') {
      /*
       * ICONCORE is set.  Reset traps to allow dump after abnormal termination.
       */
      dodump++;
      signal(SIGFPE, SIG_DFL);
      signal(SIGSEGV, SIG_DFL);
      }
   }

/*
 * env_err - print an error mesage about the value of an environment
 *  variable.
 */
static void env_err(msg, name, val)
char *msg;
char *name;
char *val;
{
   char msg_buf[100];

   strncpy(msg_buf, msg, 99);
   strncat(msg_buf, ": ", 99 - (int)strlen(msg_buf));
   strncat(msg_buf, name, 99 - (int)strlen(msg_buf));
   strncat(msg_buf, "=", 99 - (int)strlen(msg_buf));
   strncat(msg_buf, val, 99 - (int)strlen(msg_buf));
   error("", msg_buf);
}

/*
 * env_int - get the value of an integer-valued environment variable.
 */
void env_int(name, variable, non_neg, limit)
char *name;
word *variable;
int non_neg;
uword limit;
{
   char *value;
   char *s;
   register uword n = 0;
   register uword d;
   int sign = 1;

   if ((value = getenv(name)) == NULL || *value == '\0')
      return;

   s = value;
   if (*s == '-') {
      if (non_neg)
	 env_err("environment variable out of range", name, value);
      sign = -1;
      ++s;
      }
   else if (*s == '+')
      ++s;
   while (isdigit(*s)) {
      d = *s++ - '0';
      /*
       * See if 10 * n + d > limit, but do it so there can be no overflow.
       */
      if ((d > (uword)(limit / 10 - n) * 10 + limit % 10) && (limit > 0))
	 env_err("environment variable out of range", name, value);
      n = n * 10 + d;
      }
   if (*s != '\0')
      env_err("environment variable not numeric", name, value);
   *variable = sign * n;
}

/*
 * Termination routines.
 */

/*
 * Produce run-time error 204 on floating-point traps.
 */

void fpetrap(int sig)
   {
   fatalerr(204, NULL);
   }

/*
 * Produce run-time error 300 on QUIT (^\) signal.
 */

void quittrap(int sig)
   {
   fatalerr(300, NULL);
   }

/*
 * Produce run-time error 302 on segmentation faults.
 */
void segvtrap(int sig)
   {
   static int n = 0;

   if (n != 0) {			/* only try traceback once */
      fprintf(stderr, "[Traceback failed]\n");
      exit(1);
      }
   n++;
   fatalerr(302, NULL);
   exit(1);
   }

/*
 * error - print error message from s1 and s2; used only in startup code.
 */
void error(s1, s2)
char *s1, *s2;
   {
   if (!s1)
      fprintf(stderr, "error in startup code\n%s\n", s2);
   else
      fprintf(stderr, "error in startup code\n%s: %s\n", s1, s2);
   fflush(stderr);
   if (dodump)
      abort();
   c_exit(EXIT_FAILURE);
   }

/*
 * syserr - print s as a system error.
 */
void syserr(s)
char *s;
   {
   fprintf(stderr, "System error");
   if (pfp == NULL)
      fprintf(stderr, " in startup code\n");
   else {
      fprintf(stderr, " at line %ld in %s\n", (long)findline(ipc.opnd),
	 findfile(ipc.opnd));
      }
   if (s != NULL)
      fprintf(stderr, "%s\n", s);
   fflush(stderr);
   if (dodump)
      abort();
   c_exit(EXIT_FAILURE);
   }

/*
 * c_exit(i) - flush all buffers and exit with status i.
 */
void c_exit(i)
int i;
{

   if (k_dump && ixinited) {
      fprintf(stderr,"\nTermination dump:\n\n");
      fflush(stderr);
      fprintf(stderr,"co-expression #%ld(%ld)\n",
	 (long)BlkLoc(k_current)->coexpr.id,
	 (long)BlkLoc(k_current)->coexpr.size);
      fflush(stderr);
      xdisp(pfp,glbl_argp,k_level,stderr);
      }
   genprofile();		/* report execution profile, if enabled */
   exit(i);

}

/*
 * err() is called if an erroneous situation occurs in the virtual
 *  machine code.  It is typed as int to avoid declaration problems
 *  elsewhere.
 */
int err()
{
   syserr("call to 'err'\n");
   return 1;		/* unreachable; make compilers happy */
}

/*
 * fatalerr - disable error conversion and call run-time error routine.
 */
void fatalerr(n, v)
int n;
dptr v;
   {
   IntVal(kywd_err) = 0;
   err_msg(n, v);
   }

/*
 * pstrnmcmp - compare names in two pstrnm structs; used for qsort.
 */
int pstrnmcmp(a,b)
struct pstrnm *a, *b;
{
  return strcmp(a->pstrep, b->pstrep);
}

/*
 * datainit - initialize some global variables.
 */
void datainit()
   {

   /*
    * Initializations that cannot be performed statically (at least for
    * some compilers).					[[I?]]
    */

   k_errout.fd = stderr;
   StrLen(k_errout.fname) = 7;
   StrLoc(k_errout.fname) = "&errout";
   k_errout.status = Fs_Write;

   if (k_input.fd == NULL)
      k_input.fd = stdin;
   StrLen(k_input.fname) = 6;
   StrLoc(k_input.fname) = "&input";
   k_input.status = Fs_Read;

   if (k_output.fd == NULL)
      k_output.fd = stdout;
   StrLen(k_output.fname) = 7;
   StrLoc(k_output.fname) = "&output";
   k_output.status = Fs_Write;

   IntVal(kywd_pos) = 1;
   IntVal(kywd_ran) = 0;
   StrLen(kywd_prog) = strlen(prog_name);
   StrLoc(kywd_prog) = prog_name;
   StrLen(k_subject) = 0;
   StrLoc(k_subject) = "";

#ifdef MSwindows
   if (i != EXIT_SUCCESS)
   {
      char exit_msg[40];

      sprintf(exit_msg, "Terminated with exit code %d", i);
      MessageBox(NULL, exit_msg, prog_name, MB_OK | MB_ICONSTOP);
   }
#endif					/* defined(MSwindows) */

   StrLen(blank) = 1;
   StrLoc(blank) = " ";
   StrLen(emptystr) = 0;
   StrLoc(emptystr) = "";
   BlkLoc(nullptr) = (union block *)NULL;
   StrLen(lcase) = 26;
   StrLoc(lcase) = "abcdefghijklmnopqrstuvwxyz";
   StrLen(letr) = 1;
   StrLoc(letr) = "r";
   IntVal(nulldesc) = 0;
   k_errorvalue = nulldesc;
   IntVal(onedesc) = 1;
   StrLen(ucase) = 26;
   StrLoc(ucase) = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
   IntVal(zerodesc) = 0;

   maps2 = nulldesc;
   maps3 = nulldesc;

   qsort((char *)pntab,pnsize,sizeof(struct pstrnm), (int(*)())pstrnmcmp);
   }

#ifdef WinGraphics
static void MSStartup(HINSTANCE hInstance, HINSTANCE hPrevInstance)
   {
   WNDCLASS wc;
   if (!hPrevInstance) {
      wc.style = CS_HREDRAW | CS_VREDRAW;
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
#endif					/* WinGraphics */
