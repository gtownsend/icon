/*
 * External declarations for the linker.
 */

#include "../h/rt.h"

/*
 * Miscellaneous external declarations.
 */

extern FILE *infile;		/* current input file */
extern FILE *outfile;		/* linker output file */
extern FILE *dbgfile;		/* debug file */
extern char inname[];		/* input file name */
extern char icnname[];		/* source program file name */
extern int lineno;		/* source program line number (from ucode) */
extern int colmno;		/* source program column number */

extern int lstatics;		/* total number of statics */
extern int argoff;		/* stack offset counter for arguments */
extern int dynoff;		/* stack offset counter for locals */
extern int static1;		/* first static in procedure */
extern int nlocal;		/* number of locals in local table */
extern int nconst;		/* number of constants in constant table */
extern int nrecords;		/* number of records in program */
extern int trace;		/* initial setting of &trace */
extern char ixhdr[];		/* header line for direct execution */
extern char *iconx;		/* location of iconx */
extern int hdrloc;		/* location to place hdr block at */
extern struct lfile *llfiles;	/* list of files to link */

/*
 * Structures for symbol table entries.
 */

struct lentry {			/* local table entry */
   word  l_name;		/*   index into string space of variable name */
   int l_flag;			/*   variable flags */
   union {			/*   value field */
      int staticid;		/*     unique id for static variables */
      word offset;		/*     stack offset for args and locals */
      struct gentry *global;	/*     global table entry */
      } l_val;
   };

struct gentry {			/* global table entry */
   struct gentry *g_blink;	/*   link for bucket chain */
   word g_name;			/*   index into string space of variable name */
   int g_flag;			/*   variable flags */
   int g_nargs;			/*   number of args or fields */
   int g_procid;		/*   procedure or record id */
   word g_pc;			/*   position in icode of object */
   int g_index;			/*   "index" in global table */
   struct gentry **g_refs;	/*   other globals referenced, if a proc */
   struct gentry *g_next;	/*   next global in table */
   };

struct centry {			/* constant table entry */
   int c_flag;			/*   type of literal flag */
   union xval c_val;		/*   value field */
   int c_length;		/*   length of literal string */
   word c_pc;			/*   position in icode of object */
   };

struct ientry {			/* identifier table entry */
   struct ientry *i_blink;	/*   link for bucket chain */
   word i_name;			/*   index into string space of string */
   int i_length;		/*   length of string */
   };

struct fentry {			/* field table header entry */
   struct fentry *f_blink;	/*   link for bucket chain */
   word f_name;			/*   index into string space of field name */
   int f_fid;			/*   field id */
   struct rentry *f_rlist;	/*   head of list of records */
   struct fentry *f_nextentry;	/*   next field name in allocation order */
   };

struct rentry {			/* field table record list entry */
   struct rentry *r_link;	/*   link for list of records */
   struct gentry *r_gp;		/*   global entry for record */
   int r_fnum;			/*   offset of field within record */
   };

#include "lfile.h"

/*
 * Flag values in symbol tables.
 */

#define F_Global	    01	/* variable declared global externally */
#define F_Unref		    02	/* procedure is unreferenced */
#define F_Proc		    04	/* procedure */
#define F_Record	   010	/* record */
#define F_Dynamic	   020	/* variable declared local dynamic */
#define F_Static	   040	/* variable declared local static */
#define F_Builtin	  0100	/* identifier refers to built-in procedure */
#define F_ImpError	  0400	/* procedure has default error */
#define F_Argument	 01000	/* variable is a formal parameter */
#define F_IntLit	 02000	/* literal is an integer */
#define F_RealLit	 04000	/* literal is a real */
#define F_StrLit	010000	/* literal is a string */
#define F_CsetLit	020000	/* literal is a cset */

/*
 * Symbol table region pointers.
 */

extern struct gentry **lghash;	/* hash area for global table */
extern struct ientry **lihash;	/* hash area for identifier table */
extern struct fentry **lfhash;	/* hash area for field table */

extern struct lentry *lltable;	/* local table */
extern struct centry *lctable;	/* constant table */
extern struct ipc_fname *fnmtbl; /* table associating ipc with file name */
extern struct ipc_line *lntable; /* table associating ipc with line number */
extern char *lsspace;		/* string space */
extern word *labels;		/* label table */
extern char *codeb;		/* generated code space */

extern struct ipc_fname *fnmfree; /* free pointer for ipc/file name tbl */
extern struct ipc_line *lnfree;	/* free pointer for ipc/line number tbl */
extern word lsfree;		/* free index for string space */
extern char *codep;		/* free pointer for code space */

extern struct fentry *lffirst;	/* first field table entry */
extern struct fentry *lflast;	/* last field table entry */
extern struct gentry *lgfirst;	/* first global table entry */
extern struct gentry *lglast;	/* last global table entry */


/*
 * Hash computation macros.
 */

#define ghasher(x)	(((word)x)&gmask)	/* for global table */
#define fhasher(x)	(((word)x)&fmask)	/* for field table */

/*
 * Machine-dependent constants.
 */

#define RkBlkSize(gp) ((9*WordSize)+(gp)->g_nargs * sizeof(struct descrip))
