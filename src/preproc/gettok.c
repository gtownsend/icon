/*
 * This files contains routines for getting the "next" token.
 */
#include "../preproc/preproc.h"
#include "../preproc/ptoken.h"
#include "../preproc/pproto.h"

/*
 * next_tok - get the next raw token. No macros are expanded here (although
 *  the tokens themselves may be the result of a macro expansion initiated
 *  at a "higher" level). Only #line directives are processed here.
 */
struct token *next_tok()
   {
   struct token *t, *t1;
   struct tok_lst *tlst;
   struct char_src *cs;
   struct str_buf *sbuf;
   char *s;
   char *fname;
   int n;

   if (src_stack->flag == DummySrc)
      return NULL;    /* source stack is empty - end of input */

   /*
    * See if a directive pushed back any tokens.
    */
   if (src_stack->ntoks > 0)
      return src_stack->toks[--src_stack->ntoks];

   switch (src_stack->flag) {
      case CharSrc:
         /*
          * Tokens from a raw character "stream".
          */
         t = tokenize();
         if (t != NULL && src_stack->u.cs->f != NULL)
            t->flag |= LineChk;
         if (t != NULL && t->tok_id == PpLine) {
            /*
             * #line directives must be processed here so they are not
             *  put in macros.
             */
            cs = src_stack->u.cs;
            t1 = NULL;

            /*
             * Get the line number from the directive.
             */
            advance_tok(&t1);
            if (t1->tok_id != PpNumber)
               errt1(t1, "#line requires an integer argument");
            n = 0;
            for (s = t1->image; *s != '\0'; ++s) {
               if (*s >= '0' && *s <= '9')
                  n = 10 * n + (*s - '0');
               else
                  errt1(t1, "#line requires an integer argument");
               }

            /*
             * Get the file name, if there is one, from the directive.
             */
            advance_tok(&t1);
            fname = NULL;
            if (t1->tok_id == StrLit) {
               sbuf = get_sbuf();
               for (s = t1->image; *s != '\0'; ++s) {
                  if (s[0] == '\\' && (s[1] == '\\' || s[1] == '"'))
                     ++s;
                  AppChar(*sbuf, *s);
                  }
               fname = str_install(sbuf);
               rel_sbuf(sbuf);
               advance_tok(&t1);
               }
            if (t1->tok_id != PpDirEnd)
               errt1(t1, "syntax error in #line");

            /*
             * Note the effect of the line directive in the character
             *  source. Line number changes are handled as a relative
             *  adjustments to the line numbers of following lines.
             */
            if (fname != NULL)
               cs->fname = fname;
            cs->line_adj = n - cs->line_buf[next_char - first_char + 1];
            if (*next_char == '\n')
               ++cs->line_adj;  /* the next lines contains no characters */

            t = next_tok();     /* the caller does not see #line directives */
            }
         break;

      case MacExpand:
         /*
          * Tokens from macro expansion.
          */
         t = mac_tok();
         break;

      case TokLst:
         /*
          * Tokens from a macro argument.
          */
         tlst = src_stack->u.tlst;
         if (tlst == NULL)
            t = NULL;
         else {
            t = copy_t(tlst->t);
            src_stack->u.tlst = tlst->next;
            }
         break;

      case PasteLsts:
         /*
          * Tokens from token Pasting.
          */
         return paste();
      }

   if (t == NULL) {
      /*
       * We have exhausted this entry on the source stack without finding
       *  a token to return.
       */
      pop_src();
      return next_tok();
      }
   else
      return t;
   }

/*
 * Get the next raw non-white space token, freeing token that the argument
 *  used to point to.
 */
void nxt_non_wh(tp)
struct token **tp;
   {
   register struct token *t;

   t = next_tok();
   while (t != NULL && t->tok_id == WhiteSpace) {
      free_t(t);
      t = next_tok();
      }
   free_t(*tp);
   *tp = t;
   }

/*
 * advance_tok - skip past white space after expanding macros and
 *  executing preprocessor directives. This routine may only be
 *  called from within a preprocessor directive because it assumes
 *  it will not see EOF (the input routines insure that a terminating
 *  new-line, and thus, for a directive, the PpDirEnd token, will be
 *  seen immediately before EOF).
 */
void advance_tok(tp)
struct token **tp;
   {
   struct token *t;

   t = interp_dir();
   while (t->tok_id == WhiteSpace) {
      free_t(t);
      t = interp_dir();
      }
   free_t(*tp);
   *tp = t;
   }

/*
 * merge_whsp - merge a sequence of white space tokens into one token,
 *  returning it along with the next token. Whether these are raw or
 *  processed tokens depends on the token source function, t_src.
 */
void merge_whsp(whsp, next_t, t_src)
struct token **whsp;
struct token **next_t;
struct token *(*t_src)(void);
   {
   struct token *t1;
   struct str_buf *sbuf;
   int line = -1;
   char *fname = "";
   char *s;

   free_t(*whsp);
   t1 = (*t_src)();
   if (t1 == NULL || t1->tok_id != WhiteSpace)
      *whsp  = NULL;   /* no white space here */
   else {
      *whsp = t1;
      t1 = (*t_src)();
      if (t1 != NULL && t1->tok_id == WhiteSpace) {
         if (whsp_image == NoSpelling) {
            /*
             * We don't care what the white space looks like, so
             *  discard the rest of it.
             */
            while (t1 != NULL && t1->tok_id == WhiteSpace) {
               free_t(t1);
               t1 = (*t_src)();
               }
            }
         else {
            /*
             * Must actually merge white space. Put it all white space
             *  in a string buffer and use that as the image of the merged
             *  token. The line number and file name of the new token
             *  is that of the last token whose line number and file
             *  name is important for generating #line directives in
             *  the output.
             */
            sbuf = get_sbuf();
            if ((*whsp)->flag & LineChk) {
               line = (*whsp)->line;
               fname = (*whsp)->fname;
               }
            for (s = (*whsp)->image; *s != '\0'; ++s) {
               AppChar(*sbuf, *s);
               if (*s == '\n' && line != -1)
                  ++line;
               }
            while (t1 != NULL && t1->tok_id == WhiteSpace) {
               if (t1->flag & LineChk) {
                  line = t1->line;
                  fname = t1->fname;
                  }
               for (s = t1->image; *s != '\0'; ++s) {
                  AppChar(*sbuf, *s);
                  if (*s == '\n' && line != -1)
                     ++line;
                  }
               free_t(t1);
               t1 = (*t_src)();
               }
            (*whsp)->image = str_install(sbuf);
            rel_sbuf(sbuf);
            if (t1 != NULL && !(t1->flag & LineChk) && line != -1) {
               t1->flag |= LineChk;
               t1->line = line;
               t1->fname = fname;
               }
            }
         }
      }
   *next_t = t1;
   }
