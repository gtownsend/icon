/*
 * llex.c -- lexical analysis routines.
 */

#include "link.h"
#include "tproto.h"
#include "tglobals.h"
#include "opcode.h"

int nlflag = 0;		/* newline last seen */

#define tonum(c)	(isdigit(c) ? (c - '0') : ((c & 037) + 9))

/*
 * getopc - get an opcode from infile, return the opcode number (via
 *  binary search of opcode table), and point id at the name of the opcode.
 */
int getopc(id)
char **id;
   {
   register char *s;
   register struct opentry *p;
   register int test;
   word indx;
   int low, high, cmp;

   indx = getstr();
   if (indx == -1)
      return EOF;
   s = &lsspace[indx];
   low = 0;
   high = NOPCODES;
   do {
      test = (low + high) / 2;
      p = &optable[test];
      if ((cmp = strcmp(p->op_name, s)) < 0)
         low = test + 1;
      else if (cmp > 0)
         high = test;
      else {
         *id = p->op_name;
         return (p->op_code);
         }
      } while (low < high);
   *id = s;
   return 0;
   }

/*
 * getid - get an identifier from infile, put it in the identifier
 *  table, and return a index to it.
 */
word getid()
   {
   word indx;

   indx = getstr();
   if (indx == -1)
      return EOF;
   return putident((int)strlen(&lsspace[indx])+1, 1);
   }

/*
 * getstr - get an identifier from infile and return an index to it.
 */
word getstr()
   {
   register int c;
   register word indx;

   indx = lsfree;
   while ((c = getc(infile)) == ' ' || c == '\t') ;
   if (c == EOF)
      return -1;
   while (c != ' ' && c != '\t' && c != '\n' && c != ',' && c != EOF) {
      if (indx >= stsize)
         lsspace = (char *)trealloc(lsspace, NULL, &stsize, 1, 1,
            "string space");
      lsspace[indx++] = c;
      c = getc(infile);
      }
   if (indx >= stsize)
      lsspace = (char *)trealloc(lsspace, NULL, &stsize, 1, 1,
         "string space");
   lsspace[indx] = '\0';
   nlflag = (c == '\n');
   return lsfree;
   }

/*
 * getrest - get the rest of the line from infile, put it in the identifier
 *  table, and return its index in the string space.
 */
word getrest()
   {
   register int c;
   register word indx;

   indx = lsfree;
   while ((c = getc(infile)) != '\n' && c != EOF) {
      if (indx >= stsize)
         lsspace = (char *)trealloc(lsspace, NULL, &stsize, 1, 1,
            "string space");
      lsspace[indx++] = c;
      }
   if (indx >= stsize)
      lsspace = (char *)trealloc(lsspace, NULL, &stsize, 1, 1,
         "string space");
   lsspace[indx++] = '\0';
   nlflag = (c == '\n');
   return putident((int)(indx - lsfree), 1);
   }

/*
 * getdec - get a decimal integer from infile, and return it.
 */
int getdec()
   {
   register int c, n;
   int sign = 1, rv;

   n = 0;
   while ((c = getc(infile)) == ' ' || c == '\t') ;
   if (c == EOF)
      return 0;
   if (c == '-') {
      sign = -1;
      c = getc(infile);
      }
   while (c >= '0' && c <= '9') {
      n = n * 10 + (c - '0');
      c = getc(infile);
      }
   nlflag = (c == '\n');
   rv = n * sign;
   return rv;					/* some compilers ... */
   }

/*
 * getoct - get an octal number from infile, and return it.
 */
int getoct()
   {
   register int c, n;

   n = 0;
   while ((c = getc(infile)) == ' ' || c == '\t') ;
   if (c == EOF)
      return 0;
   while (c >= '0' && c <= '7') {
      n = (n << 3) | (c - '0');
      c = getc(infile);
      }
   nlflag = (c == '\n');
   return n;
   }

/*
 *  Get integer, but if it's too large for a long, put the string via wp
 *   and return -1.
 */
long getint(j,wp)
   int j;
   word *wp;
   {
   register int c;
   int over = 0;
   register word indx;
   double result = 0;
   long lresult = 0;
   double radix;
   long iradix;

   ++j;   /* incase we need to add a '\0' and make it into a string */
   if (lsfree + j >= stsize)
      lsspace = (char *)trealloc(lsspace, NULL, &stsize, 1, j, "string space");
   indx = lsfree;

   while ((c = getc(infile)) >= '0' && c <= '9') {
      lsspace[indx++] = c;
      result = result * 10 + (c - '0');
      lresult = lresult * 10 + (c - '0');
      if (result <= MinLong || result >= MaxLong) {
         over = 1;			/* flag overflow */
         result = 0;			/* reset to avoid fp exception */
         }
      }
   if (c == 'r' || c == 'R') {
      lsspace[indx++] = c;
      radix = result;
      iradix = (long)result;
      lresult = 0;
      result = 0;
      while ((c = getc(infile)) != 0) {
         lsspace[indx++] = c;
         if (isdigit(c) || isalpha(c))
            c = tonum(c);
         else
            break;
         result = result * radix + c;
         lresult = lresult * iradix + c;
         if (result <= MinLong || result >= MaxLong) {
            over = 1;			/* flag overflow */
            result = 0;			/* reset to avoid fp exception */
            }
         }
      }
   nlflag = (c == '\n');
   if (!over)
      return lresult;			/* integer is small enough */
   else {				/* integer is too large */
      lsspace[indx++] = '\0';
      *wp = putident((int)(indx - lsfree), 1); /* convert integer to string */
      return -1;			/* indicate integer is too big */
      }
   }

/*
 * getreal - get an Icon real number from infile, and return it.
 */
double getreal()
   {
   double n;
   register int c, d, e;
   int esign;
   register char *s, *ep;
   char cbuf[128];

   s = cbuf;
   d = 0;
   while ((c = getc(infile)) == '0')
      ;
   while (c >= '0' && c <= '9') {
      *s++ = c;
      d++;
      c = getc(infile);
      }
   if (c == '.') {
      if (s == cbuf)
         *s++ = '0';
      *s++ = c;
      while ((c = getc(infile)) >= '0' && c <= '9')
         *s++ = c;
      }
   ep = s;
   if (c == 'e' || c == 'E') {
      *s++ = c;
      if ((c = getc(infile)) == '+' || c == '-') {
         esign = (c == '-');
         *s++ = c;
         c = getc(infile);
         }
      else
         esign = 0;
      e = 0;
      while (c >= '0' && c <= '9') {
         e = e * 10 + c - '0';
         *s++ = c;
         c = getc(infile);
         }
      if (esign) e = -e;
      e += d - 1;
      if (abs(e) >= LogHuge)
         *ep = '\0';
      }
   *s = '\0';
   n = atof(cbuf);
   nlflag = (c == '\n');
   return n;
   }

/*
 * getlab - get a label ("L" followed by a number) from infile,
 *  and return the number.
 */

int getlab()
   {
   register int c;

   while ((c = getc(infile)) != 'L' && c != EOF && c != '\n') ;
   if (c == 'L')
      return getdec();
   nlflag = (c == '\n');
   return 0;
   }

/*
 * getstrlit - get a string literal from infile, as a string
 *  of octal bytes, and return its index into the string table.
 */
word getstrlit(l)
register int l;
   {
   register word indx;

   if (lsfree + l >= stsize)
      lsspace = (char *)trealloc(lsspace, NULL, &stsize, 1, l, "string space");
   indx = lsfree;
   while (!nlflag && l--)
      lsspace[indx++] = getoct();
   if (indx >= stsize)
      lsspace = (char *)trealloc(lsspace, NULL, &stsize, 1, 1,
         "string space");
   lsspace[indx++] = '\0';
   return putident((int)(indx-lsfree), 1);
   }

/*
 * newline - skip to next line.
 */
void newline()
   {
   register int c;

   if (!nlflag) {
      while ((c = getc(infile)) != '\n' && c != EOF) ;
      }
   nlflag = 0;
   }
