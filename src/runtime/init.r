/*
 * File: init.r
 * Initialization, termination, and such.
 * Contents: readhdr, init/icon_init, envset, env_err, env_int,
 *  fpe_trap, inttrag, segvtrap, error, syserr, c_exit, err,
 *  fatalerr, pstrnmcmp, datainit, [loadicode, savepstate, loadpstate]
 */

#if !COMPILER
#include "../h/header.h"

static	FILE	*readhdr	(char *name, struct header *hdr);
#endif					/* !COMPILER */

#if SCCX_MX
extern  int         thisIsIconx;
extern  char        settingsname[];
extern  setint_t    sizevar;
#endif  /* SCCX_MX */

/*
 * Prototypes.
 */

static void	env_err		(char *msg, char *name, char *val);
FILE		*pathOpen       (char *fname, char *mode);

/*
 * The following code is operating-system dependent [@init.01].  Declarations
 *   that are system-dependent.
 */

#if PORT
   /* probably needs something more */
Deliberate Syntax Error
#endif					/* PORT */

#if AMIGA
int chkbreak;				/* if nonzero, check for ^C */
  /* These override environment variables if set from ToolTypes. */
uword WBstrsize = 0;
uword WBblksize = 0;
uword WBmstksize = 0;
#endif					/* AMIGA */

#if MSDOS
#if HIGHC_386
int _fmode = 0;				/* force CR-LF on std.. files */
#endif					/* HIGHC_386 */
#endif					/* MSDOS */

#if OS2

char modname[256];			/* Character string for module name */
#passthru HMODULE modhandle;		/* Handle of loaded module */
char loadmoderr[256];			/* Error message if loadmodule fails */
#define RT_ICODE 0x4843 		/* Resource type id is 'IC' */
unsigned long icoderesid;		/* Resource ID from caller */
char *icoderes; 			/* Pointer to the icode resource data */
int use_resource = 0;			/* Set to TRUE if using a resource */
int stubexe;				/* TRUE if resource attached to executable */
#endif					/* OS2 */

#if ARM || ATARI_ST || MACINTOSH || MVS || VM || UNIX || VMS
   /* nothing needed */
#endif					/* ARM || ATARI_ST || MACINTOSH ... */

/*
 * End of operating-system specific code.
 */

char *prog_name;			/* name of icode file */

#if !COMPILER
#define OpDef(p,n,s,u) int Cat(O,p) (dptr cargp);
#include "../h/odefs.h"
#undef OpDef

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

/*
 * A number of important variables follow.
 */

int line_info;				/* flag: line information is available */
char *file_name = NULL;			/* source file for current execution point */
int line_num = 0;			/* line number for current execution point */
struct b_proc *op_tbl;			/* operators available for string invocation */

extern struct errtab errtab[];		/* error numbers and messages */

word mstksize = MStackSize;		/* initial size of main stack */
word stksize = StackSize;		/* co-expression stack size */

int k_level = 0;			/* &level */

#ifndef MultiThread
struct descrip k_main;			/* &main */
#endif					/* MultiThread */

int set_up = 0;				/* set-up switch */

char *currend = NULL;			/* current end of memory region */


word qualsize = QualLstSize;		/* size of quallist for fixed regions */

word memcushion = RegionCushion;	/* memory region cushion factor */
word memgrowth = RegionGrowth;		/* memory region growth factor */

uword stattotal = 0;			/* cumulative total static allocation */
#ifndef MultiThread
uword strtotal = 0;			/* cumulative total string allocation */
uword blktotal = 0;			/* cumulative total block allocation */
#endif					/* MultiThread */

int dodump;				/* if nonzero, core dump on error */
int noerrbuf;				/* if nonzero, do not buffer stderr */

struct descrip maps2;			/* second cached argument of map */
struct descrip maps3;			/* third cached argument of map */

#ifndef MultiThread
struct descrip k_current;		/* current expression stack pointer */
int k_errornumber = 0;			/* &errornumber */
char *k_errortext = "";			/* &errortext */
struct descrip k_errorvalue;		/* &errorvalue */
int have_errval = 0;			/* &errorvalue has legal value */
int t_errornumber = 0;			/* tentative k_errornumber value */
int t_have_val = 0;			/* tentative have_errval flag */
struct descrip t_errorvalue;		/* tentative k_errorvalue value */
#endif					/* MultiThread */

struct b_coexpr *stklist;	/* base of co-expression block list */

struct tend_desc *tend = NULL;  /* chain of tended descriptors */

struct region rootstring, rootblock;

#ifndef MultiThread
dptr glbl_argp = NULL;		/* argument pointer */
dptr globals, eglobals;			/* pointer to global variables */
dptr gnames, egnames;			/* pointer to global variable names */
dptr estatics;				/* pointer to end of static variables */

struct region *curstring, *curblock;
#endif					/* MultiThread */

#if COMPILER
struct p_frame *pfp = NULL;	/* procedure frame pointer */

int debug_info;				/* flag: is debugging information available */
int err_conv;				/* flag: is error conversion supported */
int largeints;				/* flag: large integers are supported */

struct b_coexpr *mainhead;		/* &main */

#else					/* COMPILER */

int debug_info=1;			/* flag: debugging information IS available */
int err_conv=1;				/* flag: error conversion IS supported */

int op_tbl_sz = (sizeof(init_op_tbl) / sizeof(struct b_proc));
struct pf_marker *pfp = NULL;		/* Procedure frame pointer */

#ifndef MaxHeader
#define MaxHeader MaxHdr
#endif					/* MaxHeader */

#ifdef MultiThread
struct progstate *curpstate;		/* lastop accessed in program state */
struct progstate rootpstate;
#else					/* MultiThread */

struct b_coexpr *mainhead;		/* &main */

char *code;				/* interpreter code buffer */
char *ecode;				/* end of interpreter code buffer */
word *records;				/* pointer to record procedure blocks */

int *ftabp;				/* pointer to record/field table */
#ifdef FieldTableCompression
word ftabwidth;				/* field table entry width */
word foffwidth;				/* field offset entry width */
unsigned char *ftabcp, *focp;		/* pointers to record/field table */
short *ftabsp, *fosp;			/* pointers to record/field table */

int *fo;				/* field offset (row in field table) */
char *bm;				/* bitmap array of valid field bits */
#endif					/* FieldTableCompression */

dptr fnames, efnames;			/* pointer to field names */
dptr statics;				/* pointer to static variables */
char *strcons;				/* pointer to string constant table */
struct ipc_fname *filenms, *efilenms;	/* pointer to ipc/file name table */
struct ipc_line *ilines, *elines;	/* pointer to ipc/line number table */
#endif					/* MultiThread */



#ifdef TallyOpt
word tallybin[16];			/* counters for tallying */
int tallyopt = 0;			/* want tally results output? */
#endif					/* TallyOpt */

#ifdef ExecImages
int dumped = 0;				/* non-zero if reloaded from dump */
#endif					/* ExecImages */

word *stack;				/* Interpreter stack */
word *stackend; 			/* End of interpreter stack */


#ifdef MultipleRuns
extern word coexp_ser;
extern word list_ser;
extern word set_ser;
extern word table_ser;
extern int first_time;
#endif					/* MultipleRuns */
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

#if MSDOS
   int thisIsAnExeFile = 0;
   char bytesThatBeginEveryExe[2] = {0,0};
   unsigned short originalExeBytesMod512, originalExePages;
   unsigned long originalExeBytes;
#if SCCX_MX
   char drive[260];
   char dir[260];
   char file[260];
   char ext[260];
   FILE*   setPtr;
   int     i, c;
#endif                                  /* SCCX_MX */
#endif					/* MSDOS */

   if (!name)

#ifdef PresentationManager
      error(NULL, "An icode file was not specified.\nExecution can't proceed.");
#else					/* PresentationManager */
      error(name, "No interpreter file supplied");
#endif					/* PresentationManager */

   /*
    * Try adding the suffix if the file name doesn't end in it.
    */
   n = strlen(name);

#if MSDOS
#if ZTC_386
   if (n >= 4 && !strcmp(".exe", name + n - 4)) {
#else					/* ZTC_386 */
   if (n >= 4 && !stricmp(".exe", name + n - 4)) {
#endif					/* ZTC_386 */
      thisIsAnExeFile = 1;
      fname = pathOpen(name, ReadBinary);
         /*
          * ixhdr's code for calling iconx from an .exe passes iconx the
          * full path of the .exe, so using pathOpen() seems redundant &
          * potentially inefficient. However, pathOpen() first checks for a
          * complete path, & if one is present, doesn't search Path; & since
          * MS-DOS has a limited line length, it'd be possible for ixhdr
          * to check whether the full path will fit, & if not, use only the
          * name. The only price for this additional robustness would be
          * the time pathOpen() spends checking for a path, which is trivial.
          */
      }
   else {
#endif					/* MSDOS */

   if (n <= 4 || (strcmp(name+n-4,IcodeSuffix) != 0
   && strcmp(name+n-4,IcodeASuffix) != 0)) {
      char tname[100];
      if ((int)strlen(name) + 5 > 100)
	 error(name, "icode file name too long");
      strcpy(tname,name);

#if MVS
   {
      char *p;
      if (p = strchr(name, '(')) {
	 tname[p-name] = '\0';
      }
#endif					/* MVS */

      strcat(tname,IcodeSuffix);

#if MVS
      if (p) strcat(tname,p);
   }
#endif					/* MVS */

#if MSDOS || OS2
      fname = pathOpen(tname,ReadBinary);	/* try to find path */
#else					/* MSDOS || OS2 */
      fname = fopen(tname, ReadBinary);
#endif					/* MSDOS || OS2 */

#if NT
    /*
     * tried appending .exe, now try .bat or .cmd
     */
    if (fname == NULL) {
       strcpy(tname,name);
       strcat(tname,".bat");
       fname = pathOpen(tname, ReadBinary);
       if (fname == NULL) {
          strcpy(tname,name);
          strcat(tname,".cmd");
          fname = pathOpen(tname, ReadBinary);
          }
      }
#endif					/* NT */

      }

   if (fname == NULL)			/* try the name as given */

#if MSDOS || OS2
      fname = pathOpen(name, ReadBinary);
#else					/* MSDOS || OS2 */
      fname = fopen(name, ReadBinary);
#endif					/* MSDOS || OS2 */

#if MSDOS
      } /* end if (n >= 4 && !stricmp(".exe", name + n - 4)) */
#endif					/* MSDOS */

   if (fname == NULL)
      return NULL;

   {
   static char errmsg[] = "can't read interpreter file header";

#ifdef Header

#if MSDOS && !NT
   #error
   deliberate syntax error

  /*
   * The MSDOS .exe-handling code assumes & requires that the executable
   * .exe be followed immediately by the icode itself (actually header.h).
   * This is because the following Header fseek() is relative to the
   * beginning of the file, which in a .exe is the beginning of the
   * executable code, not the beginning of some Icon thing; & I can't
   * check & fix all the Header-handling logic because hdr.h wasn't
   * included with my MS-DOS distribution so I don't even know what it does,
   * let alone how to keep from breaking it. We're safe as long as
   * Header & MSDOS are disjoint.
   */
#endif                                  /* MSDOS && !NT */

#ifdef ShellHeader
   char buf[200];

   for (;;) {
      if (fgets(buf, sizeof buf-1, fname) == NULL)
	 error(name, errmsg);
#if NT
      if (strncmp(buf, "rem [executable Icon binary follows]", 36) == 0)
#else					/* NT */
      if (strncmp(buf, "[executable Icon binary follows]", 32) == 0)
#endif					/* NT */
	 break;
      }

   while ((n = getc(fname)) != EOF && n != '\f')	/* read thru \f\n\0 */
      ;
   getc(fname);
   getc(fname);
#else					/* ShellHeader */
   if (fseek(fname, (long)MaxHeader, 0) == -1)
      error(name, errmsg);
#endif					/* ShellHeader */
#endif					/* Header */

#if MSDOS && !NT
   if (thisIsAnExeFile) {
        static char exe_errmsg[] = "can't read MS-DOS .exe header";
#if SCCX_MX
        if( thisIsIconx)
        {
            originalExeBytes = sizevar.value;
        }
        else
#endif                                  /* SCCX_MX */
        {
            fread (&bytesThatBeginEveryExe,
                    sizeof bytesThatBeginEveryExe, 1, fname);
            if (bytesThatBeginEveryExe[0] != 'M' ||
                bytesThatBeginEveryExe[1] != 'Z')
            {
                error(name, exe_errmsg);
            }
            fread (&originalExeBytesMod512,
                    sizeof originalExeBytesMod512, 1, fname);
            fread (&originalExePages, sizeof originalExePages, 1, fname);
            originalExeBytes = (originalExePages - 1)*512 +
                                originalExeBytesMod512;
        }
        if (fseek(fname, originalExeBytes, 0))
            error(name, errmsg);
        if (ferror(fname) || feof(fname) || !originalExeBytes)
            error(name, exe_errmsg);
   }
#endif                                  /* MSDOS && !NT */

   if (fread((char *)hdr, sizeof(char), sizeof(*hdr), fname) != sizeof(*hdr))
      error(name, errmsg);
   }

   return fname;
   }
#endif

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
#if !COMPILER
   FILE *fname = NULL;
   word cbread, longread();
#endif					/* COMPILER */

#if OS2
   char *p1, *p2;
   int rc;

   /* Determine if we are to load from a resource or not */
   if (stubexe || name[0] == '(' ) {
	use_resource = 1;
	if (name[0] == '(') {
	   /* Extract module name */
	   for(p1 = &name[1],p2 = modname; *p1 && *p1 != ':'; p1++, p2++)
	      *p2 = *p1;
	   *(p2+1) = '\0';

	   /* Extract resource id */
	   p1++;			/* Skip colon */
	   while(isspace(*p1)) p1++;

	   icoderesid = atol(p1);	/* convert to numeric value */

	   if (strcmp("*",modname) != 0) {
	      rc = DosLoadModule(loadmoderr,sizeof(loadmoderr),
				 modname,&modhandle);
	      }
	   else {
	      modhandle = 0;
	      }
	   }
	else {				/* Direct executable */
	    modhandle = 0;
	    icoderesid = 1;
	   }
	rc = DosGetResource(modhandle,RT_ICODE,icoderesid,&icoderes);

	prog_name = argv[0];
    }
    else {
	use_resource = 0;
	prog_name = name;
    }
#if PresentationManager
    PMInitialize();
#endif
#else					/* OS2 */

   prog_name = name;			/* Set icode file name */

#endif					/* OS2 */

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

/*
 * The following code is operating-system dependent [@init.02].  Set traps.
 */

#if PORT
   /* probably needs something */
Deliberate Syntax Error
#endif					/* PORT */

#if AMIGA
   signal(SIGFPE, SigFncCast fpetrap);
#endif					/* AMIGA */

#if ARM
   signal(SIGFPE, SigFncCast fpetrap);
   signal(SIGSEGV, SigFncCast segvtrap);
#endif					/* ARM */

#if ATARI_ST
#endif					/* ATARI_ST */

#if MACINTOSH
#if MPW
   {
      void MacInit(void);
      void SetFloatTrap(void (*fpetrap)());
      void fpetrap();

      MacInit();
      SetFloatTrap(fpetrap);
   }
#endif					/* MPW */
#endif					/* MACINTOSH */

#if MSDOS
#if MICROSOFT || TURBO || ZTC_386 || SCCX_MX
   signal(SIGFPE, SigFncCast fpetrap);
#endif					/* MICROSOFT || TURBO || ZTC_386 || SCCX_MX */
#endif					/* MSDOS */

#if MVS || VM
#if SASC
   cosignal(SIGFPE, SigFncCast fpetrap);           /* catch in all coprocs */
   cosignal(SIGSEGV, SigFncCast segvtrap);
#endif					/* SASC */
#endif					/* MVS || VM */

#if OS2 || BORLAND_286 || BORLAND_386
   signal(SIGFPE, SigFncCast fpetrap);
   signal(SIGSEGV, SigFncCast segvtrap);
#endif					/* OS2 || BORLAND_286 ... */

#if UNIX || VMS
   signal(SIGSEGV, SigFncCast segvtrap);
#ifdef PYRAMID
   {
   struct sigvec a;

   a.sv_handler = fpetrap;
   a.sv_mask = 0;
   a.sv_onstack = 0;
   sigvec(SIGFPE, &a, 0);
   sigsetmask(1 << SIGFPE);
   }
#else					/* PYRAMID */
   signal(SIGFPE, SigFncCast fpetrap);
#endif					/* PYRAMID */
#endif					/* UNIX || VMS */

/*
 * End of operating-system specific code.
 */

#if !COMPILER
#ifdef ExecImages
   /*
    * If reloading from a dumped out executable, skip most of init and
    *  just set up the buffer for stderr and do the timing initializations.
    */
   if (dumped)
      goto btinit;
#endif					/* ExecImages */
#endif					/* COMPILER */

   /*
    * Initialize data that can't be initialized statically.
    */

   datainit();

#if COMPILER
   IntVal(kywd_trc) = trc_init;
#endif					/* COMPILER */

#if !COMPILER
#if OS2
   if (use_resource)
	memcpy(&hdr,icoderes,sizeof(hdr));
   else {
       fname = readhdr(name,&hdr);
       if (fname == NULL) {
#ifdef PresentationManager
	   ConsoleFlags |= OutputToBuf;
	   fprintf(stderr, "Cannot locate the icode file: %s.\n", name);
	   error(NULL, "Execution cannot proceed.");
#else					/* PresentationManager */
	   error(name, "cannot open interpreter file");
#endif					/* PresentationManager */
       }
#else					/* OS2 */
   fname = readhdr(name,&hdr);
   if (fname == NULL) {
      error(name, "cannot open interpreter file");
#endif					/* OS2 */
      }

   k_trace = hdr.trace;

#endif					/* COMPILER */

   /*
    * Examine the environment and make appropriate settings.    [[I?]]
    */
   envset();

   /*
    * Convert stack sizes from words to bytes.
    */

#ifndef SCO_XENIX
   stksize *= WordSize;
   mstksize *= WordSize;
#else					/* SCO_XENIX */
   /*
    * This is a work-around for bad generated code for *= (as above)
    *  produced by the SCO XENIX C Compiler for the large memory model.
    *  It relies on the fact that WordSize is 4.
    */
   stksize += stksize;
   stksize += stksize;
   mstksize += mstksize;
   mstksize += mstksize;
#endif					/* SCO_XENIX */

#if IntBits == 16
   if (mstksize > MaxBlock)
      fatalerr(316, NULL);
   if (stksize > MaxBlock)
      fatalerr(318, NULL);
#endif					/* IntBits == 16 */

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
   mainhead = (struct b_coexpr *)malloc((msize)sizeof(struct b_coexpr));
#else					/* COMPILER */
   stack = (word *)malloc((msize)mstksize);
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
#if OS2
   if (use_resource) {
	memcpy(code,icoderes+sizeof(hdr),hdr.hsize);
	DosFreeResource(icoderes);
	if (modhandle) DosFreeModule(modhandle);
   }
   else {
       if ((cbread = longread(code, sizeof(char), (long)hdr.hsize, fname)) !=
	  hdr.hsize) {
#ifdef PresentationManager
	  ConsoleFlags |= OutputToBuf;
	  fprintf(stderr, "Invalid icode file: %s.\n", name);
	  fprintf(stderr,"Could only read %ld (of %ld) bytes of code.\n",
		  (long)cbread, (long)hdr.hsize);
	  error(NULL, NULL);
#else					/* PresentationManager */
	  fprintf(stderr,"Tried to read %ld bytes of code, got %ld\n",
	    (long)hdr.hsize,(long)cbread);
	  error(name, "bad icode file");
#endif					/* PresentationManager */
	  }
       fclose(fname);
    }
#else					/* OS2 */
   if ((cbread = longread(code, sizeof(char), (long)hdr.hsize, fname)) !=
      hdr.hsize) {
      fprintf(stderr,"Tried to read %ld bytes of code, got %ld\n",
	(long)hdr.hsize,(long)cbread);
      error(name, "bad icode file");
      }
   fclose(fname);
#endif					/* OS2 */

/*
 * Make sure the version number of the icode matches the interpreter version.
 */

   if (strcmp((char *)hdr.config,IVersion)) {
#ifdef PresentationManager
      ConsoleFlags |= OutputToBuf;
      fprintf(stderr, "Icode version mismatch in \'%s\':\n", name);
      fprintf(stderr, "    actual version: %s\n",(char *)hdr.config);
      fprintf(stderr, "    expected version: %s\n",IVersion);
      fprintf(stderr, "Execution of \'%s\' cannot proceed.", name);
      error(NULL, NULL);
#else					/* PresentationManager */
      fprintf(stderr,"icode version mismatch in %s\n", name);
      fprintf(stderr,"\ticode version: %s\n",(char *)hdr.config);
      fprintf(stderr,"\texpected version: %s\n",IVersion);
      error(name, "cannot run");
#endif					/* PresentationManager */
      }
#endif					/* !COMPILER */

   /*
    * Initialize the event monitoring system, if configured.
    */

#ifdef EventMon
   EVInit();
#endif					/* EventMon */

   /*
    * Check command line for redirected standard I/O.
    *  Assign a channel to the terminal if KeyboardFncs are enabled on VMS.
    */

#if VMS
   redirect(argcp, argv, 0);
#ifdef KeyboardFncs
   assign_channel_to_terminal();
#endif					/* KeyboardFncs */
#endif					/* VMS */

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

#if !COMPILER
#ifdef ExecImages
btinit:
#endif					/* ExecImages */
#endif					/* COMPILER */

/*
 * The following code is operating-system dependent [@init.03].  Allocate and
 *  assign a buffer to stderr if possible.
 */

#if PORT
   /* probably nothing */
Deliberate Syntax Error
#endif					/* PORT */

#if AMIGA || MVS || VM
   /* not done */
#endif					/* AMIGA */

#if ARM || ATARI_ST || MACINTOSH || UNIX || OS2 || VMS


   if (noerrbuf)
      setbuf(stderr, NULL);
   else {
      char *buf;

      buf = (char *)malloc((msize)BUFSIZ);
      if (buf == NULL)
	 fatalerr(305, NULL);
      setbuf(stderr, buf);
      }
#endif					/* ARM || ATARI_ST || MACINTOSH ... */

#if MSDOS
#if !HIGHC_386
   if (noerrbuf)
      setbuf(stderr, NULL);
   else {
#ifdef MSWindows
      char buf[BUFSIZ];
#else					/* MSWindows */
      char *buf;

      buf = (char *)malloc((msize)BUFSIZ);
      if (buf == NULL)
	 fatalerr(305, NULL);
#endif					/* MSWindows */
      setbuf(stderr, buf);
      }
#endif					/* !HIGHC_386 */
#endif					/* MSDOS */

/*
 * End of operating-system specific code.
 */

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
   env_int(TRACE, &k_trace, 0, (uword)0);
   env_int(COEXPSIZE, &stksize, 1, (uword)MaxUnsigned);
   env_int(STRSIZE, &ssize, 1, (uword)MaxBlock);
   env_int(HEAPSIZE, &abrsize, 1, (uword)MaxBlock);
#ifndef BSD_4_4_LITE
   env_int(BLOCKSIZE, &abrsize, 1, (uword)MaxBlock);    /* synonym */
#endif					/* BSD_4_4_LITE */
   env_int(BLKSIZE, &abrsize, 1, (uword)MaxBlock);      /* synonym */
   env_int(MSTKSIZE, &mstksize, 1, (uword)MaxUnsigned);
   env_int(QLSIZE, &qualsize, 1, (uword)MaxBlock);
   env_int("IXCUSHION", &memcushion, 1, (uword)100);	/* max 100 % */
   env_int("IXGROWTH", &memgrowth, 1, (uword)10000);	/* max 100x growth */

/*
 * The following code is operating-system dependent [@init.04].  Check any
 *  system-dependent environment variables.
 */

#if PORT
   /* nothing to do */
Deliberate Syntax Error
#endif					/* PORT */

#if AMIGA
   if ((p = getenv("CHECKBREAK")) != NULL)
      chkbreak++;
   if (WBstrsize != 0 && WBstrsize <= MaxBlock) ssize = WBstrsize;
   if (WBblksize != 0 && WBblksize <= MaxBlock) abrsize = WBblksize;
   if (WBmstksize != 0 && WBmstksize <= (uword) MaxUnsigned) mstksize = WBmstksize; 
#endif					/* AMIGA */

#if ARM || ATARI_ST || MACINTOSH || MSDOS || MVS || OS2 || UNIX || VM || VMS
   /* nothing to do */
#endif					/* ARM || ATARI_ST || ... */

/*
 * End of operating-system specific code.
 */

   if ((p = getenv(ICONCORE)) != NULL && *p != '\0') {

/*
 * The following code is operating-system dependent [@init.05].  Set trap to
 *  give dump on abnormal termination if ICONCORE is set.
 */

#if PORT
   /* can't handle */
Deliberate Syntax Error
#endif					/* PORT */

#if AMIGA || ATARI_ST || MACINTOSH
   /* can't handle */
#endif					/* AMIGA || ATARI_ST || ... */

#if ARM || OS2
      signal(SIGSEGV, SIG_DFL);
      signal(SIGFPE, SIG_DFL);
#endif					/* ARM || OS2 */

#if MSDOS
#if TURBO || BORLAND_286 || BORLAND_386
      signal(SIGFPE, SIG_DFL);
#endif					/* TURBO || BORLAND_286 ... */
#endif					/* MSDOS */

#if MVS || VM
      /* Really nothing to do. */
#endif					/* MVS || VM */

#if UNIX || VMS
      signal(SIGSEGV, SIG_DFL);
#endif					/* UNIX || VMS */

/*
 * End of operating-system specific code.
 */
      dodump++;
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

void fpetrap()
   {
   fatalerr(204, NULL);
   }

/*
 * Produce run-time error 320 on ^C interrupts. Not used at present,
 *  since malfunction may occur during traceback.
 */
void inttrap()
   {
   fatalerr(320, NULL);
   }

/*
 * Produce run-time error 302 on segmentation faults.
 */
void segvtrap()
   {
   static int n = 0;

   if (n != 0) {			/* only try traceback once */
      fprintf(stderr, "[Traceback failed]\n");
      exit(1);
      }
   n++;

#if MVS || VM
#if SASC
   btrace(0);
#endif					/* SASC */
#endif					/* MVS || VM */

   fatalerr(302, NULL);
   exit(1);
   }

/*
 * error - print error message from s1 and s2; used only in startup code.
 */
void error(s1, s2)
char *s1, *s2;
   {

#ifdef PresentationManager
   ConsoleFlags |= OutputToBuf;
   if (!s1 && s2)
      fprintf(stderr, s2);
   else if (s1 && s2)
      fprintf(stderr, "%s: %s\n", s1, s2);
#else					/* PresentationManager */
   if (!s1)
      fprintf(stderr, "error in startup code\n%s\n", s2);
   else
      fprintf(stderr, "error in startup code\n%s: %s\n", s1, s2);
#endif					/* PresentationManager */

   fflush(stderr);

#ifdef PresentationManager
   /* bring up the message box to display the error we constructed */
   WinMessageBox(HWND_DESKTOP, HWND_DESKTOP, ConsoleStringBuf,
		"Icon Runtime Initialization", 0,
		MB_OK|MB_ICONHAND|MB_MOVEABLE);
#endif					/* PresentationManager */

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


#ifdef PresentationManager
   ConsoleFlags |= OutputToBuf;
#endif					/* PresentationManager */
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
#ifdef PresentationManager
  error(NULL, NULL);
#endif					/* PresentationManager */

   fflush(stderr);
   if (dodump)
      abort();
   c_exit(EXIT_FAILURE);
   }

#ifdef ConsoleWindow
void closelogfile()
{
   if (flog) {
      extern char *lognam;
      extern char tmplognam[];
      FILE *flog2;
      int i;
      fclose(flog);

      /*
       * copy to the permanent file name
       */
      if ((flog = fopen(tmplognam, "r")) &&
	  (flog2 = fopen(lognam, "w"))) {
	 while ((i = getc(flog)) != EOF)
	    putc(i, flog2);
	 fclose(flog);
	 fclose(flog2);
	 remove(tmplognam);
	 }

      free(lognam);
      flog = NULL;
      }
}
#endif					/* ConsoleWindow */

/*
 * c_exit(i) - flush all buffers and exit with status i.
 */
void c_exit(i)
int i;
{
#ifdef ConsoleWindow
   char *msg = "Strike any key to close console...";
#endif					/* ConsoleWindow */

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

   if (k_dump && set_up) {
      fprintf(stderr,"\nTermination dump:\n\n");
      fflush(stderr);
      fprintf(stderr,"co-expression #%ld(%ld)\n",
	 (long)BlkLoc(k_current)->coexpr.id,
	 (long)BlkLoc(k_current)->coexpr.size);
      fflush(stderr);
      xdisp(pfp,glbl_argp,k_level,stderr);
      }

#ifdef MultipleRuns
   /*
    * Free allocated memory so application can continue.
    */

   xmfree();
#endif					/* MultipleRuns */


#ifdef ConsoleWindow
   /*
    * if the console was used for anything, pause it
    */
   if (ConsoleBinding) {
#if BORLAND_286
      fputs(msg, ConsoleBinding);
#else
      char label[256], tossanswer[256];
      struct descrip answer;

      wputstr((wbp)ConsoleBinding, msg, strlen(msg));

      strcpy(tossanswer, "label=");
      strncpy(tossanswer+6, StrLoc(kywd_prog), StrLen(kywd_prog));
      tossanswer[ 6 + StrLen(kywd_prog) ] = '\0';
      strcat(tossanswer, " - execution terminated");
      wattrib((wbp)ConsoleBinding, tossanswer, strlen(tossanswer),
              &answer, tossanswer);
#endif
      waitkey(ConsoleBinding);
      }
/* undo the #define exit c_exit */
#undef exit
#passthru #undef exit

   closelogfile();

#endif					/* ConsoleWindow */

#ifdef MSWindows
   PostQuitMessage(0);
   while (wstates != NULL) pollevent();
#endif					/* MSWindows */

#if TURBO || BORLAND_286 || BORLAND_386
   flushall();
   _exit(i);
#else					/* TURBO || BORLAND_286 ... */
#ifdef PresentationManager
   /* tell thread 1 to shut down */
   WinPostQueueMsg(HMainMessageQueue, WM_QUIT, (MPARAM)0, (MPARAM)0);
   /* bye, bye */
   InterpThreadShutdown();
#else					/* PresentationManager */
   exit(i);
#endif					/* PresentationManager */
#endif					/* TURBO || BORLAND_286 ... */

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
#ifdef MSWindows
   extern FILE *finredir, *fouredir, *ferredir;
#endif					/* MSWindows */

   /*
    * Initializations that cannot be performed statically (at least for
    * some compilers).					[[I?]]
    */

#ifdef MultiThread
   k_errout.title = T_File;
   k_input.title = T_File;
   k_output.title = T_File;
#endif					/* MultiThread */

#ifdef MSWindows
   if (ferredir != NULL)
      k_errout.fd = ferredir;
   else
#endif					/* MSWindows */
   k_errout.fd = stderr;
   StrLen(k_errout.fname) = 7;
   StrLoc(k_errout.fname) = "&errout";
   k_errout.status = Fs_Write;

#ifdef MSWindows
   if (finredir != NULL)
      k_input.fd = finredir;
   else
#endif					/* MSWindows */
   if (k_input.fd == NULL)
      k_input.fd = stdin;
   StrLen(k_input.fname) = 6;
   StrLoc(k_input.fname) = "&input";
   k_input.status = Fs_Read;

#ifdef MSWindows
   if (fouredir != NULL)
      k_output.fd = fouredir;
   else
#endif					/* MSWindows */
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

#ifdef MultipleRuns
   /*
    * Initializations required for repeated program runs
    */
					/* In this module:	*/
   k_level = 0;				/* &level */
   k_errornumber = 0;			/* &errornumber */
   k_errortext = "";			/* &errortext */
   currend = NULL;			/* current end of memory region */


   mstksize = MStackSize;		/* initial size of main stack */
   stksize = StackSize;			/* co-expression stack size */
   ssize = MaxStrSpace;			/* initial string space size (bytes) */
   abrsize = MaxAbrSize;		/* initial size of allocated block
					     region (bytes) */
   qualsize = QualLstSize;		/* size of quallist for fixed regions */

   dodump = 0;				/* produce dump on error */

#ifdef ExecImages
   dumped = 0;				/* This is a dumped image. */
#endif					/* ExecImages */

					/* In module interp.r:	*/
   pfp = 0;				/* Procedure frame pointer */
   sp = NULL;				/* Stack pointer */


					/* In module rmemmgt.r:	*/
   coexp_ser = 2;
   list_ser = 1;
   set_ser = 1;
   table_ser = 1;

   coll_stat = 0;
   coll_str = 0;
   coll_blk = 0;
   coll_tot = 0;

					/* In module time.c: */
   first_time = 1;


#endif					/* MultipleRuns */
#endif					/* COMPILER */

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
#endif						/* EventMon */
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
