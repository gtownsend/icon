/*
 *  Global variables.
 */

extern char *runtime;

#ifndef Global
#define Global extern
#define Init(v)
#endif					/* Global */

/*
 * Variables related to command processing.
 */
Global char *progname	Init("iconc");	/* program name for diagnostics */

Global int debug_info	Init(0);	/* -fd, -t: generate debugging info */
Global int err_conv     Init(0);	/* -fe: support error conversion */

#ifdef LargeInts
   Global int largeints	Init(1);	/* -fl: support large integers */
#else					/* LargeInts */
   Global int largeints	Init(0);	/* -fl: support large integers */
#endif					/* LargeInts */

Global int line_info	Init(0);	/* -fn, -fd, -t: generate line info */
Global int m4pre	Init(0);	/* -m: use m4 preprocessor? */
Global int str_inv	Init(0);	/* -fs: enable full string invocation */
Global int trace	Init(0);	/* -t: initial &trace value */
Global int uwarn	Init(0);	/* -u: warn about undefined ids? */
Global int just_type_trace Init(0);	/* -T: suppress C code */
Global int verbose      Init(1);	/* -s, -v: level of verbosity */
Global int pponly       Init(0);	/* -E: preprocess only */

Global char *c_comp     Init(CComp);    /* -C: C compiler */
Global char *c_opts     Init(COpts);    /* -p: options for C compiler */

/*
 * Flags turned off by the -n option.
 */
Global int opt_cntrl	Init(1);	/* do control flow optimization */
Global int opt_sgnl	Init(1);	/* do signal handling optimizations */
Global int do_typinfer	Init(1);	/* do type inference */
Global int allow_inline Init(1);	/* allow expanding operations in line */

/*
 * Files.
 */
Global FILE *codefile	Init(0);	/* C code output - primary file */
Global FILE *inclfile	Init(0);	/* C code output - include file */
