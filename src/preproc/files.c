/*
 * This file contains routines for setting up characters sources from
 *  files. It contains code to handle the search for include files.
 */
#include "../preproc/preproc.h"
#include "../preproc/pproto.h"

#if CYGWIN
   #include <limits.h>
   #include <string.h>
   #include <sys/cygwin.h>
#endif					/* CYGWIN */

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

   #if CYGWIN
      char posix_path[ _POSIX_PATH_MAX + 1 ];
      cygwin_conv_to_posix_path( fname, posix_path );
      fname = strdup( posix_path );
   #endif				/* CYGWIN */

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
         ++prefix;
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

   #if CYGWIN
      char *incl_var;
      static char *sysdir = "/usr/include";
      static char *windir = "/usr/include/w32api";
      n_paths = 2;
   
      incl_var = getenv("C_INCLUDE_PATH");
      if (incl_var != NULL) {
         /*
          * Add one entry for evry non-empty, colon-separated string in incl_var.
          */
         char *dir_start, *dir_end;
   
         dir_start = incl_var;
         while( ( dir_end = strchr( dir_start, ':' ) ) != NULL ) {
            if (dir_end > dir_start) ++n_paths;
            dir_start = dir_end + 1;
            }
         if ( *dir_start != '\0' )
            ++n_paths;     /* One path after the final ':' */
         }
   #else				/* CYGWIN */
      static char *sysdir = "/usr/include/";
      n_paths = 1;
   #endif				/* CYGWIN */

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

   /*
    * Get the locations from the -I options to the preprocessor.
    */
   for (i = 0; opt_lst[i] != '\0'; ++i)
      if (opt_lst[i] == 'I') {
         s = opt_args[i];
         s1 = alloc(strlen(s) + 1);
         strcpy(s1, s);

         #if CYGWIN
            /*
             * Run s1 through cygwin_conv_to_posix_path; if the posix path
             * differs from s1, reset s1 to a copy of the posix path.
             */
            {
               char posix_path[ _POSIX_PATH_MAX ];
               cygwin_conv_to_posix_path( s1, posix_path );
               if (strcmp( s1, posix_path ) != 0) {
                  free( s1 );
                  s1 = salloc( posix_path );
                  }
               }
         #endif				/* CYGWIN */

         incl_search[j++] = s1;
         }

   /*
    *  Establish the standard locations to search after the -I options
    *  on the preprocessor.
    */
   #if CYGWIN
      if (incl_var != NULL) {
         /*
          * The C_INCLUDE_PATH components are carved out of a copy of incl_var.
          * The colons after non-empty directory names are replaced by null
          * chars, and the pointers to the start of these names are stored
	  *  in inc_search.
          */
         char *dir_start, *dir_end;
   
         dir_start = salloc( incl_var );
         while( ( dir_end = strchr( dir_start, ':' ) ) != NULL ) {
            if (dir_end > dir_start) {
               incl_search[j++] = dir_start;
               *dir_end = '\0';
               }
            dir_start = dir_end + 1;
            }
         if ( *dir_start != '\0' )
            incl_search[j++] = dir_start;
         }
   
      /* Finally, add the system dir(s) */
      incl_search[j++] = sysdir;
      incl_search[j++] = windir;
   #else
      incl_search[n_paths - 1] = sysdir;
   #endif				/* CYGWIN */

   incl_search[n_paths] = NULL;
   }
