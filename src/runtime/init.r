/*
 * File: init.r
 * Initialization, termination, and such.
 * Contents: readhdr, init/icon_init, envset, env_err, env_int,
 *  fpe_trap, inttrag, segvtrap, error, syserr, c_exit, err,
 *  fatalerr, pstrnmcmp, datainit, [loadicode, savepstate, loadpstate]
 */

static void	env_err		(char *msg, char *name, char *val);
FILE		*pathOpen       (char *fname, char *mode);

#if !COMPILER
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
#endif					/* !COMPILER */

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

#ifndef MultiThread
   struct descrip k_main;	/* &main */
#endif					/* MultiThread */

int ixinited = 0;		/* set-up switch */

char *currend = NULL;		/* current end of memory region */


word qualsize = QualLstSize;	/* size of quallist for fixed regions */

word memcushion = RegionCushion;  /* memory region cushion factor */
word memgrowth = RegionGrowth;	  /* memory region growth factor */

uword stattotal = 0;		/* cumulative total static allocation */
#ifndef MultiThread
   uword strtotal = 0;		/* cumulative total string allocation */
   uword blktotal = 0;		/* cumulative total block allocation */
#endif					/* MultiThread */

int dodump;			/* if nonzero, core dump on error */
int noerrbuf;			/* if nonzero, do not buffer stderr */

struct descrip maps2;		/* second cached argument of map */
struct descrip maps3;		/* third cached argument of map */

#ifndef MultiThread
   struct descrip k_current;	/* current expression stack pointer */
   int k_errornumber = 0;	/* &errornumber */
   char *k_errortext = "";	/* &errortext */
   struct descrip k_errorvalue;	/* &errorvalue */
   int have_errval = 0;		/* &errorvalue has legal value */
   int t_errornumber = 0;	/* tentative k_errornumber value */
   int t_have_val = 0;		/* tentative have_errval flag */
   struct descrip t_errorvalue;	/* tentative k_errorvalue value */
#endif					/* MultiThread */

struct b_coexpr *stklist;	/* base of co-expression block list */

struct tend_desc *tend = NULL;  /* chain of tended descriptors */

struct region rootstring, rootblock;

#ifndef MultiThread
   dptr glbl_argp = NULL;	/* argument pointer */
   dptr globals, eglobals;	/* pointer to global variables */
   dptr gnames, egnames;	/* pointer to global variable names */
   dptr estatics;		/* pointer to end of static variables */
   struct region *curstring, *curblock;
   #if !COMPILER
      int n_globals = 0;	/* number of globals */
      int n_statics = 0;	/* number of statics */
   #endif				/* !COMPILER */
#endif					/* MultiThread */

#if COMPILER
   struct p_frame *pfp = NULL;	/* procedure frame pointer */

   int debug_info;		/* flag: is debugging information available */
   int err_conv;		/* flag: is error conversion supported */
   int largeints;		/* flag: large integers are supported */

   struct b_coexpr *mainhead;	/* &main */

#else					/* COMPILER */

   int debug_info=1;		/* flag: debugging information IS available */
   int err_conv=1;		/* flag: error conversion IS supported */

   int op_tbl_sz = (sizeof(init_op_tbl) / sizeof(struct b_proc));
   struct pf_marker *pfp = NULL;  /* Procedure frame pointer */

   #ifdef MultiThread
      struct progstate *curpstate;  /* lastop accessed in program state */
      struct progstate rootpstate;
   #else				/* MultiThread */

      struct b_coexpr *mainhead;  /* &main */

      char *code;		/* interpreter code buffer */
      char *ecode;		/* end of interpreter code buffer */
      word *records;		/* pointer to record procedure blocks */

      int *ftabp;		/* pointer to record/field table */

      #ifdef FieldTableCompression
         word ftabwidth;		/* field table entry width */
         word foffwidth;		/* field offset entry width */
         unsigned char *ftabcp, *focp;	/* pointers to record/field table */
         short *ftabsp, *fosp;		/* pointers to record/field table */

         int *fo;		/* field offset (row in field table) */
         char *bm;		/* bitmap array of valid field bits */
      #endif				/* FieldTableCompression */

      dptr fnames, efnames;	/* pointer to field names */
      dptr statics;		/* pointer to static variables */
      char *strcons;		/* pointer to string constant table */
      struct ipc_fname *filenms, *efilenms; /* pointer to ipc/file name table */
      struct ipc_line *ilines, *elines;	/* pointer to ipc/line number table */
   #endif				/* MultiThread */

   #ifdef TallyOpt
      word tallybin[16];	/* counters for tallying */
      int tallyopt = 0;		/* want tally results output? */
   #endif				/* TallyOpt */

   word *stack;			/* Interpreter stack */
   word *stackend;		/* End of interpreter stack */

#endif					/* COMPILER */

#if !COMPILER

/*
 * Open the icode file and read the header.
 * Used by icon_init() as well as MultiThread's loadicode()
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

#endif					/* !COMPILER */

/*
 * init/icon_init - initialize memory and prepare for Icon execution.
 */
#if !COMPILER
   struct header hdr;
#endif					/* !COMPILER */

#if COMPILER
   void init(name, argcp, argv, trc_init)
   char *name;
   int *argcp;
   char *argv[];
   int trc_init;
#else					/* COMPILER */
   void icon_init(name, argcp, argv)
   char *name;
   int *argcp;
   char *argv[];
#endif					/* COMPILER */

   {
   int delete_icode = 0;
#if !COMPILER
   FILE *fname = NULL;
   word cbread, longread();
#endif					/* COMPILER */

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
   {
      char *itval = getenv("ICODE_TEMP");
      int nlen = strlen(name);
      if (itval != NULL && itval[nlen] == ':' && strncmp(name,itval,nlen)==0) {
         delete_icode = 1;
         prog_name = itval + nlen + 1;
         }
      }

#if COMPILER
   curstring = &rootstring;
   curblock  = &rootblock;
   rootstring.size = MaxStrSpace;
   rootblock.size  = MaxAbrSize;
#else					/* COMPILER */

#ifdef MultiThread
   /*
    * initialize root pstate
    */
   curpstate = &rootpstate;
   rootpstate.parentdesc = nulldesc;
   rootpstate.eventmask= nulldesc;
   rootpstate.opcodemask = nulldesc;
   rootpstate.eventcode= nulldesc;
   rootpstate.eventval = nulldesc;
   rootpstate.eventsource = nulldesc;
   rootpstate.Glbl_argp = NULL;
   MakeInt(0, &(rootpstate.Kywd_err));
   MakeInt(1, &(rootpstate.Kywd_pos));
   StrLen(rootpstate.ksub) = 0;
   StrLoc(rootpstate.ksub) = "";
   MakeInt(hdr.trace, &(rootpstate.Kywd_trc));
   StrLen(rootpstate.Kywd_prog) = strlen(prog_name);
   StrLoc(rootpstate.Kywd_prog) = prog_name;
   MakeInt(0, &(rootpstate.Kywd_ran));
   rootpstate.K_errornumber = 0;
   rootpstate.T_errornumber = 0;
   rootpstate.Have_errval = 0;
   rootpstate.T_have_val = 0;
   rootpstate.K_errortext = "";
   rootpstate.K_errorvalue = nulldesc;
   rootpstate.T_errorvalue = nulldesc;

#ifdef Graphics
   MakeInt(0,&(rootpstate.AmperX));
   MakeInt(0,&(rootpstate.AmperY));
   MakeInt(0,&(rootpstate.AmperRow));
   MakeInt(0,&(rootpstate.AmperCol));
   MakeInt(0,&(rootpstate.AmperInterval));
   rootpstate.LastEventWin = nulldesc;
   rootpstate.Kywd_xwin[XKey_Window] = nulldesc;
#endif					/* Graphics */

   rootpstate.Coexp_ser = 2;
   rootpstate.List_ser  = 1;
   rootpstate.Set_ser   = 1;
   rootpstate.Table_ser = 1;
   rootpstate.stringregion = &rootstring;
   rootpstate.blockregion = &rootblock;

#else					/* MultiThread */

   curstring = &rootstring;
   curblock  = &rootblock;
#endif					/* MultiThread */

   rootstring.size = MaxStrSpace;
   rootblock.size  = MaxAbrSize;
#endif					/* COMPILER */

#if !COMPILER
   op_tbl = (struct b_proc*)init_op_tbl;
#endif					/* !COMPILER */

#ifdef Double
   if (sizeof(struct size_dbl) != sizeof(double))
      syserr("Icon configuration does not handle double alignment");
#endif					/* Double */

   /*
    * Catch floating-point traps and memory faults.
    */
   signal(SIGFPE, fpetrap);
   signal(SIGSEGV, segvtrap);

   /*
    * Initialize data that can't be initialized statically.
    */

   datainit();

   #if COMPILER
      IntVal(kywd_trc) = trc_init;
   #else				/* COMPILER */
      fname = readhdr(name,&hdr);
      if (fname == NULL)
         error(name, "cannot open interpreter file");
      k_trace = hdr.trace;
   #endif				/* COMPILER */

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
#if COMPILER
   initalloc();
#else					/* COMPILER */
#ifdef MultiThread
   initalloc(hdr.hsize,&rootpstate);
#else					/* MultiThread */
   initalloc(hdr.hsize);
#endif					/* MultiThread */
#endif					/* COMPILER */

#if !COMPILER
   /*
    * Establish pointers to icode data regions.		[[I?]]
    */
   ecode = code + hdr.Records;
   records = (word *)ecode;
   ftabp = (int *)(code + hdr.Ftab);
#ifdef FieldTableCompression
   fo = (int *)(code + hdr.Fo);
   focp = (unsigned char *)(fo);
   fosp = (short *)(fo);
   if (hdr.FoffWidth == 1) {
      bm = (char *)(focp + hdr.Nfields);
      }
   else if (hdr.FoffWidth == 2) {
      bm = (char *)(fosp + hdr.Nfields);
      }
   else
      bm = (char *)(fo + hdr.Nfields);

   ftabwidth = hdr.FtabWidth;
   foffwidth = hdr.FoffWidth;
   ftabcp = (unsigned char *)(code + hdr.Ftab);
   ftabsp = (short *)(code + hdr.Ftab);
#endif					/* FieldTableCompression */
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
#endif					/* COMPILER */

   /*
    * Allocate stack and initialize &main.
    */

#if COMPILER
   mainhead = (struct b_coexpr *)malloc(sizeof(struct b_coexpr));
#else					/* COMPILER */
   stack = (word *)malloc(mstksize);
   mainhead = (struct b_coexpr *)stack;

#endif					/* COMPILER */

   if (mainhead == NULL)
#if COMPILER
      err_msg(305, NULL);
#else					/* COMPILER */
      fatalerr(303, NULL);
#endif					/* COMPILER */

   mainhead->title = T_Coexpr;
   mainhead->id = 1;
   mainhead->size = 1;			/* pretend main() does an activation */
   mainhead->nextstk = NULL;
   mainhead->es_tend = NULL;
   mainhead->freshblk = nulldesc;	/* &main has no refresh block. */
					/*  This really is a bug. */
#ifdef MultiThread
   mainhead->program = &rootpstate;
#endif					/* MultiThread */
#if COMPILER
   mainhead->file_name = "";
   mainhead->line_num = 0;
#endif					/* COMPILER */

#ifdef Coexpr
   Protect(mainhead->es_actstk = alcactiv(), fatalerr(0,NULL));
   pushact(mainhead, mainhead);
#endif					/* Coexpr */

   /*
    * Point &main at the co-expression block for the main procedure and set
    *  k_current, the pointer to the current co-expression, to &main.
    */
   k_main.dword = D_Coexpr;
   BlkLoc(k_main) = (union block *) mainhead;
   k_current = k_main;

#if !COMPILER
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
      remove(name);

/*
 * Make sure the version number of the icode matches the interpreter version.
 */
   if (strcmp((char *)hdr.config,IVersion)) {
      fprintf(stderr,"icode version mismatch in %s\n", name);
      fprintf(stderr,"\ticode version: %s\n",(char *)hdr.config);
      fprintf(stderr,"\texpected version: %s\n",IVersion);
      error(name, "cannot run");
      }
#endif					/* !COMPILER */

   /*
    * Initialize the event monitoring system, if configured.
    */

#ifdef EventMon
   EVInit();
#endif					/* EventMon */

#if !COMPILER
   /*
    * Resolve references from icode to run-time system.
    */
#ifdef MultiThread
   resolve(NULL);
#else					/* MultiThread */
   resolve();
#endif					/* MultiThread */
#endif					/* COMPILER */

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
      fprintf(stderr, " in startup code");
   else {
#if COMPILER
      if (line_info)
	 fprintf(stderr, " at line %d in %s", line_num, file_name);
#else					/* COMPILER */
      fprintf(stderr, " at line %ld in %s", (long)findline(ipc.opnd),
	 findfile(ipc.opnd));
#endif					/* COMPILER */
      }
   fprintf(stderr, "\n%s\n", s);
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

#ifdef EventMon
   if (curpstate != NULL) {
      EVVal((word)i, E_Exit);
      }
#endif					/* EventMon */

#ifdef MultiThread
   if (curpstate != NULL && curpstate->parent != NULL) {
      /* might want to get to the lterm somehow, instead */
      while (1) {
	 struct descrip dummy;
	 co_chng(curpstate->parent->Mainhead, NULL, &dummy, A_Cofail, 1);
	 }
      }
#endif					/* MultiThread */

#ifdef TallyOpt
   {
   int j;

   if (tallyopt) {
      fprintf(stderr,"tallies: ");
      for (j=0; j<16; j++)
	 fprintf(stderr," %ld", (long)tallybin[j]);
	 fprintf(stderr,"\n");
	 }
      }
#endif					/* TallyOpt */

   if (k_dump && ixinited) {
      fprintf(stderr,"\nTermination dump:\n\n");
      fflush(stderr);
      fprintf(stderr,"co-expression #%ld(%ld)\n",
	 (long)BlkLoc(k_current)->coexpr.id,
	 (long)BlkLoc(k_current)->coexpr.size);
      fflush(stderr);
      xdisp(pfp,glbl_argp,k_level,stderr);
      }

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

#ifdef MultiThread
   k_errout.title = T_File;
   k_input.title = T_File;
   k_output.title = T_File;
#endif					/* MultiThread */

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

#ifdef EventMon
/*
 *  Initialization needed for event monitoring
 */

   BlkLoc(csetdesc) = (union block *)&fullcs;
   BlkLoc(rzerodesc) = (union block *)&realzero;

#endif					/* EventMon */

   maps2 = nulldesc;
   maps3 = nulldesc;

   #if !COMPILER
      qsort((char *)pntab,pnsize,sizeof(struct pstrnm), (int(*)())pstrnmcmp);
   #endif				/* COMPILER */

   }

#ifdef MultiThread
/*
 * loadicode - initialize memory particular to a given icode file
 */
struct b_coexpr * loadicode(name, theInput, theOutput, theError, bs, ss, stk)
char *name;
struct b_file *theInput, *theOutput, *theError;
C_integer bs, ss, stk;
   {
   struct b_coexpr *coexp;
   struct progstate *pstate;
   struct header hdr;
   FILE *fname = NULL;
   word cbread, longread();

   /*
    * open the icode file and read the header
    */
   fname = readhdr(name,&hdr);
   if (fname == NULL)
      return NULL;

   /*
    * Allocate memory for icode and the struct that describes it
    */
     Protect(coexp = alccoexp(hdr.hsize, stk),
      { fprintf(stderr,"can't malloc new icode region\n");c_exit(EXIT_FAILURE);});

   pstate = coexp->program;
   /*
    * Initialize values.
    */
   pstate->hsize = hdr.hsize;
   pstate->parent= NULL;
   pstate->parentdesc= nulldesc;
   pstate->opcodemask= nulldesc;
   pstate->eventmask= nulldesc;
   pstate->eventcode= nulldesc;
   pstate->eventval = nulldesc;
   pstate->eventsource = nulldesc;
   pstate->K_current.dword = D_Coexpr;

   MakeInt(0, &(pstate->Kywd_err));
   MakeInt(1, &(pstate->Kywd_pos));
   MakeInt(0, &(pstate->Kywd_ran));

   StrLen(pstate->Kywd_prog) = strlen(prog_name);
   StrLoc(pstate->Kywd_prog) = prog_name;
   StrLen(pstate->ksub) = 0;
   StrLoc(pstate->ksub) = "";
   MakeInt(hdr.trace, &(pstate->Kywd_trc));

#ifdef EventMon
   pstate->Linenum = pstate->Column = pstate->Lastline = pstate->Lastcol = 0;
#endif					/* EventMon */
   pstate->Lastop = 0;
   /*
    * might want to override from TRACE environment variable here.
    */

   /*
    * Establish pointers to icode data regions.		[[I?]]
    */
   pstate->Mainhead= ((struct b_coexpr *)pstate)-1;
   pstate->K_main.dword = D_Coexpr;
   BlkLoc(pstate->K_main) = (union block *) pstate->Mainhead;
   pstate->Code    = (char *)(pstate + 1);
   pstate->Ecode    = (char *)(pstate->Code + hdr.Records);
   pstate->Records = (word *)(pstate->Code + hdr.Records);
   pstate->Ftabp   = (int *)(pstate->Code + hdr.Ftab);
#ifdef FieldTableCompression
   pstate->Fo = (int *)(pstate->Code + hdr.Fo);
   pstate->Focp =   (unsigned char *)(pstate->Fo);
   pstate->Fosp =   (short *)(pstate->Fo);
   pstate->Foffwidth = hdr.FoffWidth;
   if (hdr.FoffWidth == 1) {
      pstate->Bm = (char *)(pstate->Focp + hdr.Nfields);
      }
   else if (hdr.FoffWidth == 2) {
      pstate->Bm = (char *)(pstate->Fosp + hdr.Nfields);
      }
   else
      pstate->Bm = (char *)(pstate->Fo + hdr.Nfields);
   pstate->Ftabwidth= hdr.FtabWidth;
   pstate->Foffwidth = hdr.FoffWidth;
   pstate->Ftabcp   = (unsigned char *)(pstate->Code + hdr.Ftab);
   pstate->Ftabsp   = (short *)(pstate->Code + hdr.Ftab);
#endif					/* FieldTableCompression */
   pstate->Fnames  = (dptr)(pstate->Code + hdr.Fnames);
   pstate->Globals = pstate->Efnames = (dptr)(pstate->Code + hdr.Globals);
   pstate->Gnames  = pstate->Eglobals = (dptr)(pstate->Code + hdr.Gnames);
   pstate->NGlobals = pstate->Eglobals - pstate->Globals;
   pstate->Statics = pstate->Egnames = (dptr)(pstate->Code + hdr.Statics);
   pstate->Estatics = (dptr)(pstate->Code + hdr.Filenms);
   pstate->NStatics = pstate->Estatics - pstate->Statics;
   pstate->Filenms = (struct ipc_fname *)(pstate->Estatics);
   pstate->Efilenms = (struct ipc_fname *)(pstate->Code + hdr.linenums);
   pstate->Ilines = (struct ipc_line *)(pstate->Efilenms);
   pstate->Elines = (struct ipc_line *)(pstate->Code + hdr.Strcons);
   pstate->Strcons = (char *)(pstate->Elines);
   pstate->K_errornumber = 0;
   pstate->T_errornumber = 0;
   pstate->Have_errval = 0;
   pstate->T_have_val = 0;
   pstate->K_errortext = "";
   pstate->K_errorvalue = nulldesc;
   pstate->T_errorvalue = nulldesc;

#ifdef Graphics
   MakeInt(0, &(pstate->AmperX));
   MakeInt(0, &(pstate->AmperY));
   MakeInt(0, &(pstate->AmperRow));
   MakeInt(0, &(pstate->AmperCol));
   MakeInt(0, &(pstate->AmperInterval));
   pstate->LastEventWin = nulldesc;
   pstate->Kywd_xwin[XKey_Window] = nulldesc;
#endif					/* Graphics */

   pstate->Coexp_ser = 2;
   pstate->List_ser = 1;
   pstate->Set_ser = 1;
   pstate->Table_ser = 1;

   pstate->stringtotal = pstate->blocktotal =
   pstate->colltot     = pstate->collstat   =
   pstate->collstr     = pstate->collblk    = 0;

   pstate->stringregion = (struct region *)malloc(sizeof(struct region));
   pstate->blockregion  = (struct region *)malloc(sizeof(struct region));
   pstate->stringregion->size = ss;
   pstate->blockregion->size = bs;

   /*
    * the local program region list starts out with this region only
    */
   pstate->stringregion->prev = NULL;
   pstate->blockregion->prev = NULL;
   pstate->stringregion->next = NULL;
   pstate->blockregion->next = NULL;
   /*
    * the global region list links this region with curpstate's
    */
   pstate->stringregion->Gprev = curpstate->stringregion;
   pstate->blockregion->Gprev = curpstate->blockregion;
   pstate->stringregion->Gnext = curpstate->stringregion->Gnext;
   pstate->blockregion->Gnext = curpstate->blockregion->Gnext;
   if (curpstate->stringregion->Gnext)
      curpstate->stringregion->Gnext->Gprev = pstate->stringregion;
   curpstate->stringregion->Gnext = pstate->stringregion;
   if (curpstate->blockregion->Gnext)
      curpstate->blockregion->Gnext->Gprev = pstate->blockregion;
   curpstate->blockregion->Gnext = pstate->blockregion;
   initalloc(0, pstate);

   pstate->K_errout = *theError;
   pstate->K_input  = *theInput;
   pstate->K_output = *theOutput;

   /*
    * Read the interpretable code and data into memory.
    */
   if ((cbread = longread(pstate->Code, sizeof(char), (long)hdr.hsize, fname))
       != hdr.hsize) {
      fprintf(stderr,"Tried to read %ld bytes of code, got %ld\n",
	(long)hdr.hsize,(long)cbread);
      error(name, "can't read interpreter code");
      }
   fclose(fname);

   /*
    * Make sure the version number of the icode matches the interpreter version
    */
   if (strcmp((char *)hdr.config,IVersion)) {
      fprintf(stderr,"icode version mismatch in %s\n", name);
      fprintf(stderr,"\ticode version: %s\n",(char *)hdr.config);
      fprintf(stderr,"\texpected version: %s\n",IVersion);
      error(name, "cannot run");
      }

   /*
    * Resolve references from icode to run-time system.
    * The first program has this done in icon_init after
    * initializing the event monitoring system.
    */
   resolve(pstate);

   return coexp;
   }
#endif					/* MultiThread */

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
