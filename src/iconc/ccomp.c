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

#define ExeFlag "-o"
#define LinkLibs " -lm"
 
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
   l->libname = libname;
   l->nm_sz = strlen(libname);
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
   {
   struct lib *l;
   char sbuf[MaxPath];			/* file name construction buffer */
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

#ifdef Graphics
   lib_sz += strlen(" -L") +
             strlen(refpath) +
	     strlen(" -lIgpx ");
   lib_sz += strlen(ICONC_XLIB);
#endif					/* Graphics */

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
   strcat(s," -lIgpx ");
   strcat(s, ICONC_XLIB);
   s += strlen(s);
#endif					/* Graphics */

   strcpy(s, LinkLibs);

   if (system(buf) != 0)
      return EXIT_FAILURE;
   strcpy(buf, "strip ");
   s = buf + 6;
   strcpy(s, exename);
   system(buf);


   return EXIT_SUCCESS;
   }
