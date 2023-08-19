#include "rtt.h"

int n_tmp_str  = 0;
int n_tmp_cset = 0;
struct sym_entry *params = NULL;

/*
 * clr_def - clear any information related to definitions.
 */
void clr_def()
   {
   struct sym_entry *sym;

   n_tmp_str = 0;
   n_tmp_cset = 0;
   while (params != NULL) {
      sym = params;
      params = params->u.param_info.next;
      free_sym(sym);
      }
   free_tend();
   if (v_len != NULL)
      free_sym(v_len);
   v_len = NULL;
   il_indx = 0;
   lbl_num = 0;
   abs_ret = SomeType;
   }

/*
 * ttol - convert a token representing an integer constant into a long
 *  integer value.
 */
long ttol(struct token *t)
{
   register long i;
   register char *s;
   int base;

   s = t->image;
   i = 0;
   base = 10;

   if (*s == '0') {
      base = 8;
      ++s;
      if (*s == 'x') {
         base = 16;
         ++s;
         }
      }
   while (*s != '\0') {
      i *= base;
      if (*s >= '0' && *s <= '9')
         i += *s++ - '0';
      else if (*s >= 'a' && *s <= 'f')
         i += *s++ - 'a' + 10;
      else if (*s >= 'A' && *s <= 'F')
         i += *s++ - 'A' + 10;
      }
   return i;
   }

struct token *chk_exct(struct token *tok)
   {
   struct sym_entry *sym;

   sym = sym_lkup(tok->image);
   if (sym->u.typ_indx != int_typ)
      errt2(tok, "exact conversions do not apply  to ", tok->image);
   return tok;
   }

/*
 * icn_typ - convert a type node into a type code for the internal
 *   representation of the data base.
 */
int icn_typ(struct node *typ)
   {
   switch (typ->nd_id) {
      case PrimryNd:
         switch (typ->tok->tok_id) {
            case Any_value:
               return TypAny;
            case Empty_type:
               return TypEmpty;
            case Variable:
               return TypVar;
            case C_Integer:
               return TypCInt;
            case C_Double:
               return TypCDbl;
            case C_String:
               return TypCStr;
            case Tmp_string:
               return TypTStr;
            case Tmp_cset:
               return TypTCset;
            }

      case SymNd:
         return typ->u[0].sym->u.typ_indx;

      default:  /* must be exact conversion */
         if (typ->tok->tok_id == C_Integer)
            return TypECInt;
         else     /* integer */
            return TypEInt;
      }
   }

