/*
 * ctrans.c - main control of the translation process.
 */
#include "../h/gsupport.h"
#include "cglobals.h"
#include "ctrans.h"
#include "csym.h"
#include "ctree.h"
#include "ctoken.h"
#include "ccode.h"
#include "cproto.h"

/*
 * Prototypes.
 */
static void	trans1		(char *filename);

/*
 * Variables.
 */
int tfatals = 0;		/* total number of fatal errors */
int twarns = 0;			/* total number of warnings */
int nocode;			/* set by lexer; unused in compiler */
int in_line;			/* current input line number */
int incol;			/* current input column number */
int peekc;			/* one-character look ahead */
struct srcfile *srclst = NULL;	/* list of source files to translate */

static char *lpath;		/* LPATH value */

/*
 * translate a number of files, returning an error count
 */
int trans()
   {
   register struct pentry *proc;
   struct srcfile *sf;

   lpath = getenv("LPATH");	/* remains null if unspecified */

   for (sf = srclst; sf != NULL; sf = sf->next)
      trans1(sf->name);	/* translate each file in turn */

   if (!pponly) {
      /*
       * Resolve undeclared references.
       */
      for (proc = proc_lst; proc != NULL; proc = proc->next)
         resolve(proc);
   
#ifdef DeBug
      symdump();
#endif					/* DeBug */
   
      if (tfatals == 0) {
         chkstrinv();  /* see what needs be available for string invocation */
         chkinv();     /* perform "naive" optimizations */
         }
   
      if (tfatals == 0)
         typeinfer();        /* perform type inference */
   
      if (just_type_trace)
         return tfatals;     /* stop without generating code */
   
      if (tfatals == 0) {
         var_dcls();         /* output declarations for globals and statics */
         const_blks();       /* output blocks for cset and real literals */
         for (proc = proc_lst; proc != NULL; proc = proc->next)
            proccode(proc);  /* output code for a procedure */
         recconstr(rec_lst); /* output code for record constructors */
/* ANTHONY */
/*
	 print_ghash();
*/
      }
    }

   /*
    * Report information about errors and warnings and be correct about it.
    */
   if (tfatals == 1)
      fprintf(stderr, "1 error; ");
   else if (tfatals > 1)
      fprintf(stderr, "%d errors; ", tfatals);
   else if (verbose > 0)
      fprintf(stderr, "No errors; ");

   if (twarns == 1)
      fprintf(stderr, "1 warning\n");
   else if (twarns > 1)
      fprintf(stderr, "%d warnings\n", twarns);
   else if (verbose > 0)
      fprintf(stderr, "no warnings\n");
   else if (tfatals > 0)
      fprintf(stderr, "\n");

#ifdef TranStats
   tokdump();
#endif					/* TranStats */

   return tfatals;
   }

/*
 * translate one file.
 */
static void trans1(filename)
char *filename;
   {
   in_line = 1;			/* start with line 1, column 0 */
   incol = 0;
   peekc = 0;			/* clear character lookahead */

   if (!ppinit(filename,lpath?lpath:".",m4pre)) {
      tfatal(filename, "cannot open source file");
      return;
      }
   if (!largeints)		/* undefine predef symbol if no -l option */
      ppdef("_LARGE_INTEGERS", (char *)NULL);
   ppdef("_MULTITASKING", (char *)NULL);	/* never defined in compiler */
   ppdef("_EVENT_MONITOR", (char *)NULL);
   ppdef("_MEMORY_MONITOR", (char *)NULL);
   ppdef("_VISUALIZATION", (char *)NULL);

   if (strcmp(filename,"-") == 0)
      filename = "stdin";
   if (verbose > 0)
      fprintf(stderr, "%s:\n",filename);

   tok_loc.n_file = filename;
   in_line = 1;

   if (pponly)
      ppecho();			/* preprocess only */
   else
   yyparse();				/* Parse the input */
      }

/*
 * writecheck - check the return code from a stdio output operation
 */
void writecheck(rc)
   int rc;

   {
   if (rc < 0)
      quit("unable to write to icode file");
   }

/*
 * lnkdcl - find file locally or on LPATH and add to source list.
 */
void lnkdcl(name)
char *name;
{
   struct srcfile **pp;
   struct srcfile *p;
   char buf[MaxPath];

   if (pathfind(buf, lpath, name, SourceSuffix))
      src_file(buf);
   else
      tfatal("cannot resolve reference to file name", name);
      }

/*
 * src_file - add the file name to the list of source files to be translated,
 *   if it is not already on the list.
 */
void src_file(name)
char *name;
   {
   struct srcfile **pp;
   struct srcfile *p;

   for (pp = &srclst; *pp != NULL; pp = &(*pp)->next)
     if (strcmp((*pp)->name, name) == 0)
        return;
   p = NewStruct(srcfile);
   p->name = salloc(name);
   p->next = NULL;
   *pp = p;
}
