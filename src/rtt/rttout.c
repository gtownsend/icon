#include "rtt.h"

#define NotId 0  /* declarator is not simple identifier */
#define IsId  1  /* declarator is simple identifier */

#define OrdFunc -1   /* indicates ordinary C function - non-token value */

/*
 * VArgAlwnc - allowance for the variable part of an argument list in the
 *  most general version of an operation. If it is too small, storage must
 *  be malloced. 3 was chosen because over 90 percent of all writes have
 *  3 or fewer arguments. It is possible that 4 would be a better number,
 *  but 5 is probably overkill.
 */
#define VArgAlwnc 3

/*
 * Prototypes for static functions.
 */
static void cnv_fnc       (struct token *t, int typcd,
                               struct node *src, struct node *dflt,
                               struct node *dest, int indent);
static void chk_conj      (struct node *n);
static void chk_nl        (int indent);
static void chk_rsltblk   (int indent);
static void comp_def      (struct node *n);
static int     does_call     (struct node *expr);
static void failure       (int indent, int brace);
static void interp_def    (struct node *n);
static int     len_sel       (struct node *sel,
                               struct parminfo *strt_prms,
                               struct parminfo *end_prms, int indent);
static void line_dir      (int nxt_line, char *new_fname);
static int     only_proto    (struct node *n);
static void parm_locs     (struct sym_entry *op_params);
static void parm_tnd      (struct sym_entry *sym);
static void prt_runerr    (struct token *t, struct node *num,
                               struct node *val, int indent);
static void prt_tok       (struct token *t, int indent);
static void prt_var       (struct node *n, int indent);
static int     real_def      (struct node *n);
static int     retval_dcltor (struct node *dcltor, int indent);
static void ret_value     (struct token *t, struct node *n,
                               int indent);
static void ret_1_arg     (struct token *t, struct node *args,
                               int typcd, char *vwrd_asgn, char *arg_rep,
                               int indent);
static int     rt_walk       (struct node *n, int indent, int brace);
static void spcl_start    (struct sym_entry *op_params);
static int     tdef_or_extr  (struct node *n);
static void tend_ary      (int n);
static void tend_init     (void);
static void tnd_var       (struct sym_entry *sym, char *strct_ptr, char *access, int indent);
static void tok_line      (struct token *t, int indent);
static void typ_asrt      (int typcd, struct node *desc,
                               struct token *tok, int indent);
static int     typ_case      (struct node *var, struct node *slct_lst,
                               struct node *dflt,
                               int (*walk)(struct node *n, int xindent,
                                 int brace), int maybe_var, int indent);
static void untend        (int indent);

extern char *progname;

int op_type = OrdFunc;  /* type of operation */
char lc_letter;         /* f = function, o = operator, k = keyword */
char uc_letter;         /* F = function, O = operator, K = keyword */
char prfx1;             /* 1st char of unique prefix for operation */
char prfx2;             /* 2nd char of unique prefix for operation */
char *fname = "";       /* current source file name */
int line = 0;           /* current source line number */
int nxt_sbuf;           /* next string buffer index */
int nxt_cbuf;           /* next cset buffer index */
int abs_ret = SomeType; /* type from abstract return(s) */

int nl = 0;             /* flag indicating the a new-line should be output */
static int no_nl = 0;   /* flag to suppress line directives */

static int ntend;       /* number of tended descriptor needed */
static char *tendstrct; /* expression to access struct of tended descriptors */
static char *rslt_loc;  /* expression to access result location */
static int varargs = 0; /* flag: operation takes variable number of arguments */

static int no_ret_val;  /* function has return statement with no value */
static struct node *fnc_head; /* header of function being "copied" to output */

/*
 * chk_nl - if a new-line is required, output it and indent the next line.
 */
static void chk_nl(indent)
int indent;
   {
   int col;

   if (nl)  {
      /*
       * new-line required.
       */
      putc('\n', out_file);
      ++line;
      for (col = 0; col < indent; ++col)
         putc(' ', out_file);
      nl = 0;
      }
   }

/*
 * line_dir - Output a line directive.
 */
static void line_dir(nxt_line, new_fname)
int nxt_line;
char *new_fname;
   {
   char *s;

   /*
    * Make sure line directives are desired in the output. Normally,
    *  blank lines surround the directive for readability. However,`
    *  a preceding blank line is suppressed at the beginning of the
    *  output file. In addition, a blank line is suppressed after
    *  the directive if it would force the line number on the directive
    *  to be 0.
    */
   if (line_cntrl) {
      fprintf(out_file, "\n");
      if (line != 0)
         fprintf(out_file, "\n");
      if (nxt_line == 1)
         fprintf(out_file, "#line %d \"", nxt_line);
      else
         fprintf(out_file, "#line %d \"", nxt_line - 1);
      for (s = new_fname; *s != '\0'; ++s) {
         if (*s == '"' || *s == '\\')
            putc('\\', out_file);
         putc(*s, out_file);
         }
      if (nxt_line == 1)
         fprintf(out_file, "\"");
      else
         fprintf(out_file, "\"\n");
      nl = 1;
      --nxt_line;
      }
    else if ((nxt_line > line || fname != new_fname) && line != 0) {
      /*
       * Line directives are disabled, but we are in a situation where
       *  one or two new-lines are desirable.
       */
      if (nxt_line > line + 1 || fname != new_fname)
         fprintf(out_file, "\n");
      nl = 1;
      --nxt_line;
      }
   line = nxt_line;
   fname = new_fname;
   }

/*
 * prt_str - print a string to the output file, possibly preceded by
 *   a new-line and indenting.
 */
void prt_str(s, indent)
char *s;
int indent;
   {
   chk_nl(indent);
   fprintf(out_file, "%s", s);
   }

/*
 * tok_line - determine if a line directive is needed to synchronize the
 *  output file name and line number with an input token.
 */
static void tok_line(t, indent)
struct token *t;
int indent;
   {
   int nxt_line;

   /*
    * Line directives may be suppressed at certain points during code
    *  output. This is done either by rtt itself using the no_nl flag, or
    *  for macros, by the preprocessor using a flag in the token.
    */
   if (no_nl)
      return;
   if (t->flag & LineChk) {
      /*
       * If blank lines can be used in place of a line directive and no
       *  more than 3 are needed, use them. If the line number and file
       *  name are correct, but we need a new-line, we must output a
       *  line directive so the line number is reset after the "new-line".
       */
      nxt_line = t->line;
      if (fname != t->fname  || line > nxt_line || line + 2 < nxt_line)
         line_dir(nxt_line, t->fname);
      else if (nl && line == nxt_line)
         line_dir(nxt_line, t->fname);
      else if (line != nxt_line) {
         nl = 1;
         --nxt_line;
         while (line < nxt_line) { /* above condition limits # interactions */
            putc('\n', out_file);
            ++line;
            }
         }
      }
   chk_nl(indent);
   }

/*
 * prt_tok - print a token.
 */
static void prt_tok(t, indent)
struct token *t;
int indent;
   {
   char *s;

   tok_line(t, indent); /* synchronize file name and line number */

   /*
    * Most tokens contain a string of their exact image. However, string
    *  and character literals lack the surrounding quotes.
    */
   s = t->image;
   switch (t->tok_id) {
      case StrLit:
         fprintf(out_file, "\"%s\"", s);
         break;
      case LStrLit:
         fprintf(out_file, "L\"%s\"", s);
         break;
      case CharConst:
         fprintf(out_file, "'%s'", s);
         break;
      case LCharConst:
         fprintf(out_file, "L'%s'", s);
         break;
      default:
         fprintf(out_file, "%s", s);
      }
   }

/*
 * untend - output code to removed the tended descriptors in this
 *  function from the global tended list.
 */
static void untend(indent)
int indent;
   {
   ForceNl();
   prt_str("tend = ", indent);
   fprintf(out_file, "%s.previous;", tendstrct);
   ForceNl();
   /*
    * For varargs operations, the tended structure might have been
    *  malloced. If so, it must be freed.
    */
   if (varargs) {
      prt_str("if (r_tendp != (struct tend_desc *)&r_tend)", indent);
      ForceNl();
      prt_str("free((pointer)r_tendp);", 2 * indent);
      }
   }

/*
 * tnd_var - output an expression to accessed a tended variable.
 */
static void tnd_var(sym, strct_ptr, access, indent)
struct sym_entry *sym;
char *strct_ptr;
char *access;
int indent;
   {
   /*
    * A variable that is a specific block pointer type must be cast
    *  to that pointer type in such a way that it can be used as either
    *  an lvalue or an rvalue:  *(struct b_??? **)&???.vword.bptr
    */
   if (strct_ptr != NULL) {
      prt_str("(*(struct ", indent);
      prt_str(strct_ptr, indent);
      prt_str("**)&", indent);
      }

   if (sym->id_type & ByRef) {
      /*
       * The tended variable is being accessed indirectly through
       *  a pointer (that is, it is accessed as the argument to a body
       *  function); dereference its identifier.
       */
      prt_str("(*", indent);
      prt_str(sym->image, indent);
      prt_str(")", indent);
      }
   else {
      if (sym->t_indx >= 0) {
         /*
          * The variable is accessed directly as part of the tended structure.
          */
         prt_str(tendstrct, indent);
         fprintf(out_file, ".d[%d]", sym->t_indx);
         }
      else {
         /*
          * This is a direct access to an operation parameter.
          */
         prt_str("r_args[", indent);
         fprintf(out_file, "%d]", sym->u.param_info.param_num + 1);
         }
      }
   prt_str(access, indent);  /* access the vword for tended pointers */
   if (strct_ptr != NULL)
      prt_str(")", indent);
   }

/*
 * prt_var - print a variable.
 */
static void prt_var(n, indent)
struct node *n;
int indent;
   {
   struct token *t;
   struct sym_entry *sym;

   t = n->tok;
   tok_line(t, indent); /* synchronize file name and line nuber */
   sym = n->u[0].sym;
   switch (sym->id_type & ~ByRef) {
      case TndDesc:
         /*
          * Simple tended descriptor.
          */
         tnd_var(sym, NULL, "", indent);
         break;
      case TndStr:
         /*
          * Tended character pointer.
          */
         tnd_var(sym, NULL, ".vword.sptr", indent);
         break;
      case TndBlk:
         /*
          * Tended block pointer.
          */
         tnd_var(sym, sym->u.tnd_var.blk_name, ".vword.bptr",
            indent);
         break;
      case RtParm:
      case DrfPrm:
         switch (sym->u.param_info.cur_loc) {
            case PrmTend:
               /*
                * Simple tended parameter.
                */
               tnd_var(sym, NULL, "", indent);
               break;
            case PrmCStr:
               /*
                * Parameter converted to a (tended) string.
                */
               tnd_var(sym, NULL, ".vword.sptr", indent);
               break;
            case PrmInt:
               /*
                * Parameter converted to a C integer.
                */
               chk_nl(indent);
               fprintf(out_file, "r_i%d", sym->u.param_info.param_num);
               break;
            case PrmDbl:
               /*
                * Parameter converted to a C double.
                */
               chk_nl(indent);
               fprintf(out_file, "r_d%d", sym->u.param_info.param_num);
               break;
            default:
               errt2(t, "Conflicting conversions for: ", t->image);
            }
         break;
      case RtParm | VarPrm:
      case DrfPrm | VarPrm:
         /*
          * Parameter representing variable part of argument list.
          */
         prt_str("(&", indent);
         if (sym->t_indx >= 0)
            fprintf(out_file, "%s.d[%d])", tendstrct, sym->t_indx);
         else
            fprintf(out_file, "r_args[%d])", sym->u.param_info.param_num + 1);
         break;
      case VArgLen:
         /*
          * Length of variable part of argument list.
          */
         prt_str("(r_nargs - ", indent);
         fprintf(out_file, "%d)", params->u.param_info.param_num);
         break;
      case RsltLoc:
         /*
          * "result" the result location of the operation.
          */
         prt_str(rslt_loc, indent);
         break;
      case Label:
         /*
          * Statement label.
          */
         prt_str(sym->image, indent);
         break;
      case OtherDcl:
         /*
          * Some other type of variable: accessed by identifier. If this
          *  is a body function, it may be passed by reference and need
          *  a level of pointer dereferencing.
          */
         if (sym->id_type & ByRef)
            prt_str("(*",indent);
         prt_str(sym->image, indent);
         if (sym->id_type & ByRef)
            prt_str(")",indent);
         break;
      }
   }

/*
 * does_call - determine if an expression contains a function call by
 *  walking its syntax tree.
 */
static int does_call(expr)
struct node *expr;
   {
   int n_subs;
   int i;

   if (expr == NULL)
      return 0;
   if (expr->nd_id == BinryNd && expr->tok->tok_id == ')')
      return 1;      /* found a function call */

   switch (expr->nd_id) {
      case ExactCnv: case PrimryNd: case SymNd:
         n_subs = 0;
         break;
      case CompNd:
         /*
          * Check field 0 below, field 1 is not a subtree, check field 2 here.
          */
         n_subs = 1;
         if (does_call(expr->u[2].child))
             return 1;
         break;
      case IcnTypNd: case PstfxNd: case PreSpcNd: case PrefxNd:
         n_subs = 1;
         break;
      case AbstrNd: case BinryNd: case CommaNd: case ConCatNd: case LstNd:
      case StrDclNd:
         n_subs = 2;
         break;
      case TrnryNd:
         n_subs = 3;
         break;
      case QuadNd:
         n_subs = 4;
         break;
      default:
         fprintf(stdout, "rtt internal error: unknown node type\n");
         exit(EXIT_FAILURE);
         }

   for (i = 0; i < n_subs; ++i)
      if (does_call(expr->u[i].child))
          return 1;

   return 0;
   }

/*
 * prt_runerr - print code to implement runerr().
 */
static void prt_runerr(t, num, val, indent)
struct token *t;
struct node *num;
struct node *val;
int indent;
   {
   if (op_type == OrdFunc)
      errt1(t, "'runerr' may not be used in an ordinary C function");

   tok_line(t, indent);  /* synchronize file name and line number */
   prt_str("{", indent);
   ForceNl();
   prt_str("err_msg(", indent);
   c_walk(num, indent, 0);                /* error number */
   if (val == NULL)
      prt_str(", NULL);", indent);        /* no offending value */
   else {
      prt_str(", &(", indent);
      c_walk(val, indent, 0);             /* offending value */
      prt_str("));", indent);
      }
   /*
    * Handle error conversion. Indicate that operation may fail because
    *  of error conversion and produce the necessary code.
    */
   cur_impl->ret_flag |= DoesEFail;
   failure(indent, 1);
   prt_str("}", indent);
   ForceNl();
   }

/*
 * typ_name - convert a type code to a string that can be used to
 *  output "T_" or "D_" type codes.
 */
char *typ_name(typcd, tok)
int typcd;
struct token *tok;
   {
   if (typcd == Empty_type)
      errt1(tok, "it is meaningless to assert a type of empty_type");
   else if (typcd == Any_value)
      errt1(tok, "it is useless to assert a type of any_value");
   else if (typcd < 0 || typcd == str_typ)
      return NULL;
   else
      return icontypes[typcd].cap_id;
   /*NOTREACHED*/
   return 0;			/* avoid gcc warning */
   }

/*
 * Produce a C conditional expression to check a descriptor for a
 *  particular type.
 */
static void typ_asrt(typcd, desc, tok, indent)
int typcd;
struct node *desc;
struct token *tok;
int indent;
   {
   tok_line(tok, indent);

   if (typcd == str_typ) {
      /*
       * Check dword for the absense of a "not qualifier" flag.
       */
      prt_str("(!((", indent);
      c_walk(desc, indent, 0);
      prt_str(").dword & F_Nqual))", indent);
      }
   else if (typcd == TypVar) {
      /*
       * Check dword for the presense of a "variable" flag.
       */
      prt_str("(((", indent);
      c_walk(desc, indent, 0);
      prt_str(").dword & D_Var) == D_Var)", indent);
      }
   else if (typcd == int_typ) {
      /*
       * If large integers are supported, an integer can be either
       *  an ordinary integer or a large integer.
       */
      ForceNl();
      prt_str("#ifdef LargeInts", 0);
      ForceNl();
      prt_str("(((", indent);
      c_walk(desc, indent, 0);
      prt_str(").dword == D_Integer) || ((", indent);
      c_walk(desc, indent, 0);
      prt_str(").dword == D_Lrgint))", indent);
      ForceNl();
      prt_str("#else\t\t\t\t\t/* LargeInts */", 0);
      ForceNl();
      prt_str("((", indent);
      c_walk(desc, indent, 0);
      prt_str(").dword == D_Integer)", indent);
      ForceNl();
      prt_str("#endif\t\t\t\t\t/* LargeInts */", 0);
      ForceNl();
      }
   else {
      /*
       * Check dword for a specific type code.
       */
      prt_str("((", indent);
      c_walk(desc, indent, 0);
      prt_str(").dword == D_", indent);
      prt_str(typ_name(typcd, tok), indent);
      prt_str(")", indent);
      }
   }

/*
 * retval_dcltor - convert the "declarator" part of function declaration
 *  into a declarator for the variable "r_retval" of the same type
 *  as the function result type, outputing the new declarator. This
 *  variable is a temporary location to store the result of the argument
 *  to a C return statement.
 */
static int retval_dcltor(dcltor, indent)
struct node *dcltor;
int indent;
   {
   int flag;

   switch (dcltor->nd_id) {
      case ConCatNd:
         c_walk(dcltor->u[0].child, indent, 0);
         retval_dcltor(dcltor->u[1].child, indent);
         return NotId;
      case PrimryNd:
         /*
          * We have reached the function name. Replace it with "r_retval"
          *  and tell caller we have found it.
          */
         prt_str("r_retval", indent);
         return IsId;
      case PrefxNd:
         /*
          * (...)
          */
         prt_str("(", indent);
         flag = retval_dcltor(dcltor->u[0].child, indent);
         prt_str(")", indent);
         return flag;
      case BinryNd:
         if (dcltor->tok->tok_id == ')') {
            /*
             * Function declaration. If this is the declarator that actually
             *  defines the function being processed, discard the paramater
             *  list including parentheses.
             */
            if (retval_dcltor(dcltor->u[0].child, indent) == NotId) {
               prt_str("(", indent);
               c_walk(dcltor->u[1].child, indent, 0);
               prt_str(")", indent);
               }
            }
         else {
            /*
             * Array.
             */
            retval_dcltor(dcltor->u[0].child, indent);
            prt_str("[", indent);
            c_walk(dcltor->u[1].child, indent, 0);
            prt_str("]", indent);
            }
         return NotId;
      }
   err1("rtt internal error detected in function retval_dcltor()");
   /*NOTREACHED*/
   return 0;			/* avoid gcc warning */
   }

/*
 * cnv_fnc - produce code to handle RTT cnv: and def: constructs.
 */
static void cnv_fnc(t, typcd, src, dflt, dest, indent)
struct token *t;
int typcd;
struct node *src;
struct node *dflt;
struct node *dest;
int indent;
   {
   int dflt_to_ptr;
   int loc;
   int is_cstr;

   if (src->nd_id == SymNd && src->u[0].sym->id_type & VarPrm)
      errt1(t, "converting entire variable part of param list not supported");

   tok_line(t, indent); /* synchronize file name and line number */

   /*
    * Initial assumptions: result of conversion is a tended location
    *   and is not tended C string.
    */
   loc = PrmTend;
   is_cstr = 0;

  /*
   * Print the name of the conversion function. If it is a conversion
   *  with a default value, determine (through dflt_to_prt) if the
   *  default value is passed by-reference instead of by-value.
   */
   prt_str(cnv_name(typcd, dflt, &dflt_to_ptr), indent);
   prt_str("(", indent);

   /*
    * Determine what parameter scope, if any, is established by this
    *  conversion. If the conversion needs a buffer, allocate it and
    *  put it in the argument list.
    */
   switch (typcd) {
      case TypCInt:
      case TypECInt:
         loc = PrmInt;
         break;
      case TypCDbl:
         loc = PrmDbl;
         break;
      case TypCStr:
         is_cstr = 1;
         break;
      case TypTStr:
         fprintf(out_file, "r_sbuf[%d], ", nxt_sbuf++);
         break;
      case TypTCset:
         fprintf(out_file, "&r_cbuf[%d], ", nxt_cbuf++);
         break;
      }

   /*
    * Output source of conversion.
    */
   prt_str("&(", indent);
   c_walk(src, indent, 0);
   prt_str("), ", indent);

   /*
    * If there is a default value, output it, taking its address if necessary.
    */
   if (dflt != NULL) {
      if (dflt_to_ptr)
         prt_str("&(", indent);
      c_walk(dflt, indent, 0);
      if (dflt_to_ptr)
         prt_str("), ", indent);
      else
         prt_str(", ", indent);
      }

   /*
    * Output the destination of the conversion. This may or may not be
    *  the same as the source.
    */
   prt_str("&(", indent);
   if (dest == NULL) {
      /*
       * Convert "in place", changing the location of a paramater if needed.
       */
      if (src->nd_id == SymNd && src->u[0].sym->id_type & (RtParm | DrfPrm)) {
         if (src->u[0].sym->id_type & DrfPrm)
            src->u[0].sym->u.param_info.cur_loc = loc;
         else
            errt1(t, "only dereferenced parameter can be converted in-place");
         }
      else if ((loc != PrmTend) | is_cstr)
         errt1(t,
            "only ordinary parameters can be converted in-place to C values");
      c_walk(src, indent, 0);
      if (is_cstr) {
         /*
          * The parameter must be accessed as a tended C string, but only
          *  now, after the "destination" code has been produced as a full
          *  descriptor.
          */
         src->u[0].sym->u.param_info.cur_loc = PrmCStr;
         }
      }
   else {
      /*
       * Convert to an explicit destination.
       */
      if (is_cstr) {
         /*
          * Access the destination as a full descriptor even though it
          *  must be declared as a tended C string.
          */
         if (dest->nd_id != SymNd || (dest->u[0].sym->id_type != TndStr &&
               dest->u[0].sym->id_type != TndDesc))
            errt1(t,
             "dest. of C_string conv. must be tended descriptor or char *");
         tnd_var(dest->u[0].sym, NULL, "", indent);
         }
      else
         c_walk(dest, indent, 0);
      }
   prt_str("))", indent);
   }

/*
 * cnv_name - produce name of conversion routine. Warning, name is
 *   constructed in a static buffer. Also determine if a default
 *   must be passed "by reference".
 */
char *cnv_name(typcd, dflt, dflt_to_ptr)
int typcd;
struct node *dflt;
int *dflt_to_ptr;
   {
   static char buf[15];
   int by_ref;

   /*
    * The names of simple conversion and defaulting conversions have
    *  the same suffixes, but different prefixes.
    */
   if (dflt == NULL)
      strcpy(buf , "cnv_");
   else
       strcpy(buf, "def_");

   by_ref = 0;
   switch (typcd) {
      case TypCInt:
         strcat(buf, "c_int");
         break;
      case TypCDbl:
         strcat(buf, "c_dbl");
         break;
      case TypCStr:
         strcat(buf, "c_str");
         break;
      case TypTStr:
         strcat(buf, "tstr");
         by_ref = 1;
         break;
      case TypTCset:
         strcat(buf, "tcset");
         by_ref = 1;
         break;
      case TypEInt:
         strcat(buf, "eint");
         break;
      case TypECInt:
         strcat(buf, "ec_int");
         break;
      default:
         if (typcd == cset_typ) {
            strcat(buf, "cset");
            by_ref = 1;
            }
         else if (typcd == int_typ)
            strcat(buf, "int");
         else if (typcd == real_typ)
            strcat(buf, "real");
         else if (typcd == str_typ) {
            strcat(buf, "str");
            by_ref = 1;
            }
      }
   if (dflt_to_ptr != NULL)
      *dflt_to_ptr = by_ref;
   return buf;
   }

/*
 * ret_value - produce code to set the result location of an operation
 *  using the expression on a return or suspend.
 */
static void ret_value(t, n, indent)
struct token *t;
struct node *n;
int indent;
   {
   struct node *caller;
   struct node *args;
   int typcd;

   if (n == NULL)
      errt1(t, "there is no default return value for run-time operations");

   if (n->nd_id == SymNd && n->u[0].sym->id_type == RsltLoc) {
      /*
       * return/suspend result;
       *
       *   result already where it needs to be.
       */
      return;
      }

   if (n->nd_id == PrefxNd && n->tok != NULL) {
      switch (n->tok->tok_id) {
         case C_Integer:
            /*
             * return/suspend C_integer <expr>;
             */
            prt_str(rslt_loc, indent);
            prt_str(".vword.integr = ", indent);
            c_walk(n->u[0].child, indent + IndentInc, 0);
            prt_str(";", indent);
            ForceNl();
            prt_str(rslt_loc, indent);
            prt_str(".dword = D_Integer;", indent);
            chkabsret(t, int_typ);  /* compare return with abstract return */
            return;
         case C_Double:
            /*
             * return/suspend C_double <expr>;
             */
            prt_str(rslt_loc, indent);
            prt_str(".vword.bptr = (union block *)alcreal(", indent);
            c_walk(n->u[0].child, indent + IndentInc, 0);
            prt_str(");", indent + IndentInc);
            ForceNl();
            prt_str(rslt_loc, indent);
            prt_str(".dword = D_Real;", indent);
            /*
             * The allocation of the real block may fail.
             */
            chk_rsltblk(indent);
            chkabsret(t, real_typ); /* compare return with abstract return */
            return;
         case C_String:
            /*
             * return/suspend C_string <expr>;
             */
            prt_str(rslt_loc, indent);
            prt_str(".vword.sptr = ", indent);
            c_walk(n->u[0].child, indent + IndentInc, 0);
            prt_str(";", indent);
            ForceNl();
            prt_str(rslt_loc, indent);
            prt_str(".dword = strlen(", indent);
            prt_str(rslt_loc, indent);
            prt_str(".vword.sptr);", indent);
            chkabsret(t, str_typ); /* compare return with abstract return */
            return;
         }
      }
   else if (n->nd_id == BinryNd && n->tok->tok_id == ')') {
      /*
       * Return value is in form of function call, see if it is really
       *  a descriptor constructor.
       */
      caller = n->u[0].child;
      args = n->u[1].child;
      if (caller->nd_id == SymNd) {
         switch (caller->tok->tok_id) {
            case IconType:
               typcd = caller->u[0].sym->u.typ_indx;
               switch (icontypes[typcd].rtl_ret) {
                  case TRetBlkP:
                     /*
                      * return/suspend <type>(<block-pntr>);
                      */
                     ret_1_arg(t, args, typcd, ".vword.bptr = (union block *)",
                        "(bp)", indent);
                     break;
                  case TRetDescP:
                     /*
                      * return/suspend <type>(<desc-pntr>);
                      */
                     ret_1_arg(t, args, typcd, ".vword.descptr = (dptr)",
                        "(dp)", indent);
                     break;
                  case TRetCharP:
                     /*
                      * return/suspend <type>(<char-pntr>);
                      */
                     ret_1_arg(t, args, typcd, ".vword.sptr = (char *)",
                        "(s)", indent);
                     break;
                  case TRetCInt:
                     /*
                      * return/suspend <type>(<integer>);
                      */
                     ret_1_arg(t, args, typcd, ".vword.integr = (word)",
                        "(i)", indent);
                     break;
                  case TRetSpcl:
                     if (typcd == str_typ) {
                        /*
                         * return/suspend string(<len>, <char-pntr>);
                         */
                        if (args == NULL || args->nd_id != CommaNd ||
                           args->u[0].child->nd_id == CommaNd)
                           errt1(t, "wrong no. of args for string(n, s)");
                        prt_str(rslt_loc, indent);
                        prt_str(".vword.sptr = ", indent);
                        c_walk(args->u[1].child, indent + IndentInc, 0);
                        prt_str(";", indent);
                        ForceNl();
                        prt_str(rslt_loc, indent);
                        prt_str(".dword = ", indent);
                        c_walk(args->u[0].child, indent + IndentInc, 0);
                        prt_str(";", indent);
                        }
                     else if (typcd == stv_typ) {
                        /*
                         * return/suspend tvsubs(<desc-pntr>, <start>, <len>);
                         */
                        if (args == NULL || args->nd_id != CommaNd ||
                           args->u[0].child->nd_id != CommaNd ||
                           args->u[0].child->u[0].child->nd_id == CommaNd)
                           errt1(t, "wrong no. of args for tvsubs(dp, i, j)");
                        no_nl = 1;
                        prt_str("SubStr(&", indent);
                        prt_str(rslt_loc, indent);
                        prt_str(", ", indent);
                        c_walk(args->u[0].child->u[0].child, indent + IndentInc,
                           0);
                        prt_str(", ", indent + IndentInc);
                        c_walk(args->u[1].child, indent + IndentInc, 0);
                        prt_str(", ", indent + IndentInc);
                        c_walk(args->u[0].child->u[1].child, indent + IndentInc,
                          0);
                        prt_str(");", indent + IndentInc);
                        no_nl = 0;
                        /*
                         * The allocation of the substring trapped variable
                         *   block may fail.
                         */
                        chk_rsltblk(indent);
                        chkabsret(t, stv_typ); /* compare to abstract return */
                        }
                     break;
                  }
               chkabsret(t, typcd); /* compare return with abstract return */
               return;
            case Named_var:
               /*
                * return/suspend named_var(<desc-pntr>);
                */
               if (args == NULL || args->nd_id == CommaNd)
                  errt1(t, "wrong no. of args for named_var(dp)");
               prt_str(rslt_loc, indent);
               prt_str(".vword.descptr = ", indent);
               c_walk(args, indent + IndentInc, 0);
               prt_str(";", indent);
               ForceNl();
               prt_str(rslt_loc, indent);
               prt_str(".dword = D_Var;", indent);
               chkabsret(t, TypVar); /* compare return with abstract return */
               return;
            case Struct_var:
               /*
                * return/suspend struct_var(<desc-pntr>, <block_pntr>);
                */
               if (args == NULL || args->nd_id != CommaNd ||
                  args->u[0].child->nd_id == CommaNd)
                  errt1(t, "wrong no. of args for struct_var(dp, bp)");
               prt_str(rslt_loc, indent);
               prt_str(".vword.descptr = (dptr)", indent);
               c_walk(args->u[1].child, indent + IndentInc, 0);
               prt_str(";", indent);
               ForceNl();
               prt_str(rslt_loc, indent);
               prt_str(".dword = D_Var + ((word *)", indent);
               c_walk(args->u[0].child, indent + IndentInc, 0);
               prt_str(" - (word *)", indent+IndentInc);
               prt_str(rslt_loc, indent);
               prt_str(".vword.descptr);", indent+IndentInc);
               ForceNl();
               chkabsret(t, TypVar); /* compare return with abstract return */
               return;
            }
         }
      }

   /*
    * If it is not one of the special returns, it is just a return of
    *  a descriptor.
    */
   prt_str(rslt_loc, indent);
   prt_str(" = ", indent);
   c_walk(n, indent + IndentInc, 0);
   prt_str(";", indent);
   chkabsret(t, SomeType); /* check for preceding abstract return */
   }

/*
 * ret_1_arg - produce code for a special return/suspend with one argument.
 */
static void ret_1_arg(t, args, typcd, vwrd_asgn, arg_rep, indent)
struct token *t;
struct node *args;
int typcd;
char *vwrd_asgn;
char *arg_rep;
int indent;
   {
   if (args == NULL || args->nd_id == CommaNd)
      errt3(t, "wrong no. of args for", icontypes[typcd].id, arg_rep);

   /*
    * Assignment to vword of result descriptor.
    */
   prt_str(rslt_loc, indent);
   prt_str(vwrd_asgn, indent);
   c_walk(args, indent + IndentInc, 0);
   prt_str(";", indent);
   ForceNl();

   /*
    * Assignment to dword of result descriptor.
    */
   prt_str(rslt_loc, indent);
   prt_str(".dword = D_", indent);
   prt_str(icontypes[typcd].cap_id, indent);
   prt_str(";", indent);
   }

/*
 * chk_rsltblk - the result value contains an allocated block, make sure
 *    the allocation succeeded.
 */
static void chk_rsltblk(indent)
int indent;
   {
   ForceNl();
   prt_str("if (", indent);
   prt_str(rslt_loc, indent);
   prt_str(".vword.bptr == NULL) {", indent);
   ForceNl();
   prt_str("err_msg(307, NULL);", indent + IndentInc);
   ForceNl();
   /*
    * Handle error conversion. Indicate that operation may fail because
    *  of error conversion and produce the necessary code.
    */
   cur_impl->ret_flag |= DoesEFail;
   failure(indent + IndentInc, 1);
   prt_str("}", indent + IndentInc);
   ForceNl();
   }

/*
 * failure - produce code for fail or efail.
 */
static void failure(indent, brace)
int indent;
int brace;
   {
   /*
    * If there are tended variables, they must be removed from the tended
    *  list. The C function may or may not return an explicit signal.
    */
   ForceNl();
   if (ntend != 0) {
      if (!brace)
         prt_str("{", indent);
      untend(indent);
      ForceNl();
      if (fnc_ret == RetSig)
         prt_str("return A_Resume;", indent);
      else
         prt_str("return;", indent);
      if (!brace) {
         ForceNl();
         prt_str("}", indent);
         }
      }
   else
      if (fnc_ret == RetSig)
         prt_str("return A_Resume;", indent);
      else
         prt_str("return;", indent);
   ForceNl();
   }

/*
 * c_walk - walk the syntax tree for extended C code and output the
 *  corresponding ordinary C. Return and indication of whether execution
 *  falls through the code.
 */
int c_walk(n, indent, brace)
struct node *n;
int indent;
int brace;
   {
   struct token *t;
   struct node *n1;
   struct sym_entry *sym;
   int fall_thru;
   int save_break;
   static int does_break = 0;
   static int may_brnchto;  /* may reach end of code by branching into middle */

   if (n == NULL)
      return 1;

   t =  n->tok;

   switch (n->nd_id) {
      case PrimryNd:
         switch (t->tok_id) {
            case Fail:
               if (op_type == OrdFunc)
                  errt1(t, "'fail' may not be used in an ordinary C function");
               cur_impl->ret_flag |= DoesFail;
               failure(indent, brace);
	       chkabsret(t, SomeType);  /* check preceding abstract return */
	       return 0;
	    case Errorfail:
	       if (op_type == OrdFunc)
		  errt1(t,
		      "'errorfail' may not be used in an ordinary C function");
	       cur_impl->ret_flag |= DoesEFail;
	       failure(indent, brace);
	       return 0;
            case Break:
	       prt_tok(t, indent);
	       prt_str(";", indent);
               does_break = 1;
               return 0;
	    default:
               /*
                * Other "primary" expressions are just their token image,
                *  possibly followed by a semicolon.
                */
	       prt_tok(t, indent);
	       if (t->tok_id == Continue)
		  prt_str(";", indent);
               return 1;
	    }
      case PrefxNd:
	 switch (t->tok_id) {
	    case Sizeof:
	       prt_tok(t, indent);                /* sizeof */
	       prt_str("(", indent);
	       c_walk(n->u[0].child, indent, 0);
	       prt_str(")", indent);
	       return 1;
	    case '{':
               /*
                * Initializer list.
                */
	       prt_tok(t, indent + IndentInc);     /* { */
	       c_walk(n->u[0].child, indent + IndentInc, 0);
	       prt_str("}", indent + IndentInc);
	       return 1;
	    case Default:
	       prt_tok(t, indent - IndentInc);     /* default (un-indented) */
	       prt_str(": ", indent - IndentInc);
	       fall_thru = c_walk(n->u[0].child, indent, 0);
               may_brnchto = 1;
               return fall_thru;
	    case Goto:
	       prt_tok(t, indent);                 /* goto */
	       prt_str(" ", indent);
	       c_walk(n->u[0].child, indent, 0);
	       prt_str(";", indent);
	       return 0;
	    case Return:
	       if (n->u[0].child != NULL)
		  no_ret_val = 0;  /* note that return statement has no value */

	       if (op_type == OrdFunc || fnc_ret == RetInt ||
		  fnc_ret == RetDbl) {
		  /*
		   * ordinary C return: ignore C_integer, C_double, and
		   *  C_string qualifiers on return expression (the first
		   *  two may legally occur when fnc_ret is RetInt or RetDbl).
		   */
		  n1 = n->u[0].child;
		  if (n1 != NULL && n1->nd_id == PrefxNd && n1->tok != NULL) {
		     switch (n1->tok->tok_id) {
			case C_Integer:
			case C_Double:
			case C_String:
			   n1 = n1->u[0].child;
			}
		     }
		  if (ntend != 0) {
                     /*
                      * There are tended variables that must be removed from
                      *  the tended list.
                      */
		     if (!brace)
			prt_str("{", indent);
		     if (does_call(n1)) {
			/*
			 * The return expression contains a function call;
                         *  the variables must remain tended while it is
                         *  computed, so compute it into a temporary variable
                         *  named r_retval.Output a declaration for r_retval;
                         *  its type must match the return type of the C
                         *  function.
                         */
			ForceNl();
			prt_str("register ", indent);
			if (op_type == OrdFunc) {
			   no_nl = 1;
			   just_type(fnc_head->u[0].child, indent, 0);
			   prt_str(" ", indent);
			   retval_dcltor(fnc_head->u[1].child, indent);
			   prt_str(";", indent);
			   no_nl = 0;
			   }
			else if (fnc_ret == RetInt)
			   prt_str("C_integer r_retval;", indent);
			else    /* fnc_ret == RetDbl */
			   prt_str("double r_retval;", indent);
			ForceNl();

                        /*
                         * Output code to compute the return value, untend
                         *  the variable, then return the value.
                         */
			prt_str("r_retval = ", indent);
			c_walk(n1, indent + IndentInc, 0);
			prt_str(";", indent);
			untend(indent);
			ForceNl();
			prt_str("return r_retval;", indent);
			}
		     else {
                        /*
                         * It is safe to untend the variables and return
                         *  the result value directly with a return
                         *  statement.
                         */
			untend(indent);
			ForceNl();
			prt_tok(t, indent);    /* return */
			prt_str(" ", indent);
			c_walk(n1, indent, 0);
			prt_str(";", indent);
			}
		     if (!brace) {
			ForceNl();
			prt_str("}", indent);
			}
		     ForceNl();
		     }
		  else {
                     /*
                      * There are no tended variable, just output the
                      *  return expression.
                      */
		     prt_tok(t, indent);     /* return */
		     prt_str(" ", indent);
		     c_walk(n1, indent, 0);
		     prt_str(";", indent);
		     }

                  /*
                   * If this is a body function, check the return against
                   *  preceding abstract returns.
                   */
		  if (fnc_ret == RetInt)
		     chkabsret(n->tok, int_typ);
                  else if (fnc_ret == RetDbl)
                     chkabsret(n->tok, real_typ);
                  }
               else {
                  /*
                   * Return from Icon operation. Indicate that the operation
                   *  returns, compute the value into the result location,
                   *  untend variables if necessary, and return a signal
                   *  if the function requires one.
                   */
                  cur_impl->ret_flag |= DoesRet;
                  ForceNl();
                  if (!brace) {
                     prt_str("{", indent);
                     ForceNl();
                     }
                  ret_value(t, n->u[0].child, indent);
                  if (ntend != 0)
                     untend(indent);
                  ForceNl();
                  if (fnc_ret == RetSig)
                     prt_str("return A_Continue;", indent);
                  else if (fnc_ret == RetNoVal)
                     prt_str("return;", indent);
                  ForceNl();
                  if (!brace) {
                     prt_str("}", indent);
                     ForceNl();
                     }
                  }
               return 0;
            case Suspend:
               if (op_type == OrdFunc)
                  errt1(t, "'suspend' may not be used in an ordinary C function"
                     );
               cur_impl->ret_flag |= DoesSusp; /* note suspension */
               ForceNl();
               if (!brace) {
                  prt_str("{", indent);
                  ForceNl();
                  }
               prt_str("register int signal;", indent + IndentInc);
               ForceNl();
               ret_value(t, n->u[0].child, indent);
               ForceNl();
               /*
                * The operator suspends by calling the success continuation
                *  if there is one or just returns if there is none. For
                *  the interpreter, interp() is the success continuation.
                *  A non-A_Resume signal from the success continuation must
                *  returned to the caller. If there are tended variables
                *  they must be removed from the tended list before a signal
                *  is returned.
                */
               if (iconx_flg) {
                  #ifdef EventMon
		  switch (op_type) {
		     case TokFunction:
		        prt_str(
		        "if ((signal = interp(G_Fsusp, r_args)) != A_Resume) {",
		           indent);
		        break;
		     case Operator:
		     case Keyword:
		        prt_str(
		        "if ((signal = interp(G_Osusp, r_args)) != A_Resume) {",
		           indent);
		        break;
		     default:
		        prt_str(
		        "if ((signal = interp(G_Csusp, r_args)) != A_Resume) {",
			   indent);
		     }
                  #else			/* EventMon */
		     prt_str(
		        "if ((signal = interp(G_Csusp, r_args)) != A_Resume) {",
		           indent);
                  #endif		/* EventMon */
		  }
               else {
                  prt_str("if (r_s_cont == (continuation)NULL) {", indent);
                  if (ntend != 0)
                     untend(indent + IndentInc);
                  ForceNl();
                  prt_str("return A_Continue;", indent + IndentInc);
                  ForceNl();
                  prt_str("}", indent + IndentInc);
                  ForceNl();
                  prt_str("else if ((signal = (*r_s_cont)()) != A_Resume) {",
                     indent);
                  }
               ForceNl();
               if (ntend != 0)
                  untend(indent + IndentInc);
               ForceNl();
               prt_str("return signal;", indent + IndentInc);
               ForceNl();
               prt_str("}", indent + IndentInc);
               if (!brace) {
                  prt_str("}", indent);
                  ForceNl();
                  }
               return 1;
            case '(':
               /*
                * Parenthesized expression.
                */
               prt_tok(t, indent);     /* ( */
               fall_thru = c_walk(n->u[0].child, indent, 0);
               prt_str(")", indent);
               return fall_thru;
            default:
               /*
                * All other prefix expressions are printed as the token
                *  image of the operation followed by the operand.
                */
               prt_tok(t, indent);
               c_walk(n->u[0].child, indent, 0);
               return 1;
            }
      case PstfxNd:
         /*
          * All postfix expressions are printed as the operand followed
          *  by the token image of the operation.
          */
         fall_thru = c_walk(n->u[0].child, indent, 0);
         prt_tok(t, indent);
         return fall_thru;
      case PreSpcNd:
         /*
          * This prefix expression (pointer indication in a declaration) needs
          *  a space after it.
          */
         prt_tok(t, indent);
         c_walk(n->u[0].child, indent, 0);
         prt_str(" ", indent);
         return 1;
      case SymNd:
         /*
          * Identifier.
          */
         prt_var(n, indent);
         return 1;
      case BinryNd:
         switch (t->tok_id) {
            case '[':
               /*
                * subscripting expression or declaration: <expr> [ <expr> ]
                */
               n1 = n->u[0].child;
               c_walk(n->u[0].child, indent, 0);
               prt_str("[", indent);
               c_walk(n->u[1].child, indent, 0);
               prt_str("]", indent);
               return 1;
            case '(':
               /*
                * cast: ( <type> ) <expr>
                */
               prt_tok(t, indent);  /* ) */
               c_walk(n->u[0].child, indent, 0);
               prt_str(")", indent);
               c_walk(n->u[1].child, indent, 0);
               return 1;
            case ')':
               /*
                * function call or declaration: <expr> ( <expr-list> )
                */
               c_walk(n->u[0].child, indent, 0);
               prt_str("(", indent);
               c_walk(n->u[1].child, indent, 0);
               prt_tok(t, indent);   /* ) */
               return call_ret(n->u[0].child);
            case Struct:
            case Union:
               /*
                * struct/union <ident>
                * struct/union <opt-ident> { <field-list> }
                */
               prt_tok(t, indent);   /* struct or union */
               prt_str(" ", indent);
               c_walk(n->u[0].child, indent, 0);
               if (n->u[1].child != NULL) {
                  /*
                   * Field declaration list.
                   */
                  prt_str(" {", indent);
                  c_walk(n->u[1].child, indent + IndentInc, 0);
                  ForceNl();
                  prt_str("}", indent);
                  }
               return 1;
            case TokEnum:
               /*
                * enum <ident>
                * enum <opt-ident> { <enum-list> }
                */
               prt_tok(t, indent);   /* enum */
               prt_str(" ", indent);
               c_walk(n->u[0].child, indent, 0);
               if (n->u[1].child != NULL) {
                  /*
                   * enumerator list.
                   */
                  prt_str(" {", indent);
                  c_walk(n->u[1].child, indent + IndentInc, 0);
                  prt_str("}", indent);
                  }
               return 1;
            case ';':
               /*
                * <type-specs> <declarator> ;
                */
               c_walk(n->u[0].child, indent, 0);
               prt_str(" ", indent);
               c_walk(n->u[1].child, indent, 0);
               prt_tok(t, indent);  /* ; */
               return 1;
            case ':':
               /*
                * <label> : <statement>
                */
               c_walk(n->u[0].child, indent, 0);
               prt_tok(t, indent);   /* : */
               prt_str(" ", indent);
               fall_thru = c_walk(n->u[1].child, indent, 0);
               may_brnchto = 1;
               return fall_thru;
            case Case:
               /*
                * case <expr> : <statement>
                */
               prt_tok(t, indent - IndentInc);  /* case (un-indented) */
               prt_str(" ", indent);
               c_walk(n->u[0].child, indent - IndentInc, 0);
               prt_str(": ", indent - IndentInc);
               fall_thru = c_walk(n->u[1].child, indent, 0);
               may_brnchto = 1;
               return fall_thru;
            case Switch:
               /*
                * switch ( <expr> ) <statement>
                *
                * <statement> is double indented so that case and default
                * statements can be un-indented and come out indented 1
                * with respect to the switch. Statements that are not
                * "labeled" with case or default are indented one more
                * than those that are labeled.
                */
               prt_tok(t, indent);  /* switch */
               prt_str(" (", indent);
               c_walk(n->u[0].child, indent, 0);
               prt_str(")", indent);
               prt_str(" ", indent);
               save_break = does_break;
               fall_thru = c_walk(n->u[1].child, indent + 2 * IndentInc, 0);
               fall_thru |= does_break;
               does_break = save_break;
               return fall_thru;
            case While: {
               struct node *n0;
               /*
                * While ( <expr> ) <statement>
                */
               n0 = n->u[0].child;
               prt_tok(t, indent);  /* while */
               prt_str(" (", indent);
               c_walk(n0, indent, 0);
               prt_str(")", indent);
               prt_str(" ", indent);
               save_break = does_break;
               c_walk(n->u[1].child, indent + IndentInc, 0);
               /*
                * check for an infinite loop, while (1) ... :
                *  a condition consisting of an IntConst with image=="1"
                *  and no breaks in the body.
                */
               if (n0->nd_id == PrimryNd && n0->tok->tok_id == IntConst &&
                   !strcmp(n0->tok->image,"1") && !does_break)
                  fall_thru = 0;
               else
                  fall_thru = 1;
               does_break = save_break;
               return fall_thru;
               }
            case Do:
               /*
                * do <statement> <while> ( <expr> )
                */
               prt_tok(t, indent);  /* do */
               prt_str(" ", indent);
               c_walk(n->u[0].child, indent + IndentInc, 0);
               ForceNl();
               prt_str("while (", indent);
               save_break = does_break;
               c_walk(n->u[1].child, indent, 0);
               does_break = save_break;
               prt_str(");", indent);
               return 1;
            case '.':
            case Arrow:
               /*
                * Field access: <expr> . <expr>  and  <expr> -> <expr>
                */
               c_walk(n->u[0].child, indent, 0);
               prt_tok(t, indent);   /* . or -> */
               c_walk(n->u[1].child, indent, 0);
               return 1;
            case Runerr:
               /*
                * runerr ( <error-number> )
                * runerr ( <error-number> , <offending-value> )
                */
               prt_runerr(t, n->u[0].child, n->u[1].child, indent);
               return 0;
            case Is:
               /*
                * is : <type> ( <expr> )
                */
               typ_asrt(icn_typ(n->u[0].child), n->u[1].child,
                  n->u[0].child->tok, indent);
               return 1;
            default:
               /*
                * All other binary expressions are infix notation and
                *  are printed with spaces around the operator.
                */
               c_walk(n->u[0].child, indent, 0);
               prt_str(" ", indent);
               prt_tok(t, indent);
               prt_str(" ", indent);
               c_walk(n->u[1].child, indent, 0);
               return 1;
            }
      case LstNd:
         /*
          * <declaration-part> <declaration-part>
          *
          * Need space between parts
          */
         c_walk(n->u[0].child, indent, 0);
         prt_str(" ", indent);
         c_walk(n->u[1].child, indent, 0);
         return 1;
      case ConCatNd:
         /*
          * <some-code> <some-code>
          *
          * Various lists of code parts that do not need space between them.
          */
         if (c_walk(n->u[0].child, indent, 0))
            return c_walk(n->u[1].child, indent, 0);
         else {
            /*
             * Cannot directly reach the second piece of code, see if
             *  it is possible to branch into it.
             */
            may_brnchto = 0;
            fall_thru = c_walk(n->u[1].child, indent, 0);
            return may_brnchto & fall_thru;
            }
      case CommaNd:
         /*
          * <expr> , <expr>
          */
         c_walk(n->u[0].child, indent, 0);
         prt_tok(t, indent);
         prt_str(" ", indent);
         return c_walk(n->u[1].child, indent, 0);
      case StrDclNd:
         /*
          * Structure field declaration. Bit field declarations have
          *  a semicolon and a field width.
          */
         c_walk(n->u[0].child, indent, 0);
         if (n->u[1].child != NULL) {
            prt_str(": ", indent);
            c_walk(n->u[1].child, indent, 0);
            }
         return 1;
      case CompNd:
         /*
          * Compound statement.
          */
         if (brace)
            tok_line(t, indent); /* just synch. file name and line number */
         else
            prt_tok(t, indent);  /* { */
         c_walk(n->u[0].child, indent, 0);
         /*
          * we are in an inner block. tended locations may need to
          *  be set to values from declaration initializations.
          */
         for (sym = n->u[1].sym; sym!= NULL; sym = sym->u.tnd_var.next) {
            if (sym->u.tnd_var.init != NULL) {
               prt_str(tendstrct, IndentInc);
               fprintf(out_file, ".d[%d]", sym->t_indx);
               switch (sym->id_type) {
                  case TndDesc:
                     prt_str(" = ", IndentInc);
                     break;
                  case TndStr:
                     prt_str(".vword.sptr = ", IndentInc);
                     break;
                  case TndBlk:
                     prt_str(".vword.bptr = (union block *)",
                        IndentInc);
                     break;
                  }
               c_walk(sym->u.tnd_var.init, 2 * IndentInc, 0);
               prt_str(";", 2 * IndentInc);
               ForceNl();
               }
            }
         /*
          * If there are no declarations, suppress braces that
          *  may be required for a one-statement body; we already
          *  have a set.
          */
         if (n->u[0].child == NULL && n->u[1].sym == NULL)
            fall_thru = c_walk(n->u[2].child, indent, 1);
         else
            fall_thru = c_walk(n->u[2].child, indent, 0);
         if (!brace) {
            ForceNl();
            prt_str("}", indent);
            }
         return fall_thru;
      case TrnryNd:
         switch (t->tok_id) {
            case '?':
               /*
                * <expr> ? <expr> : <expr>
                */
               c_walk(n->u[0].child, indent, 0);
               prt_str(" ", indent);
               prt_tok(t, indent);  /* ? */
               prt_str(" ", indent);
               c_walk(n->u[1].child, indent, 0);
               prt_str(" : ", indent);
               c_walk(n->u[2].child, indent, 0);
               return 1;
            case If:
               /*
                * if ( <expr> ) <statement>
                * if ( <expr> ) <statement> else <statement>
                */
               prt_tok(t, indent);  /* if */
               prt_str(" (", indent);
               c_walk(n->u[0].child, indent + IndentInc, 0);
               prt_str(") ", indent);
               fall_thru = c_walk(n->u[1].child, indent + IndentInc, 0);
               n1 = n->u[2].child;
               if (n1 == NULL)
                  fall_thru = 1;
               else {
                  /*
                   * There is an else statement. Don't indent an
                   *  "else if"
                   */
                  ForceNl();
                  prt_str("else ", indent);
                  if (n1->nd_id == TrnryNd && n1->tok->tok_id == If)
                     fall_thru |= c_walk(n1, indent, 0);
                  else
                     fall_thru |= c_walk(n1, indent + IndentInc, 0);
                  }
               return fall_thru;
            case Type_case:
               /*
                * type_case <expr> of { <section-list> }
                * type_case <expr> of { <section-list> <default-clause> }
                */
               return typ_case(n->u[0].child, n->u[1].child, n->u[2].child,
                  c_walk, 1, indent);
            case Cnv:
               /*
                * cnv : <type> ( <source> , <destination> )
                */
               cnv_fnc(t, icn_typ(n->u[0].child), n->u[1].child, NULL,
                  n->u[2].child,
                  indent);
               return 1;
            }
      case QuadNd:
         switch (t->tok_id) {
            case For:
               /*
                * for ( <expr> ; <expr> ; <expr> ) <statement>
                */
               prt_tok(t, indent);  /* for */
               prt_str(" (", indent);
               c_walk(n->u[0].child, indent, 0);
               prt_str("; ", indent);
               c_walk(n->u[1].child, indent, 0);
               prt_str("; ", indent);
               c_walk(n->u[2].child, indent, 0);
               prt_str(") ", indent);
               save_break = does_break;
               c_walk(n->u[3].child, indent + IndentInc, 0);
               if (n->u[1].child == NULL && !does_break)
                  fall_thru = 0;
               else
                  fall_thru = 1;
               does_break = save_break;
               return fall_thru;
            case Def:
               /*
                * def : <type> ( <source> , <default> , <destination> )
                */
               cnv_fnc(t, icn_typ(n->u[0].child), n->u[1].child, n->u[2].child,
                  n->u[3].child, indent);
               return 1;
            }
      }
   /*NOTREACHED*/
   return 0;			/* avoid gcc warning */
   }

/*
 * call_ret - decide whether a function being called might return.
 */
int call_ret(n)
struct node *n;
   {
   /*
    * Assume functions return except for c_exit(), fatalerr(), and syserr().
    */
   if (n->tok != NULL &&
      (strcmp("c_exit",   n->tok->image) == 0 ||
       strcmp("fatalerr", n->tok->image) == 0 ||
       strcmp("syserr",   n->tok->image) == 0))
      return 0;
   else
      return 1;
   }

/*
 * new_prmloc - allocate an array large enough to hold a flag for every
 *  parameter of the current operation. This flag indicates where
 *  the parameter is in terms of scopes created by conversions.
 */
struct parminfo *new_prmloc()
   {
   struct parminfo *parminfo;
   int nparams;
   int i;

   if (params == NULL)
      return NULL;
   nparams = params->u.param_info.param_num + 1;
   parminfo = alloc(nparams * sizeof(struct parminfo));
   for (i = 0; i < nparams; ++i) {
      parminfo[i].cur_loc = 0;
      parminfo [i].parm_mod = 0;
      }
   return parminfo;
   }

/*
 * ld_prmloc - load parameter location information that has been
 *  saved in an arrary into the symbol table.
 */
void ld_prmloc(parminfo)
struct parminfo *parminfo;
   {
   struct sym_entry *sym;
   int param_num;

   for (sym = params; sym != NULL; sym = sym->u.param_info.next) {
      param_num = sym->u.param_info.param_num;
      if (sym->id_type & DrfPrm) {
         sym->u.param_info.cur_loc = parminfo[param_num].cur_loc;
         sym->u.param_info.parm_mod = parminfo[param_num].parm_mod;
         }
      }
   }

/*
 * sv_prmloc - save parameter location information from the the symbol table
 *  into an array.
 */
void sv_prmloc(parminfo)
struct parminfo *parminfo;
   {
   struct sym_entry *sym;
   int param_num;

   for (sym = params; sym != NULL; sym = sym->u.param_info.next) {
      param_num = sym->u.param_info.param_num;
      if (sym->id_type & DrfPrm) {
         parminfo[param_num].cur_loc = sym->u.param_info.cur_loc;
         parminfo[param_num].parm_mod = sym->u.param_info.parm_mod;
         }
      }
   }

/*
 * mrg_prmloc - merge parameter location information in the symbol table
 *  with other information already saved in an array. This may result
 *  in conflicting location information, but conflicts are only detected
 *  when a parameter is actually used.
 */
void mrg_prmloc(parminfo)
struct parminfo *parminfo;
   {
   struct sym_entry *sym;
   int param_num;

   for (sym = params; sym != NULL; sym = sym->u.param_info.next) {
      param_num = sym->u.param_info.param_num;
      if (sym->id_type & DrfPrm) {
         parminfo[param_num].cur_loc |= sym->u.param_info.cur_loc;
         parminfo[param_num].parm_mod |= sym->u.param_info.parm_mod;
         }
      }
   }

/*
 * clr_prmloc - indicate that this execution path contributes nothing
 *   to the location of parameters.
 */
void clr_prmloc()
   {
   struct sym_entry *sym;

   for (sym = params; sym != NULL; sym = sym->u.param_info.next) {
      if (sym->id_type & DrfPrm) {
         sym->u.param_info.cur_loc = 0;
         sym->u.param_info.parm_mod = 0;
         }
      }
   }

/*
 * typ_case - translate a type_case statement into C. This is called
 *  while walking a syntax tree of either RTL code or C code; the parameter
 *  "walk" is a function used to process the subtrees within the type_case
 *  statement.
 */
static int typ_case(var, slct_lst, dflt, walk, maybe_var, indent)
struct node *var;
struct node *slct_lst;
struct node *dflt;
int (*walk)(struct node *n, int xindent, int brace);
int maybe_var;
int indent;
   {
   struct node *lst;
   struct node *select;
   struct node *slctor;
   struct parminfo *strt_prms;
   struct parminfo *end_prms;
   int remaining;
   int first;
   int fnd_slctrs;
   int maybe_str = 1;
   int dflt_lbl;
   int typcd;
   int fall_thru;
   char *s;

   /*
    * This statement involves multiple paths that may establish new
    *  scopes for parameters. Remember the starting scope information
    *  and initialize an array in which to compute the final information.
    */
   strt_prms = new_prmloc();
   sv_prmloc(strt_prms);
   end_prms = new_prmloc();

   /*
    * First look for cases that must be checked with "if" statements.
    *  These include string qualifiers and variables.
    */
   remaining = 0;      /* number of cases skipped in first pass */
   first = 1;          /* next case to be output is the first */
   if (dflt == NULL)
      fall_thru = 1;
   else
      fall_thru = 0;
   for (lst = slct_lst; lst != NULL; lst = lst->u[0].child) {
      select = lst->u[1].child;
      fnd_slctrs = 0; /* flag: found type selections for clause for this pass */
      /*
       * A selection clause may include several types.
       */
      for (slctor = select->u[0].child; slctor != NULL; slctor =
        slctor->u[0].child) {
         typcd = icn_typ(slctor->u[1].child);
         if(typ_name(typcd, slctor->u[1].child->tok) == NULL) {
            /*
             * This type must be checked with the "if". Is this the
             *  first condition checked for this clause? Is this the
             *  first clause output?
             */
            if (fnd_slctrs)
               prt_str(" || ", indent);
            else {
               if (first)
                  first = 0;
               else {
                  ForceNl();
                  prt_str("else ", indent);
                  }
               prt_str("if (", indent);
               fnd_slctrs = 1;
               }

            /*
             * Output type check
             */
            typ_asrt(typcd, var, slctor->u[1].child->tok, indent + IndentInc);

            if (typcd == str_typ)
               maybe_str = 0;  /* string has been taken care of */
            else if (typcd == Variable)
               maybe_var = 0;  /* variable has been taken care of */
            }
         else
            ++remaining;
         }
      if (fnd_slctrs) {
         /*
          * We have found and output type selections for this clause;
          *  output the body of the clause. Remember any changes to
          *  paramter locations caused by type conversions within the
          *  clause.
          */
         prt_str(") {", indent + IndentInc);
         ForceNl();
         if ((*walk)(select->u[1].child, indent + IndentInc, 1)) {
            fall_thru |= 1;
            mrg_prmloc(end_prms);
            }
         prt_str("}", indent + IndentInc);
         ForceNl();
         ld_prmloc(strt_prms);
         }
      }
   /*
    * The rest of the cases can be checked with a "switch" statement, look
    *  for them..
    */
   if (remaining == 0) {
      if (dflt != NULL) {
         /*
          * There are no cases to handle with a switch statement, but there
          *  is a default clause; handle it with an "else".
          */
         prt_str("else {", indent);
         ForceNl();
         fall_thru |= (*walk)(dflt, indent + IndentInc, 1);
         ForceNl();
         prt_str("}", indent + IndentInc);
         ForceNl();
         }
      }
   else {
      /*
       * If an "if" statement was output, the "switch" must be in its "else"
       *   clause.
       */
      if (!first)
         prt_str("else ", indent);

      /*
       * A switch statement cannot handle types that are not simple type
       *  codes. If these have not taken care of, output code to check them.
       *  This will either branch around the switch statement or into
       *  its default clause.
       */
      if (maybe_str || maybe_var) {
         dflt_lbl = lbl_num++;      /* allocate a label number */
         prt_str("{", indent);
         ForceNl();
         prt_str("if (((", indent);
         c_walk(var, indent + IndentInc, 0);
         prt_str(").dword & D_Typecode) != D_Typecode) ", indent);
         ForceNl();
         prt_str("goto L", indent + IndentInc);
         fprintf(out_file, "%d;  /* default */ ", dflt_lbl);
         ForceNl();
         }

      no_nl = 1; /* suppress #line directives */
      prt_str("switch (Type(", indent);
      c_walk(var, indent + IndentInc, 0);
      prt_str(")) {", indent + IndentInc);
      no_nl = 0;
      ForceNl();

      /*
       * Loop through the case clauses producing code for them.
       */
      for (lst = slct_lst; lst != NULL; lst = lst->u[0].child) {
         select = lst->u[1].child;
         fnd_slctrs = 0;
         /*
          * A selection clause may include several types.
          */
         for (slctor = select->u[0].child; slctor != NULL; slctor =
           slctor->u[0].child) {
            typcd = icn_typ(slctor->u[1].child);
            s = typ_name(typcd, slctor->u[1].child->tok);
            if (s != NULL) {
               /*
                * A type selection has been found that can be checked
                *  in the switch statement. Note that large integers
                *  require special handling.
                */
               fnd_slctrs = 1;

	       if (typcd == int_typ) {
		 ForceNl();
		 prt_str("#ifdef LargeInts", 0);
		 ForceNl();
		 prt_str("case T_Lrgint:  ", indent + IndentInc);
		 ForceNl();
		 prt_str("#endif /* LargeInts */", 0);
		 ForceNl();
	       }

               prt_str("case T_", indent + IndentInc);
               prt_str(s, indent + IndentInc);
               prt_str(": ", indent + IndentInc);
               }
            }
         if (fnd_slctrs) {
            /*
             * We have found and output type selections for this clause;
             *  output the body of the clause. Remember any changes to
             *  paramter locations caused by type conversions within the
             *  clause.
             */
            ForceNl();
            if ((*walk)(select->u[1].child, indent + 2 * IndentInc, 0)) {
               fall_thru |= 1;
               ForceNl();
               prt_str("break;", indent + 2 * IndentInc);
               mrg_prmloc(end_prms);
               }
            ForceNl();
            ld_prmloc(strt_prms);
            }
         }
      if (dflt != NULL) {
         /*
          * This type_case statement has a default clause. If there is
          *  a branch into this clause, output the label. Remember any
          *  changes to paramter locations caused by type conversions
          *  within the clause.
          */
         ForceNl();
         prt_str("default:", indent + 1 * IndentInc);
         ForceNl();
         if (maybe_str || maybe_var) {
            prt_str("L", 0);
            fprintf(out_file, "%d: ;  /* default */", dflt_lbl);
            ForceNl();
            }
         if ((*walk)(dflt, indent + 2 * IndentInc, 0)) {
            fall_thru |= 1;
            mrg_prmloc(end_prms);
            }
         ForceNl();
         ld_prmloc(strt_prms);
         }
      prt_str("}", indent + IndentInc);

      if (maybe_str || maybe_var) {
         if (dflt == NULL) {
            /*
             * There is a branch around the switch statement. Output
             *  the label.
             */
            ForceNl();
            prt_str("L", 0);
            fprintf(out_file, "%d: ;  /* default */", dflt_lbl);
            }
         ForceNl();
         prt_str("}", indent + IndentInc);
         }
      ForceNl();
      }

   /*
    * Put ending parameter locations into effect.
    */
   mrg_prmloc(end_prms);
   ld_prmloc(end_prms);
   if (strt_prms != NULL)
      free(strt_prms);
   if (end_prms != NULL)
      free(end_prms);
   return fall_thru;
   }

/*
 * chk_conj - see if the left argument of a conjunction is an in-place
 *   conversion of a parameter other than a conversion to C_integer or
 *   C_double. If so issue a warning.
 */
static void chk_conj(n)
struct node *n;
   {
   struct node *cnv_type;
   struct node *src;
   struct node *dest;
   int typcd;

   if (n->nd_id == BinryNd && n->tok->tok_id == And)
      n = n->u[1].child;

   switch (n->nd_id) {
      case TrnryNd:
         /*
          * Must be Cnv.
          */
         cnv_type = n->u[0].child;
         src = n->u[1].child;
         dest = n->u[2].child;
         break;
      case QuadNd:
         /*
          * Must be Def.
          */
         cnv_type = n->u[0].child;
         src = n->u[1].child;
         dest = n->u[3].child;
         break;
      default:
         return;   /* not a  conversion */
      }

   /*
    * A conversion has been found. See if it meets the criteria for
    *  issuing a warning.
    */

   if (src->nd_id != SymNd || !(src->u[0].sym->id_type & DrfPrm))
      return;  /* not a dereferenced parameter */

   typcd = icn_typ(cnv_type);
   switch (typcd) {
      case TypCInt:
      case TypCDbl:
      case TypECInt:
         return;
      }

   if (dest != NULL)
      return;   /* not an in-place convertion */

   fprintf(stderr,
    "%s: file %s, line %d, warning: in-place conversion may or may not be\n",
      progname, cnv_type->tok->fname, cnv_type->tok->line);
   fprintf(stderr, "\tundone on subsequent failure.\n");
   }

/*
 * len_sel - translate a clause form a len_case statement into a C case
 *  clause. Return an indication of whether execution falls through the
 *  clause.
 */
static int len_sel(sel, strt_prms, end_prms, indent)
struct node *sel;
struct parminfo *strt_prms;
struct parminfo *end_prms;
int indent;
   {
   int fall_thru;

   prt_str("case ", indent);
   prt_tok(sel->tok, indent + IndentInc);           /* integer selection */
   prt_str(":", indent + IndentInc);
   fall_thru = rt_walk(sel->u[0].child, indent + IndentInc, 0);/* clause body */
   ForceNl();

   if (fall_thru) {
      prt_str("break;", indent + IndentInc);
      ForceNl();
      /*
       * Remember any changes to paramter locations caused by type conversions
       *  within the clause.
       */
      mrg_prmloc(end_prms);
      }

   ld_prmloc(strt_prms);
   return fall_thru;
   }

/*
 * rt_walk - walk the part of the syntax tree containing rtt code, producing
 *   code for the most-general version of the routine.
 */
static int rt_walk(n, indent, brace)
struct node *n;
int indent;
int brace;
   {
   struct token *t, *t1;
   struct node *n1, *errnum;
   int fall_thru;

   if (n == NULL)
      return 1;

   t =  n->tok;

   switch (n->nd_id) {
      case PrefxNd:
         switch (t->tok_id) {
            case '{':
               /*
                * RTL code: { <actions> }
                */
               if (brace)
                  tok_line(t, indent); /* just synch file name and line num */
               else
                  prt_tok(t, indent);  /* { */
               fall_thru = rt_walk(n->u[0].child, indent, 1);
               if (!brace)
                  prt_str("}", indent);
               return fall_thru;
            case '!':
               /*
                * RTL type-checking and conversions: ! <simple-type-check>
                */
               prt_tok(t, indent);
               rt_walk(n->u[0].child, indent, 0);
               return 1;
            case Body:
            case Inline:
               /*
                * RTL code: body { <c-code> }
                *           inline { <c-code> }
                */
               fall_thru = c_walk(n->u[0].child, indent, brace);
               if (!fall_thru)
                  clr_prmloc();
               return fall_thru;
            }
         break;
      case BinryNd:
         switch (t->tok_id) {
            case Runerr:
               /*
                * RTL code: runerr( <message-number> )
                *           runerr( <message-number>, <descriptor> )
                */
               prt_runerr(t, n->u[0].child, n->u[1].child, indent);

               /*
                * Execution cannot continue on this execution path.
                */
               clr_prmloc();
               return 0;
            case And:
               /*
                * RTL type-checking and conversions:
                *   <type-check> && <type_check>
                */
               chk_conj(n->u[0].child);  /* is a warning needed? */
               rt_walk(n->u[0].child, indent, 0);
               prt_str(" ", indent);
               prt_tok(t, indent);       /* && */
               prt_str(" ", indent);
               rt_walk(n->u[1].child, indent, 0);
               return 1;
            case Is:
               /*
                * RTL type-checking and conversions:
                *   is: <icon-type> ( <variable> )
                */
               typ_asrt(icn_typ(n->u[0].child), n->u[1].child,
                  n->u[0].child->tok, indent);
               return 1;
            }
         break;
      case ConCatNd:
         /*
          * "Glue" for two constructs.
          */
         fall_thru = rt_walk(n->u[0].child, indent, 0);
         return fall_thru & rt_walk(n->u[1].child, indent, 0);
      case AbstrNd:
         /*
          * Ignore abstract type computations while producing C code
          *  for library routines.
          */
         return 1;
      case TrnryNd:
         switch (t->tok_id) {
            case If: {
               /*
                * RTL code for "if" statements:
                *  if <type-check> then <action>
                *  if <type-check> then <action> else <action>
                *
                *  <type-check> may include parameter conversions that create
                *  new scoping. It is necessary to keep track of paramter
                *  types and locations along success and failure paths of
                *  these conversions. The "then" and "else" actions may
                *  also establish new scopes.
                */
               struct parminfo *then_prms = NULL;
               struct parminfo *else_prms;

               /*
                * Save the current parameter locations. These are in
                *  effect on the failure path of any type conversions
                *  in the condition of the "if".
                */
               else_prms = new_prmloc();
               sv_prmloc(else_prms);

               prt_tok(t, indent);       /* if */
               prt_str(" (", indent);
               n1 = n->u[0].child;
               rt_walk(n1, indent + IndentInc, 0);   /* type check */
               prt_str(") {", indent);

               /*
                * If the condition is negated, the failure path is to the "then"
                *  and the success path is to the "else".
                */
               if (n1->nd_id == PrefxNd && n1->tok->tok_id == '!') {
                  then_prms = else_prms;
                  else_prms = new_prmloc();
                  sv_prmloc(else_prms);
                  ld_prmloc(then_prms);
                  }

               /*
                * Then Clause.
                */
               fall_thru = rt_walk(n->u[1].child, indent + IndentInc, 1);
               ForceNl();
               prt_str("}", indent + IndentInc);

               /*
                * Determine if there is an else clause and merge parameter
                *  location information from the alternate paths through
                *  the statement.
                */
               n1 = n->u[2].child;
               if (n1 == NULL) {
                  if (fall_thru)
                     mrg_prmloc(else_prms);
                  ld_prmloc(else_prms);
                  fall_thru = 1;
                  }
               else {
                  if (then_prms == NULL)
                     then_prms = new_prmloc();
                  if (fall_thru)
                     sv_prmloc(then_prms);
                  ld_prmloc(else_prms);
                  ForceNl();
                  prt_str("else {", indent);
                  if (rt_walk(n1, indent + IndentInc, 1)) {  /* else clause */
                     fall_thru = 1;
                     mrg_prmloc(then_prms);
                     }
                  ForceNl();
                  prt_str("}", indent + IndentInc);
                  ld_prmloc(then_prms);
                  }
               ForceNl();
               if (then_prms != NULL)
                  free(then_prms);
               if (else_prms != NULL)
                  free(else_prms);
               }
               return fall_thru;
            case Len_case: {
               /*
                * RTL code:
                *   len_case <variable> of {
                *      <integer>: <action>
                *        ...
                *      default: <action>
                *      }
                */
               struct parminfo *strt_prms;
               struct parminfo *end_prms;

               /*
                * A case may contain parameter conversions that create new
                *  scopes. Remember the parameter locations at the start
                *  of the len_case statement.
                */
               strt_prms = new_prmloc();
               sv_prmloc(strt_prms);
               end_prms = new_prmloc();

               n1 = n->u[0].child;
               if (!(n1->u[0].sym->id_type & VArgLen))
	          errt1(t, "len_case must select on length of vararg");
               /*
                * The len_case statement is implemented as a C switch
                *  statement.
                */
               prt_str("switch (", indent);
               prt_var(n1, indent);
               prt_str(") {", indent);
               ForceNl();
               fall_thru = 0;
               for (n1 = n->u[1].child; n1->nd_id == ConCatNd;
                  n1 = n1->u[0].child)
                     fall_thru |= len_sel(n1->u[1].child, strt_prms, end_prms,
                        indent + IndentInc);
               fall_thru |= len_sel(n1, strt_prms, end_prms,
                  indent + IndentInc);

               /*
                * Handle default clause.
                */
               prt_str("default:", indent + IndentInc);
               ForceNl();
               fall_thru |= rt_walk(n->u[2].child, indent + 2 * IndentInc, 0);
               ForceNl();
               prt_str("}", indent + IndentInc);
               ForceNl();

               /*
                * Put into effect the location of parameters at the end
                *  of the len_case statement.
                */
               mrg_prmloc(end_prms);
               ld_prmloc(end_prms);
               if (strt_prms != NULL)
                  free(strt_prms);
               if (end_prms != NULL)
                  free(end_prms);
               }
               return fall_thru;
            case Type_case: {
               /*
                * RTL code:
                *   type_case <variable> of {
                *       <icon_type> : ... <icon_type> : <action>
                *          ...
                *       }
                *
                *   last clause may be: default: <action>
                */
               int maybe_var;
               struct node *var;
               struct sym_entry *sym;

               /*
                * If we can determine that the value being checked is
                *  not a variable reference, we don't have to produce code
                *  to check for that possibility.
                */
               maybe_var = 1;
               var = n->u[0].child;
               if (var->nd_id == SymNd) {
                  sym = var->u[0].sym;
                  switch(sym->id_type) {
                     case DrfPrm:
                     case OtherDcl:
                     case TndDesc:
                     case TndStr:
                     case RsltLoc:
                        if (sym->nest_lvl > 1) {
                           /*
                            * The thing being tested is either a
                            *  dereferenced parameter or a local
                            *  descriptor which could only have been
                            *  set by a conversion which does not
                            *  produce a variable reference.
                            */
                           maybe_var = 0;
                           }
                      }
                  }
               return typ_case(var, n->u[1].child, n->u[2].child, rt_walk,
                  maybe_var, indent);
               }
            case Cnv:
               /*
                * RTL code: cnv: <type> ( <source> )
                *           cnv: <type> ( <source> , <destination> )
                */
               cnv_fnc(t, icn_typ(n->u[0].child), n->u[1].child, NULL,
                  n->u[2].child, indent);
               return 1;
            case Arith_case: {
               /*
                * arith_case (<variable>, <variable>) of {
                *   C_integer: <statement>
                *   integer: <statement>
                *   C_double: <statement>
                *   }
                *
                * This construct does type conversions and provides
                *  alternate execution paths. It is necessary to keep
                *  track of parameter locations.
                */
               struct parminfo *strt_prms;
               struct parminfo *end_prms;
               struct parminfo *tmp_prms;

               strt_prms = new_prmloc();
               sv_prmloc(strt_prms);
               end_prms = new_prmloc();
               tmp_prms = new_prmloc();

               fall_thru = 0;

               n1 = n->u[2].child;   /* contains actions for the 3 cases */

               /*
                * Set up an error number node for use in runerr().
                */
               t1 = copy_t(t);
               t1->tok_id = IntConst;
               t1->image = "102";
               errnum = node0(PrimryNd, t1);

               /*
                * Try converting both arguments to a C_integer.
                */
               tok_line(t, indent);
               prt_str("if (", indent);
               cnv_fnc(t, TypECInt, n->u[0].child, NULL, NULL, indent);
               prt_str(" && ", indent);
               cnv_fnc(t, TypECInt, n->u[1].child, NULL, NULL, indent);
               prt_str(") ", indent);
               ForceNl();
               if (rt_walk(n1->u[0].child, indent + IndentInc, 0)) {
                  fall_thru |= 1;
                  mrg_prmloc(end_prms);
                  }
               ForceNl();

               /*
                * Try converting both arguments to an integer.
                */
               prt_str("#ifdef LargeInts", 0);
               ForceNl();
               ld_prmloc(strt_prms);
               tok_line(t, indent);
               prt_str("else if (", indent);
               cnv_fnc(t, TypEInt, n->u[0].child, NULL, NULL, indent);
               prt_str(" && ", indent);
               cnv_fnc(t, TypEInt, n->u[1].child, NULL, NULL, indent);
               prt_str(") ", indent);
               ForceNl();
               if (rt_walk(n1->u[1].child, indent + IndentInc, 0)) {
                  fall_thru |= 1;
                  mrg_prmloc(end_prms);
                  }
               ForceNl();
               prt_str("#endif\t\t\t\t\t/* LargeInts */", 0);
               ForceNl();

               /*
                * Try converting both arguments to a C_double
                */
               ld_prmloc(strt_prms);
               prt_str("else {", indent);
               ForceNl();
               tok_line(t, indent + IndentInc);
               prt_str("if (!", indent + IndentInc);
               cnv_fnc(t, TypCDbl, n->u[0].child, NULL, NULL,
                  indent + IndentInc);
               prt_str(")", indent + IndentInc);
               ForceNl();
               sv_prmloc(tmp_prms);   /* use original parm locs for error */
               ld_prmloc(strt_prms);
               prt_runerr(t, errnum, n->u[0].child, indent + 2 * IndentInc);
               ld_prmloc(tmp_prms);
               tok_line(t, indent + IndentInc);
               prt_str("if (!", indent + IndentInc);
               cnv_fnc(t, TypCDbl, n->u[1].child, NULL, NULL,
                  indent + IndentInc);
               prt_str(") ", indent + IndentInc);
               ForceNl();
               sv_prmloc(tmp_prms);   /* use original parm locs for error */
               ld_prmloc(strt_prms);
               prt_runerr(t, errnum, n->u[1].child, indent + 2 * IndentInc);
               ld_prmloc(tmp_prms);
               if (rt_walk(n1->u[2].child, indent + IndentInc, 0)) {
                  fall_thru |= 1;
                  mrg_prmloc(end_prms);
                  }
               ForceNl();
               prt_str("}", indent + IndentInc);
               ForceNl();

               ld_prmloc(end_prms);
               free(strt_prms);
               free(end_prms);
               free(tmp_prms);
               free_tree(errnum);
               return fall_thru;
               }
            }
      case QuadNd:
         /*
          * RTL code: def: <type> ( <source> , <default>)
          *           def: <type> ( <source> , <default> , <destination> )
          */
         cnv_fnc(t, icn_typ(n->u[0].child), n->u[1].child, n->u[2].child,
            n->u[3].child, indent);
         return 1;
      }
   /*NOTREACHED*/
   return 0;  /* avoid gcc warning */
   }

/*
 * spcl_dcls - print special declarations for tended variables, parameter
 *  conversions, and buffers.
 */
void spcl_dcls(op_params)
struct sym_entry *op_params; /* operation parameters or NULL */
   {
   register struct sym_entry *sym;
   struct sym_entry *sym1;

   /*
    * Output declarations for buffers and locations to hold conversions
    *  to C values.
    */
   spcl_start(op_params);

   /*
    * Determine if this operation takes a variable number of arguments.
    *  Use that information in deciding how large a tended array to
    *  declare.
    */
   varargs = (op_params != NULL && op_params->id_type & VarPrm);
   if (varargs)
      tend_ary(ntend + VArgAlwnc - 1);
   else
      tend_ary(ntend);

   if (varargs) {
      /*
       * This operation takes a variable number of arguments. A declaration
       *  for a tended array has been made that will usually hold them, but
       *  sometimes it is necessary to malloc() a tended array at run
       *  time. Produce code to check for this.
       */
      cur_impl->ret_flag |= DoesEFail;  /* error conversion from allocation */
      prt_str("struct tend_desc *r_tendp;", IndentInc);
      ForceNl();
      prt_str("int r_n;\n", IndentInc);
      ++line;
      ForceNl();
      prt_str("if (r_nargs <= ", IndentInc);
      fprintf(out_file, "%d)", op_params->u.param_info.param_num + VArgAlwnc);
      ForceNl();
      prt_str("r_tendp = (struct tend_desc *)&r_tend;", 2 * IndentInc);
      ForceNl();
      prt_str("else {", IndentInc);
      ForceNl();
      prt_str(
       "r_tendp = (struct tend_desc *)malloc((sizeof(struct tend_desc)",
         2 * IndentInc);
      ForceNl();
      prt_str("", 3 * IndentInc);
      fprintf(out_file, "+ (r_nargs + %d) * sizeof(struct descrip)));",
         ntend - 2 - op_params->u.param_info.param_num);
      ForceNl();
      prt_str("if (r_tendp == NULL) {", 2 * IndentInc);
      ForceNl();
      prt_str("err_msg(305, NULL);", 3 * IndentInc);
      ForceNl();
      prt_str("return A_Resume;", 3 * IndentInc);
      ForceNl();
      prt_str("}", 3 * IndentInc);
      ForceNl();
      prt_str("}", 2 * IndentInc);
      ForceNl();
      tendstrct = "(*r_tendp)";
      }
   else
      tendstrct = "r_tend";

   /*
    * Produce code to initialize the tended array. These are for tended
    *  declarations and parameters.
    */
   tend_init();  /* initializations for tended declarations. */
   if (varargs) {
      /*
       * This operation takes a variable number of arguments. Produce code
       *  to dereference or copy this into its portion of the tended
       *  array.
       */
      prt_str("for (r_n = ", IndentInc);
      fprintf(out_file, "%d; r_n < r_nargs; ++r_n)",
          op_params->u.param_info.param_num);
      ForceNl();
      if (op_params->id_type & DrfPrm) {
         prt_str("deref(&r_args[r_n], &", IndentInc * 2);
         fprintf(out_file, "%s.d[r_n + %d]);", tendstrct, ntend - 1 -
            op_params->u.param_info.param_num);
         }
      else {
         prt_str(tendstrct, IndentInc * 2);
         fprintf(out_file, ".d[r_n + %d] = r_args[r_n];", ntend - 1 -
            op_params->u.param_info.param_num);
         }
      ForceNl();
      sym = op_params->u.param_info.next;
      }
   else
      sym = op_params; /* no variable part of arg list */

   /*
    * Go through the fixed part of the parameter list, producing code
    *  to copy/dereference parameters into the tended array.
    */
   while (sym != NULL) {
      /*
       * A there may be identifiers for dereferenced and/or undereferenced
       *  versions of a paramater. If there are both, sym1 references the
       *  second identifier.
       */
      sym1 = sym->u.param_info.next;
      if (sym1 != NULL && sym->u.param_info.param_num !=
         sym1->u.param_info.param_num)
            sym1 = NULL;    /* the next entry is not for the same parameter */

      /*
       * If there are not enough arguments to supply a value for this
       *  parameter, set it to the null value.
       */
      prt_str("if (", IndentInc);
      fprintf(out_file, "r_nargs > %d) {", sym->u.param_info.param_num);
      ForceNl();
      parm_tnd(sym);
      if (sym1 != NULL) {
         ForceNl();
         parm_tnd(sym1);
         }
      ForceNl();
      prt_str("} else {", IndentInc);
      ForceNl();
      prt_str(tendstrct, IndentInc * 2);
      fprintf(out_file, ".d[%d].dword = D_Null;", sym->t_indx);
      if (sym1 != NULL) {
         ForceNl();
         prt_str(tendstrct, IndentInc * 2);
         fprintf(out_file, ".d[%d].dword = D_Null;", sym1->t_indx);
         }
      ForceNl();
      prt_str("}", 2 * IndentInc);
      ForceNl();
      if (sym1 == NULL)
         sym = sym->u.param_info.next;
      else
         sym = sym1->u.param_info.next;
      }

   /*
    * Finish setting up the tended array structure and link it into the tended
    *  list.
    */
   if (ntend != 0) {
      prt_str(tendstrct, IndentInc);
      if (varargs)
         fprintf(out_file, ".num = %d + Max(r_nargs - %d, 0);", ntend - 1,
            op_params->u.param_info.param_num);
      else
         fprintf(out_file, ".num = %d;", ntend);
      ForceNl();
      prt_str(tendstrct, IndentInc);
      prt_str(".previous = tend;", IndentInc);
      ForceNl();
      prt_str("tend = (struct tend_desc *)&", IndentInc);
      fprintf(out_file, "%s;", tendstrct);
      ForceNl();
      }
   }

/*
 * spcl_start - do initial work for outputing special declarations. Output
 *  declarations for buffers and locations to hold conversions to C values.
 *  Determine what tended locations are needed for parameters.
 */
static void spcl_start(op_params)
struct sym_entry *op_params;
   {
   ForceNl();
   if (n_tmp_str > 0) {
      prt_str("char r_sbuf[", IndentInc);
      fprintf(out_file, "%d][MaxCvtLen];", n_tmp_str);
      ForceNl();
      }
   if (n_tmp_cset > 0) {
      prt_str("struct b_cset r_cbuf[", IndentInc);
      fprintf(out_file, "%d];", n_tmp_cset);
      ForceNl();
      }
   if (tend_lst == NULL)
      ntend = 0;
   else
      ntend = tend_lst->t_indx + 1;
   parm_locs(op_params); /* see what parameter conversion there are */
   }

/*
 * tend_ary - write struct containing array of tended descriptors.
 */
static void tend_ary(n)
int n;
   {
   if (n == 0)
      return;
   prt_str("struct {", IndentInc);
   ForceNl();
   prt_str("struct tend_desc *previous;", 2 * IndentInc);
   ForceNl();
   prt_str("int num;", 2 * IndentInc);
   ForceNl();
   prt_str("struct descrip d[", 2 * IndentInc);
   fprintf(out_file, "%d];", n);
   ForceNl();
   prt_str("} r_tend;\n", 2 * IndentInc);
   ++line;
   ForceNl();
   }

/*
 * tend_init - produce code to initialize entries in the tended array
 *  corresponding to tended declarations. Default initializations are
 *  supplied when there is none in the declaration.
 */
static void tend_init()
   {
   register struct init_tend *tnd;

   for (tnd = tend_lst; tnd != NULL; tnd = tnd->next) {
      switch (tnd->init_typ) {
         case TndDesc:
            /*
             * Simple tended declaration.
             */
            prt_str(tendstrct, IndentInc);
            if (tnd->init == NULL)
               fprintf(out_file, ".d[%d].dword = D_Null;", tnd->t_indx);
            else {
               fprintf(out_file, ".d[%d] = ", tnd->t_indx);
               c_walk(tnd->init, 2 * IndentInc, 0);
               prt_str(";", 2 * IndentInc);
               }
            break;
         case TndStr:
            /*
             * Tended character pointer.
             */
            prt_str(tendstrct, IndentInc);
            if (tnd->init == NULL)
               fprintf(out_file, ".d[%d] = emptystr;", tnd->t_indx);
            else {
               fprintf(out_file, ".d[%d].dword = 0;", tnd->t_indx);
               ForceNl();
               prt_str(tendstrct, IndentInc);
               fprintf(out_file, ".d[%d].vword.sptr = ", tnd->t_indx);
               c_walk(tnd->init, 2 * IndentInc, 0);
               prt_str(";", 2 * IndentInc);
               }
            break;
         case TndBlk:
            /*
             * A tended block pointer of some kind.
             */
            prt_str(tendstrct, IndentInc);
            if (tnd->init == NULL)
               fprintf(out_file, ".d[%d] = nullptr;", tnd->t_indx);
            else {
               fprintf(out_file, ".d[%d].dword = F_Ptr | F_Nqual;",tnd->t_indx);
               ForceNl();
               prt_str(tendstrct, IndentInc);
               fprintf(out_file, ".d[%d].vword.bptr = (union block *)",
                   tnd->t_indx);
               c_walk(tnd->init, 2 * IndentInc, 0);
               prt_str(";", 2 * IndentInc);
               }
            break;
         }
      ForceNl();
      }
   }

/*
 * parm_tnd - produce code to put a parameter in its tended location.
 */
static void parm_tnd(sym)
struct sym_entry *sym;
   {
   /*
    * A parameter may either be dereferenced into its tended location
    *  or copied.
    */
   if (sym->id_type & DrfPrm) {
      prt_str("deref(&r_args[", IndentInc * 2);
      fprintf(out_file, "%d], &%s.d[%d]);", sym->u.param_info.param_num,
         tendstrct, sym->t_indx);
      }
   else {
      prt_str(tendstrct, IndentInc * 2);
      fprintf(out_file, ".d[%d] = r_args[%d];", sym->t_indx,
         sym->u.param_info.param_num);
      }
   }

/*
 * parm_locs - determine what locations are needed to hold parameters and
 *  their conversions. Produce declarations for the C_integer and C_double
 *  locations.
 */
static void parm_locs(op_params)
struct sym_entry *op_params;
   {
   struct sym_entry *next_parm;

   /*
    * Parameters are stored in reverse order: Recurse down the list
    *  and perform processing on the way back.
    */
   if (op_params == NULL)
      return;
   next_parm = op_params->u.param_info.next;
   parm_locs(next_parm);

   /*
    * For interpreter routines, extra tended descriptors are only needed
    *  when both dereferenced and undereferenced values are requested.
    */
   if (iconx_flg && (next_parm == NULL ||
      op_params->u.param_info.param_num != next_parm->u.param_info.param_num))
      op_params->t_indx = -1;
   else
      op_params->t_indx = ntend++;
   if (op_params->u.param_info.non_tend & PrmInt) {
      prt_str("C_integer r_i", IndentInc);
      fprintf(out_file, "%d;", op_params->u.param_info.param_num);
      ForceNl();
      }
   if (op_params->u.param_info.non_tend & PrmDbl) {
      prt_str("double r_d", IndentInc);
      fprintf(out_file, "%d;", op_params->u.param_info.param_num);
      ForceNl();
      }
   }

/*
 * real_def - see if a declaration really defines storage.
 */
static int real_def(n)
struct node *n;
   {
   struct node *dcl_lst;

   dcl_lst = n->u[1].child;
   /*
    * If no variables are being defined this must be a tag declaration.
    */
   if (dcl_lst == NULL)
      return 0;

   if (only_proto(dcl_lst))
      return 0;

   if (tdef_or_extr(n->u[0].child))
      return 0;

   return 1;
   }

/*
 * only_proto - see if this declarator list contains only function prototypes.
 */
static int only_proto(n)
struct node *n;
   {
   switch (n->nd_id) {
      case CommaNd:
         return only_proto(n->u[0].child) & only_proto(n->u[1].child);
      case ConCatNd:
         /*
          * Optional pointer.
          */
         return only_proto(n->u[1].child);
      case BinryNd:
         switch (n->tok->tok_id) {
            case '=':
               return only_proto(n->u[0].child);
            case '[':
               /*
                * At this point, assume array declarator is not part of
                *  prototype.
                */
               return 0;
            case ')':
               /*
                * Prototype (or forward declaration).
                */
               return 1;
            }
      case PrefxNd:
         /*
          * Parenthesized.
          */
         return only_proto(n->u[0].child);
      case PrimryNd:
         /*
          * At this point, assume it is not a prototype.
          */
         return 0;
      }
   err1("rtt internal error detected in function only_proto()");
   /*NOTREACHED*/
   return 0;  /* avoid gcc warning */
   }

/*
 * tdef_or_extr - see if this is a typedef or extern.
 */
static int tdef_or_extr(n)
struct node *n;
   {
   switch (n->nd_id) {
      case LstNd:
         return tdef_or_extr(n->u[0].child) | tdef_or_extr(n->u[1].child);
      case BinryNd:
         /*
          * struct, union, or enum.
          */
         return 0;
      case PrimryNd:
         if (n->tok->tok_id == Extern || n->tok->tok_id == Typedef)
            return 1;
         else
            return 0;
      }
   err1("rtt internal error detected in function tdef_or_extr()");
   /*NOTREACHED*/
   return 0;  /* avoid gcc warning */
   }

/*
 * dclout - output an ordinary global C declaration.
 */
void dclout(n)
struct node *n;
   {
   if (!enable_out)
      return;        /* output disabled */
   if (real_def(n))
      def_fnd = 1;   /* this declaration defines a run-time object */
   c_walk(n, 0, 0);
   free_tree(n);
   }

/*
 * fncout - output code for a C function.
 */
void fncout(head, prm_dcl, block)
struct node *head;
struct node *prm_dcl;
struct node *block;
   {
   if (!enable_out)
      return;       /* output disabled */

   def_fnd = 1;     /* this declaration defines a run-time object */

   nxt_sbuf = 0;    /* clear number of string buffers */
   nxt_cbuf = 0;    /* clear number of cset buffers */

   /*
    * Output the function header and the parameter declarations.
    */
   fnc_head = head;
   c_walk(head, 0, 0);
   prt_str(" ",  0);
   c_walk(prm_dcl, 0, 0);
   prt_str(" ", 0);

   /*
    * Handle outer block.
    */
   prt_tok(block->tok, IndentInc);          /* { */
   c_walk(block->u[0].child, IndentInc, 0); /* non-tended declarations */
   spcl_dcls(NULL);                         /* tended declarations */
   no_ret_val = 1;
   c_walk(block->u[2].child, IndentInc, 0); /* statement list */
   if (ntend != 0 && no_ret_val) {
      /*
       * This function contains no return statements with values, assume
       *  that the programmer is using the implicit return at the end
       *  of the function and update the tending of descriptors.
       */
      untend(IndentInc);
      }
   ForceNl();
   prt_str("}", IndentInc);
   ForceNl();

   /*
    * free storage.
    */
   free_tree(head);
   free_tree(prm_dcl);
   free_tree(block);
   pop_cntxt();
   clr_def();
   }

/*
 * defout - output operation definitions (except for constant keywords)
 */
void defout(n)
struct node *n;
   {
   struct sym_entry *sym, *sym1;

   if (!enable_out)
      return;       /* output disabled */

   nxt_sbuf = 0;
   nxt_cbuf = 0;

   /*
    * Somewhat different code is produced for the interpreter and compiler.
    */
   if (iconx_flg)
      interp_def(n);
   else
      comp_def(n);

   free_tree(n);
   /*
    * The declarations for the declare statement are not associated with
    *  any compound statement and must be freed here.
    */
   sym = dcl_stk->tended;
   while (sym != NULL) {
      sym1 = sym;
      sym = sym->u.tnd_var.next;
      free_sym(sym1);
      }
   while (decl_lst != NULL) {
      sym1 = decl_lst;
      decl_lst = decl_lst->u.declare_var.next;
      free_sym(sym1);
      }
   op_type = OrdFunc;
   pop_cntxt();
   clr_def();
   }

/*
 * comp_def - output code for the compiler for operation definitions.
 */
static void comp_def(n)
struct node *n;
   {
   #ifdef Rttx
      fprintf(stdout,
         "rtt was compiled to only support the interpreter, use -x\n");
      exit(EXIT_FAILURE);
   #else				/* Rttx */
   struct sym_entry *sym;
   struct node *n1;
   FILE *f_save;

   char buf1[5];
   char buf[MaxPath];
   char *cname;
   long min_result;
   long max_result;
   int ret_flag;
   int resume;
   char *name;
   char *s;

   f_save = out_file;

   /*
    * Note if the result location is explicitly referenced and note
    *  how it is accessed in the generated code.
    */
   cur_impl->use_rslt = sym_lkup(str_rslt)->u.referenced;
   rslt_loc = "(*r_rslt)";

   /*
    * In several contexts, letters are used to distinguish kinds of operations.
    */
   switch (op_type) {
      case TokFunction:
         lc_letter = 'f';
         uc_letter = 'F';
         break;
      case Keyword:
         lc_letter = 'k';
         uc_letter = 'K';
         break;
      case Operator:
         lc_letter = 'o';
         uc_letter = 'O';
      }
   prfx1 = cur_impl->prefix[0];
   prfx2 = cur_impl->prefix[1];

   if (op_type != Keyword) {
      /*
       * First pass through the operation: produce most general routine.
       */
      fnc_ret = RetSig;  /* most general routine always returns a signal */

      /*
       * Compute the file name in which to output the function.
       */
      sprintf(buf1, "%c_%c%c", lc_letter, prfx1, prfx2);
      cname = salloc(makename(buf, SourceDir, buf1, CSuffix));
      if ((out_file = fopen(cname, "w")) == NULL)
         err2("cannot open output file", cname);
      else
         addrmlst(cname, out_file);

      prologue(); /* output standard comments and preprocessor directives */

      /*
       * Output function header that corresponds to standard calling
       *  convensions. The function name is constructed from the letter
       *  for the operation type, the prefix that makes the function
       *  name unique, and the name of the operation.
       */
      fprintf(out_file, "int %c%c%c_%s(r_nargs, r_args, r_rslt, r_s_cont)\n",
         uc_letter, prfx1, prfx2, cur_impl->name);
      fprintf(out_file, "int r_nargs;\n");
      fprintf(out_file, "dptr r_args;\n");
      fprintf(out_file, "dptr r_rslt;\n");
      fprintf(out_file, "continuation r_s_cont;");
      fname = cname;
      line = 12;
      ForceNl();
      prt_str("{", IndentInc);
      ForceNl();

      /*
       * Output ordinary declarations from declare clause.
       */
      for (sym = decl_lst; sym != NULL; sym = sym->u.declare_var.next) {
         c_walk(sym->u.declare_var.tqual, IndentInc, 0);
         prt_str(" ", IndentInc);
         c_walk(sym->u.declare_var.dcltor, IndentInc, 0);
         if ((n1 = sym->u.declare_var.init) != NULL) {
            prt_str(" = ", IndentInc);
            c_walk(n1, IndentInc, 0);
            }
         prt_str(";", IndentInc);
         }

      /*
       * Output code for special declarations along with code to initial
       *  them. This includes buffers and tended locations for parameters
       *  and tended variables.
       */
      spcl_dcls(params);

      if (rt_walk(n, IndentInc, 0)) {  /* body of operation */
         if (n->nd_id == ConCatNd)
            s = n->u[1].child->tok->fname;
         else
            s = n->tok->fname;
         fprintf(stderr, "%s: file %s, warning: ", progname, s);
         fprintf(stderr, "execution may fall off end of operation \"%s\"\n",
             cur_impl->name);
         }

      ForceNl();
      prt_str("}\n", IndentInc);
      if (fclose(out_file) != 0)
         err2("cannot close ", cname);
      put_c_fl(cname, 1);  /* note name of output file for operation */
      }

   /*
    * Second pass through operation: produce in-line code and special purpose
    *  routines.
    */
   for (sym = params; sym != NULL; sym = sym->u.param_info.next)
      if (sym->id_type & DrfPrm)
         sym->u.param_info.cur_loc = PrmTend;  /* reset location of parameter */
   in_line(n);

   /*
    * Insure that the fail/return/suspend statements are consistent
    *  with the result sequence indicated.
    */
   min_result = cur_impl->min_result;
   max_result = cur_impl->max_result;
   ret_flag = cur_impl->ret_flag;
   resume = cur_impl->resume;
   name = cur_impl->name;
   if (min_result == NoRsltSeq && ret_flag & (DoesFail|DoesRet|DoesSusp))
      err2(name,
         ": result sequence of {}, but fail, return, or suspend present");
   if (min_result != NoRsltSeq && ret_flag == 0)
      err2(name,
         ": result sequence indicated, no fail, return, or suspend present");
   if (max_result != NoRsltSeq) {
      if (max_result == 0 && ret_flag & (DoesRet|DoesSusp))
         err2(name,
            ": result sequence of 0 length, but return or suspend present");
      if (max_result != 0 && !(ret_flag & (DoesRet | DoesSusp)))
         err2(name,
            ": result sequence length > 0, but no return or suspend present");
      if ((max_result == UnbndSeq || max_result > 1 || resume) &&
         !(ret_flag & DoesSusp))
         err2(name,
            ": result sequence indicates suspension, but no suspend present");
      if ((max_result != UnbndSeq && max_result <= 1 && !resume) &&
         ret_flag & DoesSusp)
         err2(name,
            ": result sequence indicates no suspension, but suspend present");
      }
   if (min_result != NoRsltSeq && max_result != UnbndSeq &&
      min_result > max_result)
      err2(name, ": minimum result sequence length greater than maximum");

   out_file = f_save;
#endif					/* Rttx */
   }

/*
 * interp_def - output code for the interpreter for operation definitions.
 */
static void interp_def(n)
struct node *n;
   {
   struct sym_entry *sym;
   struct node *n1;
   int nparms;
   int has_underef;
   char letter;
   char *name;
   char *s;

   /*
    * Note how result location is accessed in generated code.
    */
   rslt_loc = "r_args[0]";

   /*
    * Determine if the operation has any undereferenced parameters.
    */
   has_underef = 0;
   for (sym = params; sym != NULL; sym = sym->u.param_info.next)
      if (sym->id_type  & RtParm) {
         has_underef = 1;
         break;
         }

   /*
    * Determine the nuber of parameters. A negative value is used
    *  to indicate an operation that takes a variable number of
    *  arguments.
    */
   if (params == NULL)
      nparms = 0;
   else {
      nparms = params->u.param_info.param_num + 1;
      if (params->id_type & VarPrm)
         nparms = -nparms;
      }

   fnc_ret = RetSig;  /* interpreter routine always returns a signal */
   name = cur_impl->name;

   /*
    * Determine what letter is used to prefix the operation name.
    */
   switch (op_type) {
      case TokFunction:
         letter = 'Z';
         break;
      case Keyword:
         letter = 'K';
         break;
      case Operator:
         letter = 'O';
         }

   fprintf(out_file, "\n");
   if (op_type != Keyword) {
      /*
       * Output prototype. Operations taking a variable number of arguments
       *   have an extra parameter: the number of arguments.
       */
      fprintf(out_file, "int %c%s (", letter, name);
      if (params != NULL && (params->id_type & VarPrm))
         fprintf(out_file, "int r_nargs, ");
      fprintf(out_file, "dptr r_args);\n");
      ++line;

      /*
       * Output procedure block.
       */
      switch (op_type) {
         case TokFunction:
            fprintf(out_file, "FncBlock(%s, %d, %d)\n\n", name, nparms,
               (has_underef ? -1 : 0));
            ++line;
            break;
         case Operator:
            if (strcmp(cur_impl->op,"\\") == 0)
               fprintf(out_file, "OpBlock(%s, %d, \"%s\", 0)\n\n", name, nparms,
                  "\\\\");
            else
               fprintf(out_file, "OpBlock(%s, %d, \"%s\", 0)\n\n", name, nparms,
                  cur_impl->op);
            ++line;
         }
      }

   /*
    * Output function header. Operations taking a variable number of arguments
    *   have an extra parameter: the number of arguments.
    */
   fprintf(out_file, "int %c%s(", letter, name);
   if (params != NULL && (params->id_type & VarPrm))
      fprintf(out_file, "r_nargs, ");
   fprintf(out_file, "r_args)\n");
   ++line;
   if (params != NULL && (params->id_type & VarPrm)) {
      fprintf(out_file, "int r_nargs;\n");
      ++line;
      }
   fprintf(out_file, "dptr r_args;");
   ++line;
   ForceNl();
   prt_str("{", IndentInc);

   /*
    * Output ordinary declarations from the declare clause.
    */
   ForceNl();
   for (sym = decl_lst; sym != NULL; sym = sym->u.declare_var.next) {
      c_walk(sym->u.declare_var.tqual, IndentInc, 0);
      prt_str(" ", IndentInc);
      c_walk(sym->u.declare_var.dcltor, IndentInc, 0);
      if ((n1 = sym->u.declare_var.init) != NULL) {
         prt_str(" = ", IndentInc);
         c_walk(n1, IndentInc, 0);
         }
      prt_str(";", IndentInc);
      }

   /*
    * Output special declarations and initial processing.
    */
   tendstrct = "r_tend";
   spcl_start(params);
   tend_ary(ntend);
   if (has_underef && params != NULL && params->id_type == (VarPrm | DrfPrm))
      prt_str("int r_n;\n", IndentInc);
   tend_init();

   /*
    * See which parameters need to be dereferenced. If all are dereferenced,
    *  it is done by before the routine is called.
    */
   if (has_underef) {
      sym = params;
      if (sym != NULL && sym->id_type & VarPrm) {
         if (sym->id_type & DrfPrm) {
            /*
             * There is a variable part of the parameter list and it
             *  must be dereferenced.
             */
            prt_str("for (r_n = ", IndentInc);
            fprintf(out_file, "%d; r_n <= r_nargs; ++r_n)",
                sym->u.param_info.param_num + 1);
            ForceNl();
            prt_str("Deref(r_args[r_n]);", IndentInc * 2);
            ForceNl();
            }
         sym = sym->u.param_info.next;
         }

      /*
       * Produce code to dereference any fixed parameters that need to be.
       */
      while (sym != NULL) {
         if (sym->id_type & DrfPrm) {
            /*
             * Tended index of -1 indicates that the parameter can be
             *  dereferened in-place (this is the usual case).
             */
            if (sym->t_indx == -1) {
               prt_str("Deref(r_args[", IndentInc * 2);
               fprintf(out_file, "%d]);", sym->u.param_info.param_num + 1);
               }
            else {
               prt_str("deref(&r_args[", IndentInc * 2);
               fprintf(out_file, "%d], &r_tend.d[%d]);",
                  sym->u.param_info.param_num + 1, sym->t_indx);
               }
            }
         ForceNl();
         sym = sym->u.param_info.next;
         }
      }

   /*
    * Finish setting up the tended array structure and link it into the tended
    *  list.
    */
   if (ntend != 0) {
      prt_str("r_tend.num = ", IndentInc);
      fprintf(out_file, "%d;", ntend);
      ForceNl();
      prt_str("r_tend.previous = tend;", IndentInc);
      ForceNl();
      prt_str("tend = (struct tend_desc *)&r_tend;", IndentInc);
      ForceNl();
      }

   if (rt_walk(n, IndentInc, 0)) { /* body of operation */
      if (n->nd_id == ConCatNd)
         s = n->u[1].child->tok->fname;
      else
         s = n->tok->fname;
      fprintf(stderr, "%s: file %s, warning: ", progname, s);
      fprintf(stderr, "execution may fall off end of operation \"%s\"\n",
          cur_impl->name);
      }
   ForceNl();
   prt_str("}\n", IndentInc);
   }

/*
 * keyconst - produce code for a constant keyword.
 */
void keyconst(t)
struct token *t;
   {
   struct il_code *il;
   int n;

   if (iconx_flg) {
      /*
       * For the interpreter, output a C function implementing the keyword.
       */
      rslt_loc = "r_args[0]";  /* result location */

      fprintf(out_file, "\n");
      fprintf(out_file, "int K%s(r_args)\n", cur_impl->name);
      fprintf(out_file, "dptr r_args;");
      line += 2;
      ForceNl();
      prt_str("{", IndentInc);
      ForceNl();
      switch (t->tok_id) {
         case StrLit:
            prt_str(rslt_loc, IndentInc);
            prt_str(".vword.sptr = \"", IndentInc);
            n = prt_i_str(out_file, t->image, (int)strlen(t->image));
            prt_str("\";", IndentInc);
            ForceNl();
            prt_str(rslt_loc, IndentInc);
            fprintf(out_file, ".dword = %d;", n);
            break;
         case CharConst:
            prt_str("static struct b_cset cset_blk = ", IndentInc);
            cset_init(out_file, bitvect(t->image, (int)strlen(t->image)));
            ForceNl();
            prt_str(rslt_loc, IndentInc);
            prt_str(".dword = D_Cset;", IndentInc);
            ForceNl();
            prt_str(rslt_loc, IndentInc);
            prt_str(".vword.bptr = (union block *)&cset_blk;", IndentInc);
            break;
         case DblConst:
            prt_str("static struct b_real real_blk = {T_Real, ", IndentInc);
            fprintf(out_file, "%s};", t->image);
            ForceNl();
            prt_str(rslt_loc, IndentInc);
            prt_str(".dword = D_Real;", IndentInc);
            ForceNl();
            prt_str(rslt_loc, IndentInc);
            prt_str(".vword.bptr = (union block *)&real_blk;", IndentInc);
            break;
         case IntConst:
            prt_str(rslt_loc, IndentInc);
            prt_str(".dword = D_Integer;", IndentInc);
            ForceNl();
            prt_str(rslt_loc, IndentInc);
            prt_str(".vword.integr = ", IndentInc);
            prt_str(t->image, IndentInc);
            prt_str(";", IndentInc);
            break;
         }
      ForceNl();
      prt_str("return A_Continue;", IndentInc);
      ForceNl();
      prt_str("}\n", IndentInc);
      ++line;
      ForceNl();
      }
   else {
      /*
       * For the compiler, make an entry in the data base for the keyword.
       */
      cur_impl->use_rslt = 0;

      il = new_il(IL_Const, 2);
      switch (t->tok_id) {
         case StrLit:
            il->u[0].n = str_typ;
            il->u[1].s = alloc(strlen(t->image) + 3);
            sprintf(il->u[1].s, "\"%s\"", t->image);
            break;
         case CharConst:
            il->u[0].n = cset_typ;
            il->u[1].s = alloc(strlen(t->image) + 3);
            sprintf(il->u[1].s, "'%s'", t->image);
            break;
         case DblConst:
            il->u[0].n = real_typ;
            il->u[1].s = t->image;
            break;
         case IntConst:
            il->u[0].n = int_typ;
            il->u[1].s = t->image;
            break;
         }
      cur_impl->in_line = il;
      }

   /*
    * Reset the translator and free storage.
    */
   op_type = OrdFunc;
   free_t(t);
   pop_cntxt();
   clr_def();
   }

/*
 * keepdir - A preprocessor directive to be kept has been encountered.
 *   If it is #passthru, print just the body of the directive, otherwise
 *   print the whole thing.
 */
void keepdir(t)
struct token *t;
   {
   char *s;

   tok_line(t, 0);
   s = t->image;
   if (strncmp(s, "#passthru", 9) == 0)
      s = s + 10;
   fprintf(out_file, "%s\n", s);
   line += 1;
   }

/*
 * prologue - print standard comments and preprocessor directives at the
 *   start of an output file.
 */
void prologue()
   {
   id_comment(out_file);
   fprintf(out_file, "%s", compiler_def);
   fprintf(out_file, "#include \"%s\"\n\n", inclname);
   }
