/*
 * getopt.c -- get command-line options.
 */

#include "../h/gsupport.h"

#ifndef SysOpt
extern char* progname;

/*
 * Based on a public domain implementation of System V
 *  getopt(3) by Keith Bostic (keith@seismo), Aug 24, 1984.
 */

#define BadCh	(int)'?'
#define EMSG	""
#define tell(m)	fprintf(stderr,"%s: %s -- %c\n",progname,m,optopt);return BadCh;

int optind = 1;		/* index into parent argv vector */
int optopt;		/* character checked for validity */
char *optarg;		/* argument associated with option */

int getopt(int nargc, char *const nargv[], const char *ostr)
   {
   static char *place = EMSG;		/* option letter processing */
   register char *oli;			/* option letter list index */

   if(!*place) {			/* update scanning pointer */
      if(optind >= nargc || *(place = nargv[optind]) != '-' || !*++place)
         return EOF;
      if (*place == '-') {		/* found "--" */
         ++optind;
         return EOF;
         }
      }					/* option letter okay? */

   if (((optopt=(int)*place++) == (int)':') || (oli=strchr(ostr,optopt)) == 0) {
      if(!*place) ++optind;
      tell("illegal option");
      }
   if (*++oli != ':') {			/* don't need argument */
      optarg = NULL;
      if (!*place) ++optind;
      }
   else {				/* need an argument */
      if (*place) optarg = place;	/* no white space */
      else if (nargc <= ++optind) {	/* no arg */
         place = EMSG;
         tell("option requires an argument");
         }
      else optarg = nargv[optind];	/* white space */
      place = EMSG;
      ++optind;
      }
   return optopt;			/* dump back option letter */
   }
#endif					/* SysOpt */
