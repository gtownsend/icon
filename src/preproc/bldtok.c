/*
 * This file contains routines for building tokens out of characters from a
 *  "character source". This source is the top element on the source stack.
 */
#include "../preproc/preproc.h"
#include "../preproc/ptoken.h"

/*
 * Prototypes for static functions.
 */
static int           pp_tok_id  (char *s);
static struct token *chck_wh_sp (struct char_src *cs);
static struct token *pp_number  (void);
static struct token *char_str   (int delim, int tok_id);
static struct token *hdr_tok    (int delim, int tok_id, struct char_src *cs);

int whsp_image = NoSpelling;    /* indicate what is in white space tokens */
struct token *zero_tok;         /* token for literal 0 */
struct token *one_tok;          /* token for literal 1 */

#include "../preproc/pproto.h"

/*
 * IsWhSp(c) - true if c is a white space character.
 */
#define IsWhSp(c) (c == ' ' || c == '\n' || c == '\t' || c == '\v' || c == '\f')

/*
 * AdvChar() - advance to next character from buffer, filling the buffer
 *   if needed.
 */
#define AdvChar() \
   if (++next_char == last_char) \
      fill_cbuf();

static int line;                   /* current line number */
static char *fname;                /* current file name */
static struct str_buf tknize_sbuf; /* string buffer */

/*
 * List of preprocessing directives and the corresponding token ids.
 */
static struct rsrvd_wrd pp_rsrvd[] = {
   PPDirectives
   {"if",      PpIf},
   {"else",    PpElse},
   {"ifdef",   PpIfdef},
   {"ifndef",  PpIfndef},
   {"elif",    PpElif},
   {"endif",   PpEndif},
   {"include", PpInclude},
   {"define",  PpDefine},
   {"undef",   PpUndef},
   {"begdef",  PpBegdef},
   {"enddef",  PpEnddef},
   {"line",    PpLine},
   {"error",   PpError},
   {"pragma",  PpPragma},
   {NULL, Invalid}};

/*
 * init_tok - initialize tokenizer.
 */
void init_tok()
   {
   struct rsrvd_wrd *rw;
   static int first_time = 1;

   if (first_time) {
      first_time = 0;
      init_sbuf(&tknize_sbuf); /* initialize string buffer */
      /*
       * install reserved words into the string table
       */
      for (rw = pp_rsrvd; rw->s != NULL; ++rw)
         rw->s = spec_str(rw->s);

      zero_tok = new_token(PpNumber, spec_str("0"), "", 0);
      one_tok = new_token(PpNumber, spec_str("1"), "", 0);
      }
   }

/*
 * pp_tok_id - see if s in the name of a preprocessing directive.
 */
static int pp_tok_id(s)
char *s;
   {
   struct rsrvd_wrd *rw;

   for (rw = pp_rsrvd; rw->s != NULL && rw->s != s; ++rw)
      ;
   return rw->tok_id;
   }

/*
 * chk_eq_sign - look ahead to next character to see if it is an equal sign.
 *  It is used for processing -D options.
 */
int chk_eq_sign()
   {
   if (*next_char == '=') {
      AdvChar();
      return 1;
      }
   else
      return 0;
   }

/*
 * chck_wh_sp - If the input is at white space, construct a white space token
 *  and return it, otherwise return NULL. This function also helps keeps track
 *  of preprocessor directive boundaries.
 */
static struct token *chck_wh_sp(cs)
struct char_src *cs;
   {
   register int c1, c2;
   struct token *t;
   int tok_id;

   /*
    * See if we are at white space or a comment.
    */
   c1 = *next_char;
   if (!IsWhSp(c1) && (c1 != '/' || next_char[1] != '*'))
      return NULL;

   /*
    * Fine the line number of the current character in the line number
    *  buffer, and correct it if we have encountered any #line directives.
    */
   line = cs->line_buf[next_char - first_char] + cs->line_adj;
   if (c1 == '\n')
      --line;      /* a new-line really belongs to the previous line */

   tok_id = WhiteSpace;
   for (;;) {
      if (IsWhSp(c1)) {
         /*
          * The next character is a white space. If we are retaining the
          *  image of the white space in the token, copy the character to
          *  the string buffer. If we are in the midst of a preprocessor
          *  directive and find a new-line, indicate the end of the
          *  the directive.
          */
         AdvChar();
         if (whsp_image != NoSpelling)
            AppChar(tknize_sbuf, c1);
         if (c1 == '\n') {
            if (cs->dir_state == Within)
               tok_id = PpDirEnd;
            cs->dir_state = CanStart;
            if (tok_id == PpDirEnd)
               break;
            }
         }
      else if (c1 == '/' && next_char[1] == '*') {
         /*
          * Start of comment. If we are retaining the image of comments,
          *  copy the characters into the string buffer.
          */
         if (whsp_image == FullImage) {
            AppChar(tknize_sbuf, '/');
            AppChar(tknize_sbuf, '*');
            }
         AdvChar();
         AdvChar();

         /*
          * Look for the end of the comment.
          */
         c1 = *next_char;
         c2 = next_char[1];
         while (c1 != '*' || c2 != '/') {
            if (c1 == EOF)
                errfl1(fname, line, "eof encountered in comment");
            AdvChar();
            if (whsp_image == FullImage)
               AppChar(tknize_sbuf, c1);
            c1 = c2;
            c2 = next_char[1];
            }

         /*
          * Determine if we are retaining the image of a comment, replacing
          *  a comment by one space character, or ignoring comments.
          */
         if (whsp_image == FullImage) {
            AppChar(tknize_sbuf, '*');
            AppChar(tknize_sbuf, '/');
            }
         else if (whsp_image == NoComment)
            AppChar(tknize_sbuf, ' ');
         AdvChar();
         AdvChar();
         }
      else
         break;         /* end of white space */
      c1 = *next_char;
      }

   /*
    * If we are not retaining the image of white space, replace it all
    *  with one space character.
    */
   if (whsp_image == NoSpelling)
      AppChar(tknize_sbuf, ' ');

   t = new_token(tok_id, str_install(&tknize_sbuf), fname, line);

   /*
    * Look ahead to see if a ## operator is next.
    */
   if (*next_char == '#' && next_char[1] == '#')
      if (tok_id == PpDirEnd)
         errt1(t, "## expressions must not cross directive boundaries");
      else {
         /*
          * Discard white space before a ## operator.
          */
         free_t(t);
         return NULL;
         }
   return t;
   }

/*
 * pp_number - Create a token for a preprocessing number (See ANSI C Standard
 *  for the syntax of such a number).
 */
static struct token *pp_number()
   {
   register int c;

   c = *next_char;
   for (;;) {
      if (c == 'e' || c == 'E') {
         AppChar(tknize_sbuf, c);
         AdvChar();
         c = *next_char;
         if (c == '+' || c == '-') {
            AppChar(tknize_sbuf, c);
            AdvChar();
            c = *next_char;
            }
         }
      else if (isdigit(c) || c == '.' || islower(c) || isupper(c) || c == '_') {
         AppChar(tknize_sbuf, c);
         AdvChar();
         c = *next_char;
         }
      else {
         return new_token(PpNumber, str_install(&tknize_sbuf), fname, line);
         }
      }
   }

/*
 * char_str - construct a token for a character constant or string literal.
 */
static struct token *char_str(delim, tok_id)
int delim;
int tok_id;
   {
   register int c;

   for (c = *next_char; c != EOF && c != '\n' &&  c != delim; c = *next_char) {
      AppChar(tknize_sbuf, c);
      if (c == '\\') {
         c = next_char[1];
         if (c == EOF || c == '\n')
            break;
         else {
            AppChar(tknize_sbuf, c);
            AdvChar();
            }
         }
      AdvChar();
      }
   if (c == EOF)
      errfl1(fname, line, "End-of-file encountered within a literal");
   if (c == '\n')
      errfl1(fname, line, "New-line encountered within a literal");
   AdvChar();
   return new_token(tok_id, str_install(&tknize_sbuf), fname, line);
   }

/*
 * hdr_tok - create a token for an #include header. The delimiter may be
 *  > or ".
 */
static struct token *hdr_tok(delim, tok_id, cs)
int delim;
int tok_id;
struct char_src *cs;
   {
   register int c;

   line = cs->line_buf[next_char - first_char] + cs->line_adj;
   AdvChar();

   for (c = *next_char; c != delim; c = *next_char) {
      if (c == EOF)
         errfl1(fname, line,
            "End-of-file encountered within a header name");
      if (c == '\n')
         errfl1(fname, line,
            "New-line encountered within a header name");
      AppChar(tknize_sbuf, c);
      AdvChar();
      }
   AdvChar();
   return new_token(tok_id, str_install(&tknize_sbuf), fname, line);
   }

/*
 * tokenize - return the next token from the character source on the top
 *  of the source stack.
 */
struct token *tokenize()
   {
   struct char_src *cs;
   struct token *t1, *t2;
   register int c;
   int tok_id;


   cs = src_stack->u.cs;

   /*
    * Check to see if the last call left a token from a look ahead.
    */
   if (cs->tok_sav != NULL) {
      t1 = cs->tok_sav;
      cs->tok_sav = NULL;
      return t1;
      }

   if (*next_char == EOF)
      return NULL;

   /*
    * Find the current line number and file name for the character
    *  source and check for white space.
    */
   line = cs->line_buf[next_char - first_char] + cs->line_adj;
   fname = cs->fname;
   if ((t1 = chck_wh_sp(cs)) != NULL)
       return t1;

   c = *next_char;  /* look at next character */
   AdvChar();

   /*
    * If the last thing we saw in this character source was white space
    *  containing a new-line, then we must look for the start of a
    *  preprocessing directive.
    */
   if (cs->dir_state == CanStart) {
      cs->dir_state = Reset;
      if  (c == '#' && *next_char != '#') {
         /*
          * Assume we are within a preprocessing directive and check
          *  for white space to discard.
          */
         cs->dir_state = Within;
         if ((t1 = chck_wh_sp(cs)) != NULL)
            if (t1->tok_id == PpDirEnd) {
               /*
                * We found a new-line, this is a null preprocessor directive.
                */
               cs->tok_sav = t1;
               AppChar(tknize_sbuf, '#');
               return new_token(PpNull, str_install(&tknize_sbuf), fname, line);
               }
            else
               free_t(t1);  /* discard white space */
         c = *next_char;
         if (islower(c) || isupper(c) || c == '_') {
            /*
             * Tokenize the identifier following the #
             */
            t1 = tokenize();
            if ((tok_id = pp_tok_id(t1->image)) == Invalid) {
               /*
                * We have a stringizing operation, not a preprocessing
                *  directive.
                */
               cs->dir_state = Reset;
               cs->tok_sav = t1;
               AppChar(tknize_sbuf, '#');
               return new_token('#', str_install(&tknize_sbuf), fname, line);
               }
            else {
               t1->tok_id = tok_id;
               if (tok_id == PpInclude) {
                  /*
                   * A header name has to be tokenized specially. Find
                   *  it, then save the token.
                   */
                  if ((t2 = chck_wh_sp(cs)) != NULL)
                     if (t2->tok_id == PpDirEnd)
                        errt1(t2, "file name missing from #include");
                     else
                        free_t(t2);
                  c = *next_char;
                  if (c == '"')
                     cs->tok_sav = hdr_tok('"', StrLit, cs);
                  else if (c == '<')
                     cs->tok_sav = hdr_tok('>', PpHeader, cs);
                  }
               /*
                * Return the token indicating the kind of preprocessor
                *  directive we have started.
                */
               return t1;
               }
            }
         else
            errfl1(fname, line,
               "# must be followed by an identifier or keyword");
         }
      }

   /*
    * Check for literals containing wide characters.
    */
   if (c == 'L') {
      if (*next_char == '\'') {
         AdvChar();
         t1 = char_str('\'', LCharConst);
         if (t1->image[0] == '\0')
            errt1(t1, "invalid character constant");
         return t1;
         }
      else if (*next_char == '"') {
         AdvChar();
         return char_str('"', LStrLit);
         }
      }

   /*
    * Check for identifier.
    */
   if (islower(c) || isupper(c) || c == '_') {
      AppChar(tknize_sbuf, c);
      c = *next_char;
      while (islower(c) || isupper(c) || isdigit(c) || c == '_') {
         AppChar(tknize_sbuf, c);
         AdvChar();
         c = *next_char;
         }
      return new_token(Identifier, str_install(&tknize_sbuf), fname, line);
     }

   /*
    * Check for number.
    */
   if (isdigit(c)) {
      AppChar(tknize_sbuf, c);
      return pp_number();
      }

   /*
    * Check for character constant.
    */
   if (c == '\'') {
      t1 = char_str(c, CharConst);
      if (t1->image[0] == '\0')
         errt1(t1, "invalid character constant");
      return t1;
      }

   /*
    * Check for string constant.
    */
   if (c == '"')
      return char_str(c, StrLit);

   /*
    * Check for operators and punctuation. Anything that does not fit these
    *  categories is a single character token.
    */
   AppChar(tknize_sbuf, c);
   switch (c) {
      case '.':
         c = *next_char;
         if (isdigit(c)) {
            /*
             * Number
             */
            AppChar(tknize_sbuf, c);
            AdvChar();
            return pp_number();
            }
         else if (c == '.' && next_char[1] == '.') {
            /*
             *  ...
             */
            AdvChar();
            AdvChar();
            AppChar(tknize_sbuf, '.');
            AppChar(tknize_sbuf, '.');
            return new_token(Ellipsis, str_install(&tknize_sbuf), fname, line);
            }
         else
            return new_token('.', str_install(&tknize_sbuf), fname, line);

      case '+':
         c = *next_char;
         if (c == '+') {
            /*
             *  ++
             */
            AppChar(tknize_sbuf, '+');
            AdvChar();
            return new_token(Incr, str_install(&tknize_sbuf), fname, line);
            }
         else if (c == '=') {
            /*
             *  +=
             */
            AppChar(tknize_sbuf, '=');
            AdvChar();
            return new_token(PlusAsgn, str_install(&tknize_sbuf), fname, line);
            }
         else
            return new_token('+', str_install(&tknize_sbuf), fname, line);

      case '-':
         c = *next_char;
         if (c == '>') {
            /*
             *  ->
             */
            AppChar(tknize_sbuf, '>');
            AdvChar();
            return new_token(Arrow, str_install(&tknize_sbuf), fname, line);
            }
         else if (c == '-') {
            /*
             *  --
             */
            AppChar(tknize_sbuf, '-');
            AdvChar();
            return new_token(Decr, str_install(&tknize_sbuf), fname, line);
            }
         else if (c == '=') {
            /*
             *  -=
             */
            AppChar(tknize_sbuf, '=');
            AdvChar();
            return new_token(MinusAsgn, str_install(&tknize_sbuf), fname,
               line);
            }
         else
            return new_token('-', str_install(&tknize_sbuf), fname, line);

      case '<':
         c = *next_char;
         if (c == '<') {
            AppChar(tknize_sbuf, '<');
            AdvChar();
            if (*next_char == '=') {
               /*
                *  <<=
                */
               AppChar(tknize_sbuf, '=');
               AdvChar();
               return new_token(LShftAsgn, str_install(&tknize_sbuf), fname,
                  line);
               }
            else
               /*
                *  <<
                */
               return new_token(LShft, str_install(&tknize_sbuf), fname, line);
            }
         else if (c == '=') {
            /*
             *  <=
             */
            AppChar(tknize_sbuf, '=');
            AdvChar();
            return new_token(Leq, str_install(&tknize_sbuf), fname, line);
            }
         else
            return new_token('<', str_install(&tknize_sbuf), fname, line);

      case '>':
         c = *next_char;
         if (c == '>') {
            AppChar(tknize_sbuf, '>');
            AdvChar();
            if (*next_char == '=') {
               /*
                *  >>=
                */
               AppChar(tknize_sbuf, '=');
               AdvChar();
               return new_token(RShftAsgn, str_install(&tknize_sbuf), fname,
                  line);
               }
            else
               /*
                *  >>
                */
               return new_token(RShft, str_install(&tknize_sbuf), fname, line);
            }
         else if (c == '=') {
            /*
             *  >=
             */
            AppChar(tknize_sbuf, '=');
            AdvChar();
            return new_token(Geq, str_install(&tknize_sbuf), fname, line);
            }
         else
            return new_token('>', str_install(&tknize_sbuf), fname, line);

      case '=':
         if (*next_char == '=') {
            /*
             *  ==
             */
            AppChar(tknize_sbuf, '=');
            AdvChar();
            return new_token(TokEqual, str_install(&tknize_sbuf), fname, line);
            }
         else
            return new_token('=', str_install(&tknize_sbuf), fname, line);

      case '!':
         if (*next_char == '=') {
            /*
             *  !=
             */
            AppChar(tknize_sbuf, '=');
            AdvChar();
            return new_token(Neq, str_install(&tknize_sbuf), fname, line);
            }
         else
            return new_token('!', str_install(&tknize_sbuf), fname, line);

      case '&':
         c = *next_char;
         if (c == '&') {
            /*
             *  &&
             */
            AppChar(tknize_sbuf, '&');
            AdvChar();
            return new_token(And, str_install(&tknize_sbuf), fname, line);
            }
         else if (c == '=') {
            /*
             *  &=
             */
            AppChar(tknize_sbuf, '=');
            AdvChar();
            return new_token(AndAsgn, str_install(&tknize_sbuf), fname, line);
            }
         else
            return new_token('&', str_install(&tknize_sbuf), fname, line);

      case '|':
         c = *next_char;
         if (c == '|') {
            /*
             *  ||
             */
            AppChar(tknize_sbuf, '|');
            AdvChar();
            return new_token(Or, str_install(&tknize_sbuf), fname, line);
            }
         else if (c == '=') {
            /*
             *  |=
             */
            AppChar(tknize_sbuf, '=');
            AdvChar();
            return new_token(OrAsgn, str_install(&tknize_sbuf), fname, line);
            }
         else
            return new_token('|', str_install(&tknize_sbuf), fname, line);

      case '*':
         if (*next_char == '=') {
            /*
             *  *=
             */
            AppChar(tknize_sbuf, '=');
            AdvChar();
            return new_token(MultAsgn, str_install(&tknize_sbuf), fname, line);
            }
         else
            return new_token('*', str_install(&tknize_sbuf), fname, line);

      case '/':
         if (*next_char == '=') {
            /*
             *  /=
             */
            AppChar(tknize_sbuf, '=');
            AdvChar();
            return new_token(DivAsgn, str_install(&tknize_sbuf), fname, line);
            }
         else
            return new_token('/', str_install(&tknize_sbuf), fname, line);

      case '%':
         if (*next_char == '=') {
            /*
             *  &=
             */
            AppChar(tknize_sbuf, '=');
            AdvChar();
            return new_token(ModAsgn, str_install(&tknize_sbuf), fname, line);
            }
         else
            return new_token('%', str_install(&tknize_sbuf), fname, line);

      case '^':
         if (*next_char == '=') {
            /*
             *  ^=
             */
            AppChar(tknize_sbuf, '=');
            AdvChar();
            return new_token(XorAsgn, str_install(&tknize_sbuf), fname, line);
            }
         else
            return new_token('^', str_install(&tknize_sbuf), fname, line);

      case '#':
         /*
          * Token pasting or stringizing operator.
          */
         if (*next_char == '#') {
            /*
             *  ##
             */
            AppChar(tknize_sbuf, '#');
            AdvChar();
            t1 =  new_token(PpPaste, str_install(&tknize_sbuf), fname, line);
            }
         else
            t1 = new_token('#', str_install(&tknize_sbuf), fname, line);

         /*
          * The operand must be in the same preprocessing directive.
          */
         if ((t2 = chck_wh_sp(cs)) != NULL)
            if (t2->tok_id == PpDirEnd)
              errt2(t2, t1->image,
               " preprocessing expression must not cross directive boundary");
            else
               free_t(t2);
         return t1;

      default:
         return new_token(c, str_install(&tknize_sbuf), fname, line);
      }
   }
