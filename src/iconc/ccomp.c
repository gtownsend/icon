/*
 * ccomp.c - routines for compiling and linking the C program produced
 *   by the translator.
 */
#include "../h/gsupport.h"
#include "cglobals.h"
#include "ctrans.h"
#include "ctree.h"
#include "ccode.h"
#include "csym.h"
#include "cproto.h"

extern char *refpath;

/*
 * The following code is operating-system dependent [@tccomp.01].  Definition
 *  of ExeFlag and LinkLibs.
 */

#if PORT
   /* something is needed */
Deliberate Syntax Error
#endif						/* PORT */

#if UNIX || AMIGA || ATARI_ST || MACINTOSH || MSDOS || MVS || VM || OS2
#define ExeFlag "-o"
#define LinkLibs " -lm"
#endif						/* UNIX ... */
 
#if VMS
#include file
#define ExeFlag "link/exe="
#define LinkLibs ""
#endif						/* VMS */

/*
 * End of operating-system specific code.
 */

/*
 * Structure to hold the list of Icon run-time libraries that must be
 *  linked in.
 */
struct lib {
   char *libname;
   int nm_sz;
   struct lib *next;
   };
static struct lib *liblst;
static int lib_sz = 0;

/*
 * addlib - add a new library to the list the must be linked.
 */
void addlib(libname)
char *libname;
   {
   static struct lib **nxtlib = &liblst;
   struct lib *l;

   l = NewStruct(lib);

/*
 * The following code is operating-system dependent [@tccomp.02].
 *   Change path syntax if necessary.
 */

#if PORT
   /* something is needed */
Deliberate Syntax Error
#endif						/* PORT */

#if UNIX || AMIGA || ATARI_ST || MACINTOSH || MSDOS || MVS || OS2 || VM
   l->libname = libname;
   l->nm_sz = strlen(libname);
#endif						/* UNIX ... */

#if VMS
   /* change directory string to VMS format */
   {
      struct fileparts *fp;
      char   *newlibname = alloc(strlen(libname) + 20);

      strcpy(newlibname, libname);
      fp = fparse(libname);
      if (strcmp(fp->name, "rt") == 0 && strcmp(fp->ext, ".olb") == 0)
         strcat(newlibname, "/lib/include=data");
      else
	 strcat(newlibname, "/lib");
      l->libname = newlibname;
      l->nm_sz = strlen(newlibname);
      }
#endif						/* VMS */

/*
 * End of operating-system specific code.
 */

   l->next = NULL;
   *nxtlib = l;
   nxtlib = &l->next;
   lib_sz += l->nm_sz + 1;
   }

/*
 * ccomp - perform C compilation and linking.
 */
int ccomp(srcname, exename)
char *srcname;
char *exename;
#if MSDOS
   {
   return EXIT_SUCCESS;			/* can't do from inside */
   }
#else					/* MSDOS */
   {
   struct lib *l;
   char sbuf[MaxFileName];		/* file name construction buffer */
   char *buf;
   char *s;
   char *dlrgint;
   int cmd_sz, opt_sz, flg_sz, exe_sz, src_sz;

   /*
    * Compute the sizes of the various parts of the command line
    *  to do the compilation.
    */
   cmd_sz = strlen(c_comp);
   opt_sz = strlen(c_opts);
   flg_sz = strlen(ExeFlag);
   exe_sz = strlen(exename);
   src_sz = strlen(srcname);
   lib_sz += strlen(LinkLibs);
   if (!largeints) {
      dlrgint = makename(sbuf, refpath, "dlrgint", ObjSuffix);
      lib_sz += strlen(dlrgint) + 1;
      }

/*
 * The following code is operating-system dependent [@tccomp.03].
 *  Construct the command line to do the compilation.
 */

#if PORT
   /* something is needed */
Deliberate Syntax Error
#endif						/* PORT */

#if AMIGA || ATARI_ST || MACINTOSH || MSDOS || MVS || VM || OS2
   /* something may be needed */
Deliberate Syntax Error
#endif						/* AMIGA || ... */

#if UNIX

#ifdef Graphics
   lib_sz += strlen(" -L") +
             strlen(refpath) +
	     strlen(" -lXpm ");
   lib_sz += strlen(ICONC_XLIB);
#endif						/* Graphics */

   buf = alloc((unsigned int)cmd_sz + opt_sz + flg_sz + exe_sz + src_sz +
			     lib_sz + 5);
   strcpy(buf, c_comp);
   s = buf + cmd_sz;
   *s++ = ' ';
   strcpy(s, c_opts);
   s += opt_sz;
   *s++ = ' ';
   strcpy(s, ExeFlag);
   s += flg_sz;
   *s++ = ' ';
   strcpy(s, exename);
   s += exe_sz;
   *s++ = ' ';
   strcpy(s, srcname);
   s += src_sz;
   if (!largeints) {
      *s++ = ' ';
      strcpy(s, dlrgint);
      s += strlen(dlrgint);
      }
   for (l = liblst; l != NULL; l = l->next) {
      *s++ = ' ';
      strcpy(s, l->libname);
      s += l->nm_sz;
      }

#ifdef Graphics
   strcpy(s," -L");
   strcat(s, refpath);
   strcat(s," -lXpm ");
   strcat(s, ICONC_XLIB);
   s += strlen(s);
#endif						/* Graphics */

   strcpy(s, LinkLibs);

   if (system(buf) != 0)
      return EXIT_FAILURE;
   strcpy(buf, "strip ");
   s = buf + 6;
   strcpy(s, exename);
   system(buf);
#endif						/* UNIX ... */

#if VMS

#ifdef Graphics
#ifdef HaveXpmFormat
   lib_sz += strlen(refpath) + strlen("Xpm/lib,");
#endif						/* HaveXpmFormat */
   lib_sz += 1 + strlen(refpath) + strlen("X11.opt/opt");
#endif						/* Graphics */

   buf = alloc((unsigned int)cmd_sz + opt_sz + flg_sz + exe_sz + src_sz +
			     lib_sz + 5);
   strcpy(buf, c_comp);
   s = buf + cmd_sz;
   strcpy(s, c_opts);
   s += opt_sz;
   *s++ = ' ';
   strcpy(s, srcname);

   if (system(buf) == 0)
      return EXIT_FAILURE;
   strcpy(buf, ExeFlag);
   s = buf + flg_sz;
   strcpy(s, exename);
   s += exe_sz;
   *s++ = ' ';
   strcpy(s, srcname);
   s += src_sz - 1;
   strcpy(s, "obj");
   s += 3;
   if (!largeints) {
      *s++ = ',';
      strcpy(s, dlrgint);
      s += strlen(dlrgint);
      }
   for (l = liblst; l != NULL; l = l->next) {
      *s++ = ',';
      strcpy(s, l->libname);
      s += l->nm_sz;
      }
#ifdef Graphics
   strcat(s, ",");
#ifdef HaveXpmFormat
   strcat(s, refpath);
   strcat(s, "Xpm/lib,");
#endif						/* HaveXpmFormat */
   strcat(s, refpath);
   strcat(s, "X11.opt/opt");
#endif						/* Graphics */

   if (system(buf) == 0)
      return EXIT_FAILURE;
#endif						/* VMS */

/*
 * End of operating-system specific code.
 */

   return EXIT_SUCCESS;
   }
#endif					/* MSDOS */
