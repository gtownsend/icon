/*
 *  Simple test of dynamic loading from Icon.
 *  Just prints its arguments, then returns pi.
 */

#include "icall.h"

int argdump(int argc, descriptor *argv)
{
   int i, j, w, c;
   char *s, *t;
   descriptor *d;

   for (i = 1; i <= argc; i++) {
      printf("%2d. [%c] ", i, IconType(argv[i]));
      d = argv + i;
      switch (IconType(*d)) {
         case 'n':
	    printf("&null");
	    break;
         case 'i':
	    printf("%ld", IntegerVal(*d));
	    break;
         case 'r':
	    printf("%g", RealVal(*d));
	    break;
         case 's':
	    printf("%s", StringVal(*d));
	    break;
	 case 'c':
	    s = (char *)d->vword;
            s += 2 * sizeof(long);		/* skip title & size */
	    t = s + 256 / 8;
	    c = 0;
	    while (s < t) {
	       w = *(int *)s;
	       for (j = 0; j < 8 * sizeof(int); j++) {
		  if (w & 1)
		     putchar(c);
		  c++;
		  w >>= 1;
		  }
	       s += sizeof(int);
	       }
	    break;
	 case 'f':
	    printf("fd=%d  (", fileno(FileVal(*d)));
	    if (FileStat(*d) & Fs_Read) putchar('r');
	    if (FileStat(*d) & Fs_Write) putchar('w');
	    putchar(')');
	    break;
         default: 
	    printf("??");
	    break;
         }
      putchar('\n');
      }
   RetReal(3.1415926535);
}
