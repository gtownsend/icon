/*
 * Structure of a tree node.
 */

typedef	struct node	*nodeptr;

/*
 * Kinds of fields in syntax tree node.
 */
union field {
   long n_val;		 /* integer-valued fields */
   char *n_str;		 /* string-valued fields */
   struct lentry *lsym;  /* fields referencing local symbol table entries */ 
   struct centry *csym;  /* fields referencing constant symbol table entries */
   struct implement *ip; /* fields referencing an operation */
   struct pentry *proc;	 /* pointer to procedure entry */
   struct rentry *rec;	 /* pointer to record entry */
#ifdef OptimizeType
   struct typinfo *typ;  /* extra type field */
#else					/* OptimizeType */
   unsigned int *typ;    /* extra type field */
#endif					/* OptimizeType */
   nodeptr n_ptr;	 /* subtree pointers */
   };

/*
 * A store is an array that maps variables types (which are given indexes)
 *  to the types stored within the variables.
 */
struct store {
   struct store *next;
   int perm;               /* flag: whether store stays across iterations */
#ifdef OptimizeType
   struct typinfo *types[1]; /* actual size is number of variables */
#else					/* OptimizeType */
   unsigned int *types[1]; /* actual size is number of variables */
#endif					/* OptimizeType */
   };

/*
 * Array of parameter types for an operation call.
 */
struct symtyps {
   int nsyms;                /* number of parameter symbols */
   struct symtyps *next;
#ifdef OptimizeType
   struct typinfo *types[1];   /* really one for every symbol */
#else					/* OptimizeType */
   unsigned int *types[1];   /* really one for every symbol */
#endif					/* OptimizeType */
   };

/*
 * definitions for maintaining allocation status.
 */
#define NotAlloc 0   /* temp var neither in use nor reserved */
#define InUnse   1   /* temp var currently contains live variable */
/*            n < 0     reserved: must be free by node with postn field = n */

#define DescTmp 1    /* allocation of descriptor temporary */
#define CIntTmp 2    /* allocation of C integer temporary */
#define CDblTmp 3    /* allocation of C double temporary */
#define SBuf    4    /* allocation of string buffer */
#define CBuf    5    /* allocation of cset buffer */

struct freetmp {     /* list of things to free at a node */
   int kind;         /*   DescTmp, CIntTmp, CDblTmp, SBuf, or CBuf */
   int indx;         /*   index into status array */
   int old;          /*   old status */
   struct freetmp *next;
   };

struct node {
   int n_type;		    /* node type */
   char *n_file;	    /* name of file containing source program */
   int n_line;		    /* line number in source program */
   int n_col;		    /* column number in source program */
   int flag;
   int *new_types;          /* pntr to array of struct types created here */
#ifdef OptimizeType
   struct typinfo *type;	    /* type of this expression */
#else					/* OptimizeType */
   unsigned int *type;		    /* type of this expression */
#endif					/* OptimizeType */
   struct store *store;     /* if needed, store saved between iterations */
   struct symtyps *symtyps; /* for operation in data base: types of arg syms */
   nodeptr lifetime;        /* lifetime of intermediate result */
   int reuse;               /* result may be reused without being recomputed */
   nodeptr intrnl_lftm;     /* lifetime of variables internal to operation */
   int postn;               /* relative position of node in execution order */
   struct freetmp *freetmp; /* temporary variables to free at this point */
   union field n_field[1];  /* node fields */
   };

/*
 * NewNode - allocate a parse tree node with "size" fields.
 */
#define NewNode(size) (struct node *)alloc((unsigned int)\
    (sizeof(struct node) + (size-1) * sizeof(union field)))

/*
 * Macros to access fields of parse tree nodes.
 */

#define Type(t)		t->n_type
#define File(t)		t->n_file
#define Line(t)		t->n_line
#define Col(t)		t->n_col
#define Tree0(t)	t->n_field[0].n_ptr
#define Tree1(t)	t->n_field[1].n_ptr
#define Tree2(t)	t->n_field[2].n_ptr
#define Tree3(t)	t->n_field[3].n_ptr
#define Tree4(t)	t->n_field[4].n_ptr
#define Val0(t)		t->n_field[0].n_val
#define Val1(t)		t->n_field[1].n_val
#define Val2(t)		t->n_field[2].n_val
#define Val3(t)		t->n_field[3].n_val
#define Val4(t)		t->n_field[4].n_val
#define Str0(t)		t->n_field[0].n_str
#define Str1(t)		t->n_field[1].n_str
#define Str2(t)		t->n_field[2].n_str
#define Str3(t)		t->n_field[3].n_str
#define LSym0(t)	t->n_field[0].lsym
#define CSym0(t)	t->n_field[0].csym
#define Impl0(t)	t->n_field[0].ip
#define Impl1(t)	t->n_field[1].ip
#define Rec1(t)		t->n_field[1].rec
#define Proc1(t)	t->n_field[1].proc
#define Typ4(t)		t->n_field[4].typ

/*
 * External declarations.
 */

extern nodeptr yylval;		/* parser's current token value */
extern struct node tok_loc;     /* "model" token holding current location */

/*
 * Node types.
 */

#define N_Activat	 1		/* activation control structure */
#define N_Alt		 2		/* alternation operator */
#define N_Apply		 3		/* procedure application */
#define N_Augop		 4		/* augmented operator */
#define N_Bar		 5		/* generator control structure */
#define N_Break		 6		/* break statement */
#define N_Case		 7		/* case statement */
#define N_Ccls		 8		/* case clause */
#define N_Clist		 9		/* list of case clauses */
#define N_Create	10		/* create control structure */
#define N_Cset		11		/* cset literal */
#define N_Elist		12		/* list of expressions */
#define N_Empty		13		/* empty expression or statement */
#define N_Field		14		/* record field reference */
#define N_Id		15		/* identifier token */
#define N_If		16		/* if-then-else statement */
#define N_Int		17		/* integer literal */
#define N_Invok		18		/* invocation */
#define N_InvOp		19		/* invoke operation */
#define N_InvProc	20		/* invoke operation */
#define N_InvRec	21		/* invoke operation */
#define N_Limit		22		/* LIMIT control structure */
#define N_Loop		23		/* while, until, every, or repeat */
#define N_Next		24		/* next statement */
#define N_Not		25		/* not prefix control structure */
#define N_Op		26		/* operator token */
#define N_Proc		27		/* procedure */
#define N_Real		28		/* real literal */
#define N_Res		29		/* reserved word token */
#define N_Ret		30		/* fail, return, or succeed */
#define N_Scan		31		/* scan-using statement */
#define N_Sect		32		/* s[i:j] (section) */
#define N_Slist		33		/* list of statements */
#define N_Str		34		/* string literal */
#define N_SmplAsgn	35		/* simple assignment to named var */ 
#define N_SmplAug	36		/* simple assignment to named var */ 

#define AsgnDirect 0  /* rhs of special := can compute directly into var */
#define AsgnCopy   1  /* special := must copy result into var */
#define AsgnDeref  2  /* special := must dereference result into var */


/*
 * Macros for constructing basic nodes.
 */

#define CsetNode(a,b)		i_str_leaf(N_Cset,&tok_loc,a,b) 
#define IdNode(a)		c_str_leaf(N_Id,&tok_loc,a) 
#define IntNode(a)		c_str_leaf(N_Int,&tok_loc,a) 
#define OpNode(a)		int_leaf(N_Op,&tok_loc,a) 
#define RealNode(a)		c_str_leaf(N_Real,&tok_loc,a) 
#define ResNode(a)		int_leaf(N_Res,&tok_loc,a) 
#define StrNode(a,b)		i_str_leaf(N_Str,&tok_loc,a,b) 

/*
 * MultiUnary - create subtree from an operator symbol that represents
 *  multiple unary operators.
 */
#define MultiUnary(a,b) multiunary(optab[Val0(a)].tok.t_word, a, b)
