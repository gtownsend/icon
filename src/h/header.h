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

   #ifdef FieldTableCompression
      short FtabWidth;		/* width of field table entries, 1 | 2 | 4 */
      short FoffWidth;		/* width of field offset entries, 1 | 2 | 4 */
      word Nfields;		/* number of field names */
      word Fo;			/* The start of the Fo array */
      word Bm;			/* The start of the Bm array */
   #endif				/* FieldTableCompression */

   word linenums;		/* location of ipc/line number table */
   word config[16];		/* icode version */
   };
