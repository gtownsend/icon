#include "../preproc/preproc.h"
#include "../preproc/pproto.h"

char *progname = "pp";

/*
 * The following code is operating-system dependent [@pmain.01]. Establish
 *  command-line options.
 */

#if PORT
static char *ostr = "CPD:I:U:o:";
static char *options = 
   "[-C] [-P] [-Dname[=[text]]] [-Uname] [-Ipath] [-ofile] [files]";
   /* may need more options */
Deliberate Syntax Error
#endif					/* PORT */

#if AMIGA || ATARI_ST || MACINTOSH
static char *ostr = "CPD:I:U:o:";
static char *options = 
   "[-C] [-P] [-Dname[=[text]]] [-Uname] [-Ipath] [-ofile] [files]";
   /* may need more options */
Deliberate Syntax Error
#endif					/* AMIGA || ATARI_ST || ... */

#if MSDOS
#if MICROSOFT || INTEL_386 || HIGHC_386 || WATCOM
   /* this really isn't right for anything but Microsoft */
static char *ostr = "CPD:I:U:o:A:Z:J";
static char *options = 
   "[-C] [-P] [-Dname[=[text]]] [-Uname] [-Ipath] [-ofile]\n    \
[-A(S|C|M|L|H)] [-Za] [-J] [files]";
#endif					/* MICROSOFT || INTEL_386 || ...  */

#if ZTC_386 
static char *ostr = "CPD:I:U:o:m:AuJWf234";
static char *options = 
   "[-C] [-P] [-Dname[=[text]]] [-Uname] [-Ipath[;path...]] [-ofile]\n    \
[-m(t|s|c|m|l|v|r|p|x|z)] [-A] [-u] [-J] [-W] [-f] [-(2|3|4)] [files]";
#endif					/* ZTC_386 */

#if TURBO || BORLAND_286 || BORLAND_386 
static char *ostr = "CPD:I:U:o:m:p";
static char *options = 
   "[-C] [-P] [-Dname[=[text]]] [-Uname] [-Ipath] [-ofile]\n    \
[-m(t|s|m|c|l|h)] [-p] [files]";
#endif					/* TURBO || BORLAND_286 ... */
#endif					/* MSDOS */
 
#if VM || MVS
static char *ostr = "CPD:I:U:o:";
static char *options =
   "<-C> <-P> <-Dname<=<text>>> <-Uname> <-Ipath> <-ofile> <files>";
   /* ??? may need more options */
#endif                                  /* VM || MVS */

#if UNIX || VMS
static char *ostr = "CPD:I:U:o:";
static char *options = 
   "[-C] [-P] [-Dname[=[text]]] [-Uname] [-Ipath] [-ofile] [files]";
#endif					/* UNIX || VMS */

/*
 * End of operating-system specific code.
 */

extern line_cntrl;

/*
 * getopt() variables
 */
extern int optind;		/* index into parent argv vector */
extern int optopt;		/* character checked for validity */
extern char *optarg;		/* argument associated with option */

int main(argc, argv)
int argc;
char **argv;
   {
   int c;
   char *opt_lst;
   char **opt_args;
   int nopts;
   FILE *out_file;

   /*
    * By default, keep the image of white space, but replace each comment
    *  by a space. By default, output #line directives.
    */
   whsp_image = NoComment;
   line_cntrl = 1;

   /*
    * The number of options that must be passed on to other phases
    *  of the preprocessor are at most as large as the entire option
    *  list.
    */
   opt_lst = (char *)alloc((unsigned int)argc);
   opt_args = (char **)alloc((unsigned int)((sizeof (char *)) * argc));
   nopts = 0;
   out_file = stdout;

   /*
    * Process options.
    */
   while ((c = getopt(argc, argv, ostr)) != EOF)
      switch (c) {
         case 'C':  /* -C - retan comments */
            whsp_image = FullImage;
            break;
          case 'P': /* -P - do not output #line directives */
            line_cntrl = 0;
            break;
         case 'D': /* -D<id><definition> - predefine an identifier */
         case 'I': /* -I<path> - location to search for standard header files */ 
         case 'U': /* -U<id> - undefine predefined identifier */

/*
 * The following code is operating-system dependent [@pmain.02]. Accept
 *  system specific options.
 */

#if PORT
   /* may need something */
Deliberate Syntax Error
#endif					/* PORT */

#if AMIGA || ATARI_ST || MACINTOSH
   /* may need something */
Deliberate Syntax Error
#endif					/* AMIGA || ATARI_ST || ... */

#if MSDOS
#if MICROSOFT
         case 'A':
         case 'J':
         case 'Z':
#endif					/* MICROSOFT */

#if ZTC_386
         case 'm':
         case 'A':
         case 'u':
         case 'J':
         case 'W':
         case 'f':
         case '2':
         case '3':
         case '4':
#endif					/* ZTC_386 */

#if TURBO || BORLAND_286 || BORLAND_386 
         case 'm':
         case 'p':
#endif					/* TURBO || BORLAND_286 ... */

#if HIGHC_386 || INTEL_386 || WATCOM
   /* something is needed */
#endif					/* HIGHC_386 || INTEL_386 || ... */
#endif					/* MSDOS */

#if MVS || VM
   /* ??? we'll see */
#endif					/* MVS || VM */

#if UNIX || VMS
   /* nothing is needed */
#endif					/* UNIX || VMS */

/*
 * End of operating-system specific code.
 */

            opt_lst[nopts] = c;
            opt_args[nopts] = optarg;
            ++nopts;
            break;
         case 'o': /* -o<file> - write output to this file */
            out_file = fopen(optarg, "w");
            if (out_file == NULL)
               err2("cannot open output file ", optarg);
            break;
         default:
            show_usage();
         }

   opt_lst[nopts] = '\0';

   /*
    * Scan file name arguments. If there are none, process standard input,
    *  indicated by the name "-".
    */
   if (optind == argc) {
      init_preproc("-", opt_lst, opt_args);
      output(out_file);
      }
   else {
      while (optind < argc)  {
         init_preproc(argv[optind], opt_lst, opt_args);
         output(out_file);
         optind++;
         }
      }

   return EXIT_SUCCESS;
   }

/*
 * Print an error message if called incorrectly.
 */
void show_usage()
   {
   fprintf(stderr, "usage: %s %s\n", progname, options);
   exit(EXIT_FAILURE);
   }
