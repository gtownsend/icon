#include "rtt.h"

/*
 * prototypes for static functions.
 */
static void add_tdef (char *name);

/*
 * refpath is used to locate the standard include files for the Icon
 *  run-time system. If patchpath has been patched in the binary of rtt,
 *  the string that was patched in is used for refpath.
 */
char *refpath;
char patchpath[MaxPath+18] = "%PatchStringHere->";

static char *ostr = "+ECPD:I:U:d:cir:st:x";

static char *options =
   "[-E] [-C] [-P] [-Dname[=[text]]] [-Uname] [-Ipath] [-dfile]\n    \
[-rpath] [-tname] [-x] [files]";

/*
 * The relative path to grttin.h and rt.h depends on whether they are
 *  interpreted as relative to where rtt.exe is or where rtt.exe is
 *  invoked.
 */
    char *grttin_path = "../src/h/grttin.h";
    char *rt_path = "../src/h/rt.h";

/*
 *  Note: rtt presently does not process system include files. If this
 *   is needed, it may be necessary to add other options that set
 *   manifest constants in such include files.  See pmain.c for the
 *   stand-alone preprocessor for examples of what's needed.
 */

char *progname = "rtt";
char *compiler_def;
FILE *out_file;
char *inclname;
int def_fnd;
char *largeints = NULL;

int iconx_flg = 0;
int enable_out = 0;

static char *curlst_nm = "rttcur.lst";
static FILE *curlst;
static char *cur_src;

extern int line_cntrl;

/*
 * tdefnm is used to construct a list of identifiers that
 *  must be treated by rtt as typedef names.
 */
struct tdefnm {
   char *name;
   struct tdefnm *next;
   };

static char *dbname = "rt.db";
static int pp_only = 0;
static char *opt_lst;
static char **opt_args;
static char *in_header;
static struct tdefnm *tdefnm_lst = NULL;

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
   int nopts;
   char buf[MaxPath];		/* file name construction buffer */
   struct fileparts *fp;

   /*
    * See if the location of include files has been patched into the
    *  rtt executable.
    */
   if ((int)strlen(patchpath) > 18)
      refpath = patchpath+18;
   else
      refpath = relfile(argv[0], "/../");

   /*
    * Initialize the string table and indicate that File must be treated
    *  as a typedef name.
    */
   init_str();
   add_tdef("FILE");

   /*
    * By default, the spelling of white space in unimportant (it can
    *  only be significant with the -E option) and #line directives
    *  are required in the output.
    */
   whsp_image = NoSpelling;
   line_cntrl = 1;

   /*
    * opt_lst and opt_args are the options and corresponding arguments
    *  that are passed along to the preprocessor initialization routine.
    *  Their number is at most the number of arguments to rtt.
    */
   opt_lst = alloc(argc);
   opt_args = alloc(argc * sizeof (char *));
   nopts = 0;

   /*
    * Process options.
    */
   while ((c = getopt(argc, argv, ostr)) != EOF)
      switch (c) {
         case 'E': /* run preprocessor only */
            pp_only = 1;
            if (whsp_image == NoSpelling)
               whsp_image = NoComment;
            break;
         case 'C':  /* retain spelling of white space, only effective with -E */
            whsp_image = FullImage;
            break;
          case 'P': /* do not produce #line directives in output */
            line_cntrl = 0;
            break;
          case 'd': /* -d name: name of data base */
            dbname = optarg;
            break;
         case 'r':  /* -r path: location of include files */
            refpath = optarg;
            break;
         case 't':  /* -t ident : treat ident as a typedef name */
            add_tdef(optarg);
            break;
         case 'x':  /* produce code for interpreter rather than compiler */
            iconx_flg = 1;
            break;

         case 'D':  /* define preprocessor symbol */
         case 'I':  /* path to search for preprocessor includes */
         case 'U':  /* undefine preprocessor symbol */
            /*
             * Save these options for the preprocessor initialization routine.
             */
            opt_lst[nopts] = c;
            opt_args[nopts] = optarg;
            ++nopts;
            break;
         default:
            show_usage();
         }

   #ifdef Rttx
      if (!iconx_flg) {
         fprintf(stdout,
            "rtt was compiled to only support the intepreter, use -x\n");
         exit(EXIT_FAILURE);
         }
   #endif				/* Rttx */

   if (iconx_flg)
      compiler_def = "#define COMPILER 0\n";
   else
      compiler_def = "#define COMPILER 1\n";
   in_header = alloc(strlen(refpath) + strlen(grttin_path) + 1);
   strcpy(in_header, refpath);
   strcat(in_header, grttin_path);
   inclname = alloc(strlen(refpath) + strlen(rt_path) + 1);
   strcpy(inclname, refpath);
   strcat(inclname, rt_path);

   opt_lst[nopts] = '\0';

   /*
    * At least one file name must be given on the command line.
    */
   if (optind == argc)
     show_usage();

   /*
    * When creating the compiler run-time system, rtt outputs a list
    *  of names of C files created, because most of the file names are
    *  not derived from the names of the input files.
    */
   if (!iconx_flg) {
      curlst = fopen(curlst_nm, "w");
      if (curlst == NULL)
         err2("cannot open ", curlst_nm);
      }

   /*
    * Unless the input is only being preprocessed, set up the in-memory data
    *  base (possibly loading it from a file).
    */
   if (!pp_only) {
      fp = fparse(dbname);
      if (*fp->ext == '\0')
         dbname = salloc(makename(buf, SourceDir, dbname, DBSuffix));
      else if (!smatch(fp->ext, DBSuffix))
         err2("bad data base name:", dbname);
      loaddb(dbname);
      }

   /*
    * Scan file name arguments, and translate the files.
    */
   while (optind < argc)  {
      trans(argv[optind]);
      optind++;
      }

   #ifndef Rttx
      /*
       * Unless the user just requested the preprocessor be run, we
       *   have created C files and updated the in-memory data base.
       *   If this is the compiler's run-time system, we must dump
       *   to data base to a file and create a list of all output files
       *   produced in all runs of rtt that created the data base.
       */
      if (!(pp_only || iconx_flg)) {
         if (fclose(curlst) != 0)
            err2("cannot close ", curlst_nm);
         dumpdb(dbname);
         full_lst("rttfull.lst");
         }
   #endif				/* Rttx */

   return EXIT_SUCCESS;
   }

/*
 * trans - translate a source file.
 */
void trans(src_file)
char *src_file;
   {
   char *cname;
   char buf[MaxPath];		/* file name construction buffer */
   char *buf_ptr;
   char *s;
   struct fileparts *fp;
   struct tdefnm *td;
   struct token *t;
   static char *test_largeints = "#ifdef LargeInts\nyes\n#endif\n";
   static int first_time = 1;

   cur_src = src_file;

   /*
    * Read standard header file for preprocessor directives and
    * typedefs, but don't write anything to output.
    */
   enable_out = 0;
   init_preproc(in_header, opt_lst, opt_args);
   str_src("<rtt initialization>", compiler_def, (int)strlen(compiler_def));
   init_sym();
   for (td = tdefnm_lst; td != NULL; td = td->next)
      sym_add(TypeDefName, td->name, OtherDcl, 1);
   init_lex();
   yyparse();
   if (first_time) {
      first_time = 0;
      /*
       * Now that the standard include files have been processed, see if
       *  Largeints is defined and make sure it matches what's in the data base.
       */
      s = "NoLargeInts";
      str_src("<rtt initialization>", test_largeints,
         (int)strlen(test_largeints));
      while ((t = preproc()) != NULL)
          if (strcmp(t->image, "yes"))
             s = "LargeInts";
      if (largeints == NULL)
         largeints = s;
      else if (strcmp(largeints, s) != 0)
         err2("header file definition of LargeInts/NoLargeInts does not match ",
            dbname);
      }
   enable_out = 1;

   /*
    * Make sure we have a .r file or standard input.
    */
   if (strcmp(cur_src, "-") == 0) {
      source("-"); /* tell preprocessor to read standard input */
      cname = salloc(makename(buf, TargetDir, "stdin", CSuffix));
      }
   else {
      fp = fparse(cur_src);
      if (*fp->ext == '\0')
         cur_src = salloc(makename(buf, SourceDir, cur_src, RttSuffix));
      else if (!smatch(fp->ext, RttSuffix))
         err2("unknown file suffix ", cur_src);
      cur_src = spec_str(cur_src);

      /*
       * For the compiler, remove from the data base the list of
       *  files produced from this input file.
       */
      if (!iconx_flg)
         clr_dpnd(cur_src);
      source(cur_src);  /* tell preprocessor to read source file */

      /*
       * For the interpreter prepend "x" to the file name for the .c file.
       */
      buf_ptr = buf;
      if (iconx_flg)
         *buf_ptr++ = 'x';
      makename(buf_ptr, TargetDir, cur_src, CSuffix);
      cname = salloc(buf);
      }

   if (pp_only)
      output(stdout); /* invoke standard preprocessor output routine */
   else {
      /*
       * For the compiler, non-RTL code is put in a file whose name
       *  is derived from input file name. The flag def_fnd indicates
       *  if anything interesting is put in the file.
       */
      def_fnd = 0;
      if ((out_file = fopen(cname, "w")) == NULL)
         err2("cannot open output file ", cname);
      else
         addrmlst(cname, out_file);
      prologue(); /* output standard comments and preprocessor directives */
      yyparse();  /* translate the input */
      fprintf(out_file, "\n");
      if (fclose(out_file) != 0)
         err2("cannot close ", cname);

      /*
       * For the Compiler, note the name of the "primary" output file
       *  in the data base and list of created files.
       */
      if (!iconx_flg)
         put_c_fl(cname, def_fnd);
      }
   }

/*
 * add_tdef - add identifier to list of typedef names.
 */
static void add_tdef(name)
char *name;
   {
   struct tdefnm *td;

   td = NewStruct(tdefnm);
   td->name = spec_str(name);
   td->next = tdefnm_lst;
   tdefnm_lst = td;
   }

/*
 * Add name of file to the output list, and if it contains "interesting"
 *  code, add it to the dependency list in the data base.
 */
void put_c_fl(fname, keep)
char *fname;
int keep;
   {
   struct fileparts *fp;

   fp = fparse(fname);
   fprintf(curlst, "%s\n", fp->name);
   if (keep)
      add_dpnd(src_lkup(cur_src), fname);
   }

/*
 * Print an error message if called incorrectly.
 */
void show_usage()
   {
   fprintf(stderr, "usage: %s %s\n", progname, options);
   exit(EXIT_FAILURE);
   }

/*
 * yyerror - error routine called by yacc.
 */
void yyerror(s)
char *s;
   {
   struct token *t;

   t = yylval.t;
   if (t == NULL)
      err2(s, " at end of file");
   else
      errt1(t, s);
   }
