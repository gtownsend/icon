/*
 *  long.c -- functions for handling long values on 16-bit computers.
 */

#include "../h/gsupport.h"

/*
 * Write a long string in int-sized chunks.
 */

long longwrite(s,len,file)
FILE *file;
char *s;
long len;
{
   long tally = 0;
   int n = 0;
   int leftover, loopnum;
   char *p;

   leftover = (int)(len % (long)MaxInt);
   for (p = s, loopnum = (int)(len / (long)MaxInt); loopnum; loopnum--) {
       n = fwrite(p,sizeof(char),MaxInt,file);
       tally += (long)n;
       p += MaxInt;
   }
   if (leftover) {
      n = fwrite(p,sizeof(char),leftover,file);
      tally += (long)n;
      }
   if (tally != len)
      return -1;
   else return tally;
   }
