/*
 * This file contains routines for setting up characters sources from
 *  files. It contains code to handle the search for include files.
 */
#include "../preproc/preproc.h"
#include "../preproc/pproto.h"

#define IsRelPath(fname) (fname[0] != '/')

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

   #if MSDOS
      char *s;

      /*
       * Convert back slashes to slashes for internal consistency.
       */
      fname = (char *)strdup(fname);
      for (s = fname; *s != '\0'; ++s)
         if (*s == '\\')
            *s = '/';
   #endif				/* MSDOS */

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
                     for (s = cs->fname; s <= end_prfx; ++s)
                        AppChar(*sbuf, *s);
                  for (s = fname; *s != '\0'; ++s)
                     AppChar(*sbuf, *s);
                  path = str_install(sbuf);
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
         f = fopen(path, "r");
         prefix = ++prefix;
         }
      rel_sbuf(sbuf);
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
    *  Determine the number of standard locations to search for
    *  header files and provide any declarations needed for the code
    *  that establishes these search locations.
    */

   #if MSDOS
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
   #endif				/* MSDOS */

   #if UNIX
      static char *sysdir = "/usr/include/";
      n_paths = 1;
   #endif				/* UNIX */

   /*
    * Count the number of -I options to the preprocessor.
    */
   for (i = 0; opt_lst[i] != '\0'; ++i)
      if (opt_lst[i] == 'I')
         ++n_paths;

   /*
    * Set up the array of standard locations to search for header files.
    */
   incl_search = alloc((n_paths + 1) * sizeof(char *));
   j = 0;


   #if MSDOS
   /*
    * Establish the standard locations to search before the -I options
    *  on the preprocessor.
    *  Get locations from -I options from the CL environment variable.
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
                  s1 = alloc(i + 1);
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
   #endif				/* MSDOS */

   /*
    * Get the locations from the -I options to the preprocessor.
    */
   for (i = 0; opt_lst[i] != '\0'; ++i)
      if (opt_lst[i] == 'I') {
         s = opt_args[i];
         s1 = alloc(strlen(s) + 1);
         strcpy(s1, s);

         #if MSDOS
            /*
             * Convert back slashes to slashes for internal consistency.
             */
            for (s = s1; *s != '\0'; ++s)
               if (*s == '\\')
                  *s = '/';
         #endif				/* MSDOS */

         incl_search[j++] = s1;
         }

   /*
    *  Establish the standard locations to search after the -I options
    *  on the preprocessor.
    */
   
   #if MSDOS
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
   #endif				/* MSDOS */

   #if UNIX
      incl_search[n_paths - 1] = sysdir;
   #endif				/* UNIX */

   incl_search[n_paths] = NULL;
   }
