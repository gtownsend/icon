/*
 * Opcode definitions used in icode.
 */

/*
 * Operators. These must be in the same order as in odefs.h.  Not very nice,
 *  but it'll have to do until we think of another way to do this.  (It's
 *  always been thus.)
 */
#define Op_Asgn		  1
#define Op_Bang		  2
#define Op_Cat		  3
#define Op_Compl	  4
#define Op_Diff		  5
#define Op_Div		  6
#define Op_Eqv		  7
#define Op_Inter	  8
#define Op_Lconcat	  9
#define Op_Lexeq	 10
#define Op_Lexge	 11
#define Op_Lexgt	 12
#define Op_Lexle	 13
#define Op_Lexlt	 14
#define Op_Lexne	 15
#define Op_Minus	 16
#define Op_Mod		 17
#define Op_Mult		 18
#define Op_Neg		 19
#define Op_Neqv		 20
#define Op_Nonnull	 21
#define Op_Null		 22
#define Op_Number	 23
#define Op_Numeq	 24
#define Op_Numge	 25
#define Op_Numgt	 26
#define Op_Numle	 27
#define Op_Numlt	 28
#define Op_Numne	 29
#define Op_Plus		 30
#define Op_Power	 31
#define Op_Random	 32
#define Op_Rasgn	 33
#define Op_Refresh	 34
#define Op_Rswap	 35
#define Op_Sect		 36
#define Op_Size		 37
#define Op_Subsc	 38
#define Op_Swap		 39
#define Op_Tabmat	 40
#define Op_Toby		 41
#define Op_Unions	 42
#define Op_Value	 43
/*
 * Other instructions.
 */
#define Op_Bscan	 44
#define Op_Ccase	 45
#define Op_Chfail	 46
#define Op_Coact	 47
#define Op_Cofail	 48
#define Op_Coret	 49
#define Op_Create	 50
#define Op_Cset		 51
#define Op_Dup		 52
#define Op_Efail	 53
#define Op_Eret		 54
#define Op_Escan	 55
#define Op_Esusp	 56
#define Op_Field	 57
#define Op_Goto		 58
#define Op_Init		 59
#define Op_Int		 60
#define Op_Invoke	 61
#define Op_Keywd	 62
#define Op_Limit	 63
#define Op_Line		 64
#define Op_Llist	 65
#define Op_Lsusp	 66
#define Op_Mark		 67
#define Op_Pfail	 68
#define Op_Pnull	 69
#define Op_Pop		 70
#define Op_Pret		 71
#define Op_Psusp	 72
#define Op_Push1	 73
#define Op_Pushn1	 74
#define Op_Real		 75
#define Op_Sdup		 76
#define Op_Str		 77
#define Op_Unmark	 78
#define Op_Var		 80
#define Op_Arg		 81
#define Op_Static	 82
#define Op_Local	 83
#define Op_Global	 84
#define Op_Mark0	 85
#define Op_Quit		 86
#define Op_FQuit	 87
#define Op_Tally	 88
#define Op_Apply	 89

/*
 * "Absolute" address operations.  These codes are inserted in the
 * icode at run-time by the interpreter to overwrite operations
 * that initially compute a location relative to locations not known until
 * the icode file is loaded.
 */
#define Op_Acset	 90
#define Op_Areal	 91
#define Op_Astr		 92
#define Op_Aglobal	 93
#define Op_Astatic	 94
#define Op_Agoto	 95
#define Op_Amark	 96

#define Op_Noop		 98

#define Op_Colm		108		/* column number */

/*
 * Declarations and such -- used by the linker but not the run-time system.
 */

#define Op_Proc		101
#define Op_Declend	102
#define Op_End		103
#define Op_Link		104
#define Op_Version	105
#define Op_Con		106
#define Op_Filen	107

/*
 * Global symbol table declarations.
 */
#define Op_Record	105
#define Op_Impl		106
#define Op_Error	107
#define Op_Trace	108
#define Op_Lab		109
#define Op_Invocable	110
