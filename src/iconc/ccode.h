/*
 * ccode.h - definitions used in code generation.
 */

/*
 * ChkPrefix - allocate a prefix to x if it has not already been done.
 */
#define ChkPrefix(x) if ((x)[0] == '\0') nxt_pre(x, pre, PrfxSz);

/*
 * sig_act - list of possible signals returned by a call and the action to be
 *  to be taken when the signal is returned: in effect a switch statement.
 */
struct sig_act {
   struct code *sig;         /* signal */
   struct code *cd;          /* action to be taken: goto, return, break */
   struct sig_act *shar_act; /* signals that share this action */
   struct sig_act *next;
   };

/*
 * val_loc - location of a value. Used for intermediate and final results
 *   of expressions.
 */
#define V_NamedVar 1  /* Icon named variable indicated by nvar */
#define V_Temp     2  /* temporary variable indicated by tmp */
#define V_ITemp    3  /* C integer temporary variable indicated by tmp */
#define V_DTemp    4  /* C double temporary variable indicated by tmp */
#define V_PRslt    5  /* procedure result location */
#define V_Const    6  /* integer constant - used for size of varargs */
#define V_CVar     7  /* C named variable */
#define V_Ignore   8  /* "trashcan" - a write-only location */

#define M_None     0  /* access simply as descriptor */
#define M_CharPtr  1  /* access v-word as "char *" */
#define M_BlkPtr   2  /* access v-word as block pointer using blk_name */
#define M_CInt     3  /* access v-word as C integer */
#define M_Addr     4  /* address of descriptor for varargs */

struct val_loc {
   int loc_type;      /* manifest constants V_* */
   int mod_access;    /* manifest constants M_* */
   char *blk_name;    /* used with M_BlkPtr */
   union {
     struct lentry *nvar; /* Icon named variable */
     int tmp;             /* index of temporary variable */
     int int_const;       /* integer constant value */
     char *name;          /* C named variable */
     } u;
   };

/*
 * "code" contains the information needed to print a piece of C code.
 *  C_... manifest constants are cd_id's. These are followed by
 *  corresponding field access expressions.
 */
#define Rslt        fld[0].vloc    /* place to put result of expression */
#define Cont        fld[1].fnc	   /* continuation function or null */

#define C_Null      0              /* no code */

#define C_NamedVar  1		   /* reference to a named variable */
/* uses Rslt */
#define NamedVar    fld[1].nvar

#define C_CallSig   2		   /* call and handling of returned signal */
#define OperName    fld[0].oper_nm /* run-time routine name or null */
/* uses Cont */
#define Flags       fld[2].n       /* flag: NeedCont, ForeignSig */
#define ArgLst      fld[3].cd	   /* argument list */
#define ContFail    fld[4].cd	   /* label/signal to goto/return on failure */
#define SigActs     fld[5].sa	   /* actions to take for returned signals */
#define NextCall    fld[6].cd	   /* for chaining calls within a continuation*/
#define NeedCont    1              /* pass NULL continuation if Cont == NULL */
#define ForeignSig  2              /* may get foreign signal from a suspend */

#define C_RetSig    3 		   /* return signal */
#define SigRef      fld[0].sigref  /* pointer to func's reference to signal */

#define C_Goto      4		   /* goto label */
#define Lbl         fld[0].cd      /* label */

#define C_Label     5		   /* statment label "Ln:" and signal "n" */
#define Container   fld[0].fnc	   /* continuation containing label */
#define SeqNum      fld[1].n	   /* sequence number, n */
#define Desc        fld[2].s	   /* description of how label/signal is used */
#define RefCnt      fld[3].n	   /* reference count for label */
#define LabFlg      fld[4].n       /* flag: FncPtrd, BndSig */
#define FncPrtd     1		   /*   function sig_n has been printed */
#define Bounding    2		   /*   this is a bounding label */

#define C_Lit      6 		   /* literal (integer, real, string, cset) */
/* uses Rslt */
#define Literal    fld[1].lit

#define C_Resume   7		   /* resume signal */
#define C_Continue 8 		   /* continue signal */
#define C_FallThru 9		   /* fall through signal */
#define C_PFail    10		   /* procedure failure */
#define C_PRet     11		   /* procedure return (result already set) */
#define C_PSusp    12		   /* procedure suspend */
#define C_Break    13		   /* break out of signal handling switch */
#define C_LBrack   14              /* '{' */
#define C_RBrack   15              /* '}' */

#define C_Create   16              /* call of create() for create expression */
/* uses Rslt */
/* uses Cont */
#define NTemps      fld[2].n	   /* number of temporary descriptors needed */
#define WrkSize     fld[3].n	   /* size of non-descriptor work area */
#define NextCreat   fld[4].cd	   /* for chaining creates in a continuation */


#define C_If       17		   /* conditional (goto or return) */
#define Cond       fld[0].cd       /* condition */
#define ThenStmt   fld[1].cd       /* what to do if condition is true */

#define C_SrcLoc   18
#define FileName    fld[0].s      /* name of source file */
#define LineNum     fld[1].n      /* line number within source file */

#define C_CdAry    19             /* array of code pieces, each with type code*/
#define A_Str      0              /* code represented as a string */
#define A_ValLoc   1              /* value location */
#define A_Intgr    2              /* integer */
#define A_ProcCont 3              /* procedure continuation */
#define A_SBuf     4              /* string buffer (integer index) */
#define A_CBuf     5              /* cset buffer (integer index) */
#define A_Ary      6              /* pointer to subarray of code pieces */
#define A_End      7              /* marker for end of array */
#define ElemTyp(i) fld[2*i].n     /* type of element i (A_* codes) */
#define Str(i)     fld[2*i+1].s   /* string in element i */
#define ValLoc(i)  fld[2*i+1].vloc /* value location in element i */
#define Intgr(i)   fld[2*i+1].n   /* integer in element i */
#define Array(i)   fld[2*i+1].cd  /* pointer to subarray in element i */

/*
 * union cd_fld - fields within a code struct.
 */
union cd_fld {
   int n;                  /* various integer values */
   char *s;                /* various string values */
   struct lentry *nvar;    /* symbol table entry for a named variable */
   struct code *cd;        /* various pointers to other pieces of code */
   struct c_fnc *fnc;      /* pointer to function information */
   struct centry *lit;     /* symbol table entry for a literal */
   struct sig_act *sa;     /* actions to take for a returned signal */
   struct sig_lst *sigref; /* pointer to func's reference to signal */
   struct val_loc *vloc;   /* value location */
   char *oper_nm;          /* name of run-time operation or NULL */
   };

/*
 * code - struct used to hold the internal representation of generated code.
 */
struct code {
   int cd_id;           /* kind of code: C_* */
   struct code *next;   /* next code fragment in list */
   struct code *prev;   /* previous code fragment in list */
   union cd_fld fld[1]; /* fields of code fragment, actual number varies */
   };

/*
 * NewCode - allocate a code structure with "size" fields.
 */
#define NewCode(size) (struct code *)alloc((unsigned int)\
    (sizeof(struct code) + (size-1) * sizeof(union cd_fld)))

/*
 * c_fnc contains information about a C function that implements a continuation.
 */
#define CF_SigOnly    1   /* this function only returns a signal */
#define CF_ForeignSig 2   /* may return foreign signal from a suspend */
#define CF_Mark       4   /* this function has been visited by fix_fncs() */
#define CF_Coexpr     8   /* this function implements a co-expression */
struct c_fnc {
   char prefix[PrfxSz+1];   /* function prefix */
   char frm_prfx[PrfxSz+1]; /* procedure frame prefix */
   int flag;	            /* CF_* flags */
   struct code cd;          /* start of code sequence */
   struct code *cursor;     /* place to insert more code into sequence */
   struct code *call_lst;   /* functions called by this function */
   struct code *creatlst;   /* list of creates in this function */
   struct sig_lst *sig_lst; /* signals returned by this function */
   int ref_cnt;             /* reference count for this function */
   struct c_fnc *next;
   };


/*
 * sig_lst - a list of signals returned by a continuation along with a count
 *  of the number of places each signal is returned.
 */
struct sig_lst {
   struct code *sig;     /* signal */
   int ref_cnt;          /* number of places returned */
   struct sig_lst *next;
   };

/*
 * op_symentry - entry in symbol table for an operation
 */
#define AdjNone  1   /* no adjustment to this argument */
#define AdjDrf   2   /* deref in place */
#define AdjNDrf  3   /* deref into a new temporary */
#define AdjCpy   4   /* copy into a new temporary */
struct op_symentry {
    int n_refs;          /* number of non-modifying references */
    int n_mods;          /* number of modifying referenced */
    int n_rets;          /* number of times directly returned from operation */
    int var_safe;        /* if arg is named var, it may be used directly */
    int adjust;          /* AdjNone, AdjInplc, or AdjToNew */
    int itmp_indx;       /* index of temporary C integer variable */
    int dtmp_indx;       /* index of temporary C double variable */
    struct val_loc *loc;
    };

extern int num_tmp;		/* number of temporary descriptor variables */
extern int num_itmp;		/* number of temporary C integer variables */
extern int num_dtmp;		/* number of temporary C double variables */
extern int num_sbuf;		/* number of string buffers */
extern int num_cbuf;		/* number of cset buffers */

extern struct code *bound_sig;  /* bounding signal for current procedure */

/*
 * statically declared "signals".
 */
extern struct code resume;
extern struct code contin;
extern struct code fallthru;
extern struct code next_fail;

extern struct val_loc ignore;    /* no values, just something to point at */
extern struct c_fnc *cur_fnc;    /* C function currently being built */
extern struct code *on_failure;  /* place to go on failure */

extern int lbl_seq_num;		/* next label sequence number */

extern char pre[PrfxSz];        /* next unused prefix */

extern struct op_symentry *cur_symtab; /* current operation symbol table */

#define SepFnc  1   /* success continuation goes in separate function */
#define SContIL 2   /* in line success continuation */
#define EndOper 3   /* success continuation goes at end of operation */

#define HasVal 1    /* type contains values */
#define HasLcl 2    /* type contains local variables */
#define HasPrm 4    /* type contains parameters */
#define HasGlb 8    /* type contains globals (including statics and elements) */
#define HasVar(x) ((x) & (HasLcl | HasPrm | HasGlb))
