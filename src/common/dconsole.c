/*
 * dconsole.c - versions of run-time support for console windows for
 *  applications that do not include the entire Icon runtime system
 *  (namely, icont and rtt).
 */
#include "../h/rt.h"

#ifdef ConsoleWindow

FILE *ConsoleBinding = NULL;
struct region *curstring, *curblock;

#define AlcBlk(var, struct_nm, t_code, nbytes) \
var = (struct struct_nm *)calloc(1, nbytes); \
if (!var) return NULL; \
var->title = t_code;

#define AlcFixBlk(var, struct_nm, t_code)\
   AlcBlk(var, struct_nm, t_code, sizeof(struct struct_nm))
/*
 * AlcVarBlk - allocate a variable-length block.
 */
#define AlcVarBlk(var, struct_nm, t_code, n_desc) \
   { \
   register uword size; \
   size = sizeof(struct struct_nm) + (n_desc - 1) * sizeof(struct descrip);\
   AlcBlk(var, struct_nm, t_code, size)\
   var->blksize = size;\
   }

struct descrip nulldesc = {D_Null};	/* null value */
struct descrip nullptr =
   {F_Ptr | F_Nqual};	                /* descriptor with null block pointer */
struct descrip emptystr; 		/* zero-length empty string */
struct descrip kywd_prog;		/* &progname */
struct descrip kywd_err = {D_Integer};  /* &error */

int t_errornumber = 0;			/* tentative k_errornumber value */
int t_have_val = 0;			/* tentative have_errval flag */
struct descrip t_errorvalue;		/* tentative k_errorvalue value */
static int list_ser;
struct tend_desc *tend;

#ifdef MSWindows
   char *getenv(char *s)
   {
   static char tmp[1537];
   DWORD rv;
   rv = GetEnvironmentVariable(s, tmp, 1536);
   if (rv > 0) return tmp;
      return NULL;
   }
#endif						/* MSWindows */

/*
 * An array of all characters for use in making one-character strings.
 */

unsigned char allchars[256] = {
     0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, 15,
    16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31,
    32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47,
    48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63,
    64, 65, 66, 67, 68, 69, 70, 71, 72, 73, 74, 75, 76, 77, 78, 79,
    80, 81, 82, 83, 84, 85, 86, 87, 88, 89, 90, 91, 92, 93, 94, 95,
    96, 97, 98, 99,100,101,102,103,104,105,106,107,108,109,110,111,
   112,113,114,115,116,117,118,119,120,121,122,123,124,125,126,127,
   128,129,130,131,132,133,134,135,136,137,138,139,140,141,142,143,
   144,145,146,147,148,149,150,151,152,153,154,155,156,157,158,159,
   160,161,162,163,164,165,166,167,168,169,170,171,172,173,174,175,
   176,177,178,179,180,181,182,183,184,185,186,187,188,189,190,191,
   192,193,194,195,196,197,198,199,200,201,202,203,204,205,206,207,
   208,209,210,211,212,213,214,215,216,217,218,219,220,221,222,223,
   224,225,226,227,228,229,230,231,232,233,234,235,236,237,238,239,
   240,241,242,243,244,245,246,247,248,249,250,251,252,253,254,255,
};
/*
 * fatalerr - disable error conversion and call run-time error routine.
 */
void fatalerr(n, v)
int n;
dptr v;
   {
   IntVal(kywd_err) = 0;
   err_msg(n, v);
   }

struct b_list *alclist(size)
uword size;
   {
   register struct b_list *blk;
   AlcFixBlk(blk, b_list, T_List)
   blk->size = size;
   blk->id = list_ser++;
   blk->listhead = NULL;
   blk->listtail = NULL;
   return blk;
   }
/*
 * alclstb - allocate a list element block in the block region.
 */

struct b_lelem *alclstb(nslots, first, nused)
uword nslots, first, nused;
   {
   register struct b_lelem *blk;
   register word i, size;

   AlcVarBlk(blk, b_lelem, T_Lelem, nslots)
   blk->nslots = nslots;
   blk->first = first;
   blk->nused = nused;
   blk->listprev = NULL;
   blk->listnext = NULL;
   /*
    * Set all elements to &null.
    */
   for (i = 0; i < nslots; i++)
      blk->lslots[i] = nulldesc;
   return blk;
   }


struct b_real *alcreal(val)
double val;
   {
   register struct b_real *blk;

   AlcFixBlk(blk, b_real, T_Real)

#ifdef Double
   /* access real values one word at a time */
   { int *rp, *rq;
     rp = (int *) &(blk->realval);
     rq = (int *) &val;
     *rp++ = *rq++;
     *rp   = *rq;
   }
#else                                   /* Double */
   blk->realval = val;
#endif                                  /* Double */

   return blk;
   }

char *alcstr(char *s, int len)
{
   register char *s1;

   s1 = (char *)alloc(len + 1);
   return strncpy(s1, s, len);
}

/*
 * initalloc - initialization routine to allocate memory regions
 */

void initalloc(codesize)
word codesize;
   {
   static char dummy[1];	/* dummy static region */

   StrLoc(kywd_prog) = "wicont";
   StrLen(kywd_prog) = strlen(StrLoc(kywd_prog));
   /*
    * Set up allocated memory.	The regions are:
    *	Allocated string region
    *	Allocate block region
    */
   curstring = (struct region *)malloc(sizeof(struct region));
   curblock = (struct region *)malloc(sizeof(struct region));
   curstring->size = 2000;
   curblock->size = 2000;
   curstring->next = curstring->prev = NULL;
   curstring->Gnext = curstring->Gprev = NULL;
   curblock->next = curblock->prev = NULL;
   curblock->Gnext = curblock->Gprev = NULL;

   if ((strfree = strbase = (char *)AllocReg(ssize)) == NULL)
      tfatal("insufficient memory for string region", NULL);
   strend = strbase + ssize;
   if ((blkfree = blkbase = (char *)AllocReg(abrsize)) == NULL)
      tfatal("insufficient memory for block region", NULL);
   blkend = blkbase + abrsize;
   }


void err_msg(n, v)
int n;
dptr v;
{
fprintf(stderr, "err_msg %d\n", n);
c_exit(1);
}

/*
 * qsearch(key,base,nel,width,compar) - binary search
 *
 *  A binary search routine with arguments similar to qsort(3).
 *  Returns a pointer to the item matching "key", or NULL if none.
 *  Based on Bentley, CACM 28,7 (July, 1985), p. 676.
 */

char * qsearch (key, base, nel, width, compar)
char * key;
char * base;
int nel, width;
int (*compar)();
{
    int l, u, m, r;
    char * a;

    l = 0;
    u = nel - 1;
    while (l <= u) {
	m = (l + u) / 2;
	a = (char *) ((char *) base + width * m);
	r = compar (a, key);
	if (r < 0)
	    l = m + 1;
	else if (r > 0)
	    u = m - 1;
	else
	    return a;
    }
    return 0;
}
/*
 * c_get - convenient C-level access to the get function
 *  returns 0 on failure, otherwise fills in res
 */
int c_get(hp,res)
struct b_list *hp;
struct descrip *res;
{
   register word i;
   register struct b_lelem *bp;

   /*
    * Fail if the list is empty.
    */
   if (hp->size <= 0)
      return 0;

   /*
    * Point bp at the first list block.  If the first block has no
    *  elements in use, point bp at the next list block.
    */
   bp = (struct b_lelem *) hp->listhead;
   if (bp->nused <= 0) {
      bp = (struct b_lelem *) bp->listnext;
      hp->listhead = (union block *) bp;
      bp->listprev = NULL;
      }

   /*
    * Locate first element and assign it to result for return.
    */
   i = bp->first;
   *res = bp->lslots[i];

   /*
    * Set bp->first to new first element, or 0 if the block is now
    *  empty.  Decrement the usage count for the block and the size
    *  of the list.
    */
   if (++i >= bp->nslots)
      i = 0;
   bp->first = i;
   bp->nused--;
   hp->size--;

   return 1;
}
/*
 * c_put - C-level, nontending list put function
 */
void c_put(l,val)
struct descrip *l;
struct descrip *val;
{
   register word i;
   register struct b_lelem *bp;  /* does not need to be tended */
   static two = 2;		/* some compilers generate bad code for
				   division by a constant that's a power of 2*/

   /*
    * Point hp at the list-header block and bp at the last
    *  list-element block.
    */
   bp = (struct b_lelem *) BlkLoc(*l)->list.listtail;
   
#ifdef EventMon 	/* initialize i so it's 0 if last list-element */
   i = 0;			/* block isn't full */
#endif				/* EventMon */

   /*
    * If the last list-element block is full, allocate a new
    *  list-element block, make it the last list-element block,
    *  and make it the next block of the former last list-element
    *  block.
    */
   if (bp->nused >= bp->nslots) {
      /*
       * Set i to the size of block to allocate.
       */
      i = ((struct b_list *)BlkLoc(*l))->size / two;
      if (i < MinListSlots)
         i = MinListSlots;
#ifdef MaxListSlots
      if (i > MaxListSlots)
         i = MaxListSlots;
#endif					/* MaxListSlots */

      /*
       * Allocate a new list element block.  If the block can't
       *  be allocated, try smaller blocks.
       */
      while ((bp = alclstb(i, (word)0, (word)0)) == NULL) {
         i /= 4;
         if (i < MinListSlots)
            fatalerr(0,NULL);
         }

      ((struct b_list *)BlkLoc(*l))->listtail->lelem.listnext =
	(union block *) bp;
      bp->listprev = ((struct b_list *)BlkLoc(*l))->listtail;
      ((struct b_list *)BlkLoc(*l))->listtail = (union block *) bp;
      }

   /*
    * Set i to position of new last element and assign val to
    *  that element.
    */
   i = bp->first + bp->nused;
   if (i >= bp->nslots)
      i -= bp->nslots;
   bp->lslots[i] = *val;

   /*
    * Adjust block usage count and current list size.
    */
   bp->nused++;
   ((struct b_list *)BlkLoc(*l))->size++;
}

/*
 * cnv_c_str - cnv:C_string(*s, *d), convert a value into a C (and Icon) string
 */
int cnv_c_str(s, d)
dptr s;
dptr d;
   {
   /*
    * Get the string to the end of the string region and append a '\0'.
    */

   if (!Qual(*s)) {
      /* if (!cnv_str(s, d)) { */
         return 0;
         /*}*/
      }
   else {
      *d = *s;
      }
   {
      register word slen = StrLen(*d);
      register char *sp, *dp;

      dp = malloc(slen+1);
      if (dp == NULL)
         fatalerr(0,NULL);

      StrLen(*d) = StrLen(*d)+1;
      sp = StrLoc(*d);
      StrLoc(*d) = dp;
      while (slen-- > 0)
         *dp++ = *sp++;
      *dp = '\0';
      }
   return 1;
   }

/*
 * itos - convert the integer num into a string using s as a buffer and
 *  making q a descriptor for the resulting string.
 */

static void itos(num, dp, s)
C_integer num;
dptr dp;
char *s;
   {
   register char *p;
   long ival;
   static char *maxneg = MaxNegInt;

   p = s + MaxCvtLen - 1;
   ival = num;

   *p = '\0';
   if (num >= 0L)
      do {
	 *--p = ival % 10L + '0';
	 ival /= 10L;
	 } while (ival != 0L);
   else {
      if (ival == -ival) {      /* max negative value */
	 p -= strlen (maxneg);
	 sprintf (p, "%s", maxneg);
         }
      else {
	ival = -ival;
	do {
	   *--p = '0' + (ival % 10L);
	   ival /= 10L;
	   } while (ival != 0L);
	*--p = '-';
	}
      }

   StrLen(*dp) = s + MaxCvtLen - 1 - p;
   StrLoc(*dp) = p;
   }
/*
 * tmp_str - Convert to temporary string.
 */
int tmp_str(sbuf, s, d)
char *sbuf;
dptr s;
dptr d;
   {
   if (Qual(*s))
      *d = *s;
   else switch (Type(*s)) {
   case T_Integer: {
         itos(IntVal(*s), d, sbuf);
	 break;
	 }
   case T_Real: {
         double res;
         GetReal(s, res);
         rtos(res, d, sbuf);
	 break;
         }
/*
   case T_Cset:
         cstos(BlkLoc(*s)->cset.bits, d, sbuf);
	 break;
*/
   default:
         return 0;
      }
   return 1;
   }

/*
 * radix - convert string s in radix r into an integer in *result.  sign
 *  will be either '+' or '-'.
 */
int radix(sign, r, s, end_s, result)
int sign;
register int r;
register char *s;
register char *end_s;
union numeric *result;
   {
   register int c;
   long num;

   if (r < 2 || r > 36)
      return CvtFail;
   c = (s < end_s) ? *s++ : ' ';
   num = 0L;
   while (isalnum(c)) {
#if !EBCDIC
   #define tonum(c)	(isdigit(c) ? (c)-'0' : 10+(((c)|(040))-'a'))
#endif
      c = tonum(c);
      if (c >= r)
	 return CvtFail;
      num = num * r + c;
      c = (s < end_s) ? *s++ : ' ';
      }

   /*
    * Skip trailing white space and make sure there is nothing else left
    *  in the string. Note, if we have already reached end-of-string,
    *  c has been set to a space.
    */
   while (isspace(c) && s < end_s)
      c = *s++;
   if (!isspace(c))
      return CvtFail;

   result->integer = (sign == '+' ? num : -num);

   return T_Integer;
   }

/*
 * rtos - convert the real number n into a string using s as a buffer and
 *  making a descriptor for the resulting string.
 */
void rtos(n, dp, s)
double n;
dptr dp;
char *s;
   {
   s++; 				/* leave room for leading zero */
   sprintf(s, "%.*g", Precision, n + 0.0);  /* format string; +0.0 avoids -0 */
   
   /*
    * Now clean up possible messes.
    */
   while (*s == ' ')			/* delete leading blanks */
      s++;
   if (*s == '.') {			/* prefix 0 to initial period */
      s--;
      *s = '0';
      }
   else if (strcmp(s, "-0.0") == 0)	/* negative zero */
      s++;
   else if (!strchr(s, '.') && !strchr(s,'e') && !strchr(s,'E'))
         strcat(s, ".0");		/* if no decimal point or exp. */
   if (s[strlen(s) - 1] == '.')		/* if decimal point is at end ... */
      strcat(s, "0");
   StrLen(*dp) = strlen(s);
   StrLoc(*dp) = s;
   }
/*
 * ston - convert a string to a numeric quantity if possible.
 * Returns a typecode or CvtFail.  Its answer is in the dptr,
 * unless its a double, in which case its in the union numeric
 * (we do this to avoid allocating a block for a real
 * that will later be used directly as a C_double).
 */
static int ston(sp, result)
dptr sp;
union numeric *result;
   {
   register char *s = StrLoc(*sp), *end_s;
   register int c;
   int realflag = 0;	/* indicates a real number */
   char msign = '+';    /* sign of mantissa */
   char esign = '+';    /* sign of exponent */
   double mantissa = 0; /* scaled mantissa with no fractional part */
   long lresult = 0;	/* integer result */
   int scale = 0;	/* number of decimal places to shift mantissa */
   int digits = 0;	/* total number of digits seen */
   int sdigits = 0;	/* number of significant digits seen */
   int exponent = 0;	/* exponent part of real number */
   double fiveto;	/* holds 5^scale */
   double power;	/* holds successive squares of 5 to compute fiveto */
   int err_no;
   char *ssave;         /* holds original ptr for bigradix */

   if (StrLen(*sp) == 0)
      return CvtFail;
   end_s = s + StrLen(*sp);
   c = *s++;

   /*
    * Skip leading white space.
    */
   while (isspace(c))
      if (s < end_s)
         c = *s++;
      else
         return CvtFail;

   /*
    * Check for sign.
    */
   if (c == '+' || c == '-') {
      msign = c;
      c = (s < end_s) ? *s++ : ' ';
      }

   ssave = s - 1;   /* set pointer to beginning of digits in case it's needed */

   /*
    * Get integer part of mantissa.
    */
   while (isdigit(c)) {
      digits++;
      if (mantissa < Big) {
	 mantissa = mantissa * 10 + (c - '0');
         lresult = lresult * 10 + (c - '0');
	 if (mantissa > 0.0)
	    sdigits++;
	 }
      else
	 scale++;
      c = (s < end_s) ? *s++ : ' ';
      }

   /*
    * Check for based integer.
    */
   if (c == 'r' || c == 'R') {
      int rv;
      rv = radix((int)msign, (int)mantissa, s, end_s, result);
      return rv;
      }

   /*
    * Get fractional part of mantissa.
    */
   if (c == '.') {
      realflag++;
      c = (s < end_s) ? *s++ : ' ';
      while (isdigit(c)) {
	 digits++;
	 if (mantissa < Big) {
	    mantissa = mantissa * 10 + (c - '0');
	    lresult = lresult * 10 + (c - '0');
	    scale--;
	    if (mantissa > 0.0)
	       sdigits++;
	    }
         c = (s < end_s) ? *s++ : ' ';
	 }
      }

   /*
    * Check that at least one digit has been seen so far.
    */
   if (digits == 0)
      return CvtFail;

   /*
    * Get exponent part.
    */
   if (c == 'e' || c == 'E') {
      realflag++;
      c = (s < end_s) ? *s++ : ' ';
      if (c == '+' || c == '-') {
	 esign = c;
         c = (s < end_s) ? *s++ : ' ';
	 }
      if (!isdigit(c))
	 return CvtFail;
      while (isdigit(c)) {
	 exponent = exponent * 10 + (c - '0');
         c = (s < end_s) ? *s++ : ' ';
	 }
      scale += (esign == '+') ? exponent : -exponent;
      }

   /*
    * Skip trailing white space and make sure there is nothing else left
    *  in the string. Note, if we have already reached end-of-string,
    *  c has been set to a space.
    */
   while (isspace(c) && s < end_s)
      c = *s++;
   if (!isspace(c))
      return CvtFail;

   /*
    * Test for integer.
    */
   if (!realflag && !scale && mantissa >= MinLong && mantissa <= MaxLong) {
      result->integer = (msign == '+' ? lresult : -lresult);
      return T_Integer;
      }

   if (!realflag)
      return CvtFail;		/* don't promote to real if integer format */

   /*
    * Rough tests for overflow and underflow.
    */
   if (sdigits + scale > LogHuge)
      return CvtFail;

   if (sdigits + scale < -LogHuge) {
      result->real = 0.0;
      return T_Real;
      }

   /*
    * Put the number together by multiplying the mantissa by 5^scale and
    *  then using ldexp() to multiply by 2^scale.
    */

   exponent = (scale > 0)? scale : -scale;
   fiveto = 1.0;
   power = 5.0;
   for (;;) {
      if (exponent & 01)
	 fiveto *= power;
      exponent >>= 1;
      if (exponent == 0)
	 break;
      power *= power;
      }
   if (scale > 0)
      mantissa *= fiveto;
   else
      mantissa /= fiveto;

   err_no = 0;
   mantissa = ldexp(mantissa, scale);
   if (err_no > 0 && mantissa > 0)
      /*
       * ldexp caused overflow.
       */
      return CvtFail;

   if (msign == '-')
      mantissa = -mantissa;
   result->real = mantissa;
   return T_Real;
   }

/*
 * cnv_c_dbl - cnv:C_double(*s, *d), convert a value directly into a C double
 */
int cnv_c_dbl(s, d)
dptr s;
double *d;
   {
     struct descrip result, cnvstr;
     char sbuf[MaxCvtLen];

   union numeric numrc;

   if (!Qual(*s)) {
      if (Type(*s) == T_Integer) {
         *d = IntVal(*s);
         return 1;
         }
      else if (Type(*s) == T_Cset) {
        tmp_str(sbuf, s, &cnvstr);
        s = &cnvstr;
        }
      else {
        return 0;
        }
      }

   /*
    * s is now an string.
    */
   switch( ston(s, &numrc) ) {
      case T_Integer:
         *d = numrc.integer;
         return 1;
      case T_Real:
         *d = numrc.real;
         return 1;
      default:
         return 0;
      }
  }

/*
 * cnv_c_int - cnv:C_integer(*s, *d), convert a value directly into a C_integer
 */
int cnv_c_int(s, d)
dptr s;
C_integer *d;
   {
   struct descrip cnvstr, result;			/* not tended */
   union numeric numrc;
   char sbuf[MaxCvtLen];

   if (!Qual(*s)) {
      if (Type(*s) == T_Integer) {
         *d = IntVal(*s);
         return 1;
         }
      else if (Type(*s) == T_Real) {
         double dbl;
         GetReal(s,dbl);
         if (dbl > MaxLong || dbl < MinLong) {
            return 0;
            }
         *d = dbl;
         return 1;
         }
      else if (Type(*s) == T_Cset) {
        tmp_str(sbuf, s, &cnvstr);
        s = &cnvstr;
        }
      else {
         return 0;
         }
      }

   /*
    * s is now a string.
    */
   switch( ston(s, &numrc) ) {
      case T_Integer: {
         *d = numrc.integer;
         return 1;
	 }
      case T_Real: {
         double dbl = numrc.real;
         if (dbl > MaxLong || dbl < MinLong) {
            return 0;
            }
         *d = dbl;
         return 1;
         }
      default:
         return 0;
      }
   }


/*
 * def_c_dbl - def:C_double(*s, df, *d), convert to C double with a
 *  default value. Default is of type C double; if used, just copy to
 *  destination.
 */

int def_c_dbl(s,df,d)
dptr s;
double df;
double *d;
   {
   if (Type(*s) == T_Null) {
      *d = df;
      return 1;
      }
   else
      return cnv_c_dbl(s,d); /* I really mean cnv:type */
   }


int def_c_int(s,df,d)
dptr s;
C_integer df;
C_integer *d;
   {
   if (Type(*s) == T_Null) {
      *d = df;
      return 1;
      }
   else
      return cnv_c_int(s,d); /* I really mean cnv:type */
   }


/*
 * the global buffer used as work space for printing string, etc 
 */
char ConsoleStringBuf[512 * 48];
char *ConsoleStringBufPtr = ConsoleStringBuf;
unsigned long ConsoleFlags = 0;			 /* Console flags */
extern int ConsolePause;
extern FILE *flog;


void closelogfile()
{
   if (flog) {
      extern char *lognam;
      extern char tmplognam[];
      FILE *flog2;
      int i;
      fclose(flog);

      /*
       * copy to the permanent file name
       */
      if ((flog = fopen(tmplognam, "r")) &&
	  (flog2 = fopen(lognam, "w"))) {
	 while ((i = getc(flog)) != EOF)
	    putc(i, flog2);
	 fclose(flog);
	 fclose(flog2);
	 remove(tmplognam);
	 }

      free(lognam);
      flog = NULL;
      }
}


/*
 * c_exit(i) - flush all buffers and exit with status i.
 */
void c_exit(i)
int i;
{
   char *msg = "Strike any key to close console...";

   /*
    * if the console was used for anything, pause it
    */
   if (ConsoleBinding && ConsolePause) {
      char label[256], tossanswer[256];

      wputstr((wbp)ConsoleBinding, msg, strlen(msg));

      strcpy(tossanswer, "label=wicont - execution terminated");
      wattr(ConsoleBinding, tossanswer, strlen(tossanswer));
      waitkey(ConsoleBinding);
      }
   if (flog) {
      extern char *lognam;
      extern char tmplognam[];
      FILE *flog2;
      int i;
      fclose(flog);

      /*
       * try to rename, then try to copy to the permanent file name
       */
      i = rename(tmplognam, lognam);
      if (i != 0) {
         if ((flog = fopen(tmplognam, "r")) &&
	     (flog2 = fopen(lognam, "w"))) {
	    while ((i = getc(flog)) != EOF)
	       putc(i, flog2);
	    fclose(flog);
	    fclose(flog2);
	    remove(tmplognam);
	    }
         }

      free(lognam);
      }
   if (wstates != NULL) {
      PostQuitMessage(i);
      pollevent();
      }

#if !MACINTOSH
   #undef exit
#endif					/* MACINTOSH */
   exit(i);
}


int strncasecmp(char *s1, char *s2, int n)
{
   int i, j;
   for(i=0;i<n;i++) {
      j = tolower(s1[i]) - tolower(s2[i]);
      if (j) return j;
      if (s1[i] == '\0') return 0; /* terminate if both at end-of-string */
      }
   return 0;
}

#endif					/* ConsoleWindow */
