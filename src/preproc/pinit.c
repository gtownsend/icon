/*
 * This file contains functions used to initialize the preprocessor,
 *  particularly those for establishing implementation-dependent standard
 *  macro definitions.
 */
#include "../preproc/preproc.h"
#include "../preproc/ptoken.h"
#include "../preproc/pproto.h"

static void define_opt   (char *s, int len, struct token *dflt);
static void do_directive (char *s);
static void mac_opts     (char *opt_lst, char **opt_args);
static void undef_opt    (char *s, int len);

struct src dummy;

/*
 * init_preproc - initialize all parts of the preprocessor, establishing
 *  the primary file as the current source of tokens.
 */
void init_preproc(fname, opt_lst, opt_args)
char *fname;
char *opt_lst;
char **opt_args;
   {

   init_str();                      /* initialize string table */
   init_tok();                      /* initialize tokenizer */
   init_macro();                    /* initialize macro table */
   init_files(opt_lst, opt_args);   /* initialize standard header locations */
   dummy.flag = DummySrc;           /* marker at bottom of source stack */
   dummy.ntoks = 0;
   src_stack = &dummy;
   mac_opts(opt_lst, opt_args);     /* process options for predefined macros */
   source(fname);                   /* establish primary source file */
   }

/*
 * mac_opts - handle options which affect what predefined macros are in
 *  effect when preprocessing starts. Some of these options may be system
 *  specific. The options may be on the command line. On some systems they
 *  may also be in environment variables or configurations files. Most
 *  systems will have some predefined macros which are not dependent on
 *  options; establish them also.
 */
static void mac_opts(opt_lst, opt_args)
char *opt_lst;
char **opt_args;
   {
   int i;

   /*
    *  Establish predefined macros and look for options in environment
    *  variables and/or configuration files that affect predefined macros.
    */

   #if UNIX
      do_directive("#define unix 1\n");
      do_directive(PPInit);   /* defines that vary between Unix systems */
   #endif				/* UNIX */

   #if MSDOS
   char *undef_model = "#undef M_I86SM\n#undef M_I86CM\n#undef M_I86MM\n"
       "#undef M_I86LM\n#undef M_I86HM\n";
   char *cl_var;

   do_directive("#define MSDOS 1\n");
   do_directive("#define M_I86 1\n");
   do_directive("#define M_I86SM 1\n");  /* small memory model */

   /*
    * Process all applicable options from the CL environment variable.
    */
   cl_var = getenv("CL");
   if (cl_var != NULL)
      while (*cl_var != '\0') {
         if (*cl_var == '/' || *cl_var == '-') {
            ++cl_var;
            switch (*cl_var) {
               case 'U':
                  /*
                   * Undefine a specific identifier. Find the identifier
                   *  by skipping white space then locating its end.
                   */
                  ++cl_var;
                  while (*cl_var == ' ' || *cl_var == '\t')
                     ++cl_var;
                  i = 0;
                  while (cl_var[i] != ' ' && cl_var[i] != '\t' &&
                    cl_var[i] != '\0')
                     ++i;
                  undef_opt(cl_var, i); /* undefine the identifier */
                  cl_var += i;
                  break;
               case 'u':
                  do_directive("#undef MSDOS\n");
                  do_directive("#undef M_I86\n");
                  do_directive("#undef NO_EXT_KEYS\n");
                  do_directive("#undef _CHAR_UNSIGED\n");
                  break;
               case 'D':
                  /*
                   * Define an identifier. If no defining string is given
                   *  define it to "1".
                   */
                  ++cl_var;
                  while (*cl_var == ' ' || *cl_var == '\t')
                     ++cl_var;
                  i = 0;
                  while (cl_var[i] != ' ' && cl_var[i] != '\t' &&
                    cl_var[i] != '\0')
                     ++i;
                  define_opt(cl_var, i, one_tok); /* define the identifier */
                  cl_var += i;
                  break;
               case 'A':
                  /*
                   * Memory model. Define corresponding identifiers after
                   *  after making sure all others are undefined.
                   */
                  ++cl_var;
                  switch (*cl_var) {
                     case 'S':
                        /*
                         * -AS - small memory model.
                         */
                        do_directive(undef_model);
                        do_directive("#define M_I86SM 1\n");
                        break;
                     case 'C':
                        /*
                         * -AC - compact memory model.
                         */
                        do_directive(undef_model);
                        do_directive("#define M_I86CM 1\n");
                       break;
                     case 'M':
                        /*
                         * -AM - medium memory model.
                         */
                        do_directive(undef_model);
                        do_directive("#define M_I86MM 1\n");
                        break;
                     case 'L':
                        /*
                         * -AL - large memory model.
                         */
                        do_directive(undef_model);
                        do_directive("#define M_I86LM 1\n");
                        break;
                     case 'H':
                        /*
                         * -AH - huge memory model.
                         */
                        do_directive(undef_model);
                        do_directive("#define M_I86LM 1\n");
                        do_directive("#define M_I86HM 1\n");
                        break;
                     }
                  break;
               case 'Z':
                  ++cl_var;
                  if (*cl_var == 'a') {
                     /*
                      * -Za
                      */
                     do_directive("#undef NO_EXT_KEYS\n");
                     do_directive("#define NO_EXT_KEYS 1\n");
                     }
                  break;
               case 'J':
                   do_directive("#undef _CHAR_UNSIGED\n");
                   do_directive("#define _CHAR_UNSIGNED 1\n");
                   break;
               }
            }
         if (*cl_var != '\0')
            ++cl_var;
         }
   #endif				/* MSDOS*/

   /*
    * look for options that affect macro definitions (-U, -D, etc).
    */
   for (i = 0; opt_lst[i] != '\0'; ++i)
      switch(opt_lst[i]) {
         case 'U':
            /*
             * Undefine and predefined identifier.
             */
            undef_opt(opt_args[i], (int)strlen(opt_args[i]));
            break;

         case 'D':
            /*
             * Define an identifier. Use "1" if no defining string is given.
             */
            define_opt(opt_args[i], (int)strlen(opt_args[i]), one_tok);
            break;

         /*
          *  Check for system specific options from command line.
          */

         #if MSDOS
         case 'A':
            /*
             * Memory model. Define corresponding identifiers after
             *  after making sure all others are undefined.
             */
            switch (*opt_args[i]) {
               case 'S':
                  /*
                   * -AS - small memory model.
                   */
                  do_directive(undef_model);
                  do_directive("#define M_I86SM 1\n");
                  break;
               case 'C':
                  /*
                   * -AC - compact memory model.
                   */
                  do_directive(undef_model);
                  do_directive("#define M_I86CM 1\n");
                  break;
               case 'M':
                  /*
                   * -AM - medium memory model.
                   */
                  do_directive(undef_model);
                  do_directive("#define M_I86MM 1\n");
                  break;
               case 'L':
                  /*
                   * -AL - large memory model.
                   */
                  do_directive(undef_model);
                  do_directive("#define M_I86LM 1\n");
                  break;
               case 'H':
                  /*
                   * -AH - huge memory model.
                   */
                  do_directive(undef_model);
                  do_directive("#define M_I86LM 1\n");
                  do_directive("#define M_I86HM 1\n");
                  break;
               default:
                  fprintf(stderr, "invalid argument to -A option: %s\n",
                     opt_args[i]);
                  show_usage();
               }
            break;
         case 'Z':
            if (*opt_args[i] == 'a') {
               do_directive("#undef NO_EXT_KEYS\n");
               do_directive("#define NO_EXT_KEYS 1\n");
               }
            else {
               fprintf(stderr, "invalid argument to -Z option: %s\n",
                  opt_args[i]);
               show_usage();
            }
            break;
         case 'J':
             do_directive("#undef _CHAR_UNSIGED\n");
             do_directive("#define _CHAR_UNSIGNED 1\n");
             break;
         #endif				/* MSDOS */

         }
   }

/*
 * str_src - establish a string, given by a character pointer and a length,
 *  as the current source of tokens.
 */
void str_src(src_name, s, len)
char *src_name;
char *s;
int len;
   {
   union src_ref ref;
   int *ip1, *ip2;

   /*
    * Create a character source with a large enought buffer for the string.
    */
   ref.cs = new_cs(src_name, NULL, len + 1);
   push_src(CharSrc, &ref);
   ip1 = ref.cs->char_buf;
   ip2 = ref.cs->line_buf;
   while (len-- > 0) {
     *ip1++ = *s++;    /* copy string to source buffer */
     *ip2++ = 0;       /* characters are from "line 0" */
     }
   *ip1 = EOF;
   *ip2 = 0;
   ref.cs->next_char = ref.cs->char_buf;
   ref.cs->last_char = ip1;
   first_char = ref.cs->char_buf;
   next_char = first_char;
   last_char = ref.cs->last_char;
   }

/*
 * do_directive - take a character string containing preprocessor
 *  directives separated by new-lines and execute them. This done
 *  by preprocessing the string.
 */
static void do_directive(s)
char *s;
   {
   str_src("<initialization>", s, (int)strlen(s));
   while (interp_dir() != NULL)
      ;
   }

/*
 * undef_opt - take the argument to a -U option and, if it is valid,
 *  undefine it.
 */
static void undef_opt(s, len)
char *s;
int len;
   {
   struct token *mname;
   int i;

   /*
    * The name is needed in the form of a token. Use the preprocessor
    *  to tokenize it.
    */
   str_src("<options>", s, len);
   mname = next_tok();
   if (mname == NULL || mname->tok_id != Identifier ||
     next_tok() != NULL) {
      fprintf(stderr, "invalid argument to -U option: ");
      for (i = 0; i < len; ++i)
         putc(s[i], stderr);    /* show offending argument */
      putc('\n', stderr);
      show_usage();
      }
   m_delete(mname);
   }

/*
 * define_opt - take an argument to a -D option and, if it is valid, perform
 *  the requested definition.
 */
static void define_opt(s, len, dflt)
char *s;
int len;
struct token *dflt;
   {
   struct token *mname;
   struct token *t;
   struct tok_lst *body;
   struct tok_lst **ptlst, **trail_whsp;
   int i;

   /*
    * The argument to -D must be tokenized.
    */
   str_src("<options>", s, len);

   /*
    * Find the macro name.
    */
   mname = next_tok();
   if (mname == NULL || mname->tok_id != Identifier) {
      fprintf(stderr, "invalid argument to -D option: ");
      for (i = 0; i < len; ++i)
         putc(s[i], stderr);
      putc('\n', stderr);
      show_usage();
      }

   /*
    * Determine if the name is followed by '='.
    */
   if (chk_eq_sign()) {
      /*
       * Macro body is given, strip leading white space
       */
      t = next_tok();
      if (t != NULL && t->tok_id == WhiteSpace) {
         free_t(t);
         t = next_tok();
         }


      /*
       * Construct the token list for body of macro. Keep track of trailing
       *  white space so it can be deleted.
       */
      body = NULL;
      ptlst = &body;
      trail_whsp = NULL;
      while (t != NULL) {
         t->flag &= ~LineChk;
         (*ptlst) = new_t_lst(t);
         if (t->tok_id == WhiteSpace)
            trail_whsp = ptlst;
         else
            trail_whsp = NULL;
         ptlst = &(*ptlst)->next;
         t = next_tok();
         }

      /*
       * strip trailing white space
       */
      if (trail_whsp != NULL) {
         free_t_lst(*trail_whsp);
         *trail_whsp = NULL;
         }
      }
   else {
      /*
       * There is no '=' after the macro name; use the supplied
       *  default value for the macro definition.
       */
      if (next_tok() == NULL)
         if (dflt == NULL)
            body = NULL;
         else
            body = new_t_lst(copy_t(dflt));
      else {
         fprintf(stderr, "invalid argument to -D option: ");
         for (i = 0; i < len; ++i)
            putc(s[i], stderr);
         putc('\n', stderr);
         show_usage();
         }
      }

   m_install(mname, NoArgs, 0, NULL, body); /* install macro definition */
   }
