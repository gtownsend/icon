/*
 * This file contains functions to evaluate constant expressions for
 *  conditional inclusion. These functions are organized as a recursive
 *  decent parser based on the C grammar presented in the ANSI C Standard
 *  document. The function eval() is called from the outside.
 */

#include "../preproc/preproc.h"
#include "../preproc/ptoken.h"

/*
 * Prototypes for static functions.
 */
static long primary        (struct token **tp, struct token *trigger);
static long unary          (struct token **tp, struct token *trigger);
static long multiplicative (struct token **tp, struct token *trigger);
static long additive       (struct token **tp, struct token *trigger);
static long shift          (struct token **tp, struct token *trigger);
static long relation       (struct token **tp, struct token *trigger);
static long equality       (struct token **tp, struct token *trigger);
static long and            (struct token **tp, struct token *trigger);
static long excl_or        (struct token **tp, struct token *trigger);
static long incl_or        (struct token **tp, struct token *trigger);
static long log_and        (struct token **tp, struct token *trigger);
static long log_or         (struct token **tp, struct token *trigger);

#include "../preproc/pproto.h"

/*
 * <primary> ::= <identifier>
 *               defined <identifier>
 *               defined '(' <identifier> ')'
 *               <number>
 *               <character-constant>
 *               '(' <conditional> ')'
 */
static long primary(tp, trigger)
struct token **tp;
struct token *trigger;
   {
   struct token *t = NULL;
   struct token *id = NULL;
   long e1;
   int i;
   int is_hex_char;
   char *s;

   switch ((*tp)->tok_id) {
      case Identifier:
         /*
          * Check for "defined", it is the only reserved word in this expression
          *  evaluation (See ANSI C Standard).
          */
         if (strcmp((*tp)->image, "defined") == 0) {
            nxt_non_wh(&t);
            if (t->tok_id == '(') {
               nxt_non_wh(&id);
               nxt_non_wh(&t);
               if (t == NULL || t->tok_id != ')')
                  errt1(id, "')' missing in 'defined' expression");
               free_t(t);
               }
            else
               id = t;
            if (id->tok_id != Identifier)
               errt1(id, "'defined' must be followed by an identifier");
            advance_tok(tp);
            if (m_lookup(id) == NULL)
               e1 = 0L;
            else
               e1 = 1L;
            free_t(id);
            }
         else {
            advance_tok(tp);
            e1 = 0L;   /* undefined: all macros have been expanded */
            }
         return e1;

      case PpNumber:
         s = (*tp)->image;
         e1 = 0L;
         if (*s == '0') {
            ++s;
            if ((*s == 'x') || (*s == 'X')) {
               /*
                * Hex constant
                */
               ++s;
               if (*s == '\0' || *s == 'u' || *s == 'U' || *s == 'l' ||
                    *s == 'L')
                  errt2(*tp, "invalid hex constant in condition of #",
                     trigger->image);
               while (*s != '\0' && *s != 'u' && *s != 'U' && *s != 'l' &&
                    *s != 'L') {
                  e1 <<= 4;
                  if (*s >= '0' && *s <= '9')
                     e1 |= *s - '0';
                  else
                     switch (*s) {
                        case 'a': case 'A': e1 |= 10; break;
                        case 'b': case 'B': e1 |= 11; break;
                        case 'c': case 'C': e1 |= 12; break;
                        case 'd': case 'D': e1 |= 13; break;
                        case 'e': case 'E': e1 |= 14; break;
                        case 'f': case 'F': e1 |= 15; break;
                        default:
                           errt2(*tp, "invalid hex constant in condition of #",
                              trigger->image);
                        }
                  ++s;
                  }
               }
            else {
               /*
                * Octal constant
                */
               while (*s != '\0' && *s != 'u' && *s != 'U' && *s != 'l' &&
                    *s != 'L') {
                  if (*s >= '0' && *s <= '7')
                     e1 = (e1 << 3) | (*s - '0');
                  else
                     errt2(*tp, "invalid octal constant in condition of #",
                        trigger->image);
                  ++s;
                  }
               }
            }
         else {
            /*
             * Decimal constant
             */
            while (*s != '\0' && *s != 'u' && *s != 'U' && *s != 'l' &&
                 *s != 'L') {
               if (*s >= '0' && *s <= '9')
                  e1 = e1 * 10 + (*s - '0');
               else
                  errt2(*tp, "invalid decimal constant in condition of #",
                     trigger->image);
               ++s;
               }
            }
            advance_tok(tp);
            /*
             * Check integer suffix for validity
             */
            if (*s == '\0')
               return e1;
            else if (*s == 'u' || *s == 'U') {
               ++s;
               if (*s == '\0')
                  return e1;
               else if ((*s == 'l' || *s == 'L') && *++s == '\0')
                  return e1;
               }
            else if (*s == 'l' || *s == 'L') {
               ++s;
               if (*s == '\0')
                  return e1;
               else if ((*s == 'u' || *s == 'U') && *++s == '\0')
                  return e1;
               }
            errt2(*tp, "invalid integer constant in condition of #",
               trigger->image);

      case CharConst:
      case LCharConst:
         /*
          * Wide characters are treated the same as characters. Only the
          *  first byte of a multi-byte character is used.
          */
         s = (*tp)->image;
         if (*s != '\\')
            e1 = (long)*s;
         else {
            /*
             * Escape sequence.
             */
            e1 = 0L;
            ++s;
            if (*s >= '0' && *s <= '7') {
                for (i = 1; i <= 3 && *s >= '0' && *s <= '7'; ++i, ++s)
                   e1 = (e1 << 3) | (*s - '0');
                if (e1 != (long)(unsigned char)e1)
                   errt1(*tp, "octal escape sequece larger than a character");
                e1 = (long)(char)e1;
                }
            else switch (*s) {
               case '\'': e1 = (long) '\''; break;
               case '"':  e1 = (long) '"';  break;
               case '?':  e1 = (long) '?';  break;
               case '\\': e1 = (long) '\\'; break;
               case 'a':  e1 = (long) Bell; break;
               case 'b':  e1 = (long) '\b'; break;
               case 'f':  e1 = (long) '\f'; break;
               case 'n':  e1 = (long) '\n'; break;
               case 'r':  e1 = (long) '\r'; break;
               case 't':  e1 = (long) '\t'; break;
               case 'v':  e1 = (long) '\v'; break;

               case 'x':
                  ++s;
                  is_hex_char = 1;
                  while (is_hex_char) {
                     if (*s >= '0' && *s <= '9')
                        e1 = (e1 << 4) | (*s - '0');
                     else switch (*s) {
                        case 'a': case 'A': e1 = (e1 << 4) | 10; break;
                        case 'b': case 'B': e1 = (e1 << 4) | 11; break;
                        case 'c': case 'C': e1 = (e1 << 4) | 12; break;
                        case 'd': case 'D': e1 = (e1 << 4) | 13; break;
                        case 'e': case 'E': e1 = (e1 << 4) | 14; break;
                        case 'f': case 'F': e1 = (e1 << 4) | 15; break;
                        default: is_hex_char = 0;
                        }
                     if (is_hex_char)
                        ++s;
                     if (e1 != (long)(unsigned char)e1)
                        errt1(*tp,"hex escape sequece larger than a character");
                     }
                  e1 = (long)(char)e1;
                  break;

               default:
                  e1 = (long) *s;
               }
            }
         advance_tok(tp);
         return e1;

      case '(':
         advance_tok(tp);
         e1 = conditional(tp, trigger);
         if ((*tp)->tok_id != ')')
            errt2(*tp, "expected ')' in conditional of #", trigger->image);
         advance_tok(tp);
         return e1;

      default:
         errt2(*tp, "syntax error in condition of #", trigger->image);
      }
   /*NOTREACHED*/
   return 0;  /* avoid gcc warning */
   }

/*
 * <unary> ::= <primary> |
 *             '+' <unary> |
 *             '-' <unary> |
 *             '~' <unary> |
 *             '!' <unary>
 */
static long unary(tp, trigger)
struct token **tp;
struct token *trigger;
   {
   switch ((*tp)->tok_id) {
      case '+':
         advance_tok(tp);
         return unary(tp, trigger);
      case '-':
         advance_tok(tp);
         return -unary(tp, trigger);
      case '~':
         advance_tok(tp);
         return ~unary(tp, trigger);
      case '!':
         advance_tok(tp);
         return !unary(tp, trigger);
      default:
         return primary(tp, trigger);
      }
   }

/*
 * <multiplicative> ::= <unary> |
 *                      <multiplicative> '*' <unary> |
 *                      <multiplicative> '/' <unary> |
 *                      <multiplicative> '%' <unary>
 */
static long multiplicative(tp, trigger)
struct token **tp;
struct token *trigger;
   {
   long e1, e2;
   int tok_id;

   e1 = unary(tp, trigger);
   tok_id = (*tp)->tok_id;
   while (tok_id == '*' || tok_id == '/' || tok_id == '%') {
      advance_tok(tp);
      e2 = unary(tp, trigger);
      switch (tok_id) {
         case '*':
            e1 = (e1 * e2);
            break;
         case '/':
            e1 = (e1 / e2);
            break;
         case '%':
            e1 = (e1 % e2);
            break;
         }
      tok_id = (*tp)->tok_id;
      }
   return e1;
   }

/*
 * <additive> ::= <multiplicative> |
 *                <additive> '+' <multiplicative> |
 *                <additive> '-' <multiplicative>
 */
static long additive(tp, trigger)
struct token **tp;
struct token *trigger;
   {
   long e1, e2;
   int tok_id;

   e1 = multiplicative(tp, trigger);
   tok_id = (*tp)->tok_id;
   while (tok_id == '+' || tok_id == '-') {
      advance_tok(tp);
      e2 = multiplicative(tp, trigger);
      if (tok_id == '+')
         e1 = (e1 + e2);
      else
         e1 = (e1 - e2);
      tok_id = (*tp)->tok_id;
      }
   return e1;
   }

/*
 * <shift> ::= <additive> |
 *             <shift> '<<' <additive> |
 *             <shift> '>>' <additive>
 */
static long shift(tp, trigger)
struct token **tp;
struct token *trigger;
   {
   long e1, e2;
   int tok_id;

   e1 = additive(tp, trigger);
   tok_id = (*tp)->tok_id;
   while (tok_id == LShft || tok_id == RShft) {
      advance_tok(tp);
      e2 = additive(tp, trigger);
      if (tok_id == LShft)
         e1 = (e1 << e2);
      else
         e1 = (e1 >> e2);
      tok_id = (*tp)->tok_id;
      }
   return e1;
   }

/*
 * <relation> ::= <shift> |
 *                <relation> '<' <shift> |
 *                <relation> '<=' <shift> |
 *                <relation> '>' <shift> |
 *                <relation> '>=' <shift>
 */
static long relation(tp, trigger)
struct token **tp;
struct token *trigger;
   {
   long e1, e2;
   int tok_id;

   e1 = shift(tp, trigger);
   tok_id = (*tp)->tok_id;
   while (tok_id == '<' || tok_id == Leq || tok_id == '>' || tok_id == Geq) {
      advance_tok(tp);
      e2 = shift(tp, trigger);
      switch (tok_id) {
         case '<':
            e1 = (e1 < e2);
            break;
         case Leq:
            e1 = (e1 <= e2);
            break;
         case '>':
            e1 = (e1 > e2);
            break;
         case Geq:
            e1 = (e1 >= e2);
            break;
         }
      tok_id = (*tp)->tok_id;
      }
   return e1;
   }

/*
 * <equality> ::= <relation> |
 *                <equality> '==' <relation> |
 *                <equality> '!=' <relation>
 */
static long equality(tp, trigger)
struct token **tp;
struct token *trigger;
   {
   long e1, e2;
   int tok_id;

   e1 = relation(tp, trigger);
   tok_id = (*tp)->tok_id;
   while (tok_id == TokEqual || tok_id == Neq) {
      advance_tok(tp);
      e2 = relation(tp, trigger);
      if (tok_id == TokEqual)
         e1 = (e1 == e2);
      else
         e1 = (e1 != e2);
      tok_id = (*tp)->tok_id;
      }
   return e1;
   }

/*
 * <and> ::= <equality> |
 *           <and> '&' <equality>
 */
static long and(tp, trigger)
struct token **tp;
struct token *trigger;
   {
   long e1, e2;

   e1 = equality(tp, trigger);
   while ((*tp)->tok_id == '&') {
      advance_tok(tp);
      e2 = equality(tp, trigger);
      e1 = (e1 & e2);
      }
   return e1;
   }

/*
 * <excl_or> ::= <and> |
 *               <excl_or> '^' <and>
 */
static long excl_or(tp, trigger)
struct token **tp;
struct token *trigger;
   {
   long e1, e2;

   e1 = and(tp, trigger);
   while ((*tp)->tok_id == '^') {
      advance_tok(tp);
      e2 = and(tp, trigger);
      e1 = (e1 ^ e2);
      }
   return e1;
   }

/*
 * <incl_or> ::= <excl_or> |
 *               <incl_or> '|' <excl_or>
 */
static long incl_or(tp, trigger)
struct token **tp;
struct token *trigger;
   {
   long e1, e2;

   e1 = excl_or(tp, trigger);
   while ((*tp)->tok_id == '|') {
      advance_tok(tp);
      e2 = excl_or(tp, trigger);
      e1 = (e1 | e2);
      }
   return e1;
   }

/*
 * <log_and> ::= <incl_or> |
 *               <log_and> '&&' <incl_or>
 */
static long log_and(tp, trigger)
struct token **tp;
struct token *trigger;
   {
   long e1, e2;

   e1 = incl_or(tp, trigger);
   while ((*tp)->tok_id == And) {
      advance_tok(tp);
      e2 = incl_or(tp, trigger);
      e1 = (e1 && e2);
      }
   return e1;
   }

/*
 * <log_or> ::= <log_and> |
 *              <log_or> '||' <log_and>
 */
static long log_or(tp, trigger)
struct token **tp;
struct token *trigger;
   {
   long e1, e2;

   e1 = log_and(tp, trigger);
   while ((*tp)->tok_id == Or) {
      advance_tok(tp);
      e2 = log_and(tp, trigger);
      e1 = (e1 || e2);
      }
   return e1;
   }

/*
 * <conditional> ::= <log_or> |
 *                   <log_or> '?' <conditional> ':' <conditional>
 */
long conditional(tp, trigger)
struct token **tp;
struct token *trigger;
   {
   long e1, e2, e3;

   e1 = log_or(tp, trigger);
   if ((*tp)->tok_id == '?') {
      advance_tok(tp);
      e2 = conditional(tp, trigger);
      if ((*tp)->tok_id != ':')
         errt2(*tp, "expected ':' in conditional of #", trigger->image);
      advance_tok(tp);
      e3 = conditional(tp, trigger);
      return e1 ? e2 : e3;
      }
   else
      return e1;
   }

/*
 * eval - get the tokens for a conditional and evaluate it to 0 or 1.
 *  trigger is the preprocessing directive that triggered the evaluation;
 *  it is used for error messages.
 */
int eval(trigger)
struct token *trigger;
   {
   struct token *t = NULL;
   int result;

   advance_tok(&t);
   result = (conditional(&t, trigger) != 0L);
   if (t->tok_id != PpDirEnd)
      errt2(t, "expected end of condition of #", trigger->image);
   free_t(t);
   return result;
   }
