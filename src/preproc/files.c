/*
 * This file contains routines for setting up characters sources from
 *  files. It contains code to handle the search for include files.
 */
#include "../preproc/preproc.h"
/*
 * The following code is operating-system dependent [@files.01].
 *  System header files needed for handling paths.
 */

#if PORT
   /* something may be needed */
Deliberate Syntax Error
#endif					/* PORT */

#if AMIGA
   /* Amiga paths are not converted. 
    *  Absolute paths have the form Volume:dir/dir/.../file
    */
#define IsRelPath(fname) (strchr(fname, ':') == NULL)
#endif					/* AMIGA */

#if ATARI_ST || VM || MVS
   /* something may be needed */
Deliberate Syntax Error
#endif					/* ATARI_ST || ... */

#if MACINTOSH
char *FileNameMacToUnix(char *fn);
char *FileNameUnixToMac(char *fn);
char *FileNameMacConvert(char *(*func)(char *),char *fn);
#define IsRelPath(fname) (fname[0] != '/')
#endif					/* MACINTOSH */

#if MSDOS
#if MICROSOFT || INTEL_386 || HIGHC_386 || ZTC_386 || WATCOM
   /* nothing is needed */
#endif					/* MICROSOFT || INTEL_386 || ... */
#if TURBO || BORLAND_286 || BORLAND_386
#include <dir.h>
#endif 					/* TURBO || BORLAND_286 ... */
#define IsRelPath(fname) (fname[0] != '/')
#endif					/* MSDOS */

#if UNIX || VMS
#define IsRelPath(fname) (fname[0] != '/')
#endif					/* UNIX || VMS */

/*
 * End of operating-system specific code.
 */

#include "../preproc/pproto.h"

/*
 * Prototype for static function.
 */
static void file_src (char *fname, FILE *f);

static char **incl_search; /* standard locations to search for header files */

/*
 * file_src - set up the structures for a characters source from a file,
 *  putting the source on the top of the stack.
 */
static void file_src(fname, f)
char *fname;
FILE *f;
   {
   union src_ref ref;

/*
 * The following code is operating-system dependent [@files.02].
 *  Insure that path syntax is in Unix format for internal consistency
 *  (note, this may not work well on all systems).
 *  In particular, relative paths may begin with a / in AmigaDOS, where
 *  /filename is equivalent to the UNIX path ../filename.
 */

#if PORT
   /* something may be needed */
Deliberate Syntax Error
#endif					/* PORT */

#if AMIGA
   /* nothing is needed */
#endif					/* AMIGA */

#if ATARI_ST || VM || MVS
   /* something may be needed */
Deliberate Syntax Error
#endif					/* ATARI_ST || ... */

#if MACINTOSH
   fname = FileNameMacConvert(FileNameMacToUnix,fname);
#endif					/* MACINTOSH */

#if MSDOS
   char *s;
   
   /*
    * Convert back slashes to slashes for internal consistency.
    */
   fname = (char *)strdup(fname);
   for (s = fname; *s != '\0'; ++s)
      if (*s == '\\')
         *s = '/';
#endif					/* MSDOS */

#if UNIX || VMS
   /* nothing is needed */
#endif					/* UNIX || VMS */

/*
 * End of operating-system specific code.
 */

   ref.cs = new_cs(fname, f, CBufSize);
   push_src(CharSrc, &ref);
   next_char = NULL;
   fill_cbuf();
   }

/*
 * source - Open the file named fname or use stdin if fname is "-". fname
 *  is the first file from which to read input (that is, the outermost file).
 */
void source(fname)
char *fname;
   {
   FILE *f;

   if (strcmp(fname, "-") == 0)
      file_src("<stdin>", stdin);
   else {
      if ((f = fopen(fname, "r")) == NULL) 
         err2("cannot open ", fname);
      file_src(fname, f);
      }
   }

/*
 * include - open the file named fname and make it the current input file. 
 */
void include(trigger, fname, system)
struct token *trigger;
char *fname;
int system;
   {
   struct str_buf *sbuf;
   char *s;
   char *path;
   char *end_prfx;
   struct src *sp;
   struct char_src *cs;
   char **prefix;
   FILE *f;

   /*
    * See if this is an absolute path name.
    */
   if (IsRelPath(fname)) {
      sbuf = get_sbuf();
#if  !MVS && !VM					/* ??? */
      f = NULL;
      if (!system) {
         /*
          * This is not a system include file, so search the locations
          *  of the "ancestor files".
          */
         sp = src_stack;
         while (f == NULL && sp != NULL) {
            if (sp->flag == CharSrc) {
               cs = sp->u.cs;
               if (cs->f != NULL) {
                  /*
                   * This character source is a file.
                   */
                  end_prfx = NULL;
                  for (s = cs->fname; *s != '\0'; ++s)
                     if (*s == '/')
                        end_prfx = s;
                  if (end_prfx != NULL) 
#if MACINTOSH
		     /*
		      * For Mac-style names, don't include the file
		      * separator character in the prefix.
		      */
                     for (s = cs->fname; s < end_prfx; ++s)
#else					/* MACINTOSH */
                     for (s = cs->fname; s <= end_prfx; ++s)
#endif					/* MACINTOSH */
                        AppChar(*sbuf, *s);
                  for (s = fname; *s != '\0'; ++s)
                     AppChar(*sbuf, *s);
                  path = str_install(sbuf);
#if MACINTOSH
		  /*
		   * Convert UNIX-style path to Mac-style.
		   */
		  path = FileNameMacConvert(FileNameUnixToMac,path);
#endif					/* MACINTOSH */
                  f = fopen(path, "r");
                  }
               }
            sp = sp->next;
            }
         }
      /*
       * Search in the locations for the system include files.
       */   
      prefix = incl_search;
      while (f == NULL && *prefix != NULL) {
         for (s = *prefix; *s != '\0'; ++s)
            AppChar(*sbuf, *s);
         if (s > *prefix && s[-1] != '/')
            AppChar(*sbuf, '/');
         for (s = fname; *s != '\0'; ++s)
            AppChar(*sbuf, *s);
         path = str_install(sbuf);
#if MACINTOSH
	 /*
	  * Convert UNIX-style path to Mac-style.
	  */
	 path = FileNameMacConvert(FileNameUnixToMac,path);
#endif					/* MACINTOSH */
         f = fopen(path, "r");
         prefix = ++prefix;
         }
      rel_sbuf(sbuf);
#else					/* !MVS && !VM */
      if (system) {
         for (s = "ddn:SYSLIB("; *s != '\0'; ++s)
            AppChar(*sbuf, *s);
         for (s = fname; *s != '\0' && *s != '.'; ++s)
            AppChar(*sbuf, *s);
         AppChar(*sbuf, ')');
         }
      else {
         char *t;
         for (s = "ddn:"; *s != '\0'; ++s)
            AppChar(*sbuf, *s);
         t = fname;
         do {
            for (s = t; *s != '/' && *s != '\0'; ++s);
            if (*s != '\0') t = s+1;
            } while (*s != '\0');
         for (s = t; *s != '.' && *s != '\0'; ++s);
         if (*s == '\0') {
            AppChar(*sbuf, 'H');
            }
         else for (++s; *s != '\0'; ++s)
            AppChar(*sbuf, *s);
         AppChar(*sbuf, '(');
         for (; *t != '.' && *t != '\0'; ++t)
            AppChar(*sbuf, *t);
         AppChar(*sbuf, ')');
         }
      path = str_install(sbuf);
      f = fopen(path, "r");
      rel_sbuf(sbuf);
#endif					/* !MVS && !VM */
      }
   else {                               /* The path is absolute. */
      path = fname;
      f = fopen(path, "r");
      }

   if (f == NULL)
      errt2(trigger, "cannot open include file ", fname);
   file_src(path, f);
   }

/*
 * init_files - Initialize this module, setting up the search path for
 *  system header files.
 */
void init_files(opt_lst, opt_args)
char *opt_lst;
char **opt_args;
   {
   int n_paths = 0;
   int i, j;
   char *s, *s1;
  
/*
 * The following code is operating-system dependent [@files.03].
 *  Determine the number of standard locations to search for
 *  header files and provide any declarations needed for the code
 *  that establishes these search locations.
 */

#if PORT
   /* probably needs something */
Deliberate Syntax Error
#endif					/* PORT */

#if VMS
   char **syspaths;
   int  vmsi;

   n_paths = vmsi = 0;
   syspaths = (char **)alloc((unsigned int)(sizeof(char *) * 2));
   if (syspaths[n_paths] = getenv("VAXC$INCLUDE")) {
      n_paths++;
      vmsi++;
      }
   if (syspaths[n_paths] = getenv("SYS$LIBRARY")) {
      n_paths++;
      vmsi++;
      }
#endif					/* VMS */

#if AMIGA
   static char *sysdir = "include:";

   n_paths = 1;
#endif					/* AMIGA */

#if ATARI_ST || VM || MVS || (MACINTOSH && !MPW && !THINK_C)
   /* probably needs something */
Deliberate Syntax Error
#endif					/* ATARI_ST || ... */

#if MACINTOSH
#if THINK_C
   char *sysdir = FileNameMacConvert(FileNameMacToUnix, "MacintoshHD:THINK C:THINK C:Standard Libraries:C headers");
   n_paths = 1;
#endif
#if MPW
   /*
    * For MPW, environment variable CIncludes says where to look.
    */
   char *sysdir = FileNameMacConvert(FileNameMacToUnix,getenv("CIncludes"));
   n_paths = 1;
#endif					/* MPW */
#endif					/* MACINTOSH */

#if MSDOS
#if HIGHC_386 || INTEL_386 || WATCOM
   /* punt for now */
   n_paths = 0;
#endif					/* HIGHC_386 || INTEL_386 || ... */

#if MICROSOFT || NT
   char *syspath;
   char *cl_var;
   char *incl_var;
   
   incl_var = getenv("INCLUDE");
   cl_var = getenv("CL");
   n_paths = 0;

   /*
    * Check the CL environment variable for -I and -X options.
    */
   if (cl_var != NULL) {
      s = cl_var;
      while (*s != '\0') {
         if (*s == '/' || *s == '-') {
            ++s;
            if (*s == 'I') {
               ++n_paths;
               ++s;
               while (*s == ' ' || *s == '\t')
                  ++s;
               while (*s != ' ' && *s != '\t' && *s != '\0')
                  ++s;
               }
            else if (*s == 'X')
               incl_var = NULL;		/* ignore INCLUDE environment var */
            }
         if (*s != '\0')
            ++s;
         }
      }

   /*
    * Check the INCLUDE environment variable for standard places to
    *  search.
    */
   if (incl_var == NULL)
      syspath = "";
   else {
      syspath = (char *)strdup(incl_var);
      if (*incl_var != '\0')
         ++n_paths;
      while (*incl_var != '\0')
         if (*incl_var++ == ';' && *incl_var != '\0')
            ++n_paths;
      }
#endif					/* MICROSOFT || NT */

#if ZTC_386
   char *syspath;
   char *cl_var;
   char *incl_var;
   
   incl_var = getenv("INCLUDE");
   cl_var = getenv("CFLAGS");
   n_paths = 0;

   /*
    * Check the CFLAGS environment variable for -I options.
    */
   if (cl_var != NULL) {
      s = cl_var;
      while (*s != '\0') {
         if (*s == '/' || *s == '-') {
            ++s;
            if (*s == 'I') {
               ++n_paths;
               ++s;
               while (*s == ' ' || *s == '\t')
                  ++s;
               while (*s != ' ' && *s != '\t' && *s != '\0') {
                  if (*s == ';')
                  	++n_paths;
                  ++s;
                  }
               }
            }
         if (*s != '\0')
            ++s;
         }
      }

   /*
    * Check the INCLUDE environment variable for standard places to
    *  search.
    */
   if (incl_var == NULL)
      syspath = "";
   else {
      syspath = (char *)strdup(incl_var);
      if (*incl_var != '\0')
         ++n_paths;
      while (*incl_var != '\0')
         if (*incl_var++ == ';' && *incl_var != '\0')
            ++n_paths;
      }
#endif					/* ZTC_386 */

#if TURBO || BORLAND_286 || BORLAND_386
    char *cfg_fname;
    FILE *cfg_file = NULL;
    struct str_buf *sbuf;
    int c;

    /*
     * Check the configuration files for -I options.
     */
    n_paths = 0;
    cfg_fname = searchpath("turboc.cfg");
    if (cfg_fname != NULL && (cfg_file = fopen(cfg_fname, "r")) != NULL) {
       c = getc(cfg_file);
       while (c != EOF) {
          if (c == '-') {
             if ((c = getc(cfg_file)) == 'I')
                ++n_paths;
             }
          else
             c = getc(cfg_file);
          }
       }
#endif 					/* TURBO || BORLAND_286 ... */

#if HIGHC_386 || INTEL_386 || WATCOM
   /* something may be needed */
#endif					/* HIGHC_386 || INTEL_386 || ... */
#endif					/* MSDOS */

#if UNIX
   static char *sysdir = "/usr/include/";

   n_paths = 1;
#endif					/* UNIX */

/*
 * End of operating-system specific code.
 */

   /*
    * Count the number of -I options to the preprocessor.
    */
   for (i = 0; opt_lst[i] != '\0'; ++i)
      if (opt_lst[i] == 'I')
         ++n_paths;

   /*
    * Set up the array of standard locations to search for header files.
    */
   incl_search = (char **)alloc((unsigned int)(sizeof(char *)*(n_paths + 1)));
   j = 0;
  
/*
 * The following code is operating-system dependent [@files.04].
 *  Establish the standard locations to search before the -I options
 *  on the preprocessor.
 */

#if PORT
   /* something may be needed */
Deliberate Syntax Error
#endif					/* PORT */

#if AMIGA
   /* nothing is needed */
#endif					/* AMIGA */

#if ATARI_ST || VM || MVS
   /* something may be needed */
Deliberate Syntax Error
#endif					/* ATARI_ST || ... */

#if MSDOS
#if MICROSOFT
   /*
    * Get locations from -I options from the CL environment variable.
    */
   if (cl_var != NULL)
      while (*cl_var != '\0') {
         if (*cl_var == '/' || *cl_var == '-') {
            ++cl_var;
            if (*cl_var == 'I') {
                  ++cl_var;
                  while (*cl_var == ' ' || *cl_var == '\t')
                     ++cl_var;
                  i = 0;
                  while (cl_var[i] != ' ' && cl_var[i] != '\t' &&
                    cl_var[i] != '\0')
                     ++i;
                  s1 = (char *) alloc((unsigned int)(i + 1));
                  strncpy(s1, cl_var, i);
                  s1[i] = '\0';
                  /*
                   * Convert back slashes to slashes for internal consistency.
                   */
                  for (s = s1; *s != '\0'; ++s)
                     if (*s == '\\')
                        *s = '/';
                  incl_search[j++] = s1;
                  cl_var += i;
               }
            }
         if (*cl_var != '\0')
            ++cl_var;
         }
#endif					/* MICROSOFT */

#if ZTC_386
   /*
    * Get locations from -I options from the CL environment variable.
    * Each -I may have multiple options separated by semi-colons.
    */
   if (cl_var != NULL)
      while (*cl_var != '\0') {
         if (*cl_var == '/' || *cl_var == '-') {
            ++cl_var;
            if (*cl_var == 'I') {
                  ++cl_var;
                  while (*cl_var == ' ' || *cl_var == '\t')
                     ++cl_var;
                  i = 0;
                  while (cl_var[i] != ' ' && cl_var[i] != '\t' &&
                    cl_var[i] != '\0') {
                     while (cl_var[i] != ' ' && cl_var[i] != '\t' &&
                       cl_var[i] != ';' && cl_var[i] != '\0')
                        ++i;
                     s1 = (char *) alloc((unsigned int)(i + 1));
                     strncpy(s1, cl_var, i);
                     s1[i] = '\0';
                     /*
                      * Convert back slashes to slashes for internal consistency.
                      */
                     for (s = s1; *s != '\0'; ++s)
                        if (*s == '\\')
                           *s = '/';
                     incl_search[j++] = s1;
                     if (cl_var[i] == ';') {
		                 cl_var += (i + 1);
		                 i = 0;
		                 }
                     }
                  cl_var += i;
               }
            }
         if (*cl_var != '\0')
            ++cl_var;
         }
#endif					/* ZTC_386 */

#if HIGHC_386 || INTEL_386 || WATCOM
   /* something is needed */
#endif					/* HIGHC_386 || INTEL_386 || ... */
#endif					/* MSDOS */

#if UNIX || VMS || MACINTOSH
   /* nothing is needed */
#endif					/* UNIX || VMS || MACINTOSH */

/*
 * End of operating-system specific code.
 */

   /*
    * Get the locations from the -I options to the preprocessor.
    */
   for (i = 0; opt_lst[i] != '\0'; ++i)
      if (opt_lst[i] == 'I') {
         s = opt_args[i];
         s1 = (char *) alloc((unsigned int)(strlen(s)+1));
         strcpy(s1, s);
         
/*
 * The following code is operating-system dependent [@files.05].
 *  Insure that path syntax is in Unix format for internal consistency
 *  (note, this may not work well on all systems).
 *  In particular, Amiga paths are left intact.
 */

#if PORT
   /* something might be needed */
Deliberate Syntax Error
#endif					/* PORT */

#if AMIGA
   /* nothing is needed */
#endif					/* AMIGA */

#if ATARI_ST || VM || MVS
   /* something might be needed */
Deliberate Syntax Error
#endif					/* ATARI_ST || ... */

#if MACINTOSH
   s1 = FileNameMacConvert(FileNameMacToUnix,s);
#endif					/* MACINTOSH */

#if MSDOS
         /*
          * Convert back slashes to slashes for internal consistency.
          */
         for (s = s1; *s != '\0'; ++s)
            if (*s == '\\')
               *s = '/';
#endif					/* MSDOS */

#if UNIX || VMS
   /* nothing is needed */
#endif					/* UNIX || VMS */

/*
 * End of operating-system specific code.
 */
         
         incl_search[j++] = s1;
         }

/*
 * The following code is operating-system dependent [@files.06].
 *  Establish the standard locations to search after the -I options
 *  on the preprocessor.
 */

#if PORT
   /* probably needs something */
Deliberate Syntax Error
#endif					/* PORT */

#if VMS
   for ( ; vmsi; vmsi--)
      incl_search[n_paths - vmsi] = syspaths[vmsi-1];
#endif					/* VMS */

#if AMIGA
   incl_search[n_paths - 1] = sysdir;
#endif					/* AMIGA */

#if ATARI_ST || VM || MVS
   /* probably needs something */
Deliberate Syntax Error
#endif					/* ATARI_ST || ... */

#if MSDOS
#if MICROSOFT
   /*
    * Get the locations from the INCLUDE environment variable.
    */
   s = syspath;
   if (*s != '\0')
      incl_search[j++] = s;
   while (*s != '\0') {
      if (*s == ';') {
         *s = '\0';
         ++s;
         if (*s != '\0')
            incl_search[j++] = s;
         }
      else {
         if (*s == '\\')
            *s = '/';
         ++s;
         }
      }
#endif					/* MICROSOFT */

#if TURBO || BORLAND_286 || BORLAND_386
    /*
     * Get the locations from the -I options in the configuration file.
     */
    if (cfg_file != NULL) {
       rewind(cfg_file);
       sbuf = get_sbuf();
       c = getc(cfg_file);
       while (c != EOF) {
          if (c == '-') {
             if ((c = getc(cfg_file)) == 'I') {
                c = getc(cfg_file);
                while (c != ' ' && c != '\t' && c != '\n' && c != EOF) {
                   AppChar(*sbuf, c);
                   c = getc(cfg_file);
                   }
                incl_search[j++] = str_install(sbuf);
                }
             }
          else
             c = getc(cfg_file);
          }
       rel_sbuf(sbuf);
       fclose(cfg_file);
       }
#endif 					/* TURBO || BORLAND_286 ... */

#if HIGHC_386 || INTEL_386 || WATCOM
  /* something is needed */
#endif					/* HIGHC_386 || INTEL_386 || ... */
#endif					/* MSDOS */

#if UNIX || MACINTOSH
   incl_search[n_paths - 1] = sysdir;
#endif					/* UNIX || MACINTOSH */

/*
 * End of operating-system specific code.
 */

   incl_search[n_paths] = NULL;
   }

#if MACINTOSH
#if MPW || THINK_C
/*
 * Extra functions specific to the Macintosh MPW implementation:
 *  functions to convert a UNIX-type file name to Mac-type
 *  and vice versa.
 *
 *  Result is pointer to a static string, or maybe a pointer
 *  to the input string if it is unchanged.
 */

static char FileName_newfn[100];

char *
FileNameUnixToMac(char *fn) {
  char *q,*e,*r;
  int full;
  
  if (strchr(fn,'/') == NULL) return fn;
  e = fn + strlen(fn);
  r = FileName_newfn;
  if (*fn == '/') {
    full = 1;
    ++fn;
  }
  else full = 0;
  for (;;) {
    (q = strchr(fn,'/')) || (q = e);
    if (fn == q || q - fn == 1 && *fn == '.') {}
    else if (q - fn == 2 && *fn == '.' && *(fn + 1) == '.')
        *r++ = ':';
    else {
      *r++ = ':';
      memcpy(r,fn,q - fn);
      r += q - fn;
    }
    if (q == e) break;
    fn = q + 1;
  }
  if (*(r - 1) == ':') *r++ = ':';
  else if (*(e - 1) == '/') *r++ = ':';
  *r = '\0';
  return full ? FileName_newfn + 1 : FileName_newfn;
}

char *
FileNameMacToUnix(char *fn) {
  char *q,*e,*r;
  
  if (strchr(fn,':') == NULL) return fn;
  r = FileName_newfn;
  if (*fn == ':') ++fn;
  else *r++ = '/';
  q = fn;
  e = fn + strlen(fn);
  for (;;) {
    while (*fn == ':') {
      ++fn;
      memcpy(r,"../",3);
      r += 3;
    }
    if (fn == e) break;
    (q = strchr(fn,':')) || (q = e);
    memcpy(r,fn,q - fn);
    r += q - fn;
    *r++ = '/';
    if (q == e) break;
    fn = q + 1;
  }
  *--r = '\0';
  return FileName_newfn;
}

/*
 *  Helper function to make filename conversions more convenient.
 *
 *  This function calls either of the two above filename conversion functions
 *  and returns the resulting filename in allocated memory.  Ownership of
 *  the allocated memory is transferred to the caller -- i.e. it is the
 *  caller's responsibility to eventually free it.
 *
 *  Example:  FileNameMacConvert(FileNameMacToUnix,":MyDir:MyFile")
 */
char *
FileNameMacConvert(char *(*func)(char *),char *fn) {
   char *newfp, *newmem;

   newfp = (*func)(fn);
   newmem = (char *)malloc(strlen(newfp) + 1);
   if (newmem == NULL) return NULL;
   strcpy(newmem,newfp);
   return newmem;
}
#endif					/* MPW || THINK_C */
#endif					/* MACINTOSH */
