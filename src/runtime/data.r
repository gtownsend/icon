/*
 * data.r -- Various interpreter data tables.
 */

#if !COMPILER

struct b_proc Bnoproc;

#ifdef EventMon
struct b_iproc mt_llist = {
   6, (sizeof(struct b_proc) - sizeof(struct descrip)), Ollist,
   0, -1,  0, 0, {sizeof( "[...]")-1, "[...]"}};
#endif					/* EventMon */


/*
 * External declarations for function blocks.
 */

#define FncDef(p,n) extern struct b_proc Cat(B,p);
#define FncDefV(p) extern struct b_proc Cat(B,p);
#passthru #undef exit
#undef exit
#include "../h/fdefs.h"
#undef FncDef
#undef FncDefV

#define OpDef(p,n,s,u) extern struct b_proc Cat(B,p);
#include "../h/odefs.h"
#undef OpDef

extern struct b_proc Bbscan;
extern struct b_proc Bescan;
extern struct b_proc Bfield;
extern struct b_proc Blimit;
extern struct b_proc Bllist;




struct b_proc *opblks[] = {
	NULL,
#define OpDef(p,n,s,u) Cat(&B,p),
#include "../h/odefs.h"
#undef OpDef
   &Bbscan,
   NULL,
   NULL,
   NULL,
   NULL,
   NULL,
   NULL,
   NULL,
   NULL,
   NULL,
   NULL,
   &Bescan,
   NULL,
   &Bfield,
   NULL,
   NULL,
   NULL,
   NULL,
   NULL,
   &Blimit,
   &Bllist,
   NULL,
   NULL,
   NULL
   };

/*
 * Array of names and corresponding functions.
 *  Operators are kept in a similar table, op_tbl.
 */

struct pstrnm pntab[] = {

#define FncDef(p,n) Lit(p), Cat(&B,p),
#define FncDefV(p) Lit(p), Cat(&B,p),
#include "../h/fdefs.h"
#undef FncDef
#undef FncDefV

	0,		 0
	};

int pnsize = (sizeof(pntab) / sizeof(struct pstrnm)) - 1;

#endif					/* COMPILER */

/*
 * Structures for built-in values.  Parts of some of these structures are
 *  initialized later. Since some C compilers cannot handle any partial
 *  initializations, all parts are initialized later if any have to be.
 */

/*
 * blankcs; a cset consisting solely of ' '.
 */
struct b_cset  blankcs = {
   T_Cset,
   1,
   cset_display(0, 0, 01, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0)
   };

/*
 * lparcs; a cset consisting solely of '('.
 */
struct b_cset  lparcs = {
   T_Cset,
   1,
   cset_display(0, 0, 0400, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0)
   };

/*
 * rparcs; a cset consisting solely of ')'.
 */
struct b_cset  rparcs = {
   T_Cset,
   1,
   cset_display(0, 0, 01000, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0)
   };

/*
 * fullcs - all 256 bits on.
 */
struct b_cset  fullcs = {
   T_Cset,
   256,
   cset_display(~0, ~0, ~0, ~0, ~0, ~0, ~0, ~0,
		~0, ~0, ~0, ~0, ~0, ~0, ~0, ~0)
   };

#if !COMPILER

/*
 * Built-in csets
 */

/*
 * &digits; bits corresponding to 0-9 are on.
 */
struct b_cset  k_digits = {
   T_Cset,
   10,

   cset_display(0,  0,	0,  0x3ff, 0,  0, 0,  0,
		0,  0,	0,  0,	 0,  0,	 0,  0)
   };

/*
 * Cset for &lcase; bits corresponding to lowercase letters are on.
 */
struct b_cset  k_lcase = {
   T_Cset,
   26,

   cset_display(0,  0,	0,  0,	0,  0,	~01,  03777,
		0,  0,	0,  0,	0,  0,	0,  0)
   };

/*
 * &ucase; bits corresponding to uppercase characters are on.
 */
struct b_cset  k_ucase = {
   T_Cset,
   26,

   cset_display(0,  0,	0,  0,	~01,  03777, 0, 0,
		0,  0,	0,  0,	0,  0,	0,  0)
   };

/*
 * &letters; bits corresponding to letters are on.
 */
struct b_cset  k_letters = {
   T_Cset,
   52,

   cset_display(0,  0,	0,  0,	~01,  03777, ~01, 03777,
		0,  0,	0,  0,	0,  0,	0,  0)
   };
#endif					/* COMPILER */

/*
 * Built-in files.
 */

#ifndef MultiThread
struct b_file  k_errout = {T_File, NULL, Fs_Write};	/* &errout */
struct b_file  k_input = {T_File, NULL, Fs_Read};	/* &input */
struct b_file  k_output = {T_File, NULL, Fs_Write};	/* &output */
#endif					/* MultiThread */

#ifdef EventMon
/*
 *  Real block needed for event monitoring.
 */
struct b_real realzero = {T_Real, 0.0};
#endif					/* EventMon */

/*
 * Keyword variables.
 */
#ifndef MultiThread
struct descrip kywd_err = {D_Integer};  /* &error */
struct descrip kywd_pos = {D_Integer};	/* &pos */
struct descrip kywd_prog;		/* &progname */
struct descrip k_subject;		/* &subject */
struct descrip kywd_ran = {D_Integer};	/* &random */
struct descrip kywd_trc = {D_Integer};	/* &trace */
struct descrip k_eventcode = {D_Null};	/* &eventcode */
struct descrip k_eventsource = {D_Null};/* &eventsource */
struct descrip k_eventvalue = {D_Null};	/* &eventvalue */

#endif					/* MultiThread */

#ifdef FncTrace
struct descrip kywd_ftrc = {D_Integer};	/* &ftrace */
#endif					/* FncTrace */

struct descrip kywd_dmp = {D_Integer};	/* &dump */

struct descrip nullptr =
   {F_Ptr | F_Nqual};	                /* descriptor with null block pointer */
struct descrip trashcan;		/* descriptor that is never read */

/*
 * Various constant descriptors.
 */

struct descrip blank;			/* one-character blank string */
struct descrip emptystr;		/* zero-length empty string */
struct descrip lcase;			/* string of lowercase letters */
struct descrip letr;			/* "r" */
struct descrip nulldesc = {D_Null};	/* null value */
struct descrip onedesc = {D_Integer};	/* integer 1 */
struct descrip ucase;			/* string of uppercase letters */
struct descrip zerodesc = {D_Integer};	/* integer 0 */

#ifdef EventMon
/*
 * Descriptors used by event monitoring.
 */
struct descrip csetdesc = {D_Cset};
struct descrip eventdesc;
struct descrip rzerodesc = {D_Real};
#endif					/* EventMon */

/*
 * An array of all characters for use in making one-character strings.
 */

unsigned char allchars[256] = {
     0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, 15,
    16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31,
    32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47,
    48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63,
    64, 65, 66, 67, 68, 69, 70, 71, 72, 73, 74, 75, 76, 77, 78, 79,
    80, 81, 82, 83, 84, 85, 86, 87, 88, 89, 90, 91, 92, 93, 94, 95,
    96, 97, 98, 99,100,101,102,103,104,105,106,107,108,109,110,111,
   112,113,114,115,116,117,118,119,120,121,122,123,124,125,126,127,
   128,129,130,131,132,133,134,135,136,137,138,139,140,141,142,143,
   144,145,146,147,148,149,150,151,152,153,154,155,156,157,158,159,
   160,161,162,163,164,165,166,167,168,169,170,171,172,173,174,175,
   176,177,178,179,180,181,182,183,184,185,186,187,188,189,190,191,
   192,193,194,195,196,197,198,199,200,201,202,203,204,205,206,207,
   208,209,210,211,212,213,214,215,216,217,218,219,220,221,222,223,
   224,225,226,227,228,229,230,231,232,233,234,235,236,237,238,239,
   240,241,242,243,244,245,246,247,248,249,250,251,252,253,254,255,
};

/*
 * Run-time error numbers and text.
 */
struct errtab errtab[] = {

   101, "integer expected or out of range",
   102, "numeric expected",
   103, "string expected",
   104, "cset expected",
   105, "file expected",
   106, "procedure or integer expected",
   107, "record expected",
   108, "list expected",
   109, "string or file expected",
   110, "string or list expected",
   111, "variable expected",
   112, "invalid type to size operation",
   113, "invalid type to random operation",
   114, "invalid type to subscript operation",
   115, "structure expected",
   116, "invalid type to element generator",
   117, "missing main procedure",
   118, "co-expression expected",
   119, "set expected",
   120, "two csets or two sets expected",
   121, "function not supported",
   122, "set or table expected",
   123, "invalid type",
   124, "table expected",
   125, "list, record, or set expected",
   126, "list or record expected",

#ifdef Graphics
   140, "window expected",
   141, "program terminated by window manager",
   142, "attempt to read/write on closed window",
   143, "malformed event queue",
   144, "window system error",
   145, "bad window attribute",
   146, "incorrect number of arguments to drawing function",
   147, "window attribute cannot be read or written as requested",
#endif					/* Graphics */

#ifdef FAttrib
   160, "bad file attribute",
#endif					/* FAttrib */

   201, "division by zero",
   202, "remaindering by zero",
   203, "integer overflow",
   204, "real overflow, underflow, or division by zero",
   205, "invalid value",
   206, "negative first argument to real exponentiation",
   207, "invalid field name",
   208, "second and third arguments to map of unequal length",
   209, "invalid second argument to open",
   210, "non-ascending arguments to detab/entab",
   211, "by value equal to zero",
   212, "attempt to read file not open for reading",
   213, "attempt to write file not open for writing",
   214, "input/output error",
   215, "attempt to refresh &main",
   216, "external function not found",

   301, "evaluation stack overflow",
   302, "memory violation",
   303, "inadequate space for evaluation stack",
   304, "inadequate space in qualifier list",
   305, "inadequate space for static allocation",
   306, "inadequate space in string region",
   307, "inadequate space in block region",
   308, "system stack overflow in co-expression",

#ifndef Coexpr
   401, "co-expressions not implemented",
#endif					/* Coexpr */
   402, "program not compiled with debugging option",

   500, "program malfunction",		/* for use by runerr() */
   600, "vidget usage error",		/* yeah! */

   0,	""
   };

#if !COMPILER
#define OpDef(p,n,s,u) int Cat(O,p) (dptr cargp);
#include "../h/odefs.h"
#undef OpDef

/*
 * When an opcode n has a subroutine call associated with it, the
 *  nth word here is the routine to call.
 */

int (*optab[])() = {
	err,
#define OpDef(p,n,s,u) Cat(O,p),
#include "../h/odefs.h"
#undef OpDef
   Obscan,
   err,
   err,
   err,
   err,
   err,
   Ocreate,
   err,
   err,
   err,
   err,
   Oescan,
   err,
   Ofield
   };

/*
 *  Keyword function look-up table.
 */
#define KDef(p,n) int Cat(K,p) (dptr cargp);
#include "../h/kdefs.h"
#undef KDef

int (*keytab[])() = {
   err,
#define KDef(p,n) Cat(K,p),
#include "../h/kdefs.h"
   };
#endif					/* !COMPILER */
