#if !COMPILER
#ifdef ExternalFunctions
/*
 * Example of calling C functions by integer codes.  Here it's
 *  one of three UNIX functions:
 *
 *    1: getpid (get process identification)
 *    2: getppid (get parent process identification)
 *    3: getpgrp (get process group)
 */

struct descrip retval;			/* for returned value */

dptr extcall(dargv, argc, ip)
dptr dargv;
int argc;
int *ip;
   {
   int retcode;
   int getpid(), getppid(), getpgrp();
    
   if (!cnv_int(dargv, dargv)) {	/* 1st argument must be a string */
      *ip = 101;			/* "integer expected" error number */
      return dargv;			/* return offending value */
      }

   switch ((int)IntVal(*dargv)) {
      case 1:				/* getpid */
         retcode = getpid();
         break;
      case 2:				/* getppid */
         retcode = getppid();
         break;
      case 3:				/* getpgrp */
         if (argc < 2) {
            *ip = 205;			/* no error number fits, really */
            return NULL;		/* no offending value */
            }
         dargv++;			/* get to next value */
         if (!cnv_int(dargv, dargv)) {	/* 2nd argument must be integer */
            *ip = 101;			/* "integer expected" error number */
            return dargv;
            }
         retcode = getpgrp(IntVal(*dargv));
         break;
      default:
         *ip = 216;			/* external function not found */
         return NULL;
      }
   MakeInt(retcode,&retval);		/* make an Icon integer for result */
   return &retval;
   }
#else ExternalFunctions
static char x;				/* prevent empty module */
#endif					/* ExternalFunctions */
#endif					/* COMPILER */
