#if !COMPILER
#ifdef ExternalFunctions
/*
 * Example of calling C functions by their names.  Here it's just
 *  chdir (change directory) or getwd (get path of current working directory).
 */

struct descrip retval;			/* for returned value */

dptr extcall(dargv, argc, ip)
dptr dargv;
int argc;
int *ip;
   {
   int len, retcode;
   int chdir(), getwd();
   char sbuf[MaxCvtLen];
    
   *ip = -1;				/* anticipate error-free execution */
   
   if (!cnv_str(dargv, dargv)) {	/* 1st argument must be a string */
      *ip = 103;			/* "string expected" error number */
      return dargv;			/* return offending value */
      }

   if (strncmp("chdir", StrLoc(*dargv), StrLen(*dargv)) == 0) {
      if (argc < 2) {			/* must be a 2nd argument */
         *ip = 103;			/* no error number fits, really */
         return NULL;			/* no offedning value */
         }
      dargv++;				/* get to next argument */
      if (!cnv_str(dargv, dargv)) {	/* 2nd argument must be a string */
         *ip = 103;			/* "string expected" error number */
         return dargv;			/* return offending value */
         }
      qtos(dargv,sbuf);			/* get C-style string in sbuf2 */
      retcode = chdir(sbuf);		/* try to change directory */
      if (retcode == -1)		/* see if chdir failed */
         return (dptr)NULL;		/* signal failure */
      return &zerodesc;			/* not a very useful result */
      }
   else if (strncmp("getwd", StrLoc(*dargv), StrLen(*dargv)) == 0) {
      dargv++;				/* get to next argument */
      retcode = getwd(sbuf);		/* get current working directory */
      if (retcode == 0)			/* see if getwd failed */
         return NULL;			/* signal failure */
      len = strlen(sbuf);		/* length of resulting string */
      StrLoc(retval) = alcstr(sbuf,len);  /* allocate and copy the string */
      if (StrLoc(retval) == NULL) {	/* allocation may fail */
          *ip = 0;
          return (dptr)NULL;		/* no offending value */
          }
      StrLen(retval) = len;
      return &retval;			/* return a pointer to the qualifier */
      }
   else {
      *ip = 216;		/* name is not one of those supported here */
      return dargv;		/* return pointer to offending value */
      }
   }
#else					/* ExternalFunctions */
static char x;				/* avoid empty module */
#endif					/* ExternalFunctions */
#endif					/* !COMPILER */
