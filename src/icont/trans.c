/*
 * trans.c - main control of the translation process.
 */

#include "../h/gsupport.h"
#include "tproto.h"
#include "../h/version.h"
#include "tglobals.h"
#include "tsym.h"
#include "tree.h"
#include "ttoken.h"

/*
 * Prototypes.
 */

static	void	trans1		(char *filename, char *tgtdir);

int tfatals;			/* number of fatal errors in file */
int afatals;			/* total number of fatal errors */
int nocode;			/* non-zero to suppress code generation */
int in_line;			/* current input line number */
int incol;			/* current input column number */
int peekc;			/* one-character look ahead */

/*
 * translate a number of files, returning an error count
 */
int trans(ifiles, tgtdir)
char **ifiles;
char *tgtdir;
   {
   afatals = 0;

   tmalloc();			/* allocate memory for translation */
   while (*ifiles) {
      trans1(*ifiles++, tgtdir);	/* translate each file in turn */
      afatals += tfatals;
      }
   tmfree();			/* free memory used for translation */

   /*
    * Report information about errors and warnings and be correct about it.
    */
   if (afatals == 1)
      report("1 error\n");
   else if (afatals > 1) {
      char tmp[12];
      sprintf(tmp, "%d errors\n", afatals);
      report(tmp);
      }
   else
      report("No errors\n");

   return afatals;
   }

/*
 * translate one file.
 */
static void trans1(filename, tgtdir)
char *filename, *tgtdir;
{
   char oname1[MaxPath];	/* buffer for constructing file name */
   char oname2[MaxPath];	/* buffer for constructing file name */

   tfatals = 0;			/* reset error counts */
   nocode = 0;			/* allow code generation */
   in_line = 1;			/* start with line 1, column 0 */
   incol = 0;
   peekc = 0;			/* clear character lookahead */

   if (!ppinit(filename,lpath,m4pre))
      quitf("cannot open %s",filename);

   if (strcmp(filename,"-") == 0)
      filename = "stdin";

   report(filename);

   if (pponly) {
      ppecho();
      return;
      }

   /*
    * Form names for the .u1 and .u2 files and open them.
    *  Write the ucode version number to the .u2 file.
    */
   makename(oname1, tgtdir, filename, U1Suffix);
   codefile = fopen(oname1, "w");
   if (codefile == NULL)
      quitf("cannot create %s", oname1);
   makename(oname2, tgtdir, filename, U2Suffix);
   globfile = fopen(oname2, "w");
   if (globfile == NULL)
      quitf("cannot create %s", oname2);
   writecheck(fprintf(globfile,"version\t%s\n",UVersion));

   tok_loc.n_file = filename;
   in_line = 1;

   tminit();				/* Initialize data structures */
   yyparse();				/* Parse the input */

   /*
    * Close the output files.
    */
   if (fclose(codefile) != 0 || fclose(globfile) != 0)
      quit("cannot close ucode file");
   if (tfatals) {
      remove(oname1);
      remove(oname2);
      }
   }

/*
 * writecheck - check the return code from a stdio output operation
 */
void writecheck(rc)
int rc;
   {
   if (rc < 0)
      quit("cannot write to ucode file");
}
