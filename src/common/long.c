/*
 *  long.c -- functions for handling long values on 16-bit computers.
 */

#include "../h/gsupport.h"

#if IntBits == 16
/*
 * Long strlen
 */

long lstrlen(s)
char *s;
{
    long l = 0;
    while(*s++) l++;
    return l;
}

/* Shell sort with some enhancements from Knuth.. */

void lqsort( base, nel, width, cmp )
char *base;
int nel;
int width;
int (*cmp)();
{
   register int i, j;
   long int gap;
   int k, tmp ;
   char *p1, *p2;

   for( gap=1; gap <= nel; gap = 3*gap + 1 ) ;

   for( gap /= 3;  gap > 0  ; gap /= 3 )
       for( i = gap; i < nel; i++ )
           for( j = i-gap; j >= 0 ; j -= gap ) {
                p1 = base + ( j     * width);
                p2 = base + ((j+gap) * width);

                if( (*cmp)( p1, p2 ) <= 0 ) break;

                for( k = width; --k >= 0 ;) {
                   tmp   = *p1;
                   *p1++ = *p2;
                   *p2++ = tmp;
                }
           }
}
#endif					/* IntBits == 16 */

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
