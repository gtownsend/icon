#include "../preproc/preproc.h"
#include "../preproc/pproto.h"

int *first_char;
int *next_char;
int *last_char;

/*
 * fill_cbuf - fill the current character buffer.
 */
void fill_cbuf()
   {
   register int c1, c2, c3;
   register int *s;
   register int *l;
   int c;
   int line;
   int changes;
   struct char_src *cs;
   FILE *f;

   cs = src_stack->u.cs;
   f = cs->f;
   s = cs->char_buf;
   l = cs->line_buf;

   if (next_char == NULL) {
      /*
       * Initial filling of buffer.
       */
      first_char = cs->char_buf;
      last_char = first_char + cs->bufsize - 3;
      cs->last_char = last_char;
      line = 1;
      /*
       * Get initial read-ahead.
       */
      if ((c2 = getc(f)) != EOF)
         c3 = getc(f);
      }
   else if (*next_char == EOF)
      return;
   else {
      /*
       * The calling routine needs at least 2 characters, so there is one
       *  left in the buffer.
       */
      *s++= *next_char;
      line = cs->line_buf[next_char - first_char];
      *l++ = line;

      /*
       * Retrieve the 2 read-ahead characters that were saved the last
       *  time the buffer was filled.
       */
      c2 = last_char[1];
      c3 = last_char[2];
      }

   next_char = first_char;

   /*
    * Fill buffer from input file.
    */
   while (s <= last_char) {
      c1 = c2;
      c2 = c3;
      c3 = getc(f);

      /*
       * The first phase of input translation is done here: trigraph
       *  translation and the deletion of backslash-newline pairs.
       */
      changes = 1;
      while (changes) {
         changes = 0;
         /*
          * check for trigraphs
          */
         if (c1 == '?' && c2 == '?') {
            c = ' ';
            switch (c3) {
               case '=':
                  c = '#';
                  break;
               case '(':
                  c = '[';
                  break;
               case '/':
                  c = '\\';
                  break;
               case ')':
                  c = ']';
                  break;
               case '\'':
                  c = '^';
                  break;
               case '<':
                  c = '{';
                  break;
               case '!':
                  c = '|';
                  break;
               case '>':
                  c = '}';
                  break;
               case '-':
                  c = '~';
                  break;
               }
            /*
             * If we found a trigraph, use it and refill the 2-character
             *  read-ahead.
             */
            if (c != ' ') {
               c1 = c;
               if ((c2 = getc(f)) != EOF)
                  c3 = getc(f);
               changes = 1;
               }
            }

         /*
          * delete backslash-newline pairs
          */
         if (c1 == '\\' && c2 == '\n') {
            ++line;
            if ((c1 = c3) != EOF)
               if ((c2 = getc(f)) != EOF)
                  c3 = getc(f);
            changes = 1;
            }
         }
      if (c1 == EOF) {
         /*
          * If last character in file is not a new-line, insert one.
          */
         if (s == first_char || s[-1] != '\n')
            *s++ = '\n';
         *s = EOF;
         last_char = s;
         cs->last_char = last_char;
         return;
         }
      if (c1 == '\n')
         ++line;
      *s++ = c1;   /* put character in buffer */
      *l++ = line;
      }

   /*
    * Save the 2 character read-ahead in the reserved space at the end
    *  of the buffer.
    */
   last_char[1] = c2;
   last_char[2] = c3;
   }
