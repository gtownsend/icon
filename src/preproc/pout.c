#include "../preproc/preproc.h"
#include "../preproc/ptoken.h"
#include "../preproc/pproto.h"

int line_cntrl;

/*
 * output - output preprocessed tokens for the current file.
 */
void output(out_file)
FILE *out_file;
   {
   struct token *t, *t1;
   struct token *saved_whsp;
   char *fname;
   char *s;
   int line;
   int nxt_line;
   int trail_nl;  /* flag: trailing character in output is a new-line */
   int  blank_ln;  /* flag: output ends with blank line */

   fname = "";
   line = -1;

   /*
    * Suppress an initial new-line in the output.
    */
   trail_nl = 1;
   blank_ln = 1;

   while ((t = preproc()) != NULL) {
      if (t->flag & LineChk) {
         /*
          * This token is significant with respect to outputting #line
          *  directives.
          */
         nxt_line = t->line;
         if (fname != t->fname  || line != nxt_line) {
            /*
             * We need a #line directive. Make sure it is preceeded by a
             *  blank line.
             */
            if (!trail_nl) {
               putc('\n', out_file);
               ++line;
               trail_nl = 1;
               }
            if (!blank_ln && (line != nxt_line || fname != t->fname)) {
               putc('\n', out_file);
               ++line;
               blank_ln = 1;
               }
            /*
             * Eliminate extra new-lines from the subsequent text before
             *  inserting line directive. This make the output look better.
             *  The line number for the directive will change if new-lines
             *  are eliminated.
             */
            saved_whsp = NULL;
            s = t->image;
            while (t->tok_id == WhiteSpace && (*s == ' ' || *s == '\n' ||
                 *s == '\t')) {
               if (*s == '\n') {
                  /*
                   * Discard any white space before the new-line and update
                   *  the line number.
                   */
                  free_t(saved_whsp);
                  saved_whsp = NULL;
                  t->image = s + 1;
                  ++t->line;
                  ++nxt_line;
                  }
               ++s;
               if (*s == '\0') {
                  /*
                   * The end of the current white space token has been
                   *  reached, see if the next token is also white space.
                   */
                  free_t(saved_whsp);
                  t1 = preproc();
                  if (t1 == NULL) {
                     /*
                      * We are at the end of the input. Don't output
                      *  a #line directive, just make sure the output
                      *  ends with a new-line.
                      */
                     free_t(t);
                     if (!trail_nl)
                        putc('\n', out_file);
                     return;
                     }
                  /*
                   * The previous token may contain non-new-line white
                   *  space, if the new token is on the same line, save
                   *  that previous token in case we want to print the
                   *  white space (this will correctly indent the new
                   *  token).
                   */
                  if (*(t->image) != '\0' && t->line == t1->line &&
                       t->fname == t1->fname)
                     saved_whsp = t;
                  else {
                     free_t(t);
                     saved_whsp = NULL;
                     }
                  t = t1;
                  s = t->image;
                  nxt_line = t->line;
                  }
               }
            if (line_cntrl) {
               /*
                * We are supposed to insert #line directives where needed.
                *  However, one or two blank lines look better when they
                *  are enough to reestablish the correct line number.
                */
               if (fname != t->fname  || line > nxt_line ||
                    line + 2 < nxt_line) {
                  /*
                   * Normally a blank line is put after the #line
                   *  directive; However, this requires decrementing
                   *  the line number and a line number of 0 is not
                   *  valid.
                   */
                  if (nxt_line > 1)
                     fprintf(out_file, "#line %d \"", nxt_line - 1);
                  else
                     fprintf(out_file, "#line %d \"", nxt_line);
                  for (s = t->fname; *s != '\0'; ++s) {
                     if (*s == '"' || *s == '\\')
                        putc('\\',out_file);
                     putc(*s, out_file);
                     }
                  fprintf(out_file, "\"\n");
                  if (nxt_line > 1)
                     fprintf(out_file, "\n"); /* blank line after directive */
                  trail_nl = 1;
                  blank_ln = 1;
                  }
               else  /* adjust line number with blank lines */
                  while (line < nxt_line) {
                     putc('\n', out_file);
                     ++line;
                     if (trail_nl)
                        blank_ln = 1;
                     trail_nl = 1;
                     }
               }
            /*
             * See if we need to indent the next token with white space
             *  saved while eliminating extra new-lines.
             */
            if (saved_whsp != NULL) {
               fprintf(out_file, "%s", saved_whsp->image);
               free_t(saved_whsp);
               if (trail_nl) {
                  blank_ln = 1;
                  trail_nl = 0;
                  }
               }
            line = t->line;
            fname = t->fname;
            }
         }

      /*
       * Print the image of the token.
       */
      if (t->tok_id == WhiteSpace) {
         /*
          * Keep track of trailing blank lines and new-lines. This
          *  information is used to make the insertion of #line
          *  directives more intelligent and to insure that the output
          *  file ends with a new-line.
          */
         for (s = t->image; *s != '\0'; ++s) {
            putc(*s, out_file);
            switch (*s) {
               case '\n':
                  if (trail_nl)
                     blank_ln = 1;
                  trail_nl = 1;
                  ++line;
                  break;

               case ' ':
               case '\t':
                  if (trail_nl)
                     blank_ln = 1;
                  trail_nl = 0;
                  break;

               default:
                  trail_nl = 0;
               }
            }
         }
      else {
         /*
          * Add delimiters to string and character literals.
          */
         switch (t->tok_id) {
            case StrLit:
               fprintf(out_file, "\"%s\"", t->image);
               break;
            case LStrLit:
               fprintf(out_file, "L\"%s\"", t->image);
               break;
            case CharConst:
               fprintf(out_file, "'%s'", t->image);
               break;
            case LCharConst:
               fprintf(out_file, "L'%s'", t->image);
               break;
            default:
               fprintf(out_file, "%s", t->image);
            }
         trail_nl = 0;
         blank_ln = 0;
         }
      free_t(t);
      }

   /*
    * Make sure output file ends with a new-line.
    */
   if (!trail_nl)
      putc('\n', out_file);
   }
