/*
 * This file contains functions used to initialize the preprocessor,
 *  particularly those for establishing implementation-dependent standard
 *  macro definitions.
 */
#include "../preproc/preproc.h"
#include "../preproc/ptoken.h"

/*
 * The following code is operating-system dependent [@p_init.01]. 
 *  #includes and #defines.
 */

#if PORT
   /* something may be needed */
Deliberate Syntax Error
#endif					/* PORT */

#if AMIGA
   /* nothing is needed */
#endif					/* AMIGA */

#if ATARI_ST
   /* something may be needed */
Deliberate Syntax Error
#endif					/* ATARI_ST */

#if MACINTOSH
   /* nothing is needed */
#endif					/* MACINTOSH */

#if MSDOS
#if MICROSOFT || HIGHC_386 || INTEL_386 || ZTC_386 || WATCOM
   /* nothing is needed */
#endif					/* MICROSOFT || HIGHC_386 || ... */
#if TURBO || BORLAND_286 || BORLAND_386
#include <dir.h>
#define Strng(s) #s
#define StrMBody(m) Strng(m)
#define CBufSz 200
#endif 					/* TURBO || BORLAND_286 ... */
#endif					/* MSDOS */
 
#if MVS
#if SASC 
char *_style = "tso:";
#endif                                  /* SASC */
#endif                                  /* MVS */

#if VM
   /* ??? */
#endif					/* VM */

#if UNIX || VMS
   /* nothing is needed */
#endif					/* UNIX || VMS */

/*
 * End of operating-system specific code.
 */

#include "../preproc/pproto.h"

/*
 * Prototypes for static functions.
 */
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
 * The following code is operating-system dependent [@p_init.02].
 *  Establish predefined macros and look for options in environment
 *  variables and/or configuration files that affect predefined macros.
 */

#if PORT
   /* something may be needed */
Deliberate Syntax Error
#endif					/* PORT */

#if AMIGA
   do_directive("#define AMIGA 1\n");
#if __SASC
   do_directive("#define __SASC 1\n");
   do_directive("#define LATTICE 0\n");
#endif                                  /* __SASC */
#endif					/* AMIGA */

#if ATARI_ST
   /* something may be needed */
Deliberate Syntax Error
#endif					/* ATARI_ST */

#if MACINTOSH
   /* nothing is needed */
#endif					/* MACINTOSH */

#if MSDOS
#if MICROSOFT
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
#endif					/* MICROSOFT */

#if ZTC_386
   char *undef_model = "#undef M_I86SM\n#undef M_I86CM\n#undef M_I86MM\n"
				"#undef M_I86LM\n#undef M_I86VM\n";
   char *undef_model2 = 
   				"#undef __SMALL__\n#undef __MEDIUM__\n"
				"#undef __COMPACT__\n#undef __LARGE__\n#undef __VCM__\n";
   char *undef_machine = 
   				"#undef M_I8086\n#undef M_I286\n#undef M_I386\n#undef M_I486\n";
   char *cl_var;
   int ms_syms = 1;			/* allow Microsoft memory and machine symbols	*/

   do_directive("#define __ZTC__ 0x304\n");

   do_directive("#define M_I86SM 1\n");
   do_directive("#define __SMALL__ 1\n");

   do_directive("#define M_I86 1\n");
   do_directive("#define M_I8086 1\n");
   do_directive("#define __I86__ 0\n");

   do_directive("#define __FPCE__ 1\n");
   do_directive("#define __FPCE_IEEE__ 1\n");

   /*
    * Process all applicable options from the CFLAGS environment variable.
    */
   cl_var = getenv("CFLAGS");
   if (cl_var != NULL)
      while (*cl_var != '\0') {
         if (*cl_var == '/' || *cl_var == '-') {
            ++cl_var;
            switch (*cl_var) {
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
	         case 'W':
	             do_directive("#undef _WINDOWS\n");
    	         do_directive("#define _WINDOWS 1\n");
    	         break;
	         case 'J':
    	         do_directive("#undef _CHAR_UNSIGNED\n");
	             do_directive("#define _CHAR_UNSIGNED 1\n");
	             break;
	         case 'A':
	             do_directive(undef_model);
    	         do_directive(undef_machine);
                 do_directive("#undef M_I86\n");
	             do_directive("#undef __STDC__\n");
    	         do_directive("#define __STDC__ 1\n");
    	         ms_syms = 0;
    	         break;
	         case 'm':
	            switch (*opt_args[i]) {
	               case 'p':
	               case 'x':
	                  do_directive("#undef DOS386\n");
	                  do_directive("#define DOS386 1\n");
                      do_directive(undef_machine);
					  do_directive("#undef __I86__\n");
	  	              do_directive("#define __I86__ 3\n");
	  	              if (ms_syms)
	                     do_directive("#define M_I386 1\n");
	  	              /* fall through	*/
	               case 't':
	               case 's':
	                  do_directive(undef_model);
	                  do_directive(undef_model2);
	  	              if (ms_syms)
	                     do_directive("#define M_I86SM 1\n");
	                  do_directive("#define __SMALL__ 1\n");
	                  break;
	               case 'c':
	                  do_directive(undef_model);
	                  do_directive(undef_model2);
	                  if (ms_syms)
                         do_directive("#define M_I86CM 1\n");
	                  do_directive("#define __COMPACT__ 1\n");
	                  break;
	               case 'm':
	                  do_directive(undef_model);
	                  do_directive(undef_model2);
	  	              if (ms_syms)
                         do_directive("#define M_I86MM 1\n");
	                  do_directive("#define __MEDIUM__ 1\n");
	                  break;
	               case 'r':
	               case 'z':
	                  do_directive("#undef DOS16RM\n");
	                  do_directive("#define DOS16RM 1\n");
                      do_directive(undef_machine);
					  do_directive("#undef __I86__\n");
	  	              do_directive("#define __I86__ 2\n");
	  	              if (ms_syms)
                         do_directive("#define M_I286 1\n");
	  	              /* fall through */
	               case 'l':
	                  do_directive(undef_model);
	                  do_directive(undef_model2);
					  if (ms_syms)
					     do_directive("#define M_I86LM 1\n");
	                  do_directive("#define __LARGE__ 1\n");
	                  break;
	               case 'v':
	                  do_directive(undef_model);
	                  do_directive(undef_model2);
					  if (ms_syms)
                         do_directive("#define M_I86VM 1\n");
	                  do_directive("#define __VCM__ 1\n");
	                  break;
	               }
    	        break;
	         case '2':
                 do_directive(undef_machine);
	             do_directive("#undef __I86__\n");
	             do_directive("#define __I86__ 2\n");
                 if (ms_syms)
	                do_directive("#define M_I286 1\n");
	             break;
	         case '3':
	             do_directive("#undef DOS386\n");
	             do_directive("#define DOS386 1\n");
                 do_directive(undef_machine);
	             do_directive("#undef __I86__\n");
	             do_directive("#define __I86__ 3\n");
                 if (ms_syms)
                    do_directive("#define M_I386 1\n");
	             break;
	         case '4':
	             do_directive("#undef DOS386\n");
	             do_directive("#define DOS386 1\n");
                 do_directive(undef_machine);
	             do_directive("#undef __I86__\n");
	             do_directive("#define __I86__ 4\n");
                 if (ms_syms)
                    do_directive("#define M_I486 1\n");
	             /* fall through */
	         case 'f':
	             do_directive("#define __INLINE_8087 1\n");
	             break;
	         case 'u':
	             do_directive(undef_model);
	             do_directive("#undef M_I86\n");
	             do_directive("#undef __ZTC__\n");
	             ms_syms = 0;
	             break;
               }
            }
         if (*cl_var != '\0')
            ++cl_var;
         }
#endif					/* ZTC_386 */

#if TURBO || BORLAND_286 ||  BORLAND_386
   char *undef_models = "#undef __TINY__\n#undef __SMALL__\n#undef __MEDIUM__\n\
#undef __COMPACT__\n#undef __LARGE__\n#undef __HUGE__\n";
   char dir_buf[60];
   char *cfg_fname;
   FILE *cfg_file;
   char cbuf[CBufSz];
   int c;
    
   do_directive("#define __MSDOS__ 1\n");
   do_directive("#define __SMALL__ 1\n");
   do_directive("#define __CDECL__ 1\n");
   sprintf(dir_buf, "#define __TURBOC__ %s\n", StrMBody(__TURBOC__));
   do_directive(dir_buf);
    
   /*
    * Process all applicable options from the configuration file.
    */
   cfg_fname = searchpath("turboc.cfg");
   if (cfg_fname != NULL && (cfg_file = fopen(cfg_fname, "r")) != NULL) {
      c = getc(cfg_file);
      while (c != EOF) {
      	  if (c == '-') {
             c = getc(cfg_file);
             switch (c) {
                case 'U':
                  /*
                   * Undefine a specific identifier. Locate the identifier's
                   *  end.
                   */
                   i = 0;
                   c = getc(cfg_file);
                   while (c != ' ' && c != '\t' && c != '\n' && c != EOF) {
                      if (i >= CBufSz) {
                         cbuf[CBufSz - 1] = '\0';
                         err2("-U argument too big: ", cbuf);
                         }
                      cbuf[i++] = c;
                      c = getc(cfg_file);
                      }
                   undef_opt(cbuf, i); /* undefine identifier */
                   break;
                case 'D':
                  /*
                   * Define an identifier. If no defining string is given,
                   *  the definition is empty.
                   */
                   i = 0;
                   c = getc(cfg_file);
                   while (c != ' ' && c != '\t' && c != '\n' && c != EOF) {
                      if (i >= CBufSz) {
                         cbuf[CBufSz - 1] = '\0';
                         err2("-D argument too big: ", cbuf);
                         }
                      cbuf[i++] = c;
                      c = getc(cfg_file);
                      }
                   define_opt(cbuf, i, NULL); /* define the identifier */
                   break;
                case 'm':
                   /*
                    * Memory model. Define the appropriate macro after
                    *  insuring that all other memory model macros are
                    *  undefined.
                    */
                   switch (c = getc(cfg_file)) {
                      case 't':
                         do_directive(undef_models);
                         do_directive("#define __TINY__ 1\n");
                         break;
                      case 's':
                         do_directive(undef_models);
                         do_directive("#define __SMALL__ 1\n");
                         break;
                      case 'm':
                         do_directive(undef_models);
                         do_directive("#define __MEDIUM__ 1\n");
                         break;
                      case 'c':
                         do_directive(undef_models);
                         do_directive("#define __COMPACT__ 1\n");
                         break;
                      case 'l':
                         do_directive(undef_models);
                         do_directive("#define __LARGE__ 1\n");
                         break;
                      case 'h':
                         do_directive(undef_models);
                         do_directive("#define __HUGE__ 1\n");
                         break;
                      }
                   break;
                case 'p':
                   do_directive("#undef __PASCAL__\n#undef __CDECL__\n");
                   do_directive("#define __PASCAL__ 1\n");
                   break;
                }
             }
          if (c != EOF)
             c = getc(cfg_file);
          }
       fclose(cfg_file);
       }
#endif 					/* TURBO || BORLAND_286 ... */

#if HIGHC_386 || INTEL_386 || WATCOM
   /* something may be needed */
#endif					/* HIGHC_386 || INTEL_386 || ... */
#endif					/* MSDOS */
 
#if MVS || VM
#if SASC 
   do_directive("#define I370 1\n");
   {
      char sascbuf[sizeof("#define __SASC__ nnnn\n")];
      sprintf(sascbuf, "#define __SASC__ %d\n", __SASC__);
      do_directive(sascbuf);
   }
#if MVS
   do_directive("#define OSVS 1\n");
#endif                                  /* MVS */
#if VM
   do_directive("#define CMS 1\n");
#endif                                  /* VM */
#endif                                  /* SASC */
#endif                                  /* MVS || VM */

#if UNIX
   do_directive("#define unix 1\n");
   do_directive(PPInit);   /* defines that vary between Unix systems */
#endif					/* UNIX */

#if VMS
   /* nothing is needed */
#endif					/* VMS */

/*
 * End of operating-system specific code.
 */

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
 * The following code is operating-system dependent [@p_init.03]. Check for
 *  system specific options from command line.
 */

#if PORT
   /* something may be needed */
Deliberate Syntax Error
#endif					/* PORT */

#if AMIGA
   /* nothing is needed */
#endif					/* AMIGA */

#if ATARI_ST
   /* something may be needed */
Deliberate Syntax Error
#endif					/* ATARI_ST */

#if MACINTOSH
   /* nothing is needed */
#endif					/* MACINTOSH */

#if MSDOS
#if MICROSOFT
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
#endif					/* MICROSOFT */

#if ZTC_386
         case 'W':
             do_directive("#undef _WINDOWS\n");
             do_directive("#define _WINDOWS 1\n");
             break;
         case 'J':
             do_directive("#undef _CHAR_UNSIGNED\n");
             do_directive("#define _CHAR_UNSIGNED 1\n");
             break;
         case 'A':
             do_directive(undef_model);
             do_directive(undef_machine);
             do_directive("#undef M_I86\n");
             do_directive("#undef __STDC__\n");
             do_directive("#define __STDC__ 1\n");
             ms_syms = 0;
             break;
         case 'm':
            switch (*opt_args[i]) {
               case 'p':
               case 'x':
                  do_directive("#undef DOS386\n");
                  do_directive("#define DOS386 1\n");
                  do_directive(undef_machine);
                  do_directive("#undef __I86__\n");
                  do_directive("#define __I86__ 3\n");
                  if (ms_syms)
                     do_directive("#define M_I386 1\n");
  	              /* fall through */
               case 't':
               case 's':
                  do_directive(undef_model);
                  do_directive(undef_model2);
                  if (ms_syms)
                     do_directive("#define M_I86SM 1\n");
                  do_directive("#define __SMALL__ 1\n");
                  break;
               case 'c':
                  do_directive(undef_model);
                  do_directive(undef_model2);
                  if (ms_syms)
                     do_directive("#define M_I86CM 1\n");
                  do_directive("#define __COMPACT__ 1\n");
                  break;
               case 'm':
                  do_directive(undef_model);
                  do_directive(undef_model2);
                  if (ms_syms)
                     do_directive("#define M_I86MM 1\n");
                  do_directive("#define __MEDIUM__ 1\n");
                  break;
               case 'r':
               case 'z':
                  do_directive("#undef DOS16RM\n");
                  do_directive("#define DOS16RM 1\n");
                  do_directive(undef_machine);
                  do_directive("#undef __I86__\n");
                  do_directive("#define __I86__ 2\n");
                  if (ms_syms)
                     do_directive("#define M_I286 1\n");
  	              /* fall through */
               case 'l':
                  do_directive(undef_model);
                  do_directive(undef_model2);
                  if (ms_syms)
                     do_directive("#define M_I86LM 1\n");
                  do_directive("#define __LARGE__ 1\n");
                  break;
               case 'v':
                  do_directive(undef_model);
                  do_directive(undef_model2);
                  if (ms_syms)
                     do_directive("#define M_I86VM 1\n");
                  do_directive("#define __VCM__ 1\n");
                  break;
               }
            break;
         case '2':
             do_directive(undef_machine);
             do_directive("#undef __I86__\n");
             do_directive("#define __I86__ 2\n");
             if (ms_syms)
                do_directive("#define M_I286 1\n");
             break;
         case '3':
             do_directive("#undef DOS386\n");
             do_directive("#define DOS386 1\n");
             do_directive(undef_machine);
             do_directive("#undef __I86__\n");
             do_directive("#define __I86__ 3\n");
             if (ms_syms)
                do_directive("#define M_I386 1\n");
             break;
         case '4':
             do_directive("#undef DOS386\n");
             do_directive("#define DOS386 1\n");
             do_directive(undef_machine);
             do_directive("#undef __I86__\n");
             do_directive("#define __I86__ 4\n");
             if (ms_syms)
                do_directive("#define M_I486 1\n");
             /* fall through */
         case 'f':
             do_directive("#define __INLINE_8087 1\n");
             break;
         case 'u':
             do_directive(undef_model);
             do_directive(undef_machine);
             do_directive("#undef M_I86\n");
             do_directive("#undef __ZTC__\n");
             ms_syms = 0;
             break;
#endif					/* ZTC_386 */

#if TURBO || BORLAND_286 || BORLAND_386
         case 'm':
            /*
             * Memory model. Define the appropriate macro after
             *  insuring that all other memory model macros are
             *  undefined.
             */
            switch (*opt_args[i]) {
               case 't':
                  do_directive(undef_models);
                  do_directive("#define __TINY__ 1\n");
                  break;
               case 's':
                  do_directive(undef_models);
                  do_directive("#define __SMALL__ 1\n");
                  break;
               case 'm':
                  do_directive(undef_models);
                  do_directive("#define __MEDIUM__ 1\n");
                  break;
               case 'c':
                  do_directive(undef_models);
                  do_directive("#define __COMPACT__ 1\n");
                  break;
               case 'l':
                  do_directive(undef_models);
                  do_directive("#define __LARGE__ 1\n");
                  break;
               case 'h':
                  do_directive(undef_models);
                  do_directive("#define __HUGE__ 1\n");
                  break;
               default:
                  fprintf(stderr, "invalid argument to -m option: %s\n",
                     opt_args[i]);
                  show_usage();
               }
            break;
         case 'p':
            do_directive("#undef __PASCAL__\n#undef __CDECL__\n");
            do_directive("#define __PASCAL__ 1\n");
            break;
#endif					/* TURBO || BORLAND_286 ... */

#if HIGHC_386 || INTEL_386
   /* something may be needed */
#endif					/* HIGHC_386 || INTEL_386 || ZTC_386 */
#endif					/* MSDOS */
 
#if MVS || VM
   /* ??? we'll see... */
#endif                                  /* MVS || VM */

#if UNIX || VMS
   /* nothing is needed */
#endif					/* UNIX || VMS */

/*
 * End of operating-system specific code.
 */
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
