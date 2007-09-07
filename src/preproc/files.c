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

   static char *sysdir = "/usr/include/";
   n_paths = 1;

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
         incl_search[j++] = s1;
         }

   /*
    *  Establish the standard locations to search after the -I options
    *  on the preprocessor.
    */
   incl_search[n_paths - 1] = sysdir;

   incl_search[n_paths] = NULL;
   }
