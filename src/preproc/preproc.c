/*
 * The functions in this file handle preprocessing directives, macro
 *  calls, and string concatenation.
 */
#include "../preproc/preproc.h"
#include "../preproc/ptoken.h"
#include "../preproc/pproto.h"

/*
 * Prototypes for static functions.
 */
static void start_select (struct token *t);
static void end_select   (struct token *t);
static void incl_file    (struct token *t);
static void define       (struct token *t);
static int     expand       (struct token *t, struct macro *m);
static void toks_to_str  (struct str_buf *sbuf, struct token *t);

/*
 * start_select - handle #if, #ifdef, #ifndef
 */
static void start_select(t)
struct token *t;
   {
   struct token *t1;
   struct tok_lst *tlst;
   int condition;
   int nesting;

   /*
    * determine if condition is true.
    */
   if (t->tok_id == PpIf)
      condition = eval(t);  /* #if - evaluate expression */
   else {
      /*
       * #ifdef or #ifndef - see if an identifier is defined.
       */
      t1 = NULL;
      nxt_non_wh(&t1);
      if (t1->tok_id != Identifier)
         errt2(t1, "identifier must follow #", t->image);
      condition = (m_lookup(t1) == NULL) ? 0 : 1;
      if (t->tok_id == PpIfndef)
         condition = !condition;
      free_t(t1);
      t1 = next_tok();
      if (t1->tok_id != PpDirEnd)
         errt2(t1, "expecting end of line following argument to #", t->image);
      free_t(t1);
      }

   /*
    * look for the branch of the conditional inclusion to take or #endif.
    */
   nesting = 0;
   while (!condition) {
      t1 = next_tok();
      if (t1 == NULL)
        errt2(t, "no matching #endif for #", t->image);
      switch (t1->tok_id) {
         case PpIf:
         case PpIfdef:
         case PpIfndef:
            /*
             * Nested #if, #ifdef, or #ifndef in a branch of a conditional
             *  that is being discarded. Contunue discarding until the
             *  nesting level returns to 0.
             */
            ++nesting;
            break;

         case PpEndif:
            /*
             * #endif found. See if this is this the end of a nested
             *  conditional or the end of the conditional we are processing.
             */
            if (nesting > 0)
              --nesting;
            else {
               /*
                * Discard any extraneous tokens on the end of the directive.
                */
               while (t->tok_id != PpDirEnd) {
                  free_t(t);
                  t = next_tok();
                  }
               free_t(t);
               free_t(t1);
               return;
               }
            break;

         case PpElif:
            /*
             * #elif found. If this is not a nested conditional, see if
             *   it has a true condition.
             */
            if (nesting == 0) {
               free_t(t);
               t = t1;
               t1 = NULL;
               condition = eval(t);
               }
            break;

         case PpElse:
            /*
             * #else found. If this is not a nested conditional, take
             *  this branch.
             */
            if (nesting == 0) {
               free_t(t);
               t = t1;
               t1 = next_tok();
               /*
                * Discard any extraneous tokens on the end of the directive.
                */
               while (t1->tok_id != PpDirEnd) {
                  free_t(t1);
                  t1 = next_tok();
                  }
               condition = 1;
               }
         }
      free_t(t1);
      }
   tlst = new_t_lst(t);
   tlst->next = src_stack->cond;
   src_stack->cond = tlst;
   }

/*
 * end_select - handle #elif, #else, and #endif
 */
static void end_select(t)
struct token *t;
   {
   struct tok_lst *tlst;
   struct token *t1;
   int nesting;

   /*
    * Make sure we are processing conditional compilation and pop it
    *  from the list of conditional nesting.
    */
   tlst = src_stack->cond;
   if (tlst == NULL)
      errt2(t, "invalid context for #", t->image);
   src_stack->cond = tlst->next;
   tlst->next = NULL;
   free_t_lst(tlst);

   /*
    * We are done with the selected branch for the conditional compilation.
    *  Skip to the matching #endif (if we are not already there). Don't
    *  be confused by nested conditionals.
    */
   nesting = 0;
   t1 = copy_t(t);
   while (t1->tok_id != PpEndif || nesting > 0) {
      switch (t1->tok_id) {
         case PpIf:
         case PpIfdef:
         case PpIfndef:
            ++nesting;
            break;

         case PpEndif:
            --nesting;
         }
      free_t(t1);
      t1 = next_tok();
      if (t1 == NULL)
        errt2(t, "no matching #endif for #", t->image);
      }
   free_t(t);

   /*
    * Discard any extraneous tokens on the end of the #endif directive.
    */
   while (t1->tok_id != PpDirEnd) {
      free_t(t1);
      t1 = next_tok();
      }
   free_t(t1);
   return;
   }

/*
 * incl_file - handle #include
 */
static void incl_file(t)
struct token *t;
   {
   struct token *file_tok, *t1;
   struct str_buf *sbuf;
   char *s;
   char *fname;
   int line;

   file_tok = NULL;
   advance_tok(&file_tok);

   /*
    * Determine what form the head file name takes.
    */
   if (file_tok->tok_id != StrLit && file_tok->tok_id != PpHeader) {
      /*
       * see if macro expansion created a name of the form <...>
       */
      t1 = file_tok;
      s = t1->image;
      fname = t1->fname;
      line = t1->line;
      if (*s != '<')
         errt1(t1, "invalid include file syntax");
      ++s;

      /*
       * Gather into a string buffer the characters from subsequent tokens
       *  until the closing '>' is found, then create a "header" token
       *  from it.
       */
      sbuf = get_sbuf();
      while (*s != '>') {
         while (*s != '\0' && *s != '>')
            AppChar(*sbuf, *s++);
         if (*s == '\0') {
            switch (t1->tok_id) {
               case StrLit:
               case LStrLit:
                  AppChar(*sbuf, '"');
                  break;
               case CharConst:
               case LCharConst:
                  AppChar(*sbuf, '\'');
                  break;
               }
            free_t(t1);
            t1 = interp_dir();
            switch (t1->tok_id) {
               case StrLit:
                  AppChar(*sbuf, '"');
                  break;
               case LStrLit:
                  AppChar(*sbuf, 'L');
                  AppChar(*sbuf, '"');
                  break;
               case CharConst:
                  AppChar(*sbuf, '\'');
                  break;
               case LCharConst:
                  AppChar(*sbuf, 'L');
                  AppChar(*sbuf, '\'');
                  break;
               case PpDirEnd:
                  errt1(t1, "invalid include file syntax");
               }
            if (t1->tok_id == WhiteSpace)
               AppChar(*sbuf, ' ');
            else
               s = t1->image;
            }
         }
      if (*++s != '\0')
         errt1(t1, "invalid include file syntax");
      free_t(t1);
      file_tok = new_token(PpHeader, str_install(sbuf), fname, line);
      rel_sbuf(sbuf);
      }

   t1 = interp_dir();
   if (t1->tok_id != PpDirEnd)
      errt1(t1, "invalid include file syntax");
   free_t(t1);

   /*
    * Add the file to the top of the token source stack.
    */
   if (file_tok->tok_id == StrLit)
      include(t, file_tok->image, 0);
   else
      include(t, file_tok->image, 1);
   free_t(file_tok);
   free_t(t);
   }

/*
 * define - handle #define and #begdef
 */
static void define(t)
struct token *t;
   {
   struct token *mname;   /* name of macro */
   int category;	  /* NoArgs for object-like macro, else number params */
   int multi_line;
   struct id_lst *prmlst; /* parameter list */
   struct tok_lst *body;  /* replacement list */
   struct token *t1;
   struct id_lst **pilst;
   struct tok_lst **ptlst;
   int nesting;

   /*
    * Get the macro name.
    */
   mname = NULL;
   nxt_non_wh(&mname);
   if (mname->tok_id != Identifier)
      errt2(mname, "syntax error in #", t->image);

   /*
    * Determine if this macro takes arguments.
    */
   prmlst = NULL;
   t1 = next_tok();
   if (t1->tok_id == '(') {
      /*
       * function like macro - gather parameter list
       */
      pilst = &prmlst;
      nxt_non_wh(&t1);
      if (t1->tok_id == Identifier) {
         category = 1;
         (*pilst) = new_id_lst(t1->image);
         pilst = &(*pilst)->next;
         nxt_non_wh(&t1);
         while (t1->tok_id == ',') {
            nxt_non_wh(&t1);
            if (t1->tok_id != Identifier)
               errt1(t1, "a parameter to a macro must be an identifier");
            ++category;
            (*pilst) = new_id_lst(t1->image);
            pilst = &(*pilst)->next;
            nxt_non_wh(&t1);
            }
         }
      else
         category = 0;
      if (t1->tok_id != ')')
         errt2(t1, "syntax error in #", t->image);
      free_t(t1);
      t1 = next_tok();
      }
   else
      category = NoArgs; /* object-like macro */

   /*
    * Gather the body of the macro.
    */
   body = NULL;
   ptlst = &body;
   if (t->tok_id == PpDefine) {  /* #define */
      multi_line = 0;
      /*
       * strip leading white space
       */
      while (t1->tok_id == WhiteSpace) {
         free_t(t1);
         t1 = next_tok();
         }

      while (t1->tok_id != PpDirEnd) {
         /*
          * Expansion of this type of macro does not trigger #line directives.
          */
         t1->flag &= ~LineChk;

         (*ptlst) = new_t_lst(t1);
         ptlst = &(*ptlst)->next;
         t1 = next_tok();
         }
      }
   else {
      /*
       *  #begdef
       */
      multi_line = 1;
      if (t1->tok_id != PpDirEnd)
         errt1(t1, "expecting new-line at end of #begdef");
      free_t(t1);

      /*
       * Gather tokens until #enddef. Nested #begdef-#enddefs are put
       *  in this macro and not processed until the macro is expanded.
       */
      nesting = 0;
      t1 = next_tok();
      while (t1 != NULL && (nesting > 0 || t1->tok_id != PpEnddef)) {
         if (t1->tok_id == PpBegdef)
            ++nesting;
         else if (t1->tok_id == PpEnddef)
            --nesting;
         (*ptlst) = new_t_lst(t1);
         ptlst = &(*ptlst)->next;
         t1 = next_tok();
         }
      if (t1 == NULL)
         errt1(t, "unexpected end-of-file in #begdef");
      free_t(t1);
      t1 = next_tok();
      if (t1->tok_id != PpDirEnd)
         errt1(t1, "expecting new-line at end of #enddef");
      }
   free_t(t1);
   free_t(t);

   /*
    * Install the macro in the macro symbol table.
    */
   m_install(mname, category, multi_line, prmlst, body);
   free_t(mname);
   }

/*
 * expand - add expansion of macro to source stack.
 */
static int expand(t, m)
struct token *t;
struct macro *m;
   {
   struct token *t1 = NULL;
   struct token *t2;
   struct token *whsp = NULL;
   union src_ref ref;
   struct tok_lst **args, **exp_args;
   struct tok_lst **tlp, **trail_whsp;
   struct src *stack_sav;
   int nparm;
   int narg;
   int paren_nest;
   int line;
   char *fname;

   ++m->ref_cnt;

   args = NULL;
   exp_args = NULL;
   if (m->category >= 0) {
      /*
       * This macro requires an argument list. Gather it, if there is one.
       */
      nparm = m->category;
      narg = 0;
      merge_whsp(&whsp, &t1, next_tok);
      if (t1 == NULL || t1->tok_id != '(') {
         /*
          * There is no argument list. Do not expand the macro, just push
          *  back the tokens we read ahead.
          */
         if (t1 != NULL)
            src_stack->toks[src_stack->ntoks++] = t1;
         if (whsp != NULL)
            src_stack->toks[src_stack->ntoks++] = whsp;
         --m->ref_cnt;
         return 0;
         }
      free_t(whsp);

      /*
       * See how many arguments we expect.
       */
      if (nparm == 0)
         nxt_non_wh(&t1);
      else {
         /*
          * Allocate an array for both raw and macro-expanded token lists
          *  for the arguments.
          */
         args = alloc(nparm * sizeof(struct tok_lst *));
         exp_args = alloc(nparm * sizeof(struct tok_lst *));

         /*
          * Gather the tokens for each argument.
          */
         paren_nest = 0;
         for ( ; narg < nparm && t1 != NULL && t1->tok_id != ')'; ++narg) {
            /*
             * Strip leading white space from the argument.
             */
            nxt_non_wh(&t1);
            tlp = &args[narg];  /* location of raw token list for this arg */
            *tlp = NULL;
            trail_whsp = NULL;
            /*
             * Gather tokens for this argument.
             */
            while (t1 != NULL && (paren_nest > 0 || (t1->tok_id != ',' &&
                 t1->tok_id != ')'))) {
               if (t1->tok_id == '(')
                  ++paren_nest;
               if (t1->tok_id == ')')
                  --paren_nest;
               t1->flag &= ~LineChk;

               /*
                * Link this token into the list for the argument. If this
                *  might be trailing white space, remember where the pointer
                *  to it is so it can be discarded later.
                */
               *tlp = new_t_lst(t1);
               if (t1->tok_id == WhiteSpace) {
                  if (trail_whsp == NULL)
                     trail_whsp = tlp;
                  }
               else
                  trail_whsp = NULL;
               tlp = &(*tlp)->next;
               t1 = next_tok();
               }
            /*
             * strip trailing white space
             */
            if (trail_whsp != NULL) {
               free_t_lst(*trail_whsp);
               *trail_whsp = NULL;
               }

            /*
             * Create a macro expanded token list for the argument. This is
             *  done by establishing a separate preprocessing context with
             *  a new source stack. The current stack must be be saved and
             *  restored.
             */
            tlp = &exp_args[narg]; /* location of expanded token list for arg */
            *tlp = NULL;
            if (src_stack->flag == CharSrc)
               src_stack->u.cs->next_char = next_char; /* save state */
            stack_sav = src_stack;
            src_stack = &dummy;
            ref.tlst = args[narg];
            push_src(TokLst, &ref); /* initial stack is list of raw tokens */
            /*
             * Get macro expanded tokens.
             */
            for (t2 = interp_dir(); t2 != NULL; t2 = interp_dir()) {
               *tlp = new_t_lst(t2);
               tlp = &(*tlp)->next;
               }
            src_stack = stack_sav;
            if (src_stack->flag == CharSrc) {
               /*
                * Restore global state for tokenizing.
                */
               first_char = src_stack->u.cs->char_buf;
               next_char = src_stack->u.cs->next_char;
               last_char = src_stack->u.cs->last_char;
               }
            }
         }
      if (t1 == NULL)
         errt2(t, "unexpected end-of-file in call to macro ", t->image);
      if (t1->tok_id != ')')
         errt2(t1, "too many arguments for macro call to ", t->image);
      if (narg < nparm)
         errt2(t1, "too few arguments for macro call to ", t->image);
      free_t(t1);
      }

   ++m->recurse;
   ref.me = new_me(m, args, exp_args);
   push_src(MacExpand, &ref);
   /*
    * Don't loose generation of #line directive before regular
    *  macros, if there should be one.
    */
   if (!m->multi_line && (t->flag & LineChk)) {
      line = t->line;
      fname = t->fname;
      t1 = next_tok();
      if (t1 != NULL) {
         if (!(t1->flag & LineChk)) {
            t1->flag |= LineChk;
            t1->line = line;
            t1->fname = fname;
            }
         src_stack->toks[src_stack->ntoks++] = t1;
         }
      }
   return 1;
   }

/*
 * toks_to_str - put in a buffer the string image of tokens up to the end of
 *    of a preprocessor directive.
 */
static void toks_to_str(sbuf, t)
struct str_buf *sbuf;
struct token *t;
   {
   char *s;

   while (t->tok_id != PpDirEnd) {
      if (t->tok_id == WhiteSpace)
         AppChar(*sbuf, ' ');
      else {
         if (t->tok_id == LCharConst || t->tok_id == LStrLit)
            AppChar(*sbuf, 'L');
         if (t->tok_id == CharConst || t->tok_id == LCharConst)
            AppChar(*sbuf, '\'');
         else if (t->tok_id == StrLit || t->tok_id == LStrLit)
            AppChar(*sbuf, '"');
         for (s = t->image; *s != '\0'; ++s)
            AppChar(*sbuf, *s);
         if (t->tok_id == CharConst || t->tok_id == LCharConst)
            AppChar(*sbuf, '\'');
         else if (t->tok_id == StrLit || t->tok_id == LStrLit)
            AppChar(*sbuf, '"');
         }
      free_t(t);
      t = next_tok();
      }
   free_t(t);
   }

/*
 * interp_dir - interpret preprocessing directives and recognize macro calls.
 */
struct token *interp_dir()
   {
   struct token *t, *t1;
   struct macro *m;
   struct str_buf *sbuf;
   char *s;

   /*
    * See if the caller pushed back any tokens
    */
   if (src_stack->ntoks > 0)
      return src_stack->toks[--src_stack->ntoks];

   for (;;) {
      t = next_tok();
      if (t == NULL)
          return NULL;

      switch (t->tok_id) {
         case PpIf:          /* #if */
         case PpIfdef:       /* #ifdef */
         case PpIfndef:      /* #endif */
            start_select(t);
            break;

         case PpElif:        /* #elif */
         case PpElse:        /* #else */
         case PpEndif:       /* #endif */
            end_select(t);
            break;

         case PpInclude:     /* #include */
            incl_file(t);
            break;

         case PpDefine:      /* #define */
         case PpBegdef:      /* #begdef */
            define(t);
            break;

         case PpEnddef:      /* #endif, but we have not seen an #begdef */
            errt1(t, "invalid context for #enddef");

         case PpUndef:       /* #undef */
            /*
             * Get the identifier and delete it from the macro symbol table.
             */
            t1 = NULL;
            nxt_non_wh(&t1);
            if (t1->tok_id != Identifier)
               errt1(t1, "#undef requires an identifier argument");
            m_delete(t1);
            free_t(t1);
            t1 = next_tok();
            if (t1->tok_id != PpDirEnd)
               errt1(t1, "syntax error for #undef");
            free_t(t1);
            free_t(t);
            break;

         case PpLine:        /* #line */
            /* this directive is handled in next_tok() */
            break;

         case PpError:       /* #error */
            /*
             * Create an error message out of the rest of the tokens
             *  in this directive.
             */
            sbuf = get_sbuf();
            t1 = NULL;
            nxt_non_wh(&t1);
            toks_to_str(sbuf, t1);
            errt1(t, str_install(sbuf));
            break;

         case PpPragma:       /* #pramga */
         case PpSkip:
            /*
             * Ignore all pragmas and all non-ANSI directives that need not
             *   be passed to the caller.
             */
            t1 = next_tok();
            while (t1->tok_id != PpDirEnd) {
               free_t(t1);
               t1 = next_tok();
               }
            free_t(t);
            free_t(t1);
            break;

         case PpKeep:
            /*
             * This is a directive special to an application using
             *  this preprocessor. Pass it on to the application.
             */
            sbuf = get_sbuf();
            AppChar(*sbuf, '#');
            for (s = t->image; *s != '\0'; ++s)
               AppChar(*sbuf, *s);
            toks_to_str(sbuf, next_tok());
            t->image = str_install(sbuf);
            rel_sbuf(sbuf);
            return t;

         case PpNull:         /* # */
            free_t(t);
            free_t(next_tok());   /* must be PpDirEnd */
            break;

         default:
            /*
             * This is not a directive, see if it is a macro name.
             */
            if (t->tok_id == Identifier && !(t->flag & NoExpand) &&
                 (m = m_lookup(t)) != NULL) {
               if (max_recurse < 0 || m->recurse < max_recurse) {
                  if (expand(t, m))
                     free_t(t);
                  else
                     return t;
                  }
               else {
                  t->flag |= NoExpand;
                  return t;
                  }
               }
            else
               return t; /* nothing special, just return it */
         }
      }
   }

/*
 * See if compiler used to build the preprocessor recognizes '\a'
 *  as the bell character.
 */

#if '\a' == Bell

   #define TokSrc interp_dir

#else					/* '\a' == Bell */

   #define TokSrc check_bell

   /*
    * fix_bell - replace \a characters which correct octal escape sequences.
    */
   static char *fix_bell(s)
   register char *s;
      {
      struct str_buf *sbuf;

      sbuf = get_sbuf();
      while (*s != '\0') {
         AppChar(*sbuf, *s);
         if (*s == '\\') {
            ++s;
            if (*s == 'a') {
               AppChar(*sbuf, '0' + ((Bell >> 6) & 7));
               AppChar(*sbuf, '0' + ((Bell >> 3) & 7));
               AppChar(*sbuf, '0' + (Bell & 7));
               }
            else
               AppChar(*sbuf, *s);
            }
         ++s;
         }
      s = str_install(sbuf);
      rel_sbuf(sbuf);
      return s;
      }

   /*
    * check_bell - check for \a in character and string constants. This is only
    *  used with compilers which don't give the standard interpretation to \a.
    */
   static struct token *check_bell()
      {
      struct token *t;
      register char *s;

      t = interp_dir();
      if (t == NULL)
         return NULL;
      switch (t->tok_id) {
         case StrLit:
         case LStrLit:
         case CharConst:
         case LCharConst:
            s = t->image;
            while (*s != '\0') {
              if (*s == '\\') {
                 if (*++s == 'a') {
                    /*
                     * There is at least one \a to replace.
                     */
                    t->image = fix_bell(t->image);
                    break;
                    }
                 }
              ++s;
              }
         }
      return t;
      }

#endif					/* '\a' == Bell */

/*
 * preproc - return the next fully preprocessed token.
 */
struct token *preproc()
   {
   struct token *t1, *whsp, *t2, *str;
   struct str_buf *sbuf;
   int i;
   char *escape_seq;
   char *s;
   char hex_char;
   int is_hex_char;

   t1 = TokSrc();
   if (t1 == NULL)
      return NULL;  /* end of file */

   /*
    * Concatenate adjacent strings. There is a potential problem if the
    *  first string ends in a octal or hex constant and the second string
    *  starts with a corresponding digit. For example the strings "\12"
    *  and  "7" should be concatenated to produce the 2 character string
    *  "\0127" not the one character string "\127". When such a situation
    *  arises, the last character of the first string is converted to a
    *  canonical 3-digit octal form.
    */
   if (t1->tok_id == StrLit || t1->tok_id == LStrLit) {
      /*
       * See what the next non-white space token is, but don't discard any
       *  white space yet.
       */
      whsp = NULL;
      merge_whsp(&whsp, &t2, TokSrc);
      if (t2 != NULL && (t2->tok_id == StrLit || t2->tok_id == LStrLit)) {
         /*
          * There are at least two adjacent string literals, concatenate them.
          */
         sbuf = get_sbuf();
         str = copy_t(t1);
         while (t2 != NULL && (t2->tok_id == StrLit || t2->tok_id == LStrLit)) {
            s = t1->image;
            while (*s != '\0') {
               if (*s == '\\') {
                  AppChar(*sbuf, *s);
                  ++s;
                  if (*s == 'x') {
                     /*
                      * Hex escape sequence.
                      */
                     hex_char = 0;
                     escape_seq = s;
                     ++s;
                     is_hex_char = 1;
                     while (is_hex_char) {
                        if (*s >= '0' && *s <= '9')
                           hex_char = (hex_char << 4) | (*s - '0');
                        else switch (*s) {
                           case 'a': case 'A':
                              hex_char = (hex_char << 4) | 10;
                              break;
                           case 'b': case 'B':
                              hex_char = (hex_char << 4) | 11;
                              break;
                           case 'c': case 'C':
                              hex_char = (hex_char << 4) | 12;
                              break;
                           case 'd': case 'D':
                              hex_char = (hex_char << 4) | 13;
                              break;
                           case 'e': case 'E':
                              hex_char = (hex_char << 4) | 14;
                              break;
                           case 'f': case 'F':
                              hex_char = (hex_char << 4) | 15;
                              break;
                           default: is_hex_char = 0;
                           }
                        if (is_hex_char)
                           ++s;
                        }
                     /*
                      * If this escape sequence is at the end of the
                      *  string and the next string starts with a
                      *  hex digit, use the canonical form, otherwise
                      *  use it as is.
                      */
                     if (*s == '\0' && isxdigit(t2->image[0])) {
                        AppChar(*sbuf, ((hex_char >> 6) & 03) + '0');
                        AppChar(*sbuf, ((hex_char >> 3) & 07) + '0');
                        AppChar(*sbuf, (hex_char        & 07) + '0');
                        }
                     else
                        while (escape_seq != s)
                           AppChar(*sbuf, *escape_seq++);
                     }
                  else if (*s >= '0' && *s <= '7') {
                     /*
                      * Octal escape sequence.
                      */
                     escape_seq = s;
                     i = 1;
                     while (i <= 3 && *s >= '0' && *s <= '7') {
                        ++i;
                        ++s;
                        }
                     /*
                      * If this escape sequence is at the end of the
                      *  string and the next string starts with an
                      *  octal digit, extend it to 3 digits, otherwise
                      *  use it as is.
                      */
                     if (*s == '\0' && t2->image[0] >= '0' &&
                           t2->image[0] <= '7' && i <= 3) {
                        AppChar(*sbuf, '0');
                        if (i <= 2)
                           AppChar(*sbuf, '0');
                        }
                     while (escape_seq != s)
                        AppChar(*sbuf, *escape_seq++);
                     }
                  }
               else {
                  /*
                   * Not an escape sequence, just copy the character to the
                   *  buffer.
                   */
                  AppChar(*sbuf, *s);
                  ++s;
                  }
               }
            free_t(t1);
            t1 = t2;

            /*
             * Get the next non-white space token, saving any skipped
             *  white space.
             */
            merge_whsp(&whsp, &t2, TokSrc);
            }

         /*
          * Copy the image of the last token into the buffer, creating
          *  the image for the concatenated token.
          */
         for (s = t1->image; *s != '\0'; ++s)
            AppChar(*sbuf, *s);
         str->image = str_install(sbuf);
         free_t(t1);
         t1 = str;
         rel_sbuf(sbuf);
         }

      /*
       * Push back any look-ahead tokens.
       */
      if (t2 != NULL)
         src_stack->toks[src_stack->ntoks++] = t2;
      if (whsp != NULL)
         src_stack->toks[src_stack->ntoks++] = whsp;
      }
   return t1;
   }
