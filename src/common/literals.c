#include "../h/gsupport.h"
#include "../h/esctab.h"

/*
 * Prototypes.
 */
unsigned short	*bitvect	(char *image, int len);
static int escape               (char **str_ptr, int *nchars_ptr);

/*
 * Within translators, csets are internally implemented as a bit vector made
 *  from an array of unsigned shorts. For portability, only the lower 16
 *  bits of these shorts are used.
 */
#define BVectIndx(c) (((unsigned char)c >> 4) & 0xf)
#define BitInShrt(c) (1 << ((unsigned char)c & 0xf))

/*
 * Macros used by escape() to advance to the next character and to
 *  test the kind of character.
 */
#define NextChar(c) ((*nchars_ptr)--, c = *(*str_ptr)++)
#define isoctal(c) ((c)>='0'&&(c)<='7')	/* macro to test for octal digit */

/*
 * escape - translate the character sequence following a '\' into the
 *   single character it represents.
 */
static int escape(str_ptr, nchars_ptr)
char **str_ptr;
int *nchars_ptr;
   {
   register int c, nc, i;

   /*
    * Note, it is impossible to have a character string ending with a '\',
    *  something must be here.
    */
   NextChar(c);
   if (isoctal(c)) {
      /*
       * translate an octal escape -- backslash followed by one, two, or three
       *  octal digits.
       */
      c -= '0';
      for (i = 2; *nchars_ptr > 0 && isoctal(**str_ptr) && i <= 3; ++i) {
         NextChar(nc);
         c = (c << 3) | (nc - '0');
         }
      return (c & 0377);
      }
   else if (c == 'x') {
      /*
       * translate a hexadecimal escape -- backslash-x followed by one or
       *  two hexadecimal digits.
       */
      c = 0;
      for (i = 1; *nchars_ptr > 0 && isxdigit(**str_ptr) && i <= 2; ++i) {
         NextChar(nc);
         if (nc >= 'a' && nc <= 'f')
            nc -= 'a' - 10;
         else if (nc >= 'A' && nc <= 'F')
            nc -= 'A' - 10;
         else if (isdigit(nc))
            nc -= '0';
         c = (c << 4) | nc;
         }
      return c;
      }
   else if (c == '^') {
      /*
       * translate a control escape -- backslash followed by caret and one
       *  character.
       */
      if (*nchars_ptr <= 0)
         return 0;           /* could only happen in a keyword */
      NextChar(c);
      return (c & 037);
      }
   else
      return esctab[c];
   }

/*
 * bitvect - convert cset literal into a bitvector
 */
unsigned short *bitvect(image, len)
char *image;
int len;
   {
   register int c;
   register unsigned short *bv;
   register int i;

   bv = alloc(BVectSize * sizeof(unsigned short));
   for (i = 0; i < BVectSize; ++i)
      bv[i] = 0;
   while (len-- > 0) {
      c = *image++;
      if (c == '\\')
         c = escape(&image, &len);
      bv[BVectIndx(c)] |= BitInShrt(c);
      }
   return bv;
   }

/*
 * cset_init - use bitvector for a cset to write an initialization for
 *    a cset block.
 */
void cset_init(f, bv)
FILE *f;
unsigned short *bv;
   {
   int size;
   unsigned short n;
   register int j;

   size = 0;
   for (j = 0; j < BVectSize; ++j)
      for (n = bv[j]; n != 0; n >>= 1)
         size += n & 1;
   fprintf(f, "{T_Cset, %d,\n", size);
   fprintf(f, "   cset_display(0x%x", bv[0]);
   for (j = 1; j < BVectSize; ++j)
      fprintf(f, ",0x%x", bv[j]);
   fprintf(f, ")\n    };\n");
   }

/*
 * prtstr - print an Icon string literal as a C string literal.
 */
int prt_i_str(f, s, len)
FILE *f;
char *s;
int len;
   {
   int c;
   int n_chars;

   n_chars = 0;
   while (len-- > 0) {
      ++n_chars;
      c = *s++;
      if (c == '\\')
         c = escape(&s, &len);
      switch (c) {
         case '\n':
            fprintf(f, "\\n");
            break;
         case '\t':
            fprintf(f, "\\t");
            break;
         case '\v':
            fprintf(f, "\\v");
            break;
         case '\b':
            fprintf(f, "\\b");
            break;
         case '\r':
            fprintf(f, "\\r");
            break;
         case '\f':
            fprintf(f, "\\f");
            break;
         case '\\':
            fprintf(f, "\\\\");
            break;
         case '\"':
            fprintf(f, "\\\"");
            break;
         default:
            if (isprint(c))
               fprintf(f, "%c", c);
            else
               fprintf(f, "\\%03o", (int)c);
         }
      }
   return n_chars;
   }
