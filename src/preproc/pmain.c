#include "../preproc/preproc.h"
#include "../preproc/pproto.h"

char *progname = "pp";

/*
 * Establish command-line options.
 */
static char *ostr = "+CPD:I:U:o:";
static char *options =
   "[-C] [-P] [-Dname[=[text]]] [-Uname] [-Ipath] [-ofile] [files]";

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
   opt_lst = alloc(argc);
   opt_args = alloc(argc * sizeof (char *));
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
