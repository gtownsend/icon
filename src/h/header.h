/*
 * Interpreter code file header - this is written at the start of
 *  an icode file after the start-up program.
 */

#define IHEADER_MAGIC 0x96C53A69	/* confirms validity of flags word */

#define IHEADER_PROFILING 0x00000001	/* set if profiling */

struct header {
   word hsize;			/* size of interpreter code */
   word trace;			/* initial value of &trace */

   word Records;
   word Ftab;			/* location of record/field table */
   word Fnames;			/* location of names of fields */
   word Globals;		/* location of global variables */
   word Gnames;			/* location of names of globals */
   word Statics;		/* location of static variables */
   word Strcons;		/* location of identifier table */
   word Filenms;		/* location of ipc/file name table */

   word linenums;		/* location of ipc/line number table */
   word iversion[4];		/* icode version */

   /*
    * Through Icon 9.5.0, the following were written from uninitialized memory;
    * they should be treated as valid only if "magic" is correct.
    */
   word magic;			/* should equal IHEADER_MAGIC */
   word flags;			/* flag bits defined above */
   word future[10];		/* padding for compatibility and future use */
   };
