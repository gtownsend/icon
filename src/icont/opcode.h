/*
 * Opcode table structure.
 */

struct opentry {
   char *op_name;		/* name of opcode */
   int   op_code;		/* opcode number */
   };

/*
 * External definitions.
 */

extern struct opentry optable[];
extern int NOPCODES;

#include "../h/opdefs.h"
