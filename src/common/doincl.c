/*
 * doincl.c -- expand include directives (recursively)
 *
 *  Usage:  doinclude [-o outfile] filename...
 *
 *  Doinclude copies a C source file, expanding non-system include directives.
 *  For each line of the form
 *	#include "filename"
 *  the named file is interpolated; all other lines are copied verbatim.
 *
 *  No error is generated if a file cannot be opened.
 */

#include "../h/rt.h"

void	doinclude	(char *fname);

#define MAXLINE 500	/* maximum line length */

FILE *outfile;		/* output file */

int main(argc, argv)
int argc;
char *argv[];
   {
   char *progname = argv[0];

   outfile = stdout;
   if (argc > 3 && strcmp(argv[1], "-o") == 0) {
      if ((outfile = fopen(argv[2], "w")) != NULL) {
         argv += 2;
         argc -= 2;
         }
      else {
         perror(argv[2]);
         exit(1);
         }
      }
   if (argc < 2) {
      fprintf(stderr, "usage: %s [-o outfile] filename...\n", progname);
      exit(1);
      }

   fprintf(outfile,
      "/***** do not edit -- this file was generated mechanically *****/\n\n");
   while (--argc > 0)
      doinclude(*++argv);
   exit(0);
   /*NOTREACHED*/
   }

void doinclude(fname)
char *fname;
   {
   FILE *f;
   char line[MAXLINE], newname[MAXLINE], *p;

   fprintf(outfile, "\n\n/****************************************");
   fprintf(outfile, "  from %s:  */\n\n", fname);
   if ((f = fopen(fname, "r")) != NULL) {
      while (fgets(line, MAXLINE, f))
         if (sscanf(line, " # include \"%s\"", newname) == 1) {
            for (p = newname; *p != '\0' && *p != '"'; p++)
               ;
            *p = '\0';				/* strip off trailing '"' */
            doinclude(newname);			/* include file */
            }
         else
            fputs(line, outfile);		/* not an include directive */
      fclose(f);
      }
   else {
      fprintf(outfile, "/* [file not found] */\n");
      }
   fprintf(outfile, "\n/****************************************");
   fprintf(outfile, "   end %s   */\n", fname);
   }
