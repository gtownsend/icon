/*
 * cstructs.h - structures and accompanying manifest constants for functions
 *  in the common subdirectory.
 */

/*
 * fileparts holds a file name broken down into parts.
 */
struct fileparts {			/* struct of file name parts */
   char *dir;				/* directory */
   char *name;				/* name */
   char *ext;				/* extension */
   };

/*
 * xval - holds references to literal constants
 */
union xval {
   long ival;		/* integer */
   double rval;		/*  real */
   word sval;		/*  offset into string space of string */
   };

/*
 * str_buf references a string buffer. Strings are built a character
 *  at a time. When a buffer "fragment" is filled, another is allocated
 *  and the the current string copied to it.
 */
struct str_buf_frag {
   struct str_buf_frag *next;     /* next buffer fragment */
   char s[1];                     /* variable size buffer, really > 1 */
   };

struct str_buf {
   unsigned int size;             /* total size of current buffer */
   char *strtimage;               /* start of string currently being built */
   char *endimage;                /* next free character in buffer */
   char *end;                     /* end of current buffer */
   struct str_buf_frag *frag_lst; /* list of buffer fragments */
   struct str_buf *next;          /* buffers can be put on free list */
   };

#define AppChar(sbuf, c) do {\
   if ((sbuf).endimage >= (sbuf).end)\
      new_sbuf(&(sbuf));\
   *((sbuf).endimage)++ = (c);\
   } while (0)

/*
 * implement contains information about the implementation of an operation.
 */
#define NoRsltSeq  -1L	     /* no result sequence: {} */
#define UnbndSeq   -2L       /* unbounded result sequence: {*} */

#define DoesRet    01	     /* operation (or "body" function) returns */
#define DoesFail   02	     /* operation (or "body" function) fails */
#define DoesSusp   04	     /* operation (or "body" function) suspends */
#define DoesEFail 010        /* fails through error conversion */
#define DoesFThru 020	     /* only "body" functions can "fall through" */

struct implement {
   struct implement *blink;   /* link for bucket chain in hash tables */
   char oper_typ;             /* 'K'=keyword, 'F'=function, 'O'=operator */
   char prefix[2];	      /* prefix to make start of name unique */
   char *name;		      /* function/operator/keyword name */
   char *op;		      /* operator symbol (operators only) */
   int nargs;		      /* number of arguments operation requires */
   int *arg_flgs;             /* array of arg flags: deref/underef, var len*/
   long min_result;	      /* minimum result sequence length */
   long max_result;	      /* maiximum result sequence length */
   int resume;		      /* flag - resumption after last result */
   int ret_flag;	      /* DoesRet, DoesFail, DoesSusp */
   int use_rslt;              /* flag - explicitly uses result location */
   char *comment;	      /* description of operation */
   int ntnds;		      /* size of tnds array */
   struct tend_var *tnds;     /* pointer to array of info about tended vars */
   int nvars;                 /* size of vars array */
   struct ord_var  *vars;     /* pointer to array of info about ordinary vars */
   struct il_code *in_line;    /* inline version of the operation */
   int iconc_flgs;	      /* flags for internal use by the compiler */
   };

/*
 * These codes are shared between the data base and rtt. They are defined
 *  here, though not all are used by the data base.
 */
#define TndDesc   1  /* a tended descriptor */
#define TndStr    2  /* a tended character pointer */
#define TndBlk    3  /* a tended block pointer */
#define OtherDcl  4  /* a declaration that is not special */
#define IsTypedef 5  /* a typedef */
#define VArgLen   6  /* identifier for length of variable parm list */
#define RsltLoc   7  /* the special result location of an operation */
#define Label     8  /* label */
#define RtParm   16  /* undereferenced parameter of run-time routine */
#define DrfPrm   32  /* dereferenced parameter of run-time routine */
#define VarPrm   64  /* variable part of parm list (with RtParm or DrfPrm) */
#define PrmMark 128  /* flag - used while recognizing params of body fnc */
#define ByRef   256  /* flag - parameter to body function passed by reference */

/*
 * Flags to indicate what types are returned from the function implementing
 *  a body. These are unsed in determining the calling conventions
 *  of the function.
 */
#define RetInt   1  /* body/function returns a C_integer */
#define RetDbl   2  /* body/function returns a C_double */
#define RetOther 4  /* body (not function itself) returns something else */
#define RetNoVal 8  /* function returns no value */
#define RetSig  16  /* function returns a signal */

/*
 * tend_var contains information about a tended variable in the "declare {...}"
 *  action of an operation.
 */
struct tend_var {
   int var_type;           /* TndDesc, TndStr, or TndBlk */
   struct il_c *init;      /* initial value from declaration */
   char *blk_name;         /* TndBlk: struct name of block */
   };

/*
 * ord_var contains information about an ordinary variable in the
 *  "declare {...}" action of an operation.
 */
struct ord_var {
   char *name;        /* name of variable */
   struct il_c *dcl;  /* declaration of variable (includes name) */
   };

/*
 * il_code has information about an action in an operation.
 */
#define IL_If1     1
#define IL_If2     2
#define IL_Tcase1  3
#define IL_Tcase2  4
#define IL_Lcase   5
#define IL_Err1    6
#define IL_Err2    7
#define IL_Lst     8
#define IL_Const   9
#define IL_Bang   10
#define IL_And    11
#define IL_Cnv1   12
#define IL_Cnv2   13
#define IL_Def1   14
#define IL_Def2   15
#define IL_Is     16
#define IL_Var    17
#define IL_Subscr 18
#define IL_Block  19
#define IL_Call   20
#define IL_Abstr  21
#define IL_VarTyp 22
#define IL_Store  23
#define IL_Compnt 24
#define IL_TpAsgn 25
#define IL_Union  26
#define IL_Inter  27
#define IL_New    28
#define IL_IcnTyp 29
#define IL_Acase  30

#define CM_Fields -1

union il_fld {
   struct il_code *fld;
   struct il_c *c_cd;
   int *vect;
   char *s;
   word n;
   };

struct il_code {
   int il_type;
   union il_fld u[1];   /* actual number of fields varies with type */
   };

/*
 * The following manifest constants are used to describe types, conversions,
 *   and returned values. Non-negative numbers are reserved for types described
 *   in the type specification system.
 */
#define TypAny    -1
#define TypEmpty  -2
#define TypVar    -3
#define TypCInt   -4
#define TypCDbl   -5
#define TypCStr   -6
#define TypEInt   -7
#define TypECInt  -8
#define TypTStr   -9
#define TypTCset -10
#define RetDesc  -11
#define RetNVar  -12
#define RetSVar  -13
#define RetNone  -14

/*
 * il_c describes a piece of C code.
 */
#define ILC_Ref    1   /* nonmodifying reference to var. in sym. tab. */
#define ILC_Mod    2   /* modifying reference to var. in sym. tab */
#define ILC_Tend   3   /* tended var. local to inline block */
#define ILC_SBuf   4   /* string buffer */
#define ILC_CBuf   5   /* cset buffer */
#define ILC_Ret    6   /* return statement */
#define ILC_Susp   7   /* suspend statement */
#define ILC_Fail   8   /* fail statement */
#define ILC_Goto   9   /* goto */
#define ILC_CGto  10   /* conditional goto */
#define ILC_Lbl   11   /* label */
#define ILC_LBrc  12   /* '{' */
#define ILC_RBrc  13   /* '}' */
#define ILC_Str   14   /* arbitrary string of code */
#define ILC_EFail 15   /* errorfail statement */

#define RsltIndx -1   /* symbol table index for "result" */

struct il_c {
   int il_c_type;
   struct il_c *code[3];
   word n;
   char *s;
   struct il_c *next;
   };

/*
 * The parameter value of a run-time operation may be in one of several
 *  different locations depending on what conversions have been done to it.
 *  These codes are shared by rtt and iconc.
 */
#define PrmTend    1   /* in tended location */
#define PrmCStr    3   /* converted to C string: tended location */
#define PrmInt     4   /* converted to C int: non-tended location */
#define PrmDbl     8   /* converted to C double: non-tended location */

/*
 * Kind of RLT return statement supported.
 */
#define TRetNone  0   /* does not support an RTL return statement */
#define TRetBlkP  1   /* block pointer */
#define TRetDescP 2   /* descriptor pointer */
#define TRetCharP 3   /* character pointer */
#define TRetCInt  4   /* C integer */
#define TRetSpcl  5   /* RLT return statement has special form & semenatics */

/*
 * Codes for dereferencing needs.
 */
#define DrfNone  0  /* not a variable type */
#define DrfGlbl  1  /* treat as a global variable */
#define DrfCnst  2  /* type of values in variable doesn't change */
#define DrfSpcl  3  /* special dereferencing: trapped variable */

/*
 * Information about an Icon type.
 */
struct icon_type {
   char *id;		/* name of type */
   int support_new;	/* supports RTL "new" construct */
   int deref;		/* dereferencing needs */
   int rtl_ret;		/* kind of RTL return supported if any */
   char *typ;		/* for variable: initial type */
   int num_comps;	/* for aggregate: number of type components */
   int compnts;		/* for aggregate: index of first component */
   char *abrv;		/* abreviation used for type tracing */
   char *cap_id;	/* name of type with first character capitalized */
   };

/*
 * Information about a component of an aggregate type.
 */
struct typ_compnt {
   char *id;		/* name of component */
   int n;		/* position of component within type aggragate */
   int var;		/* flag: this component is an Icon-level variable */
   int aggregate;	/* index of type that owns the component */
   char *abrv;		/* abreviation used for type tracing */
   };

extern int num_typs;                 /* number of types in table */
extern struct icon_type icontypes[]; /* table of icon types */

/*
 * Type inference needs to know where most of the standard types
 *  reside. Some have special uses outside operations written in
 *  RTL code, such as the null type for initializing variables, and
 *  others have special semantics, such as trapped variables.
 */
extern int str_typ;                  /* index of string type */
extern int int_typ;                  /* index of integer type */
extern int rec_typ;                  /* index of record type */
extern int proc_typ;                 /* index of procedure type */
extern int coexp_typ;                /* index of co-expression type */
extern int stv_typ;                  /* index of sub-string trapped var type */
extern int ttv_typ;                  /* index of table-elem trapped var type */
extern int null_typ;                 /* index of null type */
extern int cset_typ;                 /* index of cset type */
extern int real_typ;                 /* index of real type */
extern int list_typ;                 /* index of list type */
extern int tbl_typ;                  /* index of table type */

extern int num_cmpnts;                 /* number of aggregate components */
extern struct typ_compnt typecompnt[]; /* table of aggregate components */
extern int str_var;                    /* index of trapped string variable */
extern int trpd_tbl;                   /* index of trapped table */
extern int lst_elem;                   /* index of list element */
extern int tbl_val;                    /* index of table element value */
extern int tbl_dflt;                   /* index of table default */

/*
 * minimum number of unsigned ints needed to hold the bits of a cset - only
 *  used in translators, not in the run-time system.
 */
#define BVectSize 16
