#include "../h/gsupport.h"

/*
 * redirerr - redirect error output to the named file. '-' indicates that
 *  it should be redirected to standard out.
 */
int redirerr(p)
char *p;
   {
   if (*p == '-') {			/* redirect stderr to stdout */
      setbuf(stdout, NULL);
      setbuf(stderr, NULL);
      dup2(fileno(stdout), fileno(stderr));
      }
   else					/* redirecting to named file */
      if (freopen(p, "w", stderr) == NULL)
         return 0;
   return 1;
   }
