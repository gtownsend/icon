/*
 * rttinlin.c contains routines which produce the in-line version of an
 *  operation and put it in the data base.
 */
#include "rtt.h"

/*
 * prototypes for static functions.
 */
static struct il_code *abstrcomp (struct node *n, int indx_stor,
                                    int chng_stor, int escapes);
static void         abstrsnty (struct token *t, int typcd,
                                   int indx_stor, int chng_stor);
static int             body_anlz (struct node *n, int *does_break,
                                   int may_mod, int const_cast, int all);
static struct il_code *body_fnc  (struct node *n);
static void         chkrettyp (struct node *n);
static void         chng_ploc (int typcd, struct node *src);
static void         cnt_bufs  (struct node *cnv_typ);
static struct il_code *il_walk   (struct node *n);
static struct il_code *il_var    (struct node *n);
static int             is_addr   (struct node *dcltor, int modifier);
static void         lcl_tend  (struct node *n);
static int             mrg_abstr (int sum, int typ);
static int             strct_typ (struct node *typ, int *is_reg);

static int body_ret; /* RetInt, RetDbl, and/or RetOther for current body */
static int ret_flag; /* DoesFail, DoesRet, and/or DoesSusp for current body */
int fnc_ret;         /* RetInt, RetDbl, RetNoVal, or RetSig for current func */

#ifndef Rttx

/*
 * body_prms is a list of symbol table entries for identifiers that must
 *  be passed as parameters to the function implementing the current
 *  body statement. The id_type of an identifier may be changed in the
 *  symbol table while the body function is being produced; for example,
 *  a tended descriptor is accessed through a parameter that is a pointer
 *  to a descriptor, rather than being accessed as an element of a descriptor
 *  array in a struct.
 */
struct var_lst {
   struct sym_entry *sym;
   int id_type;            /* saved value of id_type from sym */
   struct var_lst *next;
   };
struct var_lst *body_prms;
int n_bdy_prms;		/* number of entries in body_prms list */
int rslt_loc;		/* flag: function passed addr of result descriptor */

char prfx3;		/* 3rd prefix char; used for unique body func names */

/*
 * in_line - place in the data base in-line code for an operation and
 *   produce C functions for body statements.
 */
void in_line(n)
struct node *n;
   {
   struct sym_entry *sym;
   int i;
   int nvars;
   int ntend;

   prfx3 = ' '; /* reset 3rd prefix char for body functions */

   /*
    * Set up the local symbol table in the data base for the in-line code.
    *  This symbol table has an array of entries for the tended variables
    *  in the declare statement, if there is one. Determine how large the
    *  array must be and create it.
    */
   ntend = 0;
   for (sym = dcl_stk->tended; sym != NULL; sym = sym->u.tnd_var.next)
      ++ntend;
   if (ntend == 0)
      cur_impl->tnds = NULL;
   else
      cur_impl->tnds = alloc(ntend * sizeof(struct tend_var));
   cur_impl->ntnds = ntend;
   i = 0;

   /*
    * Go back through the declarations and fill in the array for the
    *  tended part of the data base symbol table. Array entries contain
    *  an indication of the type of tended declaration, the C code to
    *  initialize the variable if there is any, and, for block pointer
    *  declarations, the type of block. rtt's symbol table is updated to
    *  contain the variable's offset into the data base's symbol table.
    *  Note that parameters are considered part of the data base's symbol
    *  table when computing the offset and il_indx initially contains
    *  their number.
    */
   for (sym = dcl_stk->tended; sym != NULL; sym = sym->u.tnd_var.next) {
      cur_impl->tnds[i].var_type = sym->id_type;
      cur_impl->tnds[i].init = inlin_c(sym->u.tnd_var.init, 0);
      cur_impl->tnds[i].blk_name = sym->u.tnd_var.blk_name;
      sym->il_indx = il_indx++;
      ++i;
      }

   /*
    * The data base's symbol table also has entries for non-tended
    *  variables from the declare statement. Each entry has the
    *  identifier for the variable and the declaration (redundantly
    *  including the identifier). Once again the offset for the data
    *  base symbol table is stored in rtt's symbol table.
    */
   nvars = -il_indx;  /* pre-subtract preceding number of entries */
   for (sym = decl_lst; sym != NULL; sym = sym->u.declare_var.next)
      sym->il_indx = il_indx++;
   nvars += il_indx;  /* compute number of entries in this part of table */
   cur_impl->nvars = nvars;
   if (nvars > 0) {
      cur_impl->vars = alloc(nvars * sizeof(struct ord_var));
      i = 0;
      for (sym = decl_lst; sym != NULL; sym = sym->u.declare_var.next) {
         cur_impl->vars[i].name = sym->image;
         cur_impl->vars[i].dcl = ilc_dcl(sym->u.declare_var.tqual,
            sym->u.declare_var.dcltor, sym->u.declare_var.init);
         ++i;
         }
      }

   abs_ret = NoAbstr;		   /* abstract clause not encountered yet */
   cur_impl->in_line = il_walk(n); /* produce in-line code for operation */
   }

/*
 * il_walk - walk the syntax tree producing in-line code.
 */
static struct il_code *il_walk(n)
struct node *n;
   {
   struct token *t;
   struct node *n1;
   struct node *n2;
   struct il_code *il;
   struct il_code *il1;
   struct sym_entry *sym;
   struct init_tend *tnd;
   int dummy_int;
   int ntend;
   int typcd;

   if (n == NULL)
      return NULL;

   t =  n->tok;

   switch (n->nd_id) {
      case PrefxNd:
         switch (t->tok_id) {
            case '{':
               /*
                * RTL code: { <actions> }
                */
               il = il_walk(n->u[0].child);
               break;
            case '!':
               /*
                * RTL type-checking and conversions: ! <simple-type-check>
                */
               il = new_il(IL_Bang, 1);
               il->u[0].fld = il_walk(n->u[0].child);
               break;
            case Body:
               /*
                * RTL code: body { <c-code> }
                */
               il = body_fnc(n);
               break;
            case Inline:
               /*
                * RTL code: inline { <c-code> }
                *
                *  An in-line code "block" in the data base starts off
                *  with an indication of whether execution falls through
                *  the code and a list of tended descriptors needed by the
                *  in-line C code. The list indicates the kind of tended
                *  descriptor. The list is determined by walking to the
                *  syntax tree for the C code; tend_lst points to its
                *  beginning. The last item in the block is the C code itself.
                */
               free_tend();
               lcl_tend(n);
               if (tend_lst == NULL)
                  ntend = 0;
               else
                  ntend = tend_lst->t_indx + 1;
               il = new_il(IL_Block, 3 + ntend);
               /*
                * Only need "fall through" info from body_anlz().
                */
               il->u[0].n = body_anlz(n->u[0].child, &dummy_int, 0, 0, 0);
               il->u[1].n = ntend;
               for (tnd = tend_lst; tnd != NULL; tnd = tnd->next)
                  il->u[2 + tnd->t_indx].n = tnd->init_typ;
               il->u[ntend + 2].c_cd = inlin_c(n->u[0].child, 0);
               if (!il->u[0].n)
                  clr_prmloc(); /* execution does not continue */
               break;
            }
         break;
      case BinryNd:
         switch (t->tok_id) {
            case Runerr:
               /*
                * RTL code: runerr( <message-number> )
                *           runerr( <message-number>, <descriptor> )
                */
               if (n->u[1].child == NULL)
                  il = new_il(IL_Err1, 1);
               else {
                  il = new_il(IL_Err2, 2);
                  il->u[1].fld = il_var(n->u[1].child);
                  }
               il->u[0].n = atol(n->u[0].child->tok->image);
               /*
                * Execution cannot continue on this execution path.
                */
               clr_prmloc();
               break;
            case And:
               /*
                * RTL type-checking and conversions:
                *   <type-check> && <type_check>
                */
               il = new_il(IL_And, 2);
               il->u[0].fld = il_walk(n->u[0].child);
               il->u[1].fld = il_walk(n->u[1].child);
               break;
            case Is:
               /*
                * RTL type-checking and conversions:
                *   is: <icon-type> ( <variable> )
                */
               il = new_il(IL_Is, 2);
               il->u[0].n = icn_typ(n->u[0].child);
               il->u[1].fld = il_var(n->u[1].child);
               break;
            }
         break;
      case ConCatNd:
         /*
          * "Glue" for two constructs.
          */
         il = new_il(IL_Lst, 2);
         il->u[0].fld = il_walk(n->u[0].child);
         il->u[1].fld = il_walk(n->u[1].child);
         break;
      case AbstrNd:
         /*
          * RTL code: abstract { <type-computations> }
          *
          *  Remember the return statement if there is one. It is used for
          *  type checking when types are easily determined.
          */
         il = new_il(IL_Abstr, 2);
         il->u[0].fld = abstrcomp(n->u[0].child, 0, 0, 0);
         il1 = abstrcomp(n->u[1].child, 0, 0, 1);
         il->u[1].fld = il1;
         if (il1 != NULL) {
            if (abs_ret != NoAbstr)
               errt1(t,"only one abstract return may be on any execution path");
            if (il1->il_type == IL_IcnTyp || il1->il_type == IL_New)
               abs_ret = il1->u[0].n;
            else
               abs_ret = SomeType;
            }
         break;
      case TrnryNd:
         switch (t->tok_id) {
            case If: {
               /*
                * RTL code for "if" statements:
                *  if <type-check> then <action>
                *  if <type-check> then <action> else <action>
                *
                *  <type-check> may include parameter conversions that create
                *  new scoping. It is necessary to keep track of parameter
                *  types and locations along success and failure paths of
                *  these conversions. The "then" and "else" actions may
                *  also establish new scopes (if a parameter is used within
                *  a overlapping scopes that conflict, it has already been
                *  detected).
                *
                *  The "then" and "else" actions may contain abstract return
                *  statements. The types of these must be "merged" in case
                *  type checking must be done on real return or suspend
                *  statements following the "if".
                */
               struct parminfo *then_prms = NULL;
               struct parminfo *else_prms;
               struct node *cond;
               struct node *else_nd;
               int sav_absret;
               int new_absret;

               /*
                * Save the current parameter locations. These are in
                *  effect on the failure path of any type conversions
                *  in the condition of the "if". Also remember any
                *  information from abstract returns.
                */
               else_prms = new_prmloc();
               sv_prmloc(else_prms);
               sav_absret = new_absret = abs_ret;

               cond = n->u[0].child;
               else_nd = n->u[2].child;

               if (else_nd == NULL)
                  il = new_il(IL_If1, 2);
               else
                  il = new_il(IL_If2, 3);
               il->u[0].fld = il_walk(cond);
               /*
                * If the condition is negated, the failure path is to the "then"
                *  and the success path is to the "else".
                */
               if (cond->nd_id == PrefxNd && cond->tok->tok_id == '!') {
                  then_prms = else_prms;
                  else_prms = new_prmloc();
                  sv_prmloc(else_prms);
                  ld_prmloc(then_prms);
                  }
               il->u[1].fld = il_walk(n->u[1].child);  /* then ... */
               if (else_nd == NULL) {
                  mrg_prmloc(else_prms);
                  ld_prmloc(else_prms);
                  }
               else {
                  if (then_prms == NULL)
                     then_prms = new_prmloc();
                  sv_prmloc(then_prms);
                  ld_prmloc(else_prms);
                  new_absret = mrg_abstr(new_absret, abs_ret);
                  abs_ret = sav_absret;
                  il->u[2].fld = il_walk(else_nd);
                  mrg_prmloc(then_prms);
                  ld_prmloc(then_prms);
                  }
               abs_ret = mrg_abstr(new_absret, abs_ret);
               if (then_prms != NULL)
                  free((char *)then_prms);
               if (else_prms != NULL)
                  free((char *)else_prms);
               }
               break;
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
               int n_cases;
               int indx;
               int sav_absret;
               int new_absret;

               /*
                * A case may contain parameter conversions that create new
                *  scopes. Remember the parameter locations at the start
                *  of the len_case statement. Also remember information
                *  about abstract type returns.
                */
               strt_prms = new_prmloc();
               sv_prmloc(strt_prms);
               end_prms = new_prmloc();
               sav_absret = new_absret = abs_ret;

               /*
                * Count the number of cases; there is at least one.
                */
               n_cases = 1;
               for (n1 = n->u[1].child; n1->nd_id == ConCatNd;
                   n1 = n1->u[0].child)
                      ++n_cases;

               /*
                * The data base entry has one slot for the number of cases,
                *  one for the default clause, and two for each case. A
                *  case includes a selection integer and an action.
                */
               il = new_il(IL_Lcase, 2 + 2 * n_cases);
               il->u[0].n = n_cases;

               /*
                * Go through the cases, adding them to the data base entry.
                *  Merge resulting parameter locations and information
                *  about abstract type returns, then restore the starting
                *  information for the next case.
                */
               indx = 2 * n_cases;
               for (n1 = n->u[1].child; n1->nd_id == ConCatNd;
                    n1 = n1->u[0].child) {
                  il->u[indx--].fld = il_walk(n1->u[1].child->u[0].child);
                  il->u[indx--].n = atol(n1->u[1].child->tok->image);
                  mrg_prmloc(end_prms);
                  ld_prmloc(strt_prms);
                  new_absret = mrg_abstr(new_absret, abs_ret);
                  abs_ret = sav_absret;
                  }
               /*
                * Last case.
                */
               il->u[indx--].fld = il_walk(n1->u[0].child);
               il->u[indx].n = atol(n1->tok->image);
               mrg_prmloc(end_prms);
               ld_prmloc(strt_prms);
               new_absret = mrg_abstr(new_absret, abs_ret);
               abs_ret = sav_absret;
               /*
                * Default clause.
                */
               il->u[1 + 2 * n_cases].fld = il_walk(n->u[2].child);
               mrg_prmloc(end_prms);
               ld_prmloc(end_prms);
               abs_ret = mrg_abstr(new_absret, abs_ret);
               if (strt_prms != NULL)
                  free((char *)strt_prms);
               if (end_prms != NULL)
                  free((char *)end_prms);
               }
               break;
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
               struct node *sel;
               struct parminfo *strt_prms;
               struct parminfo *end_prms;
               int *typ_vect;
               int n_case;
               int n_typ;
               int n_fld;
               int sav_absret;
               int new_absret;

               /*
                * A case may contain parameter conversions that create new
                *  scopes. Remember the parameter locations at the start
                *  of the type_case statement. Also remember information
                *  about abstract type returns.
                */
               strt_prms = new_prmloc();
               sv_prmloc(strt_prms);
               end_prms = new_prmloc();
               sav_absret = new_absret = abs_ret;

               /*
                * Count the number of cases.
                */
               n_case = 0;
               for (n1 = n->u[1].child; n1 != NULL; n1 = n1->u[0].child)
                  ++n_case;

               /*
                * The data base entry has one slot for the variable whose
                *  type is being tested, one for the number cases, three
                *  for each case, and, if there is default clause, one
                *  for it. Each case includes the number of types selected
                *  by the case, a vectors of those types, and the action
                *  for the case.
                */
               if (n->u[2].child == NULL) {
                  il = new_il(IL_Tcase1, 3 * n_case + 2);
                  il->u[0].fld = il_var(n->u[0].child);
                  }
               else {
                  /*
                   * There is a default clause.
                   */
                  il = new_il(IL_Tcase2, 3 * n_case + 3);
                  il->u[0].fld = il_var(n->u[0].child);
                  il->u[3 * n_case + 2].fld = il_walk(n->u[2].child);
                  mrg_prmloc(end_prms);
                  ld_prmloc(strt_prms);
                  }
               il->u[1].n = n_case;

               /*
                * Go through the cases, adding them to the data base entry.
                *  Merge resulting parameter locations and information
                *  about abstract type returns, then restore the starting
                *  information for the next case.
                */
               n_fld = 2;
               for (n1 = n->u[1].child; n1 != NULL; n1 = n1->u[0].child) {
                  /*
                   * Determine the number types selected by the case and
                   *  put the types in a vector.
                   */
                  sel = n1->u[1].child;
                  n_typ = 0;
                  for (n2 = sel->u[0].child; n2 != NULL; n2 = n2->u[0].child)
                     n_typ++;
                  il->u[n_fld++].n = n_typ;
                  typ_vect = alloc(n_typ * sizeof(int));
                  il->u[n_fld++].vect = typ_vect;
                  n_typ = 0;
                  for (n2 = sel->u[0].child; n2 != NULL; n2 = n2->u[0].child)
                     typ_vect[n_typ++] = icn_typ(n2->u[1].child);
                  /*
                   * Add code for the case to the data  base entry.
                   */
                  new_absret = mrg_abstr(new_absret, abs_ret);
                  abs_ret = sav_absret;
                  il->u[n_fld++].fld = il_walk(sel->u[1].child);
                  mrg_prmloc(end_prms);
                  ld_prmloc(strt_prms);
                  }
               ld_prmloc(end_prms);
               abs_ret = mrg_abstr(new_absret, abs_ret);
               if (strt_prms != NULL)
                  free((char *)strt_prms);
               if (end_prms != NULL)
                  free((char *)end_prms);
               }
               break;
            case Cnv: {
               /*
                * RTL code: cnv: <type> ( <source> )
                *           cnv: <type> ( <source> , <destination> )
                */
               struct node *typ;
               struct node *src;
               struct node *dst;

               typ = n->u[0].child;
               src = n->u[1].child;
               dst = n->u[2].child;
               typcd = icn_typ(typ);
               if (src->nd_id == SymNd)
                  sym = src->u[0].sym;
               else if (src->nd_id == BinryNd)
                  sym = src->u[0].child->u[0].sym; /* subscripted variable */
               else
                  errt2(src->tok, "undeclared identifier: ", src->tok->image);
               if (sym->u.param_info.parm_mod) {
                  fprintf(stderr, "%s: file %s, line %d, warning: ",
                     progname, src->tok->fname, src->tok->line);
                  fprintf(stderr, "%s may be modified\n", sym->image);
                  fprintf(stderr,
                  "\ticonc does not handle conversion of modified parameter\n");
                  }


               if (dst == NULL) {
                  il = new_il(IL_Cnv1, 2);
                  il->u[0].n = typcd;
                  il->u[1].fld = il_var(src);
                  /*
                   * This "in-place" conversion may create a new scope for the
                   *  source parameter.
                   */
                  chng_ploc(typcd, src);
                  sym->u.param_info.parm_mod |= 1;
                  }
               else {
                  il = new_il(IL_Cnv2, 3);
                  il->u[0].n = typcd;
                  il->u[1].fld = il_var(src);
                  il->u[2].c_cd = inlin_c(dst, 1);
                  }
               }
               break;
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
               struct node *var1;
               struct node *var2;
               struct parminfo *strt_prms;
               struct parminfo *end_prms;
               int sav_absret;
               int new_absret;

               strt_prms = new_prmloc();
               sv_prmloc(strt_prms);
               end_prms = new_prmloc();
               sav_absret = new_absret = abs_ret;

               var1 = n->u[0].child;
               var2 = n->u[1].child;
               n1 = n->u[2].child;   /* contains actions for the 3 cases */

               /*
                * The data base entry has a slot for each of the two variables
                *  and one for each of the three cases.
                */
               il = new_il(IL_Acase, 5);
               il->u[0].fld = il_var(var1);
               il->u[1].fld = il_var(var2);

               /*
                * The "in-place" conversions to C_integer creates new scopes.
                */
               chng_ploc(TypECInt, var1);
               chng_ploc(TypECInt, var2);
               il->u[2].fld = il_walk(n1->u[0].child);
               mrg_prmloc(end_prms);
               new_absret = mrg_abstr(new_absret, abs_ret);


               /*
                * Conversion to integer (applicable to large integers only).
                */
               ld_prmloc(strt_prms);
               abs_ret = sav_absret;
               il->u[3].fld  = il_walk(n1->u[1].child);
               mrg_prmloc(end_prms);
               new_absret = mrg_abstr(new_absret, abs_ret);

               /*
                * The "in-place" conversions to C_double creates new scopes.
                */
               ld_prmloc(strt_prms);
               abs_ret = sav_absret;
               chng_ploc(TypCDbl, var1);
               chng_ploc(TypCDbl, var2);
               il->u[4].fld  = il_walk(n1->u[2].child);
               mrg_prmloc(end_prms);

               ld_prmloc(end_prms);
               abs_ret = mrg_abstr(new_absret, abs_ret);
               free((char *)strt_prms);
               free((char *)end_prms);
               }
               break;
            }
         break;
      case QuadNd: {
         /*
          * RTL code: def: <type> ( <source> , <default>)
          *           def: <type> ( <source> , <default> , <destination> )
          */
         struct node *typ;
         struct node *src;
         struct node *dflt;
         struct node *dst;

         typ = n->u[0].child;
         src = n->u[1].child;
         dflt = n->u[2].child;
         dst = n->u[3].child;
         typcd = icn_typ(typ);
         if (dst == NULL) {
            il = new_il(IL_Def1, 3);
            il->u[0].n = typcd;
            il->u[1].fld = il_var(src);
            il->u[2].c_cd = inlin_c(dflt, 0);
            /*
             * This "in-place" conversion may create a new scope for the
             *  source parameter.
             */
            chng_ploc(typcd, src);
            }
         else {
            il = new_il(IL_Def2, 4);
            il->u[0].n = typcd;
            il->u[1].fld = il_var(src);
            il->u[2].c_cd = inlin_c(dflt, 0);
            il->u[3].c_cd = inlin_c(dst, 1);
            }
         }
         break;
      }
   return il;
   }

/*
 * il_var - produce in-line code in the data base for varibel references.
 *   These include both simple identifiers and subscripted identifiers.
 */
static struct il_code *il_var(n)
struct node *n;
   {
   struct il_code *il;

   if (n->nd_id == SymNd) {
      il = new_il(IL_Var, 1);
      il->u[0].n = n->u[0].sym->il_indx; /* offset into data base sym. tab. */
      }
   else if (n->nd_id == BinryNd) {
      /*
       * A subscripted variable.
       */
      il = new_il(IL_Subscr, 2);
      il->u[0].n = n->u[0].child->u[0].sym->il_indx; /* sym. tab. offset */
      il->u[1].n = atol(n->u[1].child->tok->image);  /* subscript */
      }
   else
      errt2(n->tok, "undeclared identifier: ", n->tok->image);
   return il;
   }

/*
 * abstrcomp - produce data base code for RTL abstract type computations.
 *  In the process, do a few sanity checks where they are easy to do.
 */
static struct il_code *abstrcomp(n, indx_stor, chng_stor, escapes)
struct node *n;
int indx_stor;
int chng_stor;
int escapes;
   {
   struct token *t;
   struct il_code *il;
   int typcd;
   int cmpntcd;

   if (n == NULL)
      return NULL;

   t =  n->tok;

   switch (n->nd_id) {
      case PrefxNd:
         switch (t->tok_id) {
            case TokType:
               /*
                * type( <variable> )
                */
               il = new_il(IL_VarTyp, 1);
               il->u[0].fld = il_var(n->u[0].child);
               break;
            case Store:
               /*
                * store[ <type> ]
                */
               il = new_il(IL_Store, 1);
               il->u[0].fld = abstrcomp(n->u[0].child, 1, 0, 0);
               break;
            }
         break;
      case PstfxNd:
         /*
          * <type> . <attrb_name>
          */
         il = new_il(IL_Compnt, 2);
         il->u[0].fld = abstrcomp(n->u[0].child, 0, 0, 0);
         switch (t->tok_id) {
            case Component:
               cmpntcd = sym_lkup(t->image)->u.typ_indx;
               il->u[1].n = cmpntcd;
               if (escapes && !typecompnt[cmpntcd].var)
                  errt3(t, typecompnt[cmpntcd].id,
                    " component is an internal reference type.\n",
                    "\t\tuse store[<type>.<component>] to \"dereference\" it");
               break;
            case All_fields:
               il->u[1].n = CM_Fields;
               break;
            }
         break;
      case IcnTypNd:
         /*
          * <icon-type>
          */
         il = new_il(IL_IcnTyp, 1);
         typcd = icn_typ(n->u[0].child);
         abstrsnty(t, typcd, indx_stor, chng_stor);
         il->u[0].n = typcd;
         break;
      case BinryNd:
         switch (t->tok_id) {
            case '=':
               /*
                * store[ <type> ] = <type>
                */
               il = new_il(IL_TpAsgn, 2);
               il->u[0].fld = abstrcomp(n->u[0].child, 1, 1, 0);
               il->u[1].fld = abstrcomp(n->u[1].child, 0, 0, 1);
               break;
            case Incr: /* union */
               /*
                * <type> ++ <type>
                */
               il = new_il(IL_Union, 2);
               il->u[0].fld = abstrcomp(n->u[0].child, indx_stor, chng_stor,
                  escapes);
               il->u[1].fld = abstrcomp(n->u[1].child, indx_stor, chng_stor,
                  escapes);
               break;
            case Intersect:
               /*
                * <type> ** <type>
                */
               il = new_il(IL_Inter, 2);
               il->u[0].fld = abstrcomp(n->u[0].child, indx_stor, chng_stor,
                  escapes);
               il->u[1].fld = abstrcomp(n->u[1].child, indx_stor, chng_stor,
                  escapes);
               break;
            case New: {
               /*
                * new <icon-type> ( <type> ,  ... )
                */
               struct node *typ;
               struct node *args;
               int nargs;

               typ = n->u[0].child;
               args = n->u[1].child;

               typcd = icn_typ(typ);
               abstrsnty(typ->tok, typcd, indx_stor, chng_stor);

               /*
                * Determine the number of arguments expected for this
                *  structure type.
                */
               if (typcd >= 0)
                  nargs = icontypes[typcd].num_comps;
               else
                  nargs  = 0;
               if (nargs == 0)
                  errt2(typ->tok,typ->tok->image," is not an aggregate type.");

               /*
                * Create the "new" construct for the data base with its type
                *  code and arguments.
                */
               il = new_il(IL_New, 2 + nargs);
               il->u[0].n = typcd;
               il->u[1].n = nargs;
               while (nargs > 1) {
                  if (args->nd_id == CommaNd)
                     il->u[1 + nargs].fld = abstrcomp(args->u[1].child, 0,0,1);
                  else
                     errt2(typ->tok, "too few arguments for new",
                        typ->tok->image);
                  args = args->u[0].child;
                  --nargs;
                  }
               if (args->nd_id == CommaNd)
                  errt2(typ->tok, "too many arguments for new",typ->tok->image);
               il->u[2].fld = abstrcomp(args, 0, 0, 1);
               }
               break;
            }
         break;
      case ConCatNd:
         /*
          * "Glue" for several side effects.
          */
         il = new_il(IL_Lst, 2);
         il->u[0].fld = abstrcomp(n->u[0].child, 0, 0, 0);
         il->u[1].fld = abstrcomp(n->u[1].child, 0, 0, 0);
         break;
      }
   return il;
   }

/*
 * abstrsnty - do some sanity checks on how this type is being used in
 *  an abstract type computation.
 */
static void abstrsnty(t, typcd, indx_stor, chng_stor)
struct token *t;
int typcd;
int indx_stor;
int chng_stor;
   {
   struct icon_type *itp;

   if ((typcd < 0) || (!indx_stor))
      return;

   itp = &icontypes[typcd];

   /*
    * This type is being used to index the store; make sure this it
    *   is a variable.
    */
   if (itp->deref == DrfNone)
      errt2(t, itp->id, " is not a variable type");

   if (chng_stor && itp->deref == DrfCnst)
      errt2(t, itp->id, " has an associated type that may not be changed");
   }

/*
 * body_anlz - walk the syntax tree for the C code in a body statment,
 *  analyzing the code to determine the interface needed by the C function
 *  which will implement it. Also determine how many buffers are needed.
 *  The value returned indicates whether it is possible for execution
 *  to fall through the the code.
 */
static int body_anlz(n, does_break, may_mod, const_cast, all)
struct node *n;   /* subtree being analyzed */
int *does_break;  /* output flag: subtree contains "break;" */
int may_mod;      /* input flag: this subtree might be assigned to */
int const_cast;   /* input flag: expression is cast to (const ...) */
int all;          /* input flag: need all information about operation */
   {
   struct token *t;
   struct node *n1, *n2, *n3;
   struct sym_entry *sym;
   struct var_lst *var_ref;
   int break_chk = 0;
   int fall_thru;
   static int may_brnchto;

   if (n == NULL)
      return 1;

   t =  n->tok;

   switch (n->nd_id) {
      case PrimryNd:
         switch (t->tok_id) {
            case Fail:
               if (all)
                  ret_flag |= DoesFail;
               return 0;
            case Errorfail:
               if (all)
                  ret_flag |= DoesEFail;
               return 0;
            case Break:
               *does_break = 1;
               return 0;
            default: /* do nothing special */
               return 1;
            }
      case PrefxNd:
         switch (t->tok_id) {
            case Return:
               if (all) {
                  ret_flag |= DoesRet;
                  chkrettyp(n->u[0].child); /* check for returning of C value */
                  }
               body_anlz(n->u[0].child, does_break, 0, 0, all);
               return 0;
            case Suspend:
               if (all) {
                  ret_flag |= DoesSusp;
                  chkrettyp(n->u[0].child); /* check for returning of C value */
                  }
               body_anlz(n->u[0].child, does_break, 0, 0, all);
               return 1;
            case '(':
               /*
                * parenthesized expression: pass along may_mod and const_cast.
                */
               return body_anlz(n->u[0].child, does_break, may_mod, const_cast,
                  all);
            case Incr: /* ++ */
            case Decr: /* -- */
               /*
                * Operand may be modified.
                */
               body_anlz(n->u[0].child, does_break, 1, 0, all);
               return 1;
            case '&':
               /*
                * Unless the address is cast to a const pointer, this
                *  might be a modifiying reference.
                */
               if (const_cast)
                  body_anlz(n->u[0].child, does_break, 0, 0, all);
               else
                  body_anlz(n->u[0].child, does_break, 1, 0, all);
               return 1;
            case Default:
               fall_thru = body_anlz(n->u[0].child, does_break, 0, 0, all);
               may_brnchto = 1;
               return fall_thru;
            case Goto:
               body_anlz(n->u[0].child, does_break, 0, 0, all);
               return 0;
            default: /* unary operations the need nothing special */
               body_anlz(n->u[0].child, does_break, 0, 0, all);
               return 1;
            }
      case PstfxNd:
         if (t->tok_id == ';')
            return body_anlz(n->u[0].child, does_break, 0, 0, all);
         else {
            /*
             * C expressions: <expr> ++
             *                <expr> --
             *
             * modify operand
             */
            return body_anlz(n->u[0].child, does_break, 1, 0, all);
            }
      case PreSpcNd:
         body_anlz(n->u[0].child, does_break, 0, 0, all);
         return 1;
      case SymNd:
         /*
          * This is an identifier.
          */
         if (!all)
             return 1;
         sym = n->u[0].sym;
         if (sym->id_type == RsltLoc) {
            /*
             * Note that this body code explicitly references the result
             *  location of the operation.
             */
            rslt_loc = 1;
            }
         else if (sym->nest_lvl == 2) {
            /*
             * This variable is local to the operation, but declared outside
             *  the body. It must passed as a parameter to the function.
             *  See if it is in the parameter list yet.
             */
            if (!(sym->id_type & PrmMark)) {
               sym->id_type |= PrmMark;
               var_ref = NewStruct(var_lst);
               var_ref->sym = sym;
               var_ref->next = body_prms;
               body_prms = var_ref;
               ++n_bdy_prms;
               }

            /*
             *  Note if the variable might be assigned to.
             */
            sym->may_mod |= may_mod;
            }
         return 1;
      case BinryNd:
         switch (t->tok_id) {
            case '[': /* subscripting */
            case '.':
               /*
                * Assignments will modify left operand.
                */
               body_anlz(n->u[0].child, does_break, may_mod, 0, all);
               body_anlz(n->u[1].child, does_break, 0, 0, all);
               return 1;
            case '(':
               /*
                * ( <type> ) expr
                */
               body_anlz(n->u[0].child, does_break, 0, 0, all);
               /*
                * See if the is a const cast.
                */
               for (n1 = n->u[0].child; n1->nd_id == LstNd; n1 = n1->u[0].child)
                  ;
               if (n1->nd_id == PrimryNd && n1->tok->tok_id == Const)
                  body_anlz(n->u[1].child, does_break, 0, 1, all);
               else
                  body_anlz(n->u[1].child, does_break, 0, 0, all);
               return 1;
            case ')':
               /*
                * function call or declaration: <expr> ( <expr-list> )
                */
               body_anlz(n->u[0].child, does_break, 0, 0, all);
               body_anlz(n->u[1].child, does_break, 0, 0, all);
               return call_ret(n->u[0].child);
            case ':':
            case Case:
               body_anlz(n->u[0].child, does_break, 0, 0, all);
               fall_thru = body_anlz(n->u[1].child, does_break, 0, 0, all);
               may_brnchto = 1;
               return fall_thru;
            case Switch:
               body_anlz(n->u[0].child, does_break, 0, 0, all);
               fall_thru = body_anlz(n->u[1].child, &break_chk, 0, 0, all);
               return fall_thru | break_chk;
            case While: {
	       struct node *n0 = n->u[0].child;
               body_anlz(n0, does_break, 0, 0, all);
               body_anlz(n->u[1].child, &break_chk, 0, 0, all);
	       /*
		* check for an infinite loop, while (1) ... :
                *  a condition consisting of an IntConst with image=="1"
                *  and no breaks in the body.
		*/
	       if (n0->nd_id == PrimryNd && n0->tok->tok_id == IntConst &&
		   !strcmp(n0->tok->image,"1") && !break_chk)
		  return 0;
               return 1;
	       }
            case Do:
               /*
                * Any "break;" statements in the body do not effect
                *  outer loops so pass along a new flag for does_break.
                */
               body_anlz(n->u[0].child, &break_chk, 0, 0, all);
               body_anlz(n->u[1].child, does_break, 0, 0, all);
               return 1;
            case Runerr:
               body_anlz(n->u[0].child, does_break, 0, 0, all);
               body_anlz(n->u[1].child, does_break, 0, 0, all);
               if (all)
                  ret_flag |= DoesEFail;  /* possibler error failure */
               return 0;
            case '=':
            case MultAsgn:  /*  *=  */
            case DivAsgn:   /*  /=  */
            case ModAsgn:   /*  %=  */
            case PlusAsgn:  /*  +=  */
            case MinusAsgn: /*  -=  */
            case LShftAsgn: /* <<=  */
            case RShftAsgn: /* >>=  */
            case AndAsgn:   /*  &=  */
            case XorAsgn:   /*  ^=  */
            case OrAsgn:    /*  |=  */
               /*
                * Left operand is modified.
                */
               body_anlz(n->u[0].child, does_break, 1, 0, all);
               body_anlz(n->u[1].child, does_break, 0, 0, all);
               return 1;
            default: /* binary operations that need nothing special */
               body_anlz(n->u[0].child, does_break, 0, 0, all);
               body_anlz(n->u[1].child, does_break, 0, 0, all);
               return 1;
            }
      case LstNd:
      case StrDclNd:
         /*
          * Some declaration code.
          */
         body_anlz(n->u[0].child, does_break, 0, 0, all);
         body_anlz(n->u[1].child, does_break, 0, 0, all);
         return 1;
      case ConCatNd:
        /*
         * <some-code> <some-code>
         */
         if (body_anlz(n->u[0].child, does_break, 0, 0, all))
            return body_anlz(n->u[1].child, does_break, 0, 0, all);
         else {
            /*
             * Cannot directly reach the second piece of code, see if
             *  it is possible to branch into it.
             */
            may_brnchto = 0;
            fall_thru = body_anlz(n->u[1].child, does_break, 0, 0, all);
            return may_brnchto & fall_thru;
            }
      case CommaNd:
         /*
          * <expr> , <expr>
          */
         fall_thru = body_anlz(n->u[0].child, does_break, 0, 0, all);
         return fall_thru & body_anlz(n->u[1].child, does_break, 0, 0, all);
      case CompNd:
         /*
          * Compound statement, look only at executable code.
          *
          *  First traverse declaration list looking for initializers.
          */
         n1 = n->u[0].child;
         while (n1 != NULL) {
            if (n1->nd_id == LstNd) {
               n2 = n1->u[1].child;
               n1 = n1->u[0].child;
               }
            else {
               n2 = n1;
               n1 = NULL;
               }

            /*
             * Get declarator list from declaration and traverse it.
             */
            n2 = n2->u[1].child;
            while (n2 != NULL) {
               if (n2->nd_id == CommaNd) {
                  n3 = n2->u[1].child;
                  n2 = n2->u[0].child;
                  }
               else {
                  n3 = n2;
                  n2 = NULL;
                  }
               if (n3->nd_id == BinryNd && n3->tok->tok_id == '=')
                   body_anlz(n3->u[1].child, does_break, 0, 0, all);
               }
            }

         /*
          * Check initializers on tended declarations.
          */
         for (sym = n->u[1].sym; sym != NULL; sym = sym->u.tnd_var.next)
            body_anlz(sym->u.tnd_var.init, does_break, 0, 0, all);

         /*
          * Do the statement list.
          */
         return body_anlz(n->u[2].child, does_break, 0, 0, all);
      case TrnryNd:
         switch (t->tok_id) {
            case Cnv:
               /*
                * extended C code: cnv: <type> ( <source> )
                *                  cnv: <type> ( <source> , <destination> )
                *
                *  For some conversions, buffers may have to be allocated.
                *  An explicit destination must be marked as modified.
                */
               if (all)
                  cnt_bufs(n->u[0].child);
               body_anlz(n->u[1].child, does_break, 0, 0, all);
               body_anlz(n->u[2].child, does_break, 1, 0, all);
               return 1;
            case If:
               /*
                * Execution falls through an if statement if it falls
                *  through either branch. A null "else" branch always
                *  falls through.
                */
               body_anlz(n->u[0].child, does_break, 0, 0, all);
               return body_anlz(n->u[1].child, does_break, 0, 0, all) |
                  body_anlz(n->u[2].child, does_break, 0, 0, all);
            case Type_case:
               /*
                * type_case <expr> of { <section-list> }
                * type_case <expr> of { <section-list> <default-clause> }
                */

               body_anlz(n->u[0].child, does_break, 0, 0, all);
               /*
                * Loop through the case clauses.
                */
               fall_thru = 0;
               for (n1 = n->u[1].child; n1 != NULL; n1 = n1->u[0].child) {
                  n2 = n1->u[1].child->u[1].child;
                  fall_thru |= body_anlz(n2, does_break, 0, 0, all);
                  }
               return fall_thru | body_anlz(n->u[2].child, does_break, 0, 0,
                  all);
            default: /* nothing special is needed for these ternary nodes */
               body_anlz(n->u[0].child, does_break, 0, 0, all);
               body_anlz(n->u[1].child, does_break, 0, 0, all);
               body_anlz(n->u[2].child, does_break, 0, 0, all);
               return 1;
               }
      case QuadNd:
         if (t->tok_id == Def) {
               /*
                * extended C code:
                *   def: <type> ( <source> , <default> )
                *   def: <type> ( <source> , <default> , <destination> )
                *
                *  For some conversions, buffers may have to be allocated.
                *  An explicit destination must be marked as modified.
                */
               if (all)
                  cnt_bufs(n->u[0].child);
               body_anlz(n->u[1].child, does_break, 0, 0, all);
               body_anlz(n->u[2].child, does_break, 0, 0, all);
               body_anlz(n->u[3].child, does_break, 1, 0, all);
               return 1;
               }
          else {  /* for */
               /*
                * Check for an infinite loop:  for (<expr>; ; <expr> ) ...
                *
                *  No ending condition and no breaks in the body.
                */
               body_anlz(n->u[0].child, does_break, 0, 0, all);
               body_anlz(n->u[1].child, does_break, 0, 0, all);
               body_anlz(n->u[2].child, does_break, 0, 0, all);
               body_anlz(n->u[3].child, &break_chk, 0, 0, all);
               if (n->u[1].child == NULL && !break_chk)
                  return 0;
               else
                  return 1;
               }
      }
   err1("rtt internal error detected in function body_anlz()");
   /* NOTREACHED */
   return 0;  /* avoid gcc warning */
   }

/*
 *  lcl_tend  - allocate any tended variables needed in this body or inline
 *   statement.
 */
static void lcl_tend(n)
struct node *n;
   {
   struct sym_entry *sym;

   if (n == NULL)
      return;

   /*
    * Walk the syntax tree until a block with declarations is found.
    */
   switch (n->nd_id) {
      case PrefxNd:
      case PstfxNd:
      case PreSpcNd:
        lcl_tend(n->u[0].child);
        break;
      case BinryNd:
      case LstNd:
      case ConCatNd:
      case CommaNd:
      case StrDclNd:
        lcl_tend(n->u[0].child);
        lcl_tend(n->u[1].child);
        break;
      case CompNd:
         /*
          * Allocate the tended variables in this block, noting that the
          *  level of nesting in this C function is one less than in the
          *  operation as a whole. Then mark the tended slots as free for
          *  use in the next block.
          */
         for (sym = n->u[1].sym; sym != NULL; sym = sym->u.tnd_var.next) {
            sym->t_indx = alloc_tnd(sym->id_type, sym->u.tnd_var.init,
               sym->nest_lvl - 1);
            }
         lcl_tend(n->u[2].child);
         sym = n->u[1].sym;
         if (sym != NULL)
            unuse(tend_lst, sym->nest_lvl - 1);
         break;
      case TrnryNd:
         lcl_tend(n->u[0].child);
         lcl_tend(n->u[1].child);
         lcl_tend(n->u[2].child);
         break;
      case QuadNd:
         lcl_tend(n->u[0].child);
         lcl_tend(n->u[1].child);
         lcl_tend(n->u[2].child);
         lcl_tend(n->u[3].child);
         break;
      }
   }

/*
 * chkrettyp - check type of return to see if it is a C integer or a
 *  C double and make note of what is found.
 */
static void chkrettyp(n)
struct node *n;
   {
   if (n->nd_id == PrefxNd && n->tok != NULL) {
      switch (n->tok->tok_id) {
         case C_Integer:
            body_ret |= RetInt;
            return;
         case C_Double:
            body_ret |= RetDbl;
            return;
         }
      }
   body_ret |= RetOther;
   }

/*
 * body_fnc - produce the function which implements a body statement.
 */
static struct il_code *body_fnc(n)
struct node *n;
   {
   struct node *compound;
   struct node *dcls;
   struct node *stmts;
   struct var_lst *var_ref;
   struct sym_entry *sym;
   struct il_code *il;
   int fall_thru;          /* flag: control can fall through end of body */
   int num_sigs;           /* number of different signals function may return */
   int bprm_indx;
   int first;
   int is_reg;
   int strct;
   int addr;
   int by_ref;
   int just_desc;
   int dummy_int;
   char buf1[6];

   char *cname;
   char buf[MaxPath];

   /*
    * Figure out the next character to use as the 3rd prefix for the
    *  name of this body function.
    */
   if (prfx3 == ' ')
      prfx3 = '0';
   else if (prfx3 == '9')
      prfx3 = 'a';
   else if (prfx3 == 'z')
      errt2(n->tok, "more than 26 body statements in", cur_impl->name);
   else
      ++prfx3;

   /*
    * Free any old body parameters and tended locations.
    */
   while (body_prms != NULL) {
      var_ref = body_prms;
      body_prms = body_prms->next;
      free((char *)var_ref);
      }
   free_tend();

   /*
    * Locate the outer declarations and statements from the body clause.
    */
   compound = n->u[0].child;
   dcls = compound->u[0].child;
   stmts = compound->u[2].child;

   /*
    * Analyze the body code to determine what the function's interface
    *  needs. body_anlz() does the work after the counters and flags
    *  are initialized.
    */
   n_tmp_str = 0;  /* number of temporary string buffers neeeded */
   n_tmp_cset = 0; /* number of temporary cset buffers needed */
   nxt_sbuf = 0;   /* next string buffer index; used in code generation */
   nxt_cbuf = 0;   /* next cset buffer index; used in code generation */
   n_bdy_prms = 0; /* number of variables needed as body function parameters */
   body_ret = 0;   /* flag: C values and/or non-C values returned */
   ret_flag = 0;   /* flag: return, suspend, fail, error fail */
   rslt_loc = 0;   /* flag: body code needs operations result location */
   fall_thru = body_anlz(compound, &dummy_int, 0, 0, 1);
   lcl_tend(n);    /* allocate tended descriptors needed */


   /*
    * Use the letter indicating operation type along with body function
    *  prefixes to construct the name of the file to hold the C code.
    */
   sprintf(buf1, "%c_%c%c%c", lc_letter, prfx1, prfx2, prfx3);
   cname = salloc(makename(buf, SourceDir, buf1, CSuffix));
   if ((out_file = fopen(cname, "w")) == NULL)
      err2("cannot open output file ", cname);
   else
      addrmlst(cname, out_file);

   prologue(); /* output standard comments and preprocessor directives */

   /*
    * If the function produces a unique signal, the function need not actually
    *  return it, and we may be able to use the return value for something
    *  else. See if this is true.
    */
   num_sigs = 0;
   if (ret_flag & DoesRet)
      ++num_sigs;
   if (ret_flag & (DoesFail  | DoesEFail))
      ++num_sigs;
   if (ret_flag & DoesSusp)
      num_sigs += 2;    /* something > 1 (success cont. may return anything) */
   if (fall_thru) {
      ret_flag |= DoesFThru;
      ++num_sigs;
      }

   if (num_sigs > 1)
      fnc_ret = RetSig;  /* Function must return a signal */
   else {
      /*
       * If the body returns a C_integer or a C_double, we can make the
       *  function directly return the C value and the compiler can decide
       *  whether to construct a descriptor.
       */
      if (body_ret == RetInt || body_ret == RetDbl)
         fnc_ret = body_ret;
      else
         fnc_ret = RetNoVal; /* Function returns nothing directly */
      }

   /*
    * Decide whether the function needs to to be passed an explicit result
    *  location (the case where "result" is explicitly referenced is handled
    *  while analyzing the body). suspend always uses the result location.
    *  return uses the result location unless the function directly
    *  returns a C value.
    */
   if (ret_flag & DoesSusp)
      rslt_loc = 1;
   else if ((ret_flag & DoesRet) && (fnc_ret != RetInt && fnc_ret != RetDbl))
      rslt_loc = 1;

   /*
    * The data base entry for the call to the body function has 8 slots
    *  for standard interface information and 2 slots for each parameter.
    */
   il = new_il(IL_Call, 8 + 2 * n_bdy_prms);
   il->u[0].n = 0;         /* reserved for internal use by compiler */
   il->u[1].n = prfx3;
   il->u[2].n = fnc_ret;
   il->u[3].n = ret_flag;
   il->u[4].n = rslt_loc;
   il->u[5].n = 0;       /* number of string buffers to pass in: set below */
   il->u[6].n = 0;       /* number of cset buffers to pass in: set below */
   il->u[7].n = n_bdy_prms;
   bprm_indx = 8;

   /*
    * Write the C function header for the body function.
    */
   switch (fnc_ret) {
      case RetSig:
         fprintf(out_file, "int ");
         break;
      case RetInt:
         fprintf(out_file, "C_integer ");
         break;
      case RetDbl:
         fprintf(out_file, "double ");
         break;
      case RetNoVal:
         fprintf(out_file, "void ");
         break;
      }
   fprintf(out_file, " %c%c%c%c_%s(", uc_letter, prfx1, prfx2, prfx3,
        cur_impl->name);
   fname = cname;
   line = 7;

   /*
    * Write parameter list, first the parenthesized list of names. Start
    *  with names of RLT variables that must be passed in.
    */
   first = 1;
   for (var_ref = body_prms; var_ref != NULL; var_ref = var_ref->next) {
      sym = var_ref->sym;
      sym->id_type &= ~PrmMark;             /* unmark entry */
      if (first)
         first = 0;
      else
         prt_str(", ", IndentInc);
      prt_str(sym->image, IndentInc);
      }

   if (fall_thru) {
      /*
       * We cannot allocate string and cset buffers locally, so any
       *   that are needed must be parameters.
       */
      if (n_tmp_str > 0) {
         if (first)
            first = 0;
         else
            prt_str(", ", IndentInc);
         prt_str("r_sbuf", IndentInc);
         }
      if (n_tmp_cset > 0) {
         if (first)
            first = 0;
         else
            prt_str(", ", IndentInc);
         prt_str("r_cbuf", IndentInc);
         }
      }

   /*
    * If the result location is needed it is passed as the next parameter.
    */
   if (rslt_loc) {
      if (first)
         first = 0;
      else
         prt_str(", ", IndentInc);
      prt_str("r_rslt", IndentInc);
      }

   /*
    * If a success continuation is needed, it goes last.
    */
   if (ret_flag & DoesSusp) {
      if (!first)
         prt_str(", ", IndentInc);
      prt_str("r_s_cont", IndentInc);
      }
   prt_str(")", IndentInc);
   ForceNl();

   /*
    * Go through the parameters to this function writing out declarations
    *  and filling in rest of data base entry. Start with RLT variables.
    */
   for (var_ref = body_prms; var_ref != NULL; var_ref = var_ref->next) {
      /*
       * Each parameters has two slots in the data base entry. One
       *  is the declaration for use by iconc in producing function
       *  prototypes. The other is the argument that must be passed as
       *  part of the call generated by iconc.
       *
       * Determine whether the parameter is passed by reference or by
       *  value (flag by_ref). Tended variables that refer to just the
       *  vword of a descriptor require special handling. They must
       *  be passed to the body function as a pointer to the entire
       *  descriptor and not just the vword. Within the function the
       *  parameter is then accessed as x->vword... This is indicated
       *  by the parameter flag just_desc.
       */
      sym = var_ref->sym;
      var_ref->id_type = sym->id_type;      /* save old id_type */
      by_ref = 0;
      just_desc = 0;
      switch (sym->id_type) {
         case TndDesc:  /* tended struct descrip x */
            by_ref = 1;
            il->u[bprm_indx++].c_cd = simpl_dcl("dptr ", 0, sym);
            break;
         case TndStr:   /* tended char *x */
         case TndBlk:   /* tended struct b_??? *x or tended union block *x */
            by_ref = 1;
            just_desc = 1;
            il->u[bprm_indx++].c_cd = simpl_dcl("dptr ", 0, sym);
            break;
         case RtParm: /* undereferenced RTL parameter */
         case DrfPrm: /* dereferenced RTL parameter */
            switch (sym->u.param_info.cur_loc) {
               case PrmTend: /* plain parameter: descriptor */
                  by_ref = 1;
                  il->u[bprm_indx++].c_cd = simpl_dcl("dptr ", 0, sym);
                  break;
               case PrmCStr: /* parameter converted to a tended C string */
                  by_ref = 1;
                  just_desc = 1;
                  il->u[bprm_indx++].c_cd = simpl_dcl("dptr ", 0, sym);
                  break;
               case PrmInt:  /* parameter converted to a C integer */
                  sym->id_type = OtherDcl;
                  if (var_ref->sym->may_mod && fall_thru)
                     by_ref = 1;
                  il->u[bprm_indx++].c_cd = simpl_dcl("C_integer ", by_ref,
                     sym);
                  break;
               case PrmDbl: /* parameter converted to a C double */
                  sym->id_type = OtherDcl;
                  if (var_ref->sym->may_mod && fall_thru)
                     by_ref =  1;
                  il->u[bprm_indx++].c_cd = simpl_dcl("double ", by_ref, sym);
                  break;
               }
            break;
         case RtParm | VarPrm:
         case DrfPrm | VarPrm:
            /*
             * Variable part of RTL parameter list: already descriptor pointer.
             */
            sym->id_type = OtherDcl;
            il->u[bprm_indx++].c_cd = simpl_dcl("dptr ", 0, sym);
            break;
         case VArgLen:
            /*
             * Number of elements in variable part of RTL parameter list:
             *  integer but not a true variable.
             */
            sym->id_type = OtherDcl;
            il->u[bprm_indx++].c_cd = simpl_dcl("int ", 0, sym);
            break;
         case OtherDcl:
            is_reg = 0;
            /*
             * Pass by reference if it is a structure or union type (but
             *  not if it is a pointer to one) or if the variable is
             *  modified and it is possible to execute more code after the
             *  body. WARNING: crude assumptions are made for typedef
             *  types.
             */
            strct = strct_typ(sym->u.declare_var.tqual, &is_reg);
            addr = is_addr(sym->u.declare_var.dcltor, '\0');
            if ((strct && !addr) || (var_ref->sym->may_mod && fall_thru))
                  by_ref = 1;
            if (is_reg && by_ref)
              errt2(sym->u.declare_var.dcltor->u[1].child->tok, sym->image,
                 " may not be declared 'register'");

            il->u[bprm_indx++].c_cd = parm_dcl(by_ref, sym);
            break;
         }

      /*
       * Determine what the iconc generated argument in a function
       *  call should look like.
       */
      il->u[bprm_indx++].c_cd = bdy_prm(by_ref, just_desc, sym,
         var_ref->sym->may_mod);

      /*
       * If it a call-by-reference parameter, indicate that the level
       *  of indirection must be taken into account within the function
       *  body.
       */
      if (by_ref)
         sym->id_type |= ByRef;
      }

   if (fall_thru) {
      /*
       * Write declarations for any needed buffer parameters.
       */
      if (n_tmp_str > 0) {
         prt_str("char (*r_sbuf)[MaxCvtLen];", 0);
         ForceNl();
         }
      if (n_tmp_cset > 0) {
         prt_str("struct b_cset *r_cbuf;", 0);
         ForceNl();
         }
      /*
       * Indicate that buffers must be allocated by compiler and not
       *  within the function.
       */
      il->u[5].n = n_tmp_str;
      il->u[6].n = n_tmp_cset;
      n_tmp_str = 0;
      n_tmp_cset = 0;
      }

   /*
    * Write declarations for result location and success continuation
    *  parameters if they are needed.
    */
   if (rslt_loc) {
      prt_str("dptr r_rslt;", 0);
      ForceNl();
      }
   if (ret_flag & DoesSusp) {
      prt_str("continuation r_s_cont;", 0);
      ForceNl();
      }

   /*
    * Output the code for the function including ordinary declaration,
    *  special declarations, and executable code.
    */
   prt_str("{", IndentInc);
   ForceNl();
   c_walk(dcls, IndentInc, 0);
   spcl_dcls(NULL);
   c_walk(stmts, IndentInc, 0);
   ForceNl();
   /*
    * If it is possible for excution to fall through to the end of
    *  the body function, and it does so, return an A_FallThru signal.
    */
   if (fall_thru) {
      if (tend_lst != NULL) {
	 prt_str("tend = tend->previous;", IndentInc);
	 ForceNl();
         }
      if (fnc_ret == RetSig) {
         prt_str("return A_FallThru;", IndentInc);
         ForceNl();
         }
      }
   prt_str("}\n", IndentInc);
   if (fclose(out_file) != 0)
      err2("cannot close ", cname);
   put_c_fl(cname, 1);

   /*
    * Restore the symbol table to its previous state. Note any parameters
    *  that were modified by the body code.
    */
   for (var_ref = body_prms; var_ref != NULL; var_ref = var_ref->next) {
      sym = var_ref->sym;
      sym->id_type = var_ref->id_type;
      if (sym->id_type & DrfPrm)
         sym->u.param_info.parm_mod |= sym->may_mod;
      sym->may_mod = 0;
      }

   if (!fall_thru)
       clr_prmloc();
   return il;
   }

/*
 * strct_typ - determine if the declaration may be for a structured type
 *   and look for register declarations.
 */
static int strct_typ(typ, is_reg)
struct node *typ;
int *is_reg;
   {
   if (typ->nd_id == LstNd) {
      return strct_typ(typ->u[0].child, is_reg) |
         strct_typ(typ->u[1].child, is_reg);
      }
   else if (typ->nd_id == PrimryNd) {
      switch (typ->tok->tok_id) {
         case Typedef:
         case Extern:
            errt2(typ->tok, "declare {...} should not contain ",
               typ->tok->image);
         case TokRegister:
            *is_reg = 1;
            return 0;
         case TypeDefName:
            if (strcmp(typ->tok->image, "word")  == 0 ||
                strcmp(typ->tok->image, "uword") == 0 ||
                strcmp(typ->tok->image, "dptr")  == 0)
               return 0;   /* assume non-structure type */
            else
               return 1;   /* might be a structure (is not C_integer) */
         default:
            return 0;
         }
      }
   else {
      /*
       * struct, union, or enum.
       */
      return 1;
      }
   }

/*
 * determine if the variable being declared evaluates to an address.
 */
static int is_addr(dcltor, modifier)
struct node *dcltor;
int modifier;
   {
   switch (dcltor->nd_id) {
      case ConCatNd:
         /*
          * pointer?
          */
         if (dcltor->u[0].child != NULL)
            modifier = '*';
         return is_addr(dcltor->u[1].child, modifier);
      case PrimryNd:
         /*
          * We have reached the name.
          */
         switch (modifier) {
            case '\0':
               return 0;
            case '*':
            case '[':
               return 1;
            case ')':
               errt1(dcltor->tok,
                  "declare {...} should not contain a prototype");
            }
      case PrefxNd:
         /*
          * (...)
          */
         return is_addr(dcltor->u[0].child, modifier);
      case BinryNd:
         /*
          * function or array.
          */
         return is_addr(dcltor->u[0].child, dcltor->tok->tok_id);
      }
   err1("rtt internal error detected in function is_addr()");
   /* NOTREACHED */
   return 0;  /* avoid gcc warning */
   }

/*
 * chgn_ploc - if this is an "in-place" conversion to a C value, change
 *  the "location" of the parameter being converted.
 */
static void chng_ploc(typcd, src)
int typcd;
struct node *src;
   {
   int loc;

   /*
    * Note, we know this is a valid conversion, because it got through
    *  pass 1.
    */
   loc = PrmTend;
   switch (typcd) {
      case TypCInt:
      case TypECInt:
         loc = PrmInt;
         break;
      case TypCDbl:
         loc = PrmDbl;
         break;
      case TypCStr:
         loc = PrmCStr;
         break;
      }
   if (loc != PrmTend)
      src->u[0].sym->u.param_info.cur_loc = loc;
   }

/*
 * cnt_bufs - See if we need to allocate a string or cset buffer for
 *  this conversion.
 */
static void cnt_bufs(cnv_typ)
struct node *cnv_typ;
   {
   if (cnv_typ->nd_id == PrimryNd)
      switch (cnv_typ->tok->tok_id) {
         case Tmp_string:
            ++n_tmp_str;
            break;
         case Tmp_cset:
            ++n_tmp_cset;
            break;
         }
   }

/*
 * mrg_abstr - merge (join) types of abstract returns on two execution paths.
 *   The type lattice has three levels: NoAbstr is bottom, SomeType is top,
 *   and individual types form the middle level.
 */
static int mrg_abstr(sum, typ)
int sum;
int typ;
   {
   if (sum == NoAbstr)
      return typ;
   else if (typ == NoAbstr)
      return sum;
   else if (sum == typ)
      return sum;
   else
      return SomeType;
   }
#endif					/* Rttx */
