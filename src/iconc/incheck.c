/*
 * incheck.c - analyze a run-time operation using type information.
 *   Determine wither the operation can be in-lined and what kinds
 *   of parameter passing optimizations can be done.
 */
#include "../h/gsupport.h"
#include "ctrans.h"
#include "cglobals.h"
#include "csym.h"
#include "ctree.h"
#include "ccode.h"
#include "cproto.h"

struct op_symentry *cur_symtab; /* symbol table for current operation */

/*
 * Prototypes for static functions.
 */
static struct code *and_cond (struct code *cd1, struct code *cd2);
static int          cnv_anlz (unsigned int typcd, struct il_code *src,
                               struct il_c *dflt, struct il_c *dest,
                               struct code **cdp);
static int          defer_il (struct il_code *il);
static int          if_anlz  (struct il_code *il);
static void      ilc_anlz (struct il_c *ilc);
static int          il_anlz  (struct il_code *il);
static void      ret_anlz (struct il_c *ilc);
static int          tc_anlz  (struct il_code *il, int has_dflt);

static int n_branches;  /* number branches caused by run-time type checking */
static int side_effect; /* abstract clause indicates side-effect */
static int n_vararg;    /* size of variable part of arg list to operation */
static int n_susp;      /* number of suspends */
static int n_ret;       /* number of returns */

/*
 * do_inlin - determine if this operation can be in-lined at the current
 *  invocation. Also gather information about how arguments are used,
 *  and determine where the success continuation for the operation
 *  should be put.
 */
int do_inlin(impl, n, cont_loc, symtab, n_va) 
struct implement *impl;
nodeptr n;
int *cont_loc;
struct op_symentry *symtab;
int n_va;
   {
   int nsyms;
   int i;

   /*
    * Copy arguments needed by other functions into globals and
    *  initialize flags and counters for information to be gathered
    *  during analysis.
    */
   cur_symtyps = n->symtyps; /* mapping from arguments to types */
   cur_symtab = symtab;      /* parameter info to be filled in */
   n_vararg = n_va;
   n_branches = 0;
   side_effect = 0;
   n_susp = 0;
   n_ret = 0;

   /*
    * Analyze the code for this operation using type information for
    *  the arguments to the invocation.
    */
   il_anlz(impl->in_line);


   /*
    * Don't in-line if there is more than one decision made based on
    *  run-time type checks (this is a heuristic).
    */
   if (n_branches > 1)
      return 0;

   /*
    * If the operation (after eliminating code not used in this context)
    *  has one suspend and no returns, the "success continuation" can
    *  be placed in-line at the suspend site. Otherwise, any suspends
    *  require a separate function for the continuation.
    */
   if (n_susp == 1 && n_ret == 0)
     *cont_loc = SContIL;    /* in-line continuation */
   else if (n_susp > 0)
     *cont_loc = SepFnc;     /* separate function for continuation */
   else
     *cont_loc = EndOper;    /* place "continuation" after the operation */

   /*
    * When an argument at the source level is an Icon variable, it is
    *  sometimes safe to use it directly in the generated code as the
    *  argument to the operation. However, it is NOT safe under the
    *  following conditions:
    *
    *    - if the operation modifies the argument.
    *    - if the operation suspends and resumes so that intervening
    *      changes to the variable would be visible as changes to the
    *      argument.
    *    - if the operation has side effects that might involve the
    *      variable and be visible as changes to the argument.
    */
   nsyms = (cur_symtyps == NULL ? 0 : cur_symtyps->nsyms);
   for (i = 0; i < nsyms; ++i)
      if (symtab[i].n_mods == 0 && n->intrnl_lftm == n && !side_effect)
        symtab[i].var_safe = 1;

   return 1;
   }

/*
 * il_anlz - analyze a piece of RTL code. Return an indication of
 *  whether execution can continue beyond it.
 */
static int il_anlz(il)
struct il_code *il;
   {
   int fall_thru;
   int ncases;
   int condition;
   int indx;
   int i, j;

   if (il == NULL)
       return 1;

   switch (il->il_type) {
      case IL_Const: /* should have been replaced by literal node */
         return 1;

      case IL_If1:
         /*
          * if-then statement. Determine whether the condition may
          *  succeed or fail. Analyze the then clause if needed.
          */
         condition = if_anlz(il->u[0].fld);
         fall_thru = 0;
         if (condition & MaybeTrue)
            fall_thru |= il_anlz(il->u[1].fld);
         if (condition & MaybeFalse)
            fall_thru = 1;
         return fall_thru;

      case IL_If2:
         /*
          * if-then-else statement. Determine whether the condition may
          *  succeed or fail. Analyze the "then" clause and the "else"
          *  clause if needed.
          */
         condition = if_anlz(il->u[0].fld);
         fall_thru = 0;
         if (condition & MaybeTrue)
            fall_thru |= il_anlz(il->u[1].fld);
         if (condition & MaybeFalse)
            fall_thru |= il_anlz(il->u[2].fld);
         return fall_thru;

      case IL_Tcase1:
         /*
          * type_case statement with no default clause.
          */
         return tc_anlz(il, 0);

      case IL_Tcase2:
         /*
          * type_case statement with a default clause.
          */
         return tc_anlz(il, 1);

      case IL_Lcase:
         /*
          * len_case statement. Determine which case matches the number
          *  of arguments.
          */
         ncases = il->u[0].n;
         indx = 1;
         for (i = 0; i < ncases; ++i) {
            if (il->u[indx++].n == n_vararg)     /* selection number */
               return il_anlz(il->u[indx].fld);  /* action */
            ++indx;
            }
         return il_anlz(il->u[indx].fld);        /* default */

      case IL_Acase: {
         /*
          * arith_case statement.
          */
         struct il_code *var1;
         struct il_code *var2;
         int maybe_int;
         int maybe_dbl;
         int chk1;
         int chk2;

         var1 = il->u[0].fld;
         var2 = il->u[1].fld;
         arth_anlz(var1, var2, &maybe_int, &maybe_dbl, &chk1, NULL,
           &chk2, NULL);

         /*
          * Analyze the selected case (note, large integer code is not
          *  currently in-lined and can be ignored).
          */
         fall_thru = 0;
         if (maybe_int)
            fall_thru |= il_anlz(il->u[2].fld);     /* C_integer action */
         if (maybe_dbl)
            fall_thru |= il_anlz(il->u[4].fld);     /* C_double action */
         return fall_thru;
         }

      case IL_Err1:
         /*
          * runerr() with no offending value.
          */
         return 0;

      case IL_Err2:
         /*
          * runerr() with an offending value. Note the reference to
          *  the offending value descriptor.
          */
         indx = il->u[1].fld->u[0].n;    /* symbol table index of variable */
         if (indx < cur_symtyps->nsyms)
            ++cur_symtab[indx].n_refs;
         return 0;

      case IL_Block:
         /*
          * inline {...} statement.
          */
         i = il->u[1].n + 2;              /* skip declaration stuff */
         ilc_anlz(il->u[i].c_cd);         /* body of block */
         return il->u[0].n;

      case IL_Call:
         /*
          * call to body function.
          */
         if (il->u[3].n & DoesSusp)
            n_susp = 2;  /* force continuation into separate function */

         /*
          * Analyze the C code for prototype parameter declarations
          *  and actual arguments. There are twice as many pieces of
          *  C code to look at as there are parameters.
          */
         j = 2 * il->u[7].n;
         i = 8;              /* index of first piece of C code */
         while (j--)
            ilc_anlz(il->u[i++].c_cd);
         return ((il->u[3].n & DoesFThru) != 0);

      case IL_Lst:
         /*
          * Two consecutive pieces of RTL code.
          */
         fall_thru = il_anlz(il->u[0].fld);
         if (fall_thru)
            fall_thru = il_anlz(il->u[1].fld);
         return fall_thru;

      case IL_Abstr:
         /*
          * abstract type computation. See if it indicates side effects.
          */
         if (il->u[0].fld != NULL)
             side_effect = 1;
         return 1;

      default:
         fprintf(stderr, "compiler error: unknown info in data base\n");
         exit(EXIT_FAILURE);
         /* NOTREACHED */
      }
   }

/*
 * if_anlz - analyze the condition of an if statement.
 */
static int if_anlz(il)
struct il_code *il;
   {
   int cond;
   int cond1;

   if (il->il_type == IL_Bang) {
      /*
       * ! <condition>, negate the result of the condition
       */
      cond1 = cond_anlz(il->u[0].fld, NULL);
      cond = 0;
      if (cond1 & MaybeTrue)
         cond = MaybeFalse;
      if (cond1 & MaybeFalse)
         cond |= MaybeTrue;
      }
   else
      cond = cond_anlz(il, NULL);
   if (cond == (MaybeTrue | MaybeFalse))
      ++n_branches;  /* must make a run-time decision */
   return cond;
   }

/*
 * cond_anlz - analyze a simple condition or the conjunction of two
 *   conditions. If cdp is not NULL, use it to return a pointer code
 *   that implements the condition.
 */
int cond_anlz(il, cdp)
struct il_code *il;
struct code **cdp;
   {
   struct code *cd1;
   struct code *cd2;
   int cond1;
   int cond2;
   int indx;

   switch (il->il_type) {
      case IL_And:
         /*
          * <cond> && <cond>
          */
         cond1 = cond_anlz(il->u[0].fld, (cdp == NULL ? NULL : &cd1));
         if (cond1 & MaybeTrue) {
            cond2 = cond_anlz(il->u[1].fld, (cdp == NULL ? NULL : &cd2));
            if (cdp != NULL) {
               if (!(cond2 & MaybeTrue))
                  *cdp = NULL;
               else
                  *cdp = and_cond(cd1, cd2);
               }
            return (cond1 & MaybeFalse) | cond2;
            }
         else {
            if (cdp != NULL)
               *cdp = cd1;
            return cond1;
            }

      case IL_Cnv1:
         /*
          * cnv:<dest-type>(<source>)
          */
         return cnv_anlz(il->u[0].n, il->u[1].fld, NULL, NULL, cdp);

      case IL_Cnv2:
         /*
          * cnv:<dest-type>(<source>,<destination>)
          */
         return cnv_anlz(il->u[0].n, il->u[1].fld, NULL, il->u[2].c_cd, cdp);

      case IL_Def1:
         /*
          * def:<dest-type>(<source>,<default-value>)
          */
         return cnv_anlz(il->u[0].n, il->u[1].fld, il->u[2].c_cd, NULL, cdp);

      case IL_Def2:
         /*
          * def:<dest-type>(<source>,<default-value>,<destination>)
          */
         return cnv_anlz(il->u[0].n, il->u[1].fld, il->u[2].c_cd, il->u[3].c_cd,
            cdp);

      case IL_Is:
         /*
          * is:<type-name>(<variable>)
          */
         indx = il->u[1].fld->u[0].n;
         cond1 = eval_is(il->u[0].n, indx);
         if (cdp == NULL) {
            if (indx < cur_symtyps->nsyms && cond1 == (MaybeTrue | MaybeFalse))
               ++cur_symtab[indx].n_refs;
            }
         else {
            if (cond1 == (MaybeTrue | MaybeFalse))
               *cdp = typ_chk(il->u[1].fld, il->u[0].n);
            else
               *cdp = NULL;
            }
         return cond1;

      default:
         fprintf(stderr, "compiler error: unknown info in data base\n");
         exit(EXIT_FAILURE);
         /* NOTREACHED */
      }
   }


/*
 * and_cond - construct && of two conditions, either of which may have
 *  been optimized away.
 */
static struct code *and_cond(cd1, cd2)
struct code *cd1;
struct code *cd2;
   {
   struct code *cd;

   if (cd1 == NULL)
      return cd2;
   else if (cd2 == NULL)
      return cd1;
   else {
      cd = alc_ary(3);
      cd->ElemTyp(0) = A_Ary;
      cd->Array(0) =            cd1;
      cd->ElemTyp(1) = A_Str;
      cd->Str(1) =              " && ";
      cd->ElemTyp(2) = A_Ary;
      cd->Array(2) =            cd2;
      return cd;
      }
   }

/*
 * cnv_anlz - analyze a type conversion. Determine whether it can succeed
 *  and, if requested, produce code to perform the conversion. Also
 *  gather information about the variables it uses.
 */
static int cnv_anlz(typcd, src, dflt, dest, cdp)
unsigned int typcd;
struct il_code *src;
struct il_c *dflt;
struct il_c *dest;
struct code **cdp;
   {
   struct val_loc *src_loc;
   int cond;
   int cnv_flags;
   int indx;

   /*
    * Find out what is going on in the default and destination subexpressions.
    *  (The information is used elsewhere.)
    */
   ilc_anlz(dflt);
   ilc_anlz(dest);

   if (cdp != NULL)
      *cdp = NULL;   /* clear code pointer in case it is not set below */

   /*
    * Determine whether the conversion may succeed, whether it may fail,
    *  and whether it may actually convert a value or use the default
    *  value when it succeeds.
    */
   indx = src->u[0].n;  /* symbol table index for source of conversion */
   cond = eval_cnv(typcd, indx, dflt != NULL, &cnv_flags);

   /*
    * Many optimizations are possible depending on whether a conversion
    *  is actually needed, whether type checking is needed, whether defaulting
    *  is done, and whether there is an explicit destination. Several
    *  optimizations are performed here; more may be added in the future.
    */
   if (!(cnv_flags & MayDefault))
      dflt = NULL;  /* demote defaulting to simple conversion */

   if (cond & MaybeTrue) {
      if (cnv_flags == MayKeep && dest == NULL) {
          /*
           * No type conversion, defaulting, or copying is needed.
           */
         if (cond & MaybeFalse) {
            /*
             * A type check is needed.
             */
            ++cur_symtab[indx].n_refs; /* non-modifying reference to source. */
            if (cdp != NULL) {
               switch (typcd) {
                  case TypECInt:
                     *cdp = typ_chk(src, TypCInt);
                     break;
                  case TypEInt:
                     *cdp = typ_chk(src, int_typ);
                     break;
                  case TypTStr:
                     *cdp = typ_chk(src, str_typ);
                     break;
                  case TypTCset:
                     *cdp = typ_chk(src, cset_typ);
                     break;
                  default:
                     *cdp = typ_chk(src, typcd);
                  }
               }
            }

         if (cdp != NULL) {
            /*
             * Conversion from an integer to a C_integer can be done without
             *  any executable code; this is not considered a real conversion.
             *  It is accomplished by changing the symbol table so only the
             *  dword of the descriptor is accessed.
             */
            switch (typcd) {
               case TypCInt:
               case TypECInt:
                  cur_symtab[indx].loc = loc_cpy(cur_symtab[indx].loc, M_CInt);
                  break;
               }
            }
         }
      else if (dest != NULL && cnv_flags == MayKeep && cond == MaybeTrue) {
         /*
          *  There is an explicit destination, but no conversion, defaulting,
          *   or type checking is needed. Just copy the value to the
          *   destination.
          */
         ++cur_symtab[indx].n_refs; /* non-modifying reference to source */
         if (cdp != NULL)  {
            src_loc = cur_symtab[indx].loc;
            switch (typcd) {
               case TypCInt:
               case TypECInt:
                  /*
                   * The value is in the dword of the descriptor.
                   */
                  src_loc = loc_cpy(src_loc, M_CInt);
                  break;
               }
            *cdp = il_copy(dest, src_loc);
            }
         }
      else if (cnv_flags == MayDefault) {
         /*
          * The default value is used.
          */
         if (dest == NULL)
            ++cur_symtab[indx].n_mods; /* modifying reference */
         if (cdp != NULL) 
            *cdp = il_dflt(typcd, src, dflt, dest);
         }
      else {
         /*
          * Produce code to do the actual conversion.
          *  Determine whether the source location is being modified
          *  or just referenced.
          */
         if (dest == NULL) {
            /*
             * "In place" conversion.
             */
            switch (typcd) {
               case TypCDbl:
               case TypCInt:
               case TypECInt:
                  /*
                   * not really converted in-place.
                   */
                  ++cur_symtab[indx].n_refs; /* non-modifying reference */
                  break;
               default:
                  ++cur_symtab[indx].n_mods; /* modifying reference */
               }
            }
         else
            ++cur_symtab[indx].n_refs; /* non-modifying reference */

         if (cdp != NULL) 
            *cdp = il_cnv(typcd, src, dflt, dest);
         }
      }
   return cond;
   }

/*
 * ilc_anlz - gather information about in-line C code.
 */
static void ilc_anlz(ilc)
struct il_c *ilc;
   {
   while (ilc != NULL) {
      switch(ilc->il_c_type) {
         case ILC_Ref:
            /* 
             * Non-modifying reference to variable
             */
            if (ilc->n != RsltIndx) {
               ++cur_symtab[ilc->n].n_refs;
               }
            break;

         case ILC_Mod:
            /* 
             * Modifying reference to variable
             */
            if (ilc->n != RsltIndx) {
               ++cur_symtab[ilc->n].n_mods;
               }
            break;

         case ILC_Ret:
            /*
             * Return statement.
             */
            ++n_ret;
            ret_anlz(ilc);
            break;

         case ILC_Susp:
            /*
             * Suspend statement.
             */
            ++n_susp;
            ret_anlz(ilc);
            break;

         case ILC_CGto:
            /*
             * Conditional goto.
             */
            ilc_anlz(ilc->code[0]);
            break;
         }
      ilc = ilc->next;
      }
   }

/*
 * ret_anlz - gather information about the in-line C code associated
 *   with a return or suspend.
 */
static void ret_anlz(ilc)
struct il_c *ilc;
   {
   int i;
   int j;

   /*
    * See if the code is simply returning a parameter.
    */
   if (ilc->n == RetDesc && ilc->code[0]->il_c_type == ILC_Ref &&
      ilc->code[0]->next == NULL) {
         j = ilc->code[0]->n;
         ++cur_symtab[j].n_refs;
         ++cur_symtab[j].n_rets;
         }
   else {
      for (i = 0; i < 3 && ilc->code[i] != NULL; ++i)
         ilc_anlz(ilc->code[i]);
      }
   }

/*
 * deref_il - dummy routine to pass to a code walk.
 */
/*ARGSUSED*/
static int defer_il(il)
struct il_code *il;
   {
   /*
    * Called for each case in a type_case statement that might be selected.
    *  However, the actual analysis of the case, if it is needed,
    *  is done elsewhere, so just return.
    */
   return 0;
   }

/*
 * findcases - determine how many cases of an type_case statement may
 *   be true. If there are two or less, determine the "if" statement
 *   that can be used (if there are more than two, the code is not
 *   in-lined).
 */
void findcases(il, has_dflt, case_anlz)
struct il_code *il;
int has_dflt;
struct case_anlz *case_anlz;
   {
   int i;

   case_anlz->n_cases = 0;
   case_anlz->typcd = -1;
   case_anlz->il_then = NULL;
   case_anlz->il_else = NULL;
   i = type_case(il, defer_il, case_anlz);
   /*
    * See if the explicit cases have accounted for all possible
    *  types that might be present.
    */
   if (i == -1) {  /* all types accounted for */
      if (case_anlz->il_else == NULL && case_anlz->il_then != NULL) {
         /*   
          * We don't need to actually check the type.
          */
         case_anlz->il_else = case_anlz->il_then;
         case_anlz->il_then = NULL;
         case_anlz->typcd = -1;
         }
      }
    else {  /* not all types accounted for */
      if (case_anlz->il_else != NULL)
         case_anlz->n_cases = 3; /* force no inlining */
      else if (has_dflt)
         case_anlz->il_else = il->u[i].fld;         /* default */
      }

   if (case_anlz->n_cases > 2)
      n_branches = 2;  /* no in-lining */
   else if (case_anlz->il_then != NULL)
      ++n_branches;
   }


/*
 * tc_anlz - analyze a type_case statement. It is only of interest for
 *   in-lining if it can be reduced to an "if" statement or an
 *   unconditional statement.
 */
static int tc_anlz(il, has_dflt)
struct il_code *il;
int has_dflt;
   {
   struct case_anlz case_anlz;
   int fall_thru;
   int indx;

   findcases(il, has_dflt, &case_anlz);

   if (case_anlz.il_else == NULL)
      fall_thru = 1;   /* either no code at all or condition with no "else" */
   else
      fall_thru = 0;   /* either unconditional or if-then-else: check code  */

   if (case_anlz.il_then != NULL) {
      fall_thru |= il_anlz(case_anlz.il_then);
      indx = il->u[0].fld->u[0].n;    /* symbol table index of variable */
      if (indx < cur_symtyps->nsyms)
         ++cur_symtab[indx].n_refs;
      }
   if (case_anlz.il_else != NULL)
      fall_thru |= il_anlz(case_anlz.il_else);
   return fall_thru;
   }

/*
 * arth_anlz - analyze the type checking of an arith_case statement.
 */
void arth_anlz(var1, var2, maybe_int, maybe_dbl, chk1, conv1p, chk2, conv2p)
struct il_code *var1;
struct il_code *var2;
int *maybe_int;
int *maybe_dbl;
int *chk1;
struct code **conv1p;
int *chk2;
struct code **conv2p;
   {
   int cond;
   int cnv_typ;


   /*
    * First do an analysis to find out which cases are needed. This is
    *  more accurate than analysing the conversions separately, but does
    *  not get all the information we need.
    */
   eval_arith(var1->u[0].n, var2->u[0].n, maybe_int, maybe_dbl);

   if (*maybe_int & (largeints | *maybe_dbl)) {
      /*
       * Too much type checking; don't bother with these cases. Force no
       *   in-lining.
       */
      n_branches += 2;
      }
   else {
      if (*maybe_int)
         cnv_typ = TypCInt;
      else
         cnv_typ = TypCDbl;

      /*
       * See exactly what kinds of conversions/type checks are needed and,
       *  if requested, generate code for them.
       */
      *chk1 = 0;
      *chk2 = 0;

      cond = cnv_anlz(cnv_typ, var1, NULL, NULL, conv1p);
      if (cond & MaybeFalse) {
         ++n_branches;          /* run-time decision */
         *chk1 = 1;
         if (var1->u[0].n < cur_symtyps->nsyms)
            ++cur_symtab[var1->u[0].n].n_refs;  /* used in runerr2() */
         }
      cond = cnv_anlz(cnv_typ, var2, NULL, NULL, conv2p);
      if (cond & MaybeFalse) {
         ++n_branches;          /* run-time decision */
         *chk2 = 1;
         if (var2->u[0].n < cur_symtyps->nsyms)
            ++cur_symtab[var2->u[0].n].n_refs;  /* used in runerr2() */
         }
      }
   }
