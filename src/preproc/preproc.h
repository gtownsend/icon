#include "../h/gsupport.h"

/*
 * If Bell is not defined, determine the default value for the "bell"
 *   character.
 */
#ifndef Bell
#define Bell '\a'
#endif					/* Bell */

#define CBufSize 256  /* size of buffer for file input */

/*
 * Identification numbers for tokens for which there are no definitions
 *   generated from a C grammar by yacc.
 */
#define WhiteSpace 1001   /* white space */
#define PpNumber   1002   /* number (integer or real) */
#define PpIf       1003   /* #if */
#define PpElse     1004   /* #else */
#define PpIfdef    1005   /* #ifdef */
#define PpIfndef   1006   /* #ifndef */
#define PpElif     1007   /* #elif */
#define PpEndif    1008   /* #endif */
#define PpInclude  1009   /* #include */
#define PpDefine   1010   /* #define */
#define PpUndef    1011   /* #undef */
#define PpLine     1012   /* #line */
#define PpError    1013   /* #error */
#define PpPragma   1014   /* #pragma */
#define PpPaste    1015   /* ## */
#define PpDirEnd   1016   /* new-line terminating a directive */
#define PpHeader   1017   /* <...> from #include */
#define PpBegdef   1018   /* #begdef */
#define PpEnddef   1019   /* #enddef */
#define PpNull     1020   /* # */
#define PpKeep     1021   /* directive specific to an application, pass along */
#define PpSkip     1022   /* directive specific to an application discard */
#define Invalid    9999   /* marker */

extern char *progname; /* name of this program: for error messages */
extern int line_cntrl; /* flag: are line directives needed in the output */

/*
 * whsp_image determines whether the spelling of white space is not retained,
 *   is retained with each comment replaced by a space, or the full spelling
 *   of white space and comments is retained.
 */
#define NoSpelling 0
#define NoComment  1
#define FullImage  2

extern int whsp_image;

extern int max_recurse;        /* how much recursion is allows in macros */
extern struct token *zero_tok; /* token "0" */
extern struct token *one_tok;  /* token "1" */

extern int *first_char;        /* first character in tokenizing buffer */
extern int *next_char;         /* next character in tokenizing buffer */
extern int *last_char;         /* last character in tokenizing buffer */

/*
 * Entry in array of preprocessor directive names.
 */
struct rsrvd_wrd {
   char *s;         /* name (without the #) */
   int tok_id;      /* token id of directive */
   };

/*
 * token flags:
 */
#define LineChk  0x1   /* A line directive may be needed in the output */
#define NoExpand 0x2   /* Don't macro expand this identifier */

/*
 * Token.
 */
struct token {
   int tok_id;		/* token identifier */
   char *image;		/* string image of token */
   char *fname;         /* file name of origin */
   int line;            /* line number of origin */
   int flag;            /* token flag, see above */
   };

/*
 * Token list.
 */
struct tok_lst {
   struct token *t;      /* token */
   struct tok_lst *next; /* next entry in list */
   };

/*
 * Identifier list.
 */
struct id_lst {
   char *id;            /* identifier */
   struct id_lst *next; /* next entry in list */
   };

/*
 * a macro, m, falls into one of several categores:
 *   those with arguments                - m.category = # args >= 0
 *   those with no arguments             - m.category = NoArgs
 *   those that may not be chaged        - m.category = FixedMac
 *   those that require special handling - m.category = SpecMac
 */
#define NoArgs   -1
#define FixedMac -2
#define SpecMac  -3

struct macro {
   char *mname;
   int category;
   int multi_line;
   struct id_lst *prmlst;
   struct tok_lst *body;
   int ref_cnt;
   int recurse;
   struct macro *next;
   };

/*
 * states for recognizing preprocessor directives
 */
#define Reset    1
#define CanStart 2   /* Just saw a new-line, look for a directive */
#define Within   3   /* Next new-line ends directive */

/*
 * Information for a source of tokens created from a character stream.
 *  The characters may come from a file, or they be in a prefilled buffer.
 */
struct char_src {
   FILE *f;               /* file, if the chars come directly from a file */
   char *fname;		  /* name of file */
   int bufsize;		  /* size of character buffer */
   int *char_buf;         /* pointer to character buffer */
   int *line_buf;	  /* buffer of lines characters come from */
   int *next_char;	  /* next unprocessed character in buffer */
   int *last_char;	  /* last character in buffer */
   int line_adj;	  /* line adjustment caused by #line directive */
   int dir_state;	  /* state w.r.t. recognizing directives */
   struct token *tok_sav; /* used to save token after look ahead */
   };

/*
 * Information for a source of tokens dirived from expanding a macro.
 */
struct mac_expand {
   struct macro *m;	      /* the macro being expanded */
   struct tok_lst **args;     /* list of arguments for macro call */
   struct tok_lst **exp_args; /* list of expanded arguments for macro call */
   struct tok_lst *rest_bdy;  /* position within the body of the macro */
   };

/*
 * Elements in a list of token lists used for token pasting.
 */
struct paste_lsts {
   struct token *trigger;   /* the token pasting operator */
   struct tok_lst *tlst;    /* the token list */
   struct paste_lsts *next; /* the next element in the list of lists */
};

/*
 * Pointers to various token sources.
 */
union src_ref {
   struct char_src *cs;      /* source is tokenized characters */
   struct mac_expand *me;    /* source is macro expansion */
   struct tok_lst *tlst;     /* source is token list (a macro argument) */
   struct paste_lsts *plsts; /* source is token lists for token pasting */
   };

/*
 * Types of token sources:
 */
#define CharSrc   0     /* tokenized characters */
#define MacExpand 1     /* macro expansion */
#define TokLst    2     /* token list */
#define PasteLsts 4	/* paste last token of 1st list to first of 2nd */
#define DummySrc  5     /* base of stack */

#define NTokSav  2      /* maximum number of tokens that can be pushed back */

struct src {
   int flag;			/* indicate what kind of source it is */
   struct tok_lst *cond;	/* list of nested conditionals in effect */
   struct token *toks[NTokSav];	/* token push-back stack for preproc() */
   int ntoks;			/* number of tokens on stack */
   struct src *next;		/* link for creating stack */
   union src_ref u;             /* pointer to specific kind of source */
   };

extern struct src dummy;       /* base of stack */

extern struct src *src_stack;  /* source stack */

