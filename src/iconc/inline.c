/*
 * inline.c - routines to put run-time routines in-line.
 */
#include "../h/gsupport.h"
#include "ctrans.h"
#include "ccode.h"
#include "csym.h"
#include "ctree.h"
#include "cproto.h"
#include "cglobals.h"

/*
 * Prototypes for static functions.
 */
static void          arth_arg  ( struct il_code *var,
                                    struct val_loc *v_orig, int chk,
                                    struct code *cnv);
static int              body_fnc  (struct il_code *il);
static void          chkforblk (void);
static void          cnv_dest  (int loc, int is_cstr,
                                    struct il_code *src, int sym_indx,
                                    struct il_c *dest, struct code *cd, int i);
static void          dwrd_asgn (struct val_loc *vloc, char *typ);
static struct il_c     *line_ilc  (struct il_c *ilc);
static int              gen_if    (struct code *cond_cd,
                                    struct il_code *il_then,
                                    struct il_code *il_else,
                                    struct val_loc **locs);
static int              gen_il    (struct il_code *il);
static void          gen_ilc   (struct il_c *il);
static void          gen_ilret (struct il_c *ilc);
static int              gen_tcase (struct il_code *il, int has_dflt);
static void          il_var    (struct il_code *il, struct code *cd,
                                    int indx);
static void          mrg_locs  (struct val_loc **locs);
static struct code     *oper_lbl  (char *s);
static void          part_asgn (struct val_loc *vloc, char *asgn,
                                    struct il_c *value);
static void          rstr_locs (struct val_loc **locs);
static struct val_loc **sav_locs  (void);
static void         sub_ilc    (struct il_c *ilc, struct code *cd, int indx);

/*
 * There are many parameters that are shared by multiple routines. There
 *  are copied into statics.
 */
static struct val_loc *rslt;     /* result location */
static struct code **scont_strt; /* label following operation code */
static struct code **scont_fail; /* resumption label for in-line suspend */
static struct c_fnc *cont;       /* success continuation */
static struct implement *impl;   /* data base entry for operation */
static int nsyms;                /* number symbols in operation symbol table */
static int n_vararg;             /* size of variable part of arg list */
static nodeptr intrnl_lftm;      /* lifetime of internal variables */
static struct val_loc **tended;  /* array of tended locals */

/*
 * gen_inlin - generate in-line code for an operation.
 */
void gen_inlin(il, r, strt, fail, c, ip, ns, st, n, dcl_var, n_va)
struct il_code *il;
struct val_loc *r;
struct code **strt;
struct code **fail;
struct c_fnc *c;
struct implement *ip;
int ns;
struct op_symentry *st;
nodeptr n;
int dcl_var;
int n_va;
   {
   struct code *cd;
   struct val_loc *tnd;
   int i;

   /*
    * Copy arguments in to globals.
    */
   rslt = r;
   scont_strt = strt;
   scont_fail = fail;
   cont = c;
   impl = ip;
   nsyms = ns;
   cur_symtab = st;
   intrnl_lftm = n->intrnl_lftm;
   cur_symtyps = n->symtyps;
   n_vararg = n_va;

   /*
    * Generate code to initialize local tended descriptors and determine
    *  how to access the descriptors.
    */
   for (i = 0; i < impl->ntnds; ++i) {
      if (cur_symtab[dcl_var].n_refs + cur_symtab[dcl_var].n_mods > 0) {
         tnd = chk_alc(NULL, n->intrnl_lftm);
          switch (impl->tnds[i].var_type) {
             case TndDesc:
                cur_symtab[dcl_var].loc = tnd; 
                break;
             case TndStr:
                cd = alc_ary(2);
                cd->ElemTyp(0) = A_ValLoc;
                cd->ValLoc(0) =                tnd;
                cd->ElemTyp(1) = A_Str;
                cd->Str(1) =                   " = emptystr;";
                cd_add(cd);
                cur_symtab[dcl_var].loc = loc_cpy(tnd, M_CharPtr);
                break;
             case TndBlk:
                cd = alc_ary(2);
                cd->ElemTyp(0) = A_ValLoc;
                cd->ValLoc(0) =                tnd;
                cd->ElemTyp(1) = A_Str;
                cd->Str(1) =                   " = nullptr;"; 
                cd_add(cd);
                cur_symtab[dcl_var].loc = loc_cpy(tnd, M_BlkPtr);
                cur_symtab[dcl_var].loc->blk_name = impl->tnds[i].blk_name;
                break;
             }
         if (impl->tnds[i].init != NULL) {
             cd = alc_ary(4);
             cd->ElemTyp(0) = A_ValLoc;
             cd->ValLoc(0) =                cur_symtab[dcl_var].loc;
             cd->ElemTyp(1) = A_Str;
             cd->Str(1) =                   " = "; 
             sub_ilc(impl->tnds[i].init, cd, 2);
             cd->ElemTyp(3) = A_Str;
             cd->Str(3) =                   ";"; 
             cd_add(cd);
             }
         }
      ++dcl_var;
      }

   /*
    * If there are local non-tended variables, generate code for the
    *  declarations, placing everything in braces.
     */
   if (impl->nvars > 0) {
      cd = NewCode(0);
      cd->cd_id = C_LBrack;    /* { */
      cd_add(cd);
      for (i = 0;  i < impl->nvars; ++i) {
         if (cur_symtab[dcl_var].n_refs + cur_symtab[dcl_var].n_mods > 0) {
            gen_ilc(impl->vars[i].dcl);
            cur_symtab[dcl_var].loc = cvar_loc(impl->vars[i].name);
            }
         ++dcl_var;
         }
      }

   gen_il(il);  /* generate executable code */

   if (impl->nvars > 0) {
      cd = NewCode(0);
      cd->cd_id = C_RBrack;    /* } */
      cd_add(cd);
      }
   }

/*
 * gen_il - generate code from a sub-tree of in-line code from the data
 *  base. Determine if execution can continue past this code.
 *
 */
static int gen_il(il)
struct il_code *il;
   {
   struct code *cd;
   struct code *cd1;
   struct il_code *il_cond;
   struct il_code *il_then;
   struct il_code *il_else;
   struct il_code *il_t;
   struct val_loc **locs;
   struct val_loc **locs1;
   struct val_loc *tnd;
   int fall_thru;
   int cond;
   int ncases;
   int indx;
   int ntended;
   int i;

   if (il == NULL)
      return 1;

   switch (il->il_type) {
      case IL_Const: /* should have been replaced by literal node */
         return 1;

      case IL_If1:
      case IL_If2:
         /*
          * if-then or if-then-else statement.
          */
         il_then = il->u[1].fld;
         if (il->il_type == IL_If2)
            il_else = il->u[2].fld;
         else
            il_else = NULL;
         il_cond = il->u[0].fld;
         if (il->u[0].fld->il_type == IL_Bang) {
            il_cond = il_cond->u[0].fld;
            il_t = il_then;
            il_then = il_else;
            il_else = il_t;
            }
         locs = sav_locs();
         cond = cond_anlz(il_cond, &cd1);
         if (cond == (MaybeTrue | MaybeFalse))
            fall_thru = gen_if(cd1, il_then, il_else, locs);
         else {
            if (cd1 != NULL) {
               cd_add(cd1);  /* condition contains needed conversions */
               cd = alc_ary(1);
               cd->ElemTyp(0) = A_Str;
               cd->Str(0) =              ";";
               cd_add(cd);
               }
            if (cond == MaybeTrue)
               fall_thru = gen_il(il_then);
            else if (cond == MaybeFalse) {
               locs1 = sav_locs();
               rstr_locs(locs);
               locs = locs1;
               fall_thru = gen_il(il_else);
               }
            mrg_locs(locs);
            }
         return fall_thru;

      case IL_Tcase1:
         /*
          * type_case statement with no default clause.
          */
         return gen_tcase(il, 0);

      case IL_Tcase2:
         /*
          * type_case statement with a default clause.
          */
         return gen_tcase(il, 1);

      case IL_Lcase:
         /*
          * len_case statement. Determine which case matches the number
          *  of arguments.
          */
         ncases = il->u[0].n;
         indx = 1;
         for (i = 0; i < ncases; ++i) {
            if (il->u[indx++].n == n_vararg)    /* selection number */
               return gen_il(il->u[indx].fld);  /* action */
            ++indx;
            }
         return gen_il(il->u[indx].fld);        /* default */

      case IL_Acase: {
         /*
          * arith_case statement.
          */
         struct il_code *var1;
         struct il_code *var2;
         struct val_loc *v_orig1;
         struct val_loc *v_orig2;
         struct code *cnv1;
         struct code *cnv2;
         int maybe_int;
         int maybe_dbl;
         int chk1;
         int chk2;

         var1 = il->u[0].fld;
         var2 = il->u[1].fld;
         v_orig1 = cur_symtab[var1->u[0].n].loc;  /* remember for error msgs */
         v_orig2 = cur_symtab[var2->u[0].n].loc;  /* remember for error msgs */
         arth_anlz(var1, var2, &maybe_int, &maybe_dbl, &chk1, &cnv1,
           &chk2, &cnv2);

         /*
          * This statement is in-lined if there is only C integer
          *  arithmetic, only C double arithmetic, or only a run-time
          *  error.
          */
         arth_arg(var1, v_orig1, chk1, cnv1);
         arth_arg(var2, v_orig2, chk2, cnv2);
         if (maybe_int)
            return gen_il(il->u[2].fld);     /* C_integer action */
         else if (maybe_dbl)
            return gen_il(il->u[4].fld);     /* C_double action */
         else
            return 0;
         }

      case IL_Err1:
         /*
          * runerr() with no offending value.
          */
         cd = alc_ary(3);
         cd->ElemTyp(0) = A_Str;
         cd->Str(0) =              "err_msg(";
         cd->ElemTyp(1) = A_Intgr;
         cd->Intgr(1) =            il->u[0].n;
         cd->ElemTyp(2) = A_Str;
         cd->Str(2) =              ", NULL);";
         cd_add(cd);
         if (err_conv)
            cd_add(sig_cd(on_failure, cur_fnc));
         for (i = 0; i < nsyms; ++i)
            cur_symtab[i].loc = NULL;
         return 0;

      case IL_Err2:
         /*
          * runerr() with an offending value. Note the reference to
          *  the offending value descriptor.
          */
         cd = alc_ary(5);
         cd->ElemTyp(0) = A_Str;
         cd->Str(0) =              "err_msg(";
         cd->ElemTyp(1) = A_Intgr;
         cd->Intgr(1) =            il->u[0].n;
         cd->ElemTyp(2) = A_Str;
         cd->Str(2) =              ", &(";
         il_var(il->u[1].fld, cd, 3);
         cd->ElemTyp(4) = A_Str;
         cd->Str(4) =              "));";
         cd_add(cd);
         if (err_conv)
            cd_add(sig_cd(on_failure, cur_fnc));
         for (i = 0; i < nsyms; ++i)
            cur_symtab[i].loc = NULL;
         return 0;

      case IL_Lst:
         /*
          * Two consecutive pieces of RTL code.
          */
         fall_thru = gen_il(il->u[0].fld);
         if (fall_thru)
            fall_thru = gen_il(il->u[1].fld);
         return fall_thru;

      case IL_Block:
         /*
          * inline {...} statement.
          *
          *  Allocate and initialize any tended locals.
          */
         ntended = il->u[1].n;
         if (ntended > 0)
            tended = (struct val_loc **)alloc((unsigned int)
               sizeof(struct val_loc *) * ntended);
         for (i = 2; i - 2 < ntended; ++i) {
             tnd = chk_alc(NULL, intrnl_lftm);
             tended[i - 2] = tnd;
             switch (il->u[i].n) {
                case TndDesc:
                   break;
                case TndStr:
                   cd = alc_ary(2);
                   cd->ElemTyp(0) = A_ValLoc;
                   cd->ValLoc(0) =                tnd;
                   cd->ElemTyp(1) = A_Str;
                   cd->Str(1) =                   " = emptystr;";
                   cd_add(cd);
                   break;
                case TndBlk:
                   cd = alc_ary(2);
                   cd->ElemTyp(0) = A_ValLoc;
                   cd->ValLoc(0) =                tnd;
                   cd->ElemTyp(1) = A_Str;
                   cd->Str(1) =                   " = nullptr;"; 
                   cd_add(cd);
                   break;
                }
            }
         gen_ilc(il->u[i].c_cd);    /* body of block */
         /*
          * See if execution can fall through this code.
          */
         if (il->u[0].n)
            return 1;
         else {
            for (i = 0; i < nsyms; ++i)
               cur_symtab[i].loc = NULL;
            return 0;
            }

      case IL_Call:
         /*
          * call to body function.
          */
         return body_fnc(il);

      case IL_Abstr:
         /*
          * abstract type computation. Only used by type inference.
          */
         return 1;

      default:
         fprintf(stderr, "compiler error: unknown info in data base\n");
         exit(1);
         /* NOTREACHED */
      }
   }

/*
 * arth_arg - in-line code to check a conversion for an arith_case statement.
 */
static void arth_arg(var, v_orig, chk, cnv)
struct il_code *var;
struct val_loc *v_orig;
int chk;
struct code *cnv;
   {
   struct code *lbl;
   struct code *cd;

   if (chk) {
      /*
       * Must check the conversion.
       */
      lbl = oper_lbl("converted");
      cd_add(lbl);
      cur_fnc->cursor = lbl->prev;        /* code goes before label */
      if (cnv != NULL) {
         cd = NewCode(2);
         cd->cd_id = C_If;
         cd->Cond = cnv;
         cd->ThenStmt = mk_goto(lbl);
         cd_add(cd);
         }
      cd = alc_ary(3);
      cd->ElemTyp(0) = A_Str;
      cd->Str(0) =              "err_msg(102, &(";
      cd->ElemTyp(1) = A_ValLoc;
      cd->ValLoc(1) =           v_orig;    /* var location before conversion */
      cd->ElemTyp(2) = A_Str;
      cd->Str(2) =              "));";
      cd_add(cd);
      if (err_conv)
         cd_add(sig_cd(on_failure, cur_fnc));
      cur_fnc->cursor = lbl;
      }
   else if (cnv != NULL) {
      cd_add(cnv);          /* conversion cannot fail  */
      cd = alc_ary(1);
      cd->ElemTyp(0) = A_Str;
      cd->Str(0) =              ";";
      cd_add(cd);
      }
   }

/*
 * body_fnc - generate code to call a body function.
 */
static int body_fnc(il)
struct il_code *il;
   {
   struct code *arg_lst;
   struct code *cd;
   struct c_fnc *cont1;
   char *oper_nm;
   int ret_val;
   int ret_flag;
   int need_rslt;
   int num_sbuf;
   int num_cbuf;
   int expl_args;
   int arglst_sz;  /* size of arg list in number of code pieces */
   int il_indx;
   int cd_indx;
   int proto_prt;
   int i;

   /*
    * Determine if a function prototype has been printed yet for this
    *  body function.
    */
   proto_prt = il->u[0].n;
   il->u[0].n = 1;

   /*
    * Construct the name of the body function.
    */
   oper_nm = (char *)alloc((unsigned int)(strlen(impl->name) + 6));
   sprintf(oper_nm, "%c%c%c%c_%s", impl->oper_typ, impl->prefix[0],
      impl->prefix[1], (char)il->u[1].n, impl->name);

   /*
    * Extract from the call the flags and other information describing
    *  the function, then use this information to deduce the arguments
    *  needed by the function.
    */
   ret_val = il->u[2].n;
   ret_flag = il->u[3].n;
   need_rslt = il->u[4].n;
   num_sbuf = il->u[5].n;
   num_cbuf = il->u[6].n;
   expl_args = il->u[7].n;

   /*
    * determine how large the argument list is.
    */
   arglst_sz = 2 * expl_args - 1;
   if (num_sbuf > 0)
      arglst_sz += 3;
   if (num_cbuf > 0)
      arglst_sz += 2;
   if (need_rslt)
      arglst_sz += 3;
   if (arglst_sz > 0)
      arg_lst = alc_ary(arglst_sz);
   else
      arg_lst = alc_ary(0);

   if (!proto_prt) {
      /*
       * Determine whether the body function returns a C integer, double,
       *  no value, or a signal.
       */
      switch (ret_val) {
         case RetInt:
            fprintf(inclfile, "C_integer %s (", oper_nm);
            break;
         case RetDbl:
            fprintf(inclfile, "double %s (", oper_nm);
            break;
         case RetNoVal:
            fprintf(inclfile, "void %s (", oper_nm);
            break;
         case RetSig:
            fprintf(inclfile, "int %s (", oper_nm);
            break;
         }
     }

   /*
    * Produce prototype and code for the explicit arguments in the
    *  function call. Note that the call entry contains C code for both.
    */
   il_indx = 8;
   cd_indx = 0;
   while (expl_args--) {
      if (cd_indx > 0) {
         /*
          * Not first entry, precede by ','.
          */
         arg_lst->ElemTyp(cd_indx) = A_Str;                /* , */
         arg_lst->Str(cd_indx) = ", ";
         if (!proto_prt)
            fprintf(inclfile, ", ");
         ++cd_indx;
         }
      if (!proto_prt)
        fprintf(inclfile, "%s", il->u[il_indx].c_cd->s);   /* parameter dcl */
      ++il_indx;
      sub_ilc(il->u[il_indx++].c_cd, arg_lst, cd_indx++);
      }

   /*
    * If string buffers are needed, allocate them and pass pointer to
    *   function.
    */
   if (num_sbuf > 0) {
      if (cd_indx > 0) {
         /*
          * Not first entry, precede by ','.
          */
         arg_lst->ElemTyp(cd_indx) = A_Str;        /* , */
         arg_lst->Str(cd_indx) = ", ";
         if (!proto_prt)
            fprintf(inclfile, ", ");
         ++cd_indx;
         }
      arg_lst->ElemTyp(cd_indx) = A_Str;
      arg_lst->Str(cd_indx) = "(char (*)[MaxCvtLen])";
      ++cd_indx;
      arg_lst->ElemTyp(cd_indx) = A_SBuf;
      arg_lst->Intgr(cd_indx) = alc_sbufs(num_sbuf, intrnl_lftm);
      if (!proto_prt)
         fprintf(inclfile, "char (*r_sbuf)[MaxCvtLen]");
      ++cd_indx;
      }

   /*
    * If cset buffers are needed, allocate them and pass pointer to
    *   function.
    */
   if (num_cbuf > 0) {
      if (cd_indx > 0) {
         /*
          * Not first entry, precede by ','.
          */
         arg_lst->ElemTyp(cd_indx) = A_Str;        /* , */
         arg_lst->Str(cd_indx) = ", ";
         if (!proto_prt)
            fprintf(inclfile, ", ");
         ++cd_indx;
         }
      arg_lst->ElemTyp(cd_indx) = A_CBuf;
      arg_lst->Intgr(cd_indx) = alc_cbufs(num_cbuf, intrnl_lftm);
      if (!proto_prt)
         fprintf(inclfile, "struct b_cset *r_cbuf");
      ++cd_indx;
      }

   /*
    * See if the function needs a pointer to the result location
    *  of the operation.
    */
   if (need_rslt) {
      if (cd_indx > 0) {
         /*
          * Not first entry, precede by ','.
          */
         arg_lst->ElemTyp(cd_indx) = A_Str;        /* , */
         arg_lst->Str(cd_indx) = ", ";
         if (!proto_prt)
            fprintf(inclfile, ", ");
         ++cd_indx;
         }
      arg_lst->ElemTyp(cd_indx) = A_Str;        /* location of result */
      arg_lst->Str(cd_indx) = "&";
      ++cd_indx;
      arg_lst->ElemTyp(cd_indx) = A_ValLoc;
      arg_lst->ValLoc(cd_indx) = rslt;
      if (!proto_prt)
         fprintf(inclfile, "dptr rslt");
      ++cd_indx;
      }

   if (!proto_prt) {
      /*
       * The last possible argument is the success continuation.
       *  If there are no arguments, indicate this in the prototype.
       */
      if (ret_flag & DoesSusp) {
         if (cd_indx > 0)
            fprintf(inclfile, ", ");
         fprintf(inclfile, "continuation succ_cont");
         }
      else if (cd_indx == 0)
         fprintf(inclfile, "void");
      fprintf(inclfile, ");\n");
      }

   /*
    * Does this call need the success continuation for the operation.
    */
   if (ret_flag & DoesSusp)
       cont1 = cont;
   else
       cont1 = NULL;

   switch (ret_val) {
      case RetInt:
         /*
          * The body function returns a C integer.
          */
         cd = alc_ary(6);
         cd->ElemTyp(0) = A_ValLoc;
         cd->ValLoc(0) =               rslt;
         cd->ElemTyp(1) = A_Str;
         cd->Str(1) =                  ".vword.integr = ";
         cd->ElemTyp(2) = A_Str;
         cd->Str(2) =                  oper_nm;
         cd->ElemTyp(3) = A_Str;
         cd->Str(3) =                  "(";
         cd->ElemTyp(4) = A_Ary;
         cd->Array(4) =                  arg_lst;
         cd->ElemTyp(5) = A_Str;
         cd->Str(5) =                  ");";
         cd_add(cd);
         dwrd_asgn(rslt, "Integer");
         cd_add(mk_goto(*scont_strt));
         break;
      case RetDbl:
         /*
          * The body function returns a C double.
          */
         cd = alc_ary(6);
         cd->ElemTyp(0) = A_ValLoc;
         cd->ValLoc(0) =               rslt;
         cd->ElemTyp(1) = A_Str;
         cd->Str(1) =                  ".vword.bptr = (union block *)alcreal(";
         cd->ElemTyp(2) = A_Str;
         cd->Str(2) =                  oper_nm;
         cd->ElemTyp(3) = A_Str;
         cd->Str(3) =                  "(";
         cd->ElemTyp(4) = A_Ary;
         cd->Array(4) =                  arg_lst;
         cd->ElemTyp(5) = A_Str;
         cd->Str(5) =                  "));";
         cd_add(cd);
         dwrd_asgn(rslt, "Real");
         chkforblk();    /* make sure the block allocation succeeded */
         cd_add(mk_goto(*scont_strt));
         break;
      case RetNoVal:
         /*
          * The body function does not directly return a value.
          */
         cd = alc_ary(4);
         cd->ElemTyp(0) = A_Str;
         cd->Str(0) =                  oper_nm;
         cd->ElemTyp(1) = A_Str;
         cd->Str(1) =                  "(";
         cd->ElemTyp(2) = A_Ary;
         cd->Array(2) =                  arg_lst;
         cd->ElemTyp(3) = A_Str;
         cd->Str(3) =                  ");";
         cd_add(cd);
         if (ret_flag & DoesFail | (err_conv && (ret_flag & DoesEFail)))
            cd_add(sig_cd(on_failure, cur_fnc));
         else if (ret_flag & DoesRet)
            cd_add(mk_goto(*scont_strt));
         break;
      case RetSig:
         /*
          * The body function returns a signal.
          */
         callo_add(oper_nm, ret_flag, cont1, 0, arg_lst, mk_goto(*scont_strt));
         break;
      }
   /*
    * See if execution can fall through this call.
    */
   if (ret_flag & DoesFThru)
      return 1;
   else {
      for (i = 0; i < nsyms; ++i)
         cur_symtab[i].loc = NULL;
      return 0;
      }
   }


/*
 * il_var - generate code for a possibly subscripted variable into
 *   an element of a code array.
 */
static void il_var(il, cd, indx)
struct il_code *il;
struct code *cd;
int indx;
   {
   struct code *cd1;

   if (il->il_type == IL_Subscr) {
      /*
       * Subscripted variable.
       */
      cd1 = cd;
      cd = alc_ary(4);
      cd1->ElemTyp(indx) = A_Ary;
      cd1->Array(indx) = cd;
      indx = 0;
      cd->ElemTyp(1) = A_Str;
      cd->Str(1) =                  "[";
      cd->ElemTyp(2) = A_Intgr;
      cd->Intgr(2) =                il->u[1].n;
      cd->ElemTyp(3) = A_Str;
      cd->Str(3) =                  "]";
      }

   /*
    * See if this is the result location of the operation or an ordinary
    *  variable.
    */
   cd->ElemTyp(indx) = A_ValLoc;
   if (il->u[0].n == RsltIndx)
      cd->ValLoc(indx) = rslt;
   else
      cd->ValLoc(indx) = cur_symtab[il->u[0].n].loc;
   }

/*
 * part_asgn - generate code for an assignment to (part of) a descriptor.
 */
static void part_asgn(vloc, asgn, value)
struct val_loc *vloc;
char *asgn;
struct il_c *value;
   {
   struct code *cd;

   cd = alc_ary(4);
   cd->ElemTyp(0) = A_ValLoc;
   cd->ValLoc(0) =                          vloc;
   cd->ElemTyp(1) = A_Str;
   cd->Str(1) =                             asgn;
   sub_ilc(value, cd, 2);                /* value */
   cd->ElemTyp(3) = A_Str;
   cd->Str(3) =                             ";";
   cd_add(cd);
   }

/*
 * dwrd_asgn - generate code to assign a type code to the dword of a descriptor.
 */
static void dwrd_asgn(vloc, typ)
struct val_loc *vloc;
char *typ;
   {
   struct code *cd;

   cd = alc_ary(4);
   cd->ElemTyp(0) = A_ValLoc;
   cd->ValLoc(0) =               vloc;
   cd->ElemTyp(1) = A_Str;
   cd->Str(1) =                  ".dword = D_";
   cd->ElemTyp(2) = A_Str;
   cd->Str(2) =                  typ;
   cd->ElemTyp(3) = A_Str;
   cd->Str(3) =                  ";";
   cd_add(cd);
   }

/*
 * sub_ilc - generate code from a sequence of C code and place it
 *  in a slot in a code array.
 */
static void sub_ilc(ilc, cd, indx)
struct il_c *ilc;
struct code *cd;
int indx;
   {
   struct il_c *ilc1;
   struct code *cd1;
   int n;

   /*
    * Count the number of pieces of C code to process.
    */
   n = 0;
   for (ilc1 = ilc; ilc1 != NULL; ilc1 = ilc1->next)
      ++n;

   /*
    * If there is only one piece of code, place it directly in the 
    *  slot of the array. Otherwise allocate a sub-array and place it
    *  in the slot.
    */
   if (n > 1) {
      cd1 = cd;
      cd = alc_ary(n);
      cd1->ElemTyp(indx) = A_Ary;
      cd1->Array(indx) = cd;
      indx = 0;
      }

   while (ilc != NULL) {
      switch (ilc->il_c_type) {
         case ILC_Ref:
         case ILC_Mod:
            /*
             * Reference to variable in symbol table.
             */
            cd->ElemTyp(indx) = A_ValLoc;
            if (ilc->n == RsltIndx)
               cd->ValLoc(indx) = rslt;
            else {
               if (ilc->s == NULL)
                  cd->ValLoc(indx) = cur_symtab[ilc->n].loc;
               else {
                  /*
                   * Access the entire descriptor.
                   */
                  cd->ValLoc(indx) = loc_cpy(cur_symtab[ilc->n].loc, M_None);
                  }
               }
            break;

         case ILC_Tend:
            /*
             * Reference to a tended variable.
             */
            cd->ElemTyp(indx) = A_ValLoc;
            cd->ValLoc(indx) = tended[ilc->n];
            break;

         case ILC_Str:
            /*
             * String representing C code.
             */
            cd->ElemTyp(indx) = A_Str;
            cd->Str(indx) = ilc->s;
            break;

         case ILC_SBuf:
            /*
             * String buffer for a conversion.
             */
            cd->ElemTyp(indx) = A_SBuf;
            cd->Intgr(indx) = alc_sbufs(1, intrnl_lftm);
            break;

         case ILC_CBuf:
            /*
             * Cset buffer for a conversion.
             */
            cd->ElemTyp(indx) = A_CBuf;
            cd->Intgr(indx) = alc_cbufs(1, intrnl_lftm);
            break;


         default:
            fprintf(stderr, "compiler error: unknown info in data base\n");
            exit(1);
         }
      ilc = ilc->next;
      ++indx;
      }

   }

/*
 * gen_ilret - generate code to set the result value from a suspend or
 *   return.
 */
static void gen_ilret(ilc)
struct il_c *ilc;
   {
   struct il_c *ilc0;
   struct code *cd;
   char *cap_id;
   int typcd;

   if (rslt == &ignore)
      return;    /* Don't bother computing the result; it's never used */

   ilc0 = ilc->code[0];
   typcd = ilc->n;

   if (typcd < 0) {
      /*
       * RTL returns that do not look like function calls to standard Icon
       *  type name.
       */
      switch (typcd) {
         case TypCInt:
            /*
             * return/suspend C_integer <expr>;
             */
            part_asgn(rslt, ".vword.integr = ", ilc0);
            dwrd_asgn(rslt, "Integer");
            break;
         case TypCDbl:
            /*
             * return/suspend C_double <expr>;
             */
            cd = alc_ary(4);
            cd->ElemTyp(0) = A_ValLoc;
            cd->ValLoc(0) =             rslt;
            cd->ElemTyp(1) = A_Str;
            cd->Str(1) =                ".vword.bptr = (union block *)alcreal(";
            sub_ilc(ilc0, cd, 2);       /* value */
            cd->ElemTyp(3) = A_Str;
            cd->Str(3) =                ");";
            cd_add(cd);
            dwrd_asgn(rslt, "Real");
            chkforblk();    /* make sure the block allocation succeeded */
            break;
         case TypCStr:
            /*
             * return/suspend C_string <expr>;
             */
            cd = alc_ary(5);
            cd->ElemTyp(0) = A_Str;
            cd->Str(0) =                  "AsgnCStr(";
            cd->ElemTyp(1) = A_ValLoc;
            cd->ValLoc(1) =               rslt;
            cd->ElemTyp(2) = A_Str;
            cd->Str(2) =                  ", ";
            sub_ilc(ilc0, cd, 3);         /* <expr> */
            cd->ElemTyp(4) = A_Str;
            cd->Str(4) =                  ");";
            cd_add(cd);
            break;
         case RetDesc:
            /*
             * return/suspend <expr>;
             */
            part_asgn(rslt, " = ", ilc0);
            break;
         case RetNVar:
            /*
             * return/suspend named_var(<desc-pntr>);
             */
            part_asgn(rslt, ".vword.descptr = ", ilc0);
            dwrd_asgn(rslt, "Var");
            break;
         case RetSVar:
            /*
             * return/suspend struct_var(<desc-pntr>, <block_pntr>);
             */
            part_asgn(rslt, ".vword.descptr = (dptr)", ilc->code[1]);
            cd = alc_ary(6);
            cd->ElemTyp(0) = A_ValLoc;
            cd->ValLoc(0) =               rslt;
            cd->ElemTyp(1) = A_Str;
            cd->Str(1) =                  ".dword = D_Var + ((word *)";
            sub_ilc(ilc0, cd, 2);       /* value */
            cd->ElemTyp(3) = A_Str;
            cd->Str(3) =                  " - (word *)";
            cd->ElemTyp(4) = A_ValLoc;
            cd->ValLoc(4) =               rslt;
            cd->ElemTyp(5) = A_Str;
            cd->Str(5) =                  ".vword.descptr);";
            cd_add(cd);
            break;
         case RetNone:
            /*
             * return/suspend result;
             *
             *  Result already set, do nothing.
             */
            break;
         default:
            fprintf(stderr,
               "compiler error: unknown RLT return in data base\n");
            exit(1);
            /* NOTREACHED */
         }
      }
   else {
      /*
       * RTL returns that look like function calls to standard Icon type
       *  names.
       */
      cap_id = icontypes[typcd].cap_id;
      switch (icontypes[typcd].rtl_ret) {
         case TRetBlkP:
            /*
             * return/suspend <type>(<block-pntr>);
             */
            part_asgn(rslt, ".vword.bptr = (union block *)", ilc0);
            dwrd_asgn(rslt, cap_id);
            break;
         case TRetDescP:
            /*
             * return/suspend <type>(<descriptor-pntr>);
             */
            part_asgn(rslt, ".vword.descptr = (dptr)", ilc0);
            dwrd_asgn(rslt, cap_id);
            break;
         case TRetCharP:
            /*
             * return/suspend <type>(<char-pntr>);
             */
            part_asgn(rslt, ".vword.sptr = (char *)", ilc0);
            dwrd_asgn(rslt, cap_id);
            break;
         case TRetCInt:
            /*
             * return/suspend <type>(<integer>);
             */
            part_asgn(rslt, ".vword.integr = (word)", ilc0);
            dwrd_asgn(rslt, cap_id);
            break;
         case TRetSpcl:
            /*
             * RTL returns that look like function calls to standard type
             *  names but take more than one argument.
             */
            if (typcd == str_typ) {
                /*
                * return/suspend string(<len>, <char-pntr>);
                */
               part_asgn(rslt, ".vword.sptr = ", ilc->code[1]);
               part_asgn(rslt, ".dword = ", ilc0);
               }
            else if (typcd == stv_typ) {
               /*
                * return/suspend substr(<desc-pntr>, <start>, <len>);
                */
               cd = alc_ary(9);
               cd->ElemTyp(0) = A_Str;
               cd->Str(0) =                  "SubStr(&";
               cd->ElemTyp(1) = A_ValLoc;
               cd->ValLoc(1) =               rslt;
               cd->ElemTyp(2) = A_Str;
               cd->Str(2) =                  ", ";
               sub_ilc(ilc0, cd, 3);
               cd->ElemTyp(4) = A_Str;
               cd->Str(4) =                  ", ";
               sub_ilc(ilc->code[2], cd, 5);
               cd->ElemTyp(6) = A_Str;
               cd->Str(6) =                  ", ";
               sub_ilc(ilc->code[1], cd, 7);
               cd->ElemTyp(8) = A_Str;
               cd->Str(8) =                  ");";
               cd_add(cd);
               chkforblk();    /* make sure the block allocation succeeded */
               }
            else {
               fprintf(stderr,
                  "compiler error: unknown RLT return in data base\n");
               exit(1);
               /* NOTREACHED */
               }
            break;
         default:
            fprintf(stderr,
               "compiler error: unknown RLT return in data base\n");
            exit(1);
            /* NOTREACHED */
         }
      }
   }

/*
 * chkforblk - generate code to make sure the allocation of a block
 *   for the result descriptor was successful.
 */
static void chkforblk()
   {
   struct code *cd;
   struct code *cd1;
   struct code *lbl;

   lbl = alc_lbl("got allocation", 0);
   cd_add(lbl);
   cur_fnc->cursor = lbl->prev;        /* code goes before label */
   cd = NewCode(2);
   cd->cd_id = C_If;
   cd1 = alc_ary(3);
   cd1->ElemTyp(0) = A_Str;
   cd1->Str(0) =                  "(";
   cd1->ElemTyp(1) = A_ValLoc;
   cd1->ValLoc(1) =               rslt;
   cd1->ElemTyp(2) = A_Str;
   cd1->Str(2) =                  ").vword.bptr != NULL";
   cd->Cond = cd1;
   cd->ThenStmt = mk_goto(lbl);
   cd_add(cd);
   cd = alc_ary(1);
   cd->ElemTyp(0) = A_Str;
   cd->Str(0) =                   "err_msg(307, NULL);";
   cd_add(cd);
   if (err_conv)
      cd_add(sig_cd(on_failure, cur_fnc));
   cur_fnc->cursor = lbl;
   }

/*
 * gen_ilc - generate code for an sequence of in-line C code.
 */
static void gen_ilc(ilc)
struct il_c *ilc;
   {
   struct il_c *ilc1;
   struct code *cd;
   struct code *cd1;
   struct code *lbl1;
   struct code *fail_sav;
   struct code **lbls;
   int max_lbl;
   int i;

   /*
    * Determine how many labels there are in the code and allocate an
    *  array to map from label numbers to labels in the code.
    */
   max_lbl = -1;
   for (ilc1 = ilc; ilc1 != NULL; ilc1 = ilc1->next) {
      switch(ilc1->il_c_type) {
         case ILC_CGto:
         case ILC_Goto:
         case ILC_Lbl:
            if (ilc1->n > max_lbl)
               max_lbl = ilc1->n;
         }
      }
   ++max_lbl;    /* adjust for 0 indexing */
   if (max_lbl > 0) {
      lbls = (struct code **)alloc((unsigned int) sizeof(struct code *) *
         max_lbl);
      for (i = 0; i < max_lbl; ++i)
         lbls[i] = NULL;
      }

   while (ilc != NULL) {
      switch(ilc->il_c_type) {
         case ILC_Ref:
         case ILC_Mod:
         case ILC_Tend:
         case ILC_SBuf:
         case ILC_CBuf:
         case ILC_Str:
            /*
             * The beginning of a sequence of code fragments that can be
             *  place on one line.
             */
            ilc = line_ilc(ilc);
            break;

         case ILC_Fail:
            /*
             * fail - perform failure action.
             */
            cd_add(sig_cd(on_failure, cur_fnc));
            break;

         case ILC_EFail:
            /*
             * errorfail - same as fail if error conversion is supported.
             */
            if (err_conv)
               cd_add(sig_cd(on_failure, cur_fnc));
            break;

         case ILC_Ret:
            /*
             * return - set result location and jump out of operation.
             */
            gen_ilret(ilc);
            cd_add(mk_goto(*scont_strt));
            break;

         case ILC_Susp:
            /*
             * suspend - set result location. If there is a success
             *  continuation, call it. Otherwise the "continuation"
             *  will be generated in-line, so set up a resumption label.
             */
            gen_ilret(ilc);
            if (cont == NULL)
               *scont_strt = cur_fnc->cursor;
            lbl1 = oper_lbl("end suspend");
            cd_add(lbl1);
            if (cont == NULL)
               *scont_fail = lbl1;
            else {
               cur_fnc->cursor = lbl1->prev; 
               fail_sav = on_failure;
               on_failure = lbl1;
               callc_add(cont);
               on_failure = fail_sav;
               cur_fnc->cursor = lbl1;
               }
            break;

         case ILC_LBrc:
            /*
             * non-deletable '{'
             */
            cd = NewCode(0);
            cd->cd_id = C_LBrack;
            cd_add(cd);
            break;

         case ILC_RBrc:
            /*
             * non-deletable '}'
             */
            cd = NewCode(0);
            cd->cd_id = C_RBrack;
            cd_add(cd);
            break;

         case ILC_CGto:
            /*
             * Conditional goto.
             */
            i = ilc->n;
            if (lbls[i] == NULL)
               lbls[i] = oper_lbl("within");
            cd = NewCode(2);
            cd->cd_id = C_If;
            cd1 = alc_ary(1);
            sub_ilc(ilc->code[0], cd1, 0);
            cd->Cond = cd1;
            cd->ThenStmt = mk_goto(lbls[i]);
            cd_add(cd);
            break;

         case ILC_Goto:
            /*
             * Goto.
             */
            i = ilc->n;
            if (lbls[i] == NULL)
               lbls[i] = oper_lbl("within");
            cd_add(mk_goto(lbls[i]));
            break;

         case ILC_Lbl:
            /*
             * Label.
             */
            i = ilc->n;
            if (lbls[i] == NULL)
               lbls[i] = oper_lbl("within");
            cd_add(lbls[i]);
            break;

         default:
            fprintf(stderr, "compiler error: unknown info in data base\n");
            exit(1);
         }
      ilc = ilc->next;
      }

   if (max_lbl > 0)
      free((char *)lbls);
   }

/*
 * line_ilc - gather a line of in-line code.
 */
static struct il_c *line_ilc(ilc)
struct il_c *ilc;
   {
   struct il_c *ilc1;
   struct il_c *last;
   struct code *cd;
   int n;
   int i;

   /*
    * Count the number of pieces in the line. Determine the last
    *  piece in the sequence; this is returned to the caller.
    */
   n = 0;
   ilc1 = ilc;
   while (ilc1 != NULL) {
      switch(ilc1->il_c_type) {
         case ILC_Ref:
         case ILC_Mod:
         case ILC_Tend:
         case ILC_SBuf:
         case ILC_CBuf:
         case ILC_Str:
            ++n;
            last = ilc1;
            ilc1 = ilc1->next;
            break;
         default: 
            ilc1 = NULL;
         }
      }

   /*
    * Construct the line.
    */
   cd = alc_ary(n);
   for (i = 0; i < n; ++i) {
      switch(ilc->il_c_type) {
         case ILC_Ref:
         case ILC_Mod:
            /*
             * Reference to variable in symbol table.
             */
            cd->ElemTyp(i) = A_ValLoc;
            if (ilc->n == RsltIndx)
               cd->ValLoc(i) = rslt;
            else
               cd->ValLoc(i) = cur_symtab[ilc->n].loc;
            break;

         case ILC_Tend:
            /*
             * Reference to a tended variable.
             */
            cd->ElemTyp(i) = A_ValLoc;
            cd->ValLoc(i) = tended[ilc->n];
            break;

         case ILC_SBuf:
            /*
             * String buffer for a conversion.
             */
            cd->ElemTyp(i) = A_SBuf;
            cd->Intgr(i) = alc_sbufs(1, intrnl_lftm);
            break;

         case ILC_CBuf:
            /*
             * Cset buffer for a conversion.
             */
            cd->ElemTyp(i) = A_CBuf;
            cd->Intgr(i) = alc_cbufs(1, intrnl_lftm);
            break;

         case ILC_Str:
            /*
             * String representing C code.
             */
            cd->ElemTyp(i) = A_Str;
            cd->Str(i) = ilc->s;
            break;

         default:
            ilc = NULL;  
         }
      ilc = ilc->next;
      }

   cd_add(cd);
   return last;
   }

/*
 * generate code to perform simple type checking.
 */
struct code *typ_chk(var, typcd)
struct il_code *var;
int typcd;
   {
   struct code *cd;

   if (typcd == int_typ && largeints) {
      /*
       * Handle large integer support specially.
       */
      cd = alc_ary(5);
      cd->ElemTyp(0) = A_Str;
      cd->Str(0) =                           "((";
      il_var(var, cd, 1);                    /* value */
      cd->ElemTyp(2) = A_Str;
      cd->Str(2) =                           ").dword == D_Integer || (";
      il_var(var, cd, 3);                    /* value */
      cd->ElemTyp(4) = A_Str;
      cd->Str(4) =                           ").dword == D_Lrgint)";
      return cd;
      }
   else if (typcd < 0) {
      /*
       * Not a standard Icon type name.
       */
      cd = alc_ary(3);
      cd->ElemTyp(0) = A_Str;
      switch (typcd) {
         case TypVar:
            cd->Str(0) =                     "(((";
            il_var(var, cd, 1);              /* value */
            cd->ElemTyp(2) = A_Str;
            cd->Str(2) =                     ").dword & D_Var) == D_Var)";
            break;
         case TypCInt:
            cd->Str(0) =                     "((";
            il_var(var, cd, 1);              /* value */
            cd->ElemTyp(2) = A_Str;
            cd->Str(2) =                     ").dword == D_Integer)";
            break;
         }
      }
   else if (typcd == str_typ) {
      cd = alc_ary(3);
      cd->ElemTyp(0) = A_Str;
      cd->Str(0) =                           "(!((";
      il_var(var, cd, 1);                    /* value */
      cd->ElemTyp(2) = A_Str;
      cd->Str(2) =                           ").dword & F_Nqual))";
      }
   else {
      cd = alc_ary(5);
      cd->ElemTyp(0) = A_Str;
      cd->Str(0) =                           "((";
      il_var(var, cd, 1);                    /* value */
      cd->ElemTyp(2) = A_Str;
      cd->Str(2) =                           ").dword == D_";
      cd->ElemTyp(3) = A_Str;
      cd->Str(3) = icontypes[typcd].cap_id;  /* type name */
      cd->ElemTyp(4) = A_Str;
      cd->Str(4) =                           ")";
      }

   return cd;
   }

/*
 * oper_lbl - generate a label with an associated comment that includes
 *   the operation name.
 */
static struct code *oper_lbl(s)
char *s;
   {
   char *sbuf;

   sbuf = (char *)alloc((unsigned int)(strlen(s) + strlen(impl->name) + 3));
   sprintf(sbuf, "%s: %s", s, impl->name);
   return alc_lbl(sbuf, 0);
   }

/*
 * sav_locs - save the current interpretation of symbols that may
 *  be affected by conversions.
 */
static struct val_loc **sav_locs()
   {
   struct val_loc **locs;
   int i;

   if (nsyms == 0)
      return NULL;

   locs = (struct val_loc **)alloc((unsigned int)(nsyms *
      sizeof(struct val_loc *)));
   for (i = 0; i < nsyms; ++i)
      locs[i] = cur_symtab[i].loc;
   return locs;
   }

/*
 * rstr_locs - restore the interpretation of symbols that may
 *  have been affected by conversions.
 */
static void rstr_locs(locs)
struct val_loc **locs;
   {
   int i;

   for (i = 0; i < nsyms; ++i)
      cur_symtab[i].loc = locs[i];
   free((char *)locs);
   }

/*
 * mrg_locs - merge the interpretations of symbols along two execution
 *  paths. Any ambiguity is caught by rtt, so differences only occur
 *  if one path involves program termination so that the symbols
 *  no longer have an interpretation along that path.
 */
static void mrg_locs(locs)
struct val_loc **locs;
   {
   int i;

   for (i = 0; i < nsyms; ++i)
      if (cur_symtab[i].loc == NULL)
         cur_symtab[i].loc = locs[i];
   free((char *)locs);
   }

/*
 * il_cnv - generate code for an in-line conversion.
 */
struct code *il_cnv(typcd, src, dflt, dest)
int typcd;
struct il_code *src;
struct il_c *dflt;
struct il_c *dest;
   {
   struct code *cd;
   struct code *cd1;
   int dflt_to_ptr;
   int loc;
   int is_cstr;
   int sym_indx;
   int n;
   int i;

   sym_indx = src->u[0].n;

   /*
    * Determine whether the address must be taken of a default value and
    *  whether the interpretation of the symbol in an in-place conversion
    *  changes.
    */
   dflt_to_ptr = 0;
   loc = PrmTend;
   is_cstr = 0;
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
      case TypEInt:
         break;
      case TypTStr:
      case TypTCset:
         dflt_to_ptr = 1;
         break;
      default:
         /*
          * Cset, real, integer, or string
          */
         if (typcd == cset_typ || typcd == str_typ)
            dflt_to_ptr = 1;
         break;
      }

  if (typcd == TypCDbl && !(eval_is(real_typ, sym_indx) & MaybeFalse)) {
     /*
      * Conversion from Icon real to C double. Just copy the C value
      *  from the block.
      */
     cd = alc_ary(5);
     cd->ElemTyp(0) = A_Str;
     cd->Str(0) =                "(GetReal(&(";
     il_var(src, cd, 1);
     cd->ElemTyp(2) = A_Str;
     cd->Str(2) =                "), ";
     cnv_dest(loc, is_cstr, src, sym_indx, dest, cd, 3);
     cd->ElemTyp(4) = A_Str;
     cd->Str(4) =                "), 1)";
     }
  else if (typcd == TypCDbl && !largeints &&
     !(eval_is(int_typ, sym_indx) & MaybeFalse)) {
     /*
      * Conversion from Icon integer (not large integer) to C double.
      *  Do as a C conversion by an assigment.
      */
     cd = alc_ary(5);
     cd->ElemTyp(0) = A_Str;
     cd->Str(0) =                "(";
     cd->ElemTyp(2) = A_Str;
     cd->Str(2) =                " = IntVal( ";
     cd->ElemTyp(4) = A_Str;
     cd->Str(4) =                "), 1)";
     /*
      * Note that cnv_dest() must be called after the source is output
      *  in case it changes the location of the parameter.
      */
     il_var(src, cd, 3);
     cnv_dest(loc, is_cstr, src, sym_indx, dest, cd, 1);
     }
   else {
      /*
       * Compute the number of code fragments required to construct the
       *  call to the conversion routine.
       */
      n = 7;
      if (dflt != NULL)
        n += 2;
   
      cd = alc_ary(n);
   
      /*
       * The names of simple conversions are distinguished from defaulting
       *  conversions by a prefix of "cnv_" or "def_".
       */
      cd->ElemTyp(0) = A_Str;
      if (dflt == NULL)
         cd->Str(0) = "cnv_";
      else
         cd->Str(0) = "def_";
   
      /*
       * Determine the name of the conversion routine.
       */
      cd->ElemTyp(1) = A_Str;    /* may be overridden */
      switch (typcd) {
         case TypCInt:
            cd->Str(1) = "c_int(&(";
            break;
         case TypCDbl:
            cd->Str(1) = "c_dbl(&(";
            break;
         case TypCStr:
            cd->Str(1) = "c_str(&(";
            break;
         case TypEInt:
            cd->Str(1) = "eint(&(";
            break;
         case TypECInt:
            cd->Str(1) = "ec_int(&(";
            break;
         case TypTStr:
            /*
             * Allocate a string buffer.
             */
            cd1 = alc_ary(3);
            cd1->ElemTyp(0) = A_Str;
            cd1->Str(0) = "tstr(";
            cd1->ElemTyp(1) = A_SBuf;
            cd1->Intgr(1) = alc_sbufs(1, intrnl_lftm);
            cd1->ElemTyp(2) = A_Str;
            cd1->Str(2) = ", (&";
            cd->ElemTyp(1) = A_Ary;
            cd->Array(1) = cd1;
            break;
         case TypTCset:
            /*
             * Allocate a cset buffer.
             */
            cd1 = alc_ary(3);
            cd1->ElemTyp(0) = A_Str;
            cd1->Str(0) = "tcset(";
            cd1->ElemTyp(1) = A_CBuf;
            cd1->Intgr(1) = alc_cbufs(1, intrnl_lftm);
            cd1->ElemTyp(2) = A_Str;
            cd1->Str(2) = ", &(";
            cd->ElemTyp(1) = A_Ary;
            cd->Array(1) = cd1;
            break;
         default:
            /*
             * Cset, real, integer, or string
             */
            if (typcd == cset_typ)
               cd->Str(1) = "cset(&(";
            else if (typcd == real_typ) 
               cd->Str(1) = "real(&(";
            else if (typcd == int_typ) 
               cd->Str(1) = "int(&(";
            else if (typcd == str_typ)
               cd->Str(1) = "str(&(";
            break;
         }
   
      il_var(src, cd, 2);
   
      cd->ElemTyp(3) = A_Str;
      if (dflt != NULL && dflt_to_ptr)
         cd->Str(3) = "), &(";
      else
         cd->Str(3) = "), ";
   
   
      /*
       * Determine if this conversion has a default value.
       */
      i = 4;
      if (dflt != NULL) {
         sub_ilc(dflt, cd, i);
         ++i;
         cd->ElemTyp(i) = A_Str;
         if (dflt_to_ptr)
            cd->Str(i) = "), ";
         else
            cd->Str(i) = ", ";
         ++i;
         }
   
      cd->ElemTyp(i) = A_Str;
      cd->Str(i) = "&(";
      ++i;
      cnv_dest(loc, is_cstr, src, sym_indx, dest, cd, i);
      ++i;
      cd->ElemTyp(i) = A_Str;
      cd->Str(i) = "))";
      }
   return cd;
   }

/*
 * il_dflt - generate code for a defaulting conversion that always defaults.
 */
struct code *il_dflt(typcd, src, dflt, dest)
int typcd;
struct il_code *src;
struct il_c *dflt;
struct il_c *dest;
   {
   struct code *cd;
   int sym_indx;

   sym_indx = src->u[0].n;

   if (typcd == TypCDbl) {
      cd = alc_ary(5);
      cd->ElemTyp(0) = A_Str;
      cd->Str(0) =                                      "(";
      cnv_dest(PrmDbl, 0, src, sym_indx, dest, cd, 1);  /* variable */
      cd->ElemTyp(2) = A_Str;
      cd->Str(2) =                                      " = ";
      sub_ilc(dflt, cd, 3);                             /* default */
      cd->ElemTyp(4) = A_Str;
      cd->Str(4) =                                      ", 1)";
      }
   else if (typcd == TypCInt || typcd == TypECInt) {
      cd = alc_ary(5);
      cd->ElemTyp(0) = A_Str;
      cd->Str(0) =                                      "(";
      cnv_dest(PrmInt, 0, src, sym_indx, dest, cd, 1);  /* variable */
      cd->ElemTyp(2) = A_Str;
      cd->Str(2) =                                      " = ";
      sub_ilc(dflt, cd, 3);                             /* default */
      cd->ElemTyp(4) = A_Str;
      cd->Str(4) =                                      ", 1)";
      }
   else if (typcd == TypTStr || typcd == str_typ) {
      cd = alc_ary(5);
      cd->ElemTyp(0) = A_Str;
      cd->Str(0) =                                      "(";
      cnv_dest(0, 0, src, sym_indx, dest, cd, 1);       /* variable */
      cd->ElemTyp(2) = A_Str;
      cd->Str(2) =                                      " = ";
      sub_ilc(dflt, cd, 3);                             /* default */
      cd->ElemTyp(4) = A_Str;
      cd->Str(4) =                                      ", 1)";
      }
   else if (typcd == TypCStr) {
      cd = alc_ary(5);
      cd->ElemTyp(0) = A_Str;
      cd->Str(0) =                                      "(AsgnCStr(";
      cnv_dest(0, 1, src, sym_indx, dest, cd, 1);       /* variable */
      cd->ElemTyp(2) = A_Str;
      cd->Str(2) =                                      ", ";
      sub_ilc(dflt, cd, 3);                             /* default */
      cd->ElemTyp(4) = A_Str;
      cd->Str(4) =                                      "), 1)";
      }
   else if (typcd == TypTCset || typcd == cset_typ) {
      cd = alc_ary(7);
      cd->ElemTyp(0) = A_Str;
      cd->Str(0) =                                      "(BlkLoc(";
      cnv_dest(0, 0, src, sym_indx, dest, cd, 1);       /* variable */
      cd->ElemTyp(2) = A_Str;
      cd->Str(2) =                                      ") = (union block *)&";
      sub_ilc(dflt, cd, 3);                             /* default */
      cd->ElemTyp(4) = A_Str;
      cd->Str(4) =                                      ", ";
      cnv_dest(0, 0, src, sym_indx, dest, cd, 5);       /* variable */
      cd->ElemTyp(6) = A_Str;
      cd->Str(6) =                                      ".dword = D_Cset, 1)";
      }
   else if (typcd == TypEInt || typcd == int_typ) {
      cd = alc_ary(7);
      cd->ElemTyp(0) = A_Str;
      cd->Str(0) =                                      "(IntVal(";
      cnv_dest(0, 0, src, sym_indx, dest, cd, 1);       /* variable */
      cd->ElemTyp(2) = A_Str;
      cd->Str(2) =                                      ") = ";
      sub_ilc(dflt, cd, 3);                             /* default */
      cd->ElemTyp(4) = A_Str;
      cd->Str(4) =                                      ", ";
      cnv_dest(0, 0, src, sym_indx, dest, cd, 5);       /* variable */
      cd->ElemTyp(6) = A_Str;
      cd->Str(6) =                                     ".dword = D_Integer, 1)";
      }
   else if (typcd == real_typ) {
      cd = alc_ary(7);
      cd->ElemTyp(0) = A_Str;
      cd->Str(0) =                                      "((BlkLoc(";
      cnv_dest(0, 0, src, sym_indx, dest, cd, 1);       /* variable */
      cd->ElemTyp(2) = A_Str;
      cd->Str(2) =                                ") = (union block *)alcreal(";
      sub_ilc(dflt, cd, 3);                             /* default */
      cd->ElemTyp(4) = A_Str;
      cd->Str(4) =                     ")) == NULL ? (fatalerr(0,NULL), 0) : (";
      cnv_dest(0, 0, src, sym_indx, dest, cd, 5);       /* variable */
      cd->ElemTyp(6) = A_Str;
      cd->Str(6) =                                     ".dword = D_Real, 1))";
      }

   return cd;
   }

/*
 * cnv_dest - output the destination of a conversion.
 */
static void cnv_dest(loc, is_cstr, src, sym_indx, dest, cd, i)
int loc;
int is_cstr;
struct il_code *src;
int sym_indx;
struct il_c *dest;
struct code *cd;
int i;
   {
   if (dest == NULL) {
      /*
       * Convert "in place", changing the location of a parameter if needed.
       */
      switch (loc) {
         case PrmInt:
            if (cur_symtab[sym_indx].itmp_indx < 0)
               cur_symtab[sym_indx].itmp_indx = alc_itmp(intrnl_lftm);
            cur_symtab[sym_indx].loc = itmp_loc(cur_symtab[sym_indx].itmp_indx);
            break;
         case PrmDbl:
            if (cur_symtab[sym_indx].dtmp_indx < 0)
               cur_symtab[sym_indx].dtmp_indx = alc_dtmp(intrnl_lftm);
            cur_symtab[sym_indx].loc = dtmp_loc(cur_symtab[sym_indx].dtmp_indx);
            break;
         }
      il_var(src, cd, i);
      if (is_cstr)
         cur_symtab[sym_indx].loc = loc_cpy(cur_symtab[sym_indx].loc,M_CharPtr);
      }
   else {
      if (is_cstr && dest->il_c_type == ILC_Mod && dest->next == NULL &&
         dest->n != RsltIndx && cur_symtab[dest->n].loc->mod_access != M_None) {
            /*
             * We are converting to a C string. The destination variable
             *  is not defined as a simple descriptor, but must be accessed
             *  as such for this conversion.
             */
            cd->ElemTyp(i) = A_ValLoc;
            cd->ValLoc(i) = loc_cpy(cur_symtab[dest->n].loc, M_None);
            }
      else 
         sub_ilc(dest, cd, i);
      }

   }

/*
 * il_copy - produce code for an optimized "conversion" that always succeeds
 *   and just copies a value from one place to another.
 */
struct code *il_copy(dest, src)
struct il_c *dest;
struct val_loc *src;
   {
   struct code *cd;

   cd = alc_ary(5);
   cd->ElemTyp(0) = A_Str;
   cd->Str(0) = "(";
   sub_ilc(dest, cd, 1);
   cd->ElemTyp(2) = A_Str;
   cd->Str(2) = " = ";
   cd->ElemTyp(3) = A_ValLoc;
   cd->ValLoc(3) = src;
   cd->ElemTyp(4) = A_Str;
   cd->Str(4) = ", 1)";
   return cd;
   }

/*
 * loc_cpy - make a copy of a reference to a value location, but change
 *  the way the location is accessed.
 */
struct val_loc *loc_cpy(loc, mod_access)
struct val_loc *loc;
int mod_access;
   {
   struct val_loc *new_loc;

   if (loc == NULL)
      return NULL;
   new_loc = NewStruct(val_loc);
   *new_loc = *loc;
   new_loc->mod_access = mod_access;
   return new_loc;
   }

/*
 * gen_tcase - generate in-line code for a type_case statement.
 */
static int gen_tcase(il, has_dflt)
struct il_code *il;
int has_dflt;
   {
   struct case_anlz case_anlz;

   /*
    * We can only get here if the type_case statement can be implemented
    *  with a no more than one type check. Determine how simple the
    *  code can be.
    */
   findcases(il, has_dflt, &case_anlz);
   if (case_anlz.il_then == NULL) {
      if (case_anlz.il_else == NULL)
         return 1;
      else
         return gen_il(case_anlz.il_else);
      }
   else
      return gen_if(typ_chk(il->u[0].fld, case_anlz.typcd), case_anlz.il_then,
         case_anlz.il_else, sav_locs());
   }

/*
 * gen_if - generate code to test a condition that might be true
 *  of false. Determine if execution can continue past this if statement.
 */
static int gen_if(cond_cd, il_then, il_else, locs)
struct code *cond_cd;
struct il_code *il_then;
struct il_code *il_else;
struct val_loc **locs;
   {
   struct val_loc **locs1;
   struct code *lbl_then;
   struct code *lbl_end;
   struct code *else_loc;
   struct code *cd;
   int fall_thru;

   lbl_then = oper_lbl("then");
   lbl_end = oper_lbl("end if");
   cd = NewCode(2);
   cd->cd_id = C_If;
   cd->Cond = cond_cd;
   cd->ThenStmt = mk_goto(lbl_then);
   cd_add(cd);
   else_loc = cur_fnc->cursor;
   cd_add(lbl_then);
   fall_thru = gen_il(il_then);
   cd_add(lbl_end);
   locs1 = sav_locs();
   rstr_locs(locs);
   cur_fnc->cursor = else_loc;  /* go back for the else clause */
   fall_thru |= gen_il(il_else);
   cd_add(mk_goto(lbl_end));
   cur_fnc->cursor = lbl_end;
   mrg_locs(locs1);
   return fall_thru;
   }
