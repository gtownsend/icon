/*
 *  tlocal.c -- functions needed for different systems.
 */

#include "../h/gsupport.h"

/*
 * The following code is operating-system dependent [@tlocal.01].
 *  Routines needed by different systems.
 */

#if PORT
/* place to put anything system specific */
Deliberate Syntax Error
#endif					/* PORT */

#if AMIGA
#if LATTICE || __SASC
unsigned long _STACK = 20000;   /*   MNEED ALSO, PLEASE */
#endif					/* LATTICE || __SASC */
#if AZTEC_C
/*
 * abs
 */
abs(i)
int i;
{
   return ((i<0)? (-i) : i);
}

/*
 * getfa - get file attribute -1 == OK, 0 == ERROR, 1 == DIRECTORY
 */
getfa()
{
   return -1;
}
#endif					/* AZTEC_C */
#endif					/* AMIGA */

#if ARM
#include "kernel.h"

/* **** The following line causes a fatal error in some C preprocessors
   **** even if ARM is 0.  Remove the comment characters for ARM.
   ****
*/
/*#define QUOTE " \"\t"*/

int armquote (char *str, char **ret)
{
	char *p;
	static char buf[255];

	if (strpbrk(str,QUOTE) == NULL)
	{
		*ret = str;
		return strlen(str);
	}

	p = buf;

	while (*str && p < &buf[255])
	{
		if (strchr(QUOTE,*str))
		{
			if (p > &buf[252])
				return -1;

			*p++ = '\\';
			*p++ = *str;
		}
		else
			*p++ = *str;

		++str;
	}

	if (p >= &buf[255])
		return -1;

	*p = 0;
	*ret = buf;
	return (p - buf);
}

/* Takes a filename, with a ".u1" suffix, and swaps it, IN PLACE, to
 * conform to Archimedes conventions (u1 as a directory).
 * Note that this is a very simplified version. It relies on the following
 * facts:
 *
 *	1. In the ucode link directives, files ALWAYS end in .u1
 *	2. The input filename is writeable.
 *	3. Files which include directory parts conform to Archimedes
 *	   format (FS:dir.dir.file). Note that Unix formats such as
 *	   "/usr/icon/lib/time" are inherently non-portable, and NOT
 *	   supported.
 *
 * This function is only called from readglob() in C.Lglob.
 */
char *flipname(char *name)
{
	char *p = name + strlen(name) - 1;
	char *q = p - 3;

	/* Copy the leafname to the end */
	while (q >= name && *q != '.' && *q != ':')
		*p-- = *q--;

	/* Insert the "U1." before the leafname */
	*p-- = '.';
	*p-- = '1';
	*p-- = 'U';

	return name;
}
#endif					/* ARM */

#if ATARI_ST

unsigned long _STACK = 10240;   /*   MNEED ALSO, PLEASE */

#endif					/* ATARI_ST */

#if MACINTOSH
#if MPW
/* Routine to set file type and creator.
*/

#include <Files.h>

void
setfile(filename,type,creator)
char *filename;
OSType type,creator;
   {
   FInfo info;

   if (getfinfo(filename,0,&info) == 0) {
      info.fdType = type;
      info.fdCreator = creator;
      setfinfo(filename,0,&info);
      }
   return;
   }


/* Routine to quote strings for MPW
*/

char *
mpwquote(s)
char *s;
   {
   static char quotechar[] =
	 " \t\n\r#;&|()6'\"/\\{}`?E[]+*GH(<>3I7";
   static char *endq = quotechar + sizeof(quotechar);
   int quote = 0;
   char c,d,*sp,*qp,*cp,*q;
   char *malloc();

   sp = s;
   while (c = *sp++) {
      cp = quotechar;
      while ((d = *cp++) && c != d)
	 ;
      if (cp != endq) {
         quote = 1;
	 break;
	 }
      }
   if (quote) {
      qp = q = malloc(4 * strlen(s) + 1);
      *qp++ = '\'';
      sp = s;
      while (c = *sp++) {
	 if (c == '\'') {
	    *qp++ = '\'';
	    *qp++ = '6';
	    *qp++ = '\'';
	    *qp++ = '\'';
	    quote = 1;
	    }
	 else *qp++ = c;
	 }
      *qp++ = '\'';
      *qp++ = '\0';
      }
   else {
      q = malloc(strlen(s) + 1);
      strcpy(q,s);
      }
   return q;
   }


/*
 * SortOptions -- sorts icont options so that options and file names can
 * appear in any order.
 */
void
SortOptions(argv)
char *argv[];
   {
   char **last,**p,*q,**op,**fp,**optlist,**filelist,opt,*s,*malloc();
   int size,error = 0;;

   /*
    * Count parameters before -x.
    */
   ++argv;
   for (last = argv; *last != NULL && strcmp(*last,"-x") != 0; ++last)
      ;
   /*
    * Allocate a work area to build separate lists of options
    * and filenames.
    */
   size = (last - argv + 1) * sizeof(char*);
   optlist = filelist = NULL;
   op = optlist = (char **)malloc(size);
   fp = filelist = (char **)malloc(size);
   if (optlist && filelist) {			/* if allocations ok */
      for (p = argv; (s = *p); ++p) {		/* loop thru args */
         if (error) break;
	 if (s[0] == '-' && (opt = s[1]) != '\0') { /* if an option */
	    if (q = strchr(Options,opt)) {	/* if valid option */
	       *op++ = s;
	       if (q[1] == ':') {		/* if has a value */
		  if (s[2] != '\0') s += 2;	/* if value in this word */
		  else s = *op++ = *++p;	/* else value in next word */
		  if (s) {			/* if next word exists */
		     if (opt == 'S') {		/* if S option */
			if (s[0] == 'h') ++s;	/* bump past h */
			if (s[0]) ++s;		/* bump past letter */
			else error = 3;		/* error -- no letter */
			if (s[0] == '\0') {	/* if value in next word */
			   if ((*op++ = *++p) == NULL)
			         error = 4;	/* error -- no next word */
			   }
			}
		     }
		  else error = 1;	/* error -- missing value */
		  }
	       }
	       else error = 2;		/* error -- invalid option */
	    }
	 else {					/* else a file */
	    *fp++ = s;
	    }
	 }
      *op = NULL;
      *fp = NULL;
      if (!error) {
	 p = argv;
	 for (op = optlist; *op; ++op) *p++ = *op;
	 for (fp = filelist; *fp; ++fp) *p++ = *fp;
	 }
      }
   if (optlist) free(optlist);
   if (filelist) free(filelist);
   return;
   }
#endif					/* MPW */
#endif					/* MACINTOSH */

#if MSDOS

#if MICROSOFT

pointer xmalloc(n)
   long n;
   {
   return calloc((size_t)n,sizeof(char));
   }
#endif					/* MICROSOFT */

#if MICROSOFT
int _stack = (8 * 1024);
#endif					/* MICROSOFT */

#if TURBO
extern unsigned _stklen = 12 * 1024;
#endif					/* TURBO */

#if ZTC_386 || SCCX_MX
#ifndef DOS386
int _stack = (8 * 1024);
#endif					/* DOS386 */
#endif					/* ZTC_386 || SCCX_MX */

#endif					/* MSDOS */

#if MVS || VM
#if SASC
#include <options.h>
char _linkage = _OPTIMIZE;

#if MVS                 /* expect dsnames, not DDnames, as file names */
char *_style = "tso:";
#define SYS_OSVS
#else					/* MVS */
#define SYS_CMS
#endif					/* MVS */

#define RES_IOUTIL
#define RES_DSNAME

#include <resident.h>

#if VM
#include <cmsexec.h>
#endif					/* VM */
/*
 * No execvp, so turn it into a call to system.  (Then caller can exit.)
 * In VM, put the ICONX command on the CMS stack, and someone else will
 * do it after we're gone.  (system would clobber the user area.)
 */
int sysexec(cmd, argv)
   char *cmd;
   char **argv;
   {
#if MVS
      char *prefix = "tso:";
#else					/* MVS */
      char *prefix = "";
#endif					/* MVS */
      int cmdlen = strlen(cmd) + strlen(prefix) + 1;
      char **p;
      char *cmdstr, *next;

      for(p = argv+1; *p; ++p)
         cmdlen += strlen(*p) + 1;
      cmdstr = malloc(cmdlen);      /* blithely ignoring failure...  */
      strcpy(cmdstr, prefix);
      strcat(cmdstr, cmd);
      next = cmdstr + strlen(prefix) + strlen(cmd);
      for (p = argv+1; *p; ++p)
         {
             *next = ' ';
             strcpy(next+1, *p);
             next += strlen(*p) + 1;
          }
      *next = '\0';
#if MVS
      return(system(cmdstr));
#else					/* MVS */
      cmspush(cmdstr);
      return EXIT_SUCCESS;
#endif					/* MVS */
   }
#endif					/* SASC */
#endif					/* MVS || VM */

#if OS2
#endif					/* OS2 */

#if UNIX
#endif					/* UNIX */

#if VMS
#endif					/* VMS */

/*
 * End of operating-system specific code.
 */

static char *tjunk;			/* avoid empty module */
