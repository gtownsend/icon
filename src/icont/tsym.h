/*
 * Structures for symbol table entries.
 */

struct tlentry {			/* local table entry */
   struct tlentry *l_blink;	/*   link for bucket chain */
   char *l_name;		/*   name of variable */
   int l_flag;			/*   variable flags */
   int l_index;			/*   "index" of local in table */
   struct tlentry *l_next;      /*   next local in table */
   };

struct tgentry {			/* global table entry */
   struct tgentry *g_blink;	/*   link for bucket chain */
   char *g_name;		/*   name of variable */
   int g_flag;			/*   variable flags */
   int g_nargs;			/*   number of args (procedure) or */
   int g_index;			/*   "index" of global in table */
   struct tgentry *g_next;      /*   next global in table */
   };				/*     number of fields (record) */

struct tcentry {			/* constant table entry */
   struct tcentry *c_blink;	/*   link for bucket chain */
   char *c_name;		/*   pointer to string */
   int c_length;		/*   length of string */
   int c_flag;			/*   type of literal flag */
   int c_index;			/*   "index" of constant in table */
   struct tcentry *c_next;      /*   next constant in table */
   };

/*
 * Flag values.
 */

#define F_Global	    01	/* variable declared global externally */
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

extern struct tlentry **lhash;	/* hash area for local table */
extern struct tgentry **ghash;	/* hash area for global table */
extern struct tcentry **chash;	/* hash area for constant table */

extern struct tlentry *lfirst;	/* first local table entry */
extern struct tlentry *llast;	/* last local table entry */
extern struct tcentry *cfirst;	/* first constant table entry */
extern struct tcentry *clast;	/* last constant table entry */
extern struct tgentry *gfirst;	/* first global table entry */
extern struct tgentry *glast;	/* last global table entry */

/*
 * Hash functions for symbol tables.
 */

#define ghasher(x)	(((word)x)&gmask)	 /* global symbol table */
#define lhasher(x)	(((word)x)&lmask)	 /* local symbol table */
#define chasher(x)	(((word)x)&cmask)	 /* constant symbol table */
