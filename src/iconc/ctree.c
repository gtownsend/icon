/*
 * ctree.c -- functions for constructing parse trees.
 */
#include "../h/gsupport.h"
#include "../h/lexdef.h"
#include "ctrans.h"
#include "ctree.h"
#include "csym.h"
#include "ctoken.h"
#include "ccode.h"
#include "cproto.h"

/*
 * prototypes for static functions.
 */
static nodeptr chk_empty (nodeptr n);
static void put_elms  (nodeptr t, nodeptr args, int slot);
static nodeptr subsc_nd  (nodeptr op, nodeptr arg1, nodeptr arg2);

/*
 *  tree[1-6] construct parse tree nodes with specified values.
 *   loc_model is a node containing the same line and column information
 *   as is needed in this node, while parameters a through d are values to
 *   be assigned to n_field[0-3]. Note that this could be done with a
 *   single routine; a separate routine for each node size is used for
 *   speed and simplicity.
 */

nodeptr tree1(type)
int type;
   {
   register nodeptr t;

   t = NewNode(0);
   t->n_type = type;
   t->n_file = NULL;
   t->n_line = 0;
   t->n_col = 0;
   t->freetmp = NULL;
   return t;
   }

nodeptr tree2(type, loc_model)
int type;
nodeptr loc_model;
   {
   register nodeptr t;

   t = NewNode(0);
   t->n_type = type;
   t->n_file = loc_model->n_file;
   t->n_line = loc_model->n_line;
   t->n_col = loc_model->n_col;
   t->freetmp = NULL;
   return t;
   }

nodeptr tree3(type, loc_model, a)
int type;
nodeptr loc_model;
nodeptr a;
   {
   register nodeptr t;

   t = NewNode(1);
   t->n_type = type;
   t->n_file = loc_model->n_file;
   t->n_line = loc_model->n_line;
   t->n_col = loc_model->n_col;
   t->freetmp = NULL;
   t->n_field[0].n_ptr = a;
   return t;
   }

nodeptr tree4(type, loc_model, a, b)
int type;
nodeptr loc_model;
nodeptr a, b;
   {
   register nodeptr t;

   t = NewNode(2);
   t->n_type = type;
   t->n_file = loc_model->n_file;
   t->n_line = loc_model->n_line;
   t->n_col = loc_model->n_col;
   t->freetmp = NULL;
   t->n_field[0].n_ptr = a;
   t->n_field[1].n_ptr = b;
   return t;
   }

nodeptr tree5(type, loc_model, a, b, c)
int type;
nodeptr loc_model;
nodeptr a, b, c;
   {
   register nodeptr t;

   t = NewNode(3);
   t->n_type = type;
   t->n_file = loc_model->n_file;
   t->n_line = loc_model->n_line;
   t->n_col = loc_model->n_col;
   t->freetmp = NULL;
   t->n_field[0].n_ptr = a;
   t->n_field[1].n_ptr = b;
   t->n_field[2].n_ptr = c;
   return t;
   }

nodeptr tree6(type, loc_model, a, b, c, d)
int type;
nodeptr loc_model;
nodeptr a, b, c, d;
   {
   register nodeptr t;

   t = NewNode(4);
   t->n_type = type;
   t->n_file = loc_model->n_file;
   t->n_line = loc_model->n_line;
   t->n_col = loc_model->n_col;
   t->freetmp = NULL;
   t->n_field[0].n_ptr = a;
   t->n_field[1].n_ptr = b;
   t->n_field[2].n_ptr = c;
   t->n_field[3].n_ptr = d;
   return t;
   }

nodeptr int_leaf(type, loc_model, a)
int type;
nodeptr loc_model;
int a;
   {
   register nodeptr t;

   t = NewNode(1);
   t->n_type = type;
   t->n_file = loc_model->n_file;
   t->n_line = loc_model->n_line;
   t->n_col = loc_model->n_col;
   t->freetmp = NULL;
   t->n_field[0].n_val = a;
   return t;
   }

nodeptr c_str_leaf(type, loc_model, a)
int type;
nodeptr loc_model;
char *a;
   {
   register nodeptr t;

   t = NewNode(1);
   t->n_type = type;
   t->n_file = loc_model->n_file;
   t->n_line = loc_model->n_line;
   t->n_col = loc_model->n_col;
   t->freetmp = NULL;
   t->n_field[0].n_str = a;
   return t;
   }

/*
 * i_str_leaf - create a leaf node containing a string and length.
 */
nodeptr i_str_leaf(type, loc_model, a, b)
int type;
nodeptr loc_model;
char *a;
int b;
   {
   register nodeptr t;

   t = NewNode(2);
   t->n_type = type;
   t->n_file = loc_model->n_file;
   t->n_line = loc_model->n_line;
   t->n_col = loc_model->n_col;
   t->freetmp = NULL;
   t->n_field[0].n_str = a;
   t->n_field[1].n_val = b;
   return t;
   }

/*
 * key_leaf - create a leaf node for a keyword.
 */
nodeptr key_leaf(loc_model, keyname)
nodeptr loc_model;
char *keyname;
   {
   register nodeptr t;
   struct implement *ip;
   struct il_code *il;
   char *s;
   int typcd;

   /*
    * Find the data base entry for the keyword, if it exists.
    */
   ip = db_ilkup(keyname, khash);

   if (ip == NULL)
      tfatal("invalid keyword", keyname);
   else if (ip->in_line == NULL)
      tfatal("keyword not installed", keyname);
   else {
      il = ip->in_line;
      s = il->u[1].s;
      if (il->il_type == IL_Const) {
        /*
         * This is a constant keyword, treat it as a literal.
         */
        t = NewNode(1);
        t->n_file = loc_model->n_file;
        t->n_line = loc_model->n_line;
        t->n_col = loc_model->n_col;
        t->freetmp = NULL;
        typcd = il->u[0].n;
        if (typcd == cset_typ) {
           t->n_type =  N_Cset;
           CSym0(t) = putlit(&s[1], F_CsetLit, strlen(s) - 2);
           }
        else if (typcd == int_typ) {
           t->n_type = N_Int;
           CSym0(t) = putlit(s, F_IntLit, 0);
           }
        else if (typcd == real_typ) {
           t->n_type = N_Real;
           CSym0(t) = putlit(s, F_RealLit, 0);
           }
        else if (typcd == str_typ) {
           t->n_type = N_Str;
           CSym0(t) = putlit(&s[1], F_StrLit, strlen(s) - 2);
           }
        return t;
        }
     }

   t = NewNode(2);
   t->n_type = N_InvOp;
   t->n_file = loc_model->n_file;
   t->n_line = loc_model->n_line;
   t->n_col = loc_model->n_col;
   t->freetmp = NULL;
   t->n_field[0].n_val = 0;      /* number of arguments */
   t->n_field[1].ip = ip;
   return t;
   }

/*
 * list_nd - create a list creation node.
 */
nodeptr list_nd(loc_model, args)
nodeptr loc_model;
nodeptr args;
   {
   register nodeptr t;
   struct implement *impl;
   int nargs;

   /*
    * Determine the number of arguments.
    */
   if (args->n_type == N_Empty)
      nargs = 0;
   else {
      nargs = 1;
      for (t = args; t->n_type == N_Elist; t = t->n_field[0].n_ptr)
         ++nargs;
      if (nargs > max_prm)
         max_prm = nargs;
      }

   impl = spec_op[ListOp];
   if (impl == NULL)
      nfatal(loc_model, "list creation not implemented", NULL);
   else if (impl->in_line == NULL)
      nfatal(loc_model, "list creation not installed", NULL);

   t = NewNode(nargs + 2);
   t->n_type = N_InvOp;
   t->n_file = loc_model->n_file;
   t->n_line = loc_model->n_line;
   t->n_col = loc_model->n_col;
   t->freetmp = NULL;
   t->n_field[0].n_val = nargs;
   t->n_field[1].ip = impl;
   if (nargs > 0)
      put_elms(t, args, nargs + 1);
   return t;
   }

/*
 * invk_nd - create a node for invocation.
 */
nodeptr invk_nd(loc_model, proc, args)
nodeptr loc_model;
nodeptr proc;
nodeptr args;
   {
   register nodeptr t;
   int nargs;

   /*
    * Determine the number of arguments.
    */
   if (args->n_type == N_Empty)
      nargs = 0;
   else {
      nargs = 1;
      for (t = args; t->n_type == N_Elist; t = t->n_field[0].n_ptr)
         ++nargs;
      if (nargs > max_prm)
         max_prm = nargs;
      }

   t = NewNode(nargs + 2);
   t->n_type = N_Invok;
   t->n_file = loc_model->n_file;
   t->n_line = loc_model->n_line;
   t->n_col = loc_model->n_col;
   t->freetmp = NULL;
   t->n_field[0].n_val = nargs;
   t->n_field[1].n_ptr = proc;
   if (nargs > 0)
      put_elms(t, args, nargs + 1);
   return t;
   }

/*
 * put_elms - convert a linked list of arguments into an array of arguments
 *  in a node.
 */
static void put_elms(t, args, slot)
nodeptr t;
nodeptr args;
int slot;
   {
   if (args->n_type == N_Elist) {
      /*
       * The linked list is in reverse argument order.
       */
      t->n_field[slot].n_ptr = chk_empty(args->n_field[1].n_ptr);
      put_elms(t, args->n_field[0].n_ptr, slot - 1);
      free(args);
      }
   else
      t->n_field[slot].n_ptr = chk_empty(args);
   }

/*
 * chk_empty - if an argument is empty, replace it with &null.
 */
static nodeptr chk_empty(n)
nodeptr n;
   {
   if (n->n_type == N_Empty)
      n = key_leaf(n, spec_str("null"));
   return n;
   }

/*
 * case_nd - create a node for a case statement.
 */
nodeptr case_nd(loc_model, expr, cases)
nodeptr loc_model;
nodeptr expr;
nodeptr cases;
   {
   register nodeptr t;
   nodeptr reverse;
   nodeptr nxt_cases;
   nodeptr ccls;

   t = NewNode(3);
   t->n_type = N_Case;
   t->n_file = loc_model->n_file;
   t->n_line = loc_model->n_line;
   t->n_col = loc_model->n_col;
   t->freetmp = NULL;
   t->n_field[0].n_ptr = expr;
   t->n_field[2].n_ptr = NULL;

   /*
    * The list of cases is in reverse order. Walk the list reversing it,
    *  and extract the default clause if one exists.
    */
   reverse = NULL;
   while (cases->n_type != N_Ccls) {
      nxt_cases = cases->n_field[0].n_ptr;
      ccls = cases->n_field[1].n_ptr;
      if (ccls->n_field[0].n_ptr->n_type == N_Res) {
         /*
          * default clause.
          */
         if (t->n_field[2].n_ptr == NULL)
            t->n_field[2].n_ptr = ccls->n_field[1].n_ptr;
         else
            nfatal(ccls, "duplicate default clause", NULL);
         }
       else {
          if (reverse == NULL) {
             reverse = cases;
             reverse->n_field[0].n_ptr = ccls;
             }
          else {
             reverse->n_field[1].n_ptr = ccls;
             cases->n_field[0].n_ptr = reverse;
             reverse = cases;
             }
         }
      cases = nxt_cases;
      }

   /*
    * Last element in list.
    */
   if (cases->n_field[0].n_ptr->n_type == N_Res) {
      /*
       * default clause.
       */
      if (t->n_field[2].n_ptr == NULL)
         t->n_field[2].n_ptr = cases->n_field[1].n_ptr;
      else
         nfatal(ccls, "duplicate default clause", NULL);
      if (reverse != NULL)
         reverse = reverse->n_field[0].n_ptr;
      }
   else {
      if (reverse == NULL)
         reverse = cases;
      else
         reverse->n_field[1].n_ptr = cases;
      }
   t->n_field[1].n_ptr = reverse;
   return t;
   }

/*
 * multiunary - construct nodes to implement a sequence of unary operators
 *  that have been lexically analyzed as one operator.
 */
nodeptr multiunary(op, loc_model, oprnd)
nodeptr loc_model;
char *op;
nodeptr oprnd;
   {
   int n;
   nodeptr nd;

   if (*op == '\0')
     return oprnd;
   for (n = 0; optab[n].tok.t_word != NULL; ++n)
      if ((optab[n].expected & Unary) & (*(optab[n].tok.t_word) == *op)) {
         nd = OpNode(n);
         nd->n_file = loc_model->n_file;
         nd->n_line = loc_model->n_line;
         nd->n_col = loc_model->n_col;
         return unary_nd(nd,multiunary(++op,loc_model,oprnd));
         }
   fprintf(stderr, "compiler error: inconsistent parsing of unary operators");
   exit(EXIT_FAILURE);
   }

/*
 * binary_nd - construct a node for a binary operator.
 */
nodeptr binary_nd(op, arg1, arg2)
nodeptr op;
nodeptr arg1;
nodeptr arg2;
   {
   register nodeptr t;
   struct implement *impl;

   /*
    * Find the data base entry for the operator.
    */
   impl = optab[Val0(op)].binary;
   if (impl == NULL)
      nfatal(op, "binary operator not implemented", optab[Val0(op)].tok.t_word);
   else if (impl->in_line == NULL)
      nfatal(op, "binary operator not installed", optab[Val0(op)].tok.t_word);

   t = NewNode(4);
   t->n_type = N_InvOp;
   t->n_file = op->n_file;
   t->n_line = op->n_line;
   t->n_col = op->n_col;
   t->freetmp = NULL;
   t->n_field[0].n_val = 2;      /* number of arguments */
   t->n_field[1].ip = impl;
   t->n_field[2].n_ptr = arg1;
   t->n_field[3].n_ptr = arg2;
   return t;
   }

/*
 * unary_nd - construct a node for a unary operator.
 */
nodeptr unary_nd(op, arg)
nodeptr op;
nodeptr arg;
   {
   register nodeptr t;
   struct implement *impl;

   /*
    * Find the data base entry for the operator.
    */
   impl = optab[Val0(op)].unary;
   if (impl == NULL)
      nfatal(op, "unary operator not implemented", optab[Val0(op)].tok.t_word);
   else if (impl->in_line == NULL)
      nfatal(op, "unary operator not installed", optab[Val0(op)].tok.t_word);

   t = NewNode(3);
   t->n_type = N_InvOp;
   t->n_file = op->n_file;
   t->n_line = op->n_line;
   t->n_col = op->n_col;
   t->freetmp = NULL;
   t->n_field[0].n_val = 1;      /* number of arguments */
   t->n_field[1].ip = impl;
   t->n_field[2].n_ptr = arg;
   return t;
   }

/*
 * buildarray - convert "multi-dimensional" subscripting into a sequence
 *  of subsripting operations.
 */
nodeptr buildarray(a,lb,e)
nodeptr a, lb, e;
   {
   register nodeptr t, t2;
   if (e->n_type == N_Elist) {
      t2 = int_leaf(lb->n_type, lb, lb->n_field[0].n_val);
      t = subsc_nd(t2, buildarray(a,lb,e->n_field[0].n_ptr),
         e->n_field[1].n_ptr);
      free(e);
      }
   else
      t = subsc_nd(lb, a, e);
   return t;
   }

/*
 * subsc_nd - construct a node for subscripting.
 */
static nodeptr subsc_nd(op, arg1, arg2)
nodeptr op;
nodeptr arg1;
nodeptr arg2;
   {
   register nodeptr t;
   struct implement *impl;

   /*
    * Find the data base entry for subscripting.
    */
   impl = spec_op[SubscOp];
   if (impl == NULL)
      nfatal(op, "subscripting not implemented", NULL);
   else if (impl->in_line == NULL)
      nfatal(op, "subscripting not installed", NULL);

   t = NewNode(4);
   t->n_type = N_InvOp;
   t->n_file = op->n_file;
   t->n_line = op->n_line;
   t->n_col = op->n_col;
   t->freetmp = NULL;
   t->n_field[0].n_val = 2;      /* number of arguments */
   t->n_field[1].ip = impl;
   t->n_field[2].n_ptr = arg1;
   t->n_field[3].n_ptr = arg2;
   return t;
   }

/*
 * to_nd - construct a node for binary to.
 */
nodeptr to_nd(op, arg1, arg2)
nodeptr op;
nodeptr arg1;
nodeptr arg2;
   {
   register nodeptr t;
   struct implement *impl;

   /*
    * Find the data base entry for to.
    */
   impl = spec_op[ToOp];
   if (impl == NULL)
      nfatal(op, "'i to j' not implemented", NULL);
   else if (impl->in_line == NULL)
      nfatal(op, "'i to j' not installed", NULL);

   t = NewNode(4);
   t->n_type = N_InvOp;
   t->n_file = op->n_file;
   t->n_line = op->n_line;
   t->n_col = op->n_col;
   t->freetmp = NULL;
   t->n_field[0].n_val = 2;      /* number of arguments */
   t->n_field[1].ip = impl;
   t->n_field[2].n_ptr = arg1;
   t->n_field[3].n_ptr = arg2;
   return t;
   }

/*
 * toby_nd - construct a node for binary to-by.
 */
nodeptr toby_nd(op, arg1, arg2, arg3)
nodeptr op;
nodeptr arg1;
nodeptr arg2;
nodeptr arg3;
   {
   register nodeptr t;
   struct implement *impl;

   /*
    * Find the data base entry for to-by.
    */
   impl = spec_op[ToByOp];
   if (impl == NULL)
      nfatal(op, "'i to j by k' not implemented", NULL);
   else if (impl->in_line == NULL)
      nfatal(op, "'i to j by k' not installed", NULL);

   t = NewNode(5);
   t->n_type = N_InvOp;
   t->n_file = op->n_file;
   t->n_line = op->n_line;
   t->n_col = op->n_col;
   t->freetmp = NULL;
   t->n_field[0].n_val = 3;      /* number of arguments */
   t->n_field[1].ip = impl;
   t->n_field[2].n_ptr = arg1;
   t->n_field[3].n_ptr = arg2;
   t->n_field[4].n_ptr = arg3;
   return t;
   }

/*
 * aug_nd - create a node for an augmented assignment.
 */
nodeptr aug_nd(op, arg1, arg2)
nodeptr op;
nodeptr arg1;
nodeptr arg2;
   {
   register nodeptr t;
   struct implement *impl;

   t = NewNode(5);
   t->n_type = N_Augop;
   t->n_file = op->n_file;
   t->n_line = op->n_line;
   t->n_col = op->n_col;
   t->freetmp = NULL;

   /*
    * Find the data base entry for assignment.
    */
   impl = optab[asgn_loc].binary;
   if (impl == NULL)
      nfatal(op, "assignment not implemented", NULL);
   t->n_field[0].ip = impl;

   /*
    * The operator table entry for the augmented assignment is
    *  immediately after the entry for the operation.
    */
   impl = optab[Val0(op) - 1].binary;
   if (impl == NULL)
      nfatal(op, "binary operator not implemented",
         optab[Val0(op) - 1].tok.t_word);
   t->n_field[1].ip = impl;

   t->n_field[2].n_ptr = arg1;
   t->n_field[3].n_ptr = arg2;
   /* t->n_field[4].typ - type of intermediate result */
   return t;
   }

/*
 * sect_nd - create a node for sectioning.
 */
nodeptr sect_nd(op, arg1, arg2, arg3)
nodeptr op;
nodeptr arg1;
nodeptr arg2;
nodeptr arg3;
   {
   register nodeptr t;
   int tok;
   struct implement *impl;
   struct implement *impl1;

   t = NewNode(5);
   t->n_file = op->n_file;
   t->n_line = op->n_line;
   t->n_col = op->n_col;
   t->freetmp = NULL;

   /*
    * Find the data base entry for sectioning.
    */
   impl = spec_op[SectOp];
   if (impl == NULL)
      nfatal(op, "sectioning not implemented", NULL);

   tok = optab[Val0(op)].tok.t_type;
   if (tok == COLON) {
      /*
       * Simple sectioning, treat as a ternary operator.
       */
      t->n_type = N_InvOp;
      t->n_field[0].n_val = 3;      /* number of arguments */
      t->n_field[1].ip = impl;
      }
   else {
      /*
       * Find the data base entry for addition or subtraction.
       */
      if (tok == PCOLON) {
         impl1 = optab[plus_loc].binary;
         if (impl1 == NULL)
            nfatal(op, "addition not implemented", NULL);
         }
      else { /* MCOLON */
         impl1 = optab[minus_loc].binary;
         if (impl1 == NULL)
            nfatal(op, "subtraction not implemented", NULL);
         }
      t->n_type = N_Sect;
      t->n_field[0].ip = impl;
      t->n_field[1].ip = impl1;
      }
   t->n_field[2].n_ptr = arg1;
   t->n_field[3].n_ptr = arg2;
   t->n_field[4].n_ptr = arg3;
   return t;
   }

/*
 * invk_main - produce an procedure invocation node with one argument for
 *  use in the initial invocation to main() during type inference.
 */
nodeptr invk_main(main_proc)
struct pentry *main_proc;
   {
   register nodeptr t;

   t = NewNode(3);
   t->n_type = N_InvProc;
   t->n_file = NULL; 
   t->n_line = 0;
   t->n_col = 0;
   t->freetmp = NULL;
   t->n_field[0].n_val = 1;               /* 1 argument */
   t->n_field[1].proc = main_proc;
   t->n_field[2].n_ptr = tree1(N_Empty);

   if (max_prm < 1)
      max_prm = 1;
   return t;
   }
