/*
 * File: rexternal.r
 *  Functions dealing with external values and their custom functions.
 *
 *  Functions in this file that declare (argc, argv) signatures
 *  follow the ipl/cfuncs/icall.h calling conventions and call
 *  dynamically loaded C functions if available for this external type.
 */

/*
 * callextfunc(func, d1, d2) -- call func(argc, argv) via icall.h conventions.
 * 
 * func() is called with argv=1 if d2 is null or argv=2 if not.
 */
struct descrip callextfunc(int (*func)(int, dptr), dptr dp1, dptr dp2) {
   struct descrip stack[3];
   int nargs = 1;

   stack[0] = nulldesc;
   stack[1] = *dp1;
   if (dp2 != NULL) {
      stack[2] = *dp2;
      nargs = 2;
      }
   if (func(nargs, stack) != 0)
      syserr("external value helper function did not succeed");
   return stack[0];
   }

/*
 * extlname(argc, argv) - return the name of the type of external value argv[1].
 */
int extlname(int argc, dptr argv)
   {
   struct b_external *block = (struct b_external *)BlkLoc(argv[1]);
   struct b_extlfuns *funcs = block->funcs;

   if (funcs->extlname != NULL) {
      funcs->extlname(1, argv);		/* call custom name function */
      if (! is:string(argv[0]))
         syserr("extlname: not a string");
      }
   else {
      argv[0].dword = 8;		/* strlen("external") */
      argv[0].vword.sptr = "external";
      }
   return 0;
   }

/*
 * extlimage(argc, argv) - return the image of external value argv[1].
 *
 * Always sets argv[0] to a valid string, but returns Error
 * if storage is not available for formatting the details.
 */
int extlimage(int argc, dptr argv)
   {
   struct b_external *block = (struct b_external *)BlkLoc(argv[1]);
   struct b_extlfuns *funcs = block->funcs;
   word len;
   int nwords;

   if (funcs->extlimage != NULL) {
      funcs->extlimage(1, argv);	/* call custom image function */
      if (! is:string(argv[0]))
         syserr("extlimage: not a string");
      return 0;
      }

   extlname(1, &argv[0]);		/* get type name, result in argv[0] */
   len = StrLen(argv[0]);
   Protect(reserve(Strings, len + 30), return Error);
   Protect(StrLoc(argv[0]) = alcstr(StrLoc(argv[0]), len), return Error);
   /*
    * to type name append "_<id>(nwords)"
    */
   nwords = ((char*)block + block->blksize - (char*)block->data) / sizeof(word);
   len += sprintf(StrLoc(argv[0]) + len, "_%ld(%d)", (long)block->id, nwords);
   StrLen(argv[0]) = len;
   return 0;
   }

/*
 * extlcmp(argc, argv) - compare two external values argv[1] and argv[2].
 */

int extlcmp(int argc, dptr argv) {
   struct b_external *block1 = (struct b_external *)BlkLoc(argv[1]);
   struct b_external *block2 = (struct b_external *)BlkLoc(argv[2]);
   struct b_extlfuns *funcs = block1->funcs;

   /*
    * If the two values share the same function list, then by definition
    * they are the same type and are compared using a custom function if
    * one is provided in the list.
    */
   if (block1->funcs == block2->funcs && funcs->extlcmp != NULL) {
      funcs->extlcmp(1, argv);		/* call custom comparison function */
      if (! is:integer(argv[0]))
         syserr("extlcmp: not an integer");
      }
   else {
      /*
       * Otherwise, sort by name and then by serial number.
       */
      struct descrip name1 = callextfunc(&extlname, &argv[1], NULL);
      struct descrip name2 = callextfunc(&extlname, &argv[2], NULL);
      long result = lexcmp(&name1, &name2);
      if (result == Equal)
         result = block1->id - block2->id;
      argv[0].dword = D_Integer;
      argv[0].vword.integr = result;
      }
   return 0;
   }

/*
 * extlcopy(argc, argv) - return a copy of external value argv[1].
 *
 * By default this is the original descriptor.
 */

int extlcopy(int argc, dptr argv) {
   struct b_external *block = (struct b_external *)BlkLoc(argv[1]);
   struct b_extlfuns *funcs = block->funcs;

   if (funcs->extlcopy != NULL) {
      funcs->extlcopy(1, argv);		/* call custom copy function */
      if (Qual(argv[0]) || Type(argv[0]) != T_External)
         syserr("extlcopy: not an external");
      }
   else {
      argv[0] = argv[1];		/* the identical external value */
      }
   return 0;
   }
