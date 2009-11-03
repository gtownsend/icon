/*
 * Interpreter code file header - this is written at the start of
 *  an icode file after the start-up program.
 */
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
   word config[16];		/* icode version */
   };
