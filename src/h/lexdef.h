/*
 * lexdef.h -- common definitions for use with the lexical analyzer.
 */

/*
 * Miscellaneous globals.
 */
extern int yychar;		/* parser's current input token type */
extern int yynerrs;		/* number of errors in parse */
extern int nocode;		/* true to suppress code generation */

extern int in_line;		/* current line number in input */
extern int incol;		/* current column number in input */
extern int peekc;		/* one character look-ahead */
extern FILE *srcfile;		/* current input file */

extern int tfatals;		/* total fatal errors */

/*
 * Token table structure.
 */

struct toktab {
   char *t_word;		/* token */
   int  t_type;			/* token type returned by yylex */
   int  t_flags;		/* flags for semicolon insertion */
   };

extern struct toktab toktab[];	/* token table */
extern struct toktab *restab[];	/* reserved word index */

#define T_Ident		&toktab[0]
#define T_Int		&toktab[1]
#define T_Real		&toktab[2]
#define T_String	&toktab[3]
#define T_Cset		&toktab[4]
#define T_Eof		&toktab[5]

/*
 * t_flags values for token table.
 */

#define Beginner 1		/* token can follow a semicolon */
#define Ender    2		/* token can precede a semicolon */

/*
 * optab contains token information along with pointers to implementation
 *  information for each operator. Special symbols are also included.
 */
#define Unary  1
#define Binary 2

struct optab {
   struct toktab tok;        /* token information for the operator symbol */
   int expected;	     /* what is expected in data base: Unary/Binary */
   struct implement *unary;  /* data base entry for unary version */
   struct implement *binary; /* data base entry for binary version */
   };

extern struct optab optab[]; /* operator table */
extern int asgn_loc;         /* index in optab of assignment */
extern int semicol_loc;      /* index in optab of semicolon */
extern int plus_loc;         /* index in optab of addition */
extern int minus_loc;        /* index in optab of subtraction */

/*
 * Miscellaneous.
 */

#define isoctal(c) ((c)>='0'&&(c)<='7')	/* macro to test for octal digit */
#define NextChar   nextchar()		/* macro to get next character */
#define PushChar(c) peekc=(c)		/* macro to push back a character */

#define Comment '#'			/* comment beginner */
#define Escape  '\\'			/* string literal escape character */
