/*
 * tree.c -- functions for constructing parse trees
 */

#include "../h/gsupport.h"
#include "tproto.h"
#include "tree.h"

/*
 *  tree[1-6] construct parse tree nodes with specified values.
 *   Parameters a and b are line and column information,
 *   while parameters c through f are values to be assigned to n_field[0-3].
 *   Note that this could be done with a single routine; a separate routine
 *   for each node size is used for speed and simplicity.
 */

nodeptr tree1(type)
int type;
   {
   register nodeptr t;

   t = NewNode(0);
   t->n_type = type;
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
   return t;
   }

nodeptr tree3(type, loc_model, c)
int type;
nodeptr loc_model;
nodeptr c;
   {
   register nodeptr t;

   t = NewNode(1);
   t->n_type = type;
   t->n_file = loc_model->n_file;
   t->n_line = loc_model->n_line;
   t->n_col = loc_model->n_col;
   t->n_field[0].n_ptr = c;
   return t;
   }

nodeptr tree4(type, loc_model, c, d)
int type;
nodeptr loc_model;
nodeptr c, d;
   {
   register nodeptr t;

   t = NewNode(2);
   t->n_type = type;
   t->n_file = loc_model->n_file;
   t->n_line = loc_model->n_line;
   t->n_col = loc_model->n_col;
   t->n_field[0].n_ptr = c;
   t->n_field[1].n_ptr = d;
   return t;
   }

nodeptr tree5(type, loc_model, c, d, e)
int type;
nodeptr loc_model;
nodeptr c, d, e;
   {
   register nodeptr t;

   t = NewNode(3);
   t->n_type = type;
   t->n_file = loc_model->n_file;
   t->n_line = loc_model->n_line;
   t->n_col = loc_model->n_col;
   t->n_field[0].n_ptr = c;
   t->n_field[1].n_ptr = d;
   t->n_field[2].n_ptr = e;
   return t;
   }

nodeptr tree6(type, loc_model, c, d, e, f)
int type;
nodeptr loc_model;
nodeptr c, d, e, f;
   {
   register nodeptr t;

   t = NewNode(4);
   t->n_type = type;
   t->n_file = loc_model->n_file;
   t->n_line = loc_model->n_line;
   t->n_col = loc_model->n_col;
   t->n_field[0].n_ptr = c;
   t->n_field[1].n_ptr = d;
   t->n_field[2].n_ptr = e;
   t->n_field[3].n_ptr = f;
   return t;
   }

nodeptr buildarray(a,lb,e,rb)
nodeptr a, lb, e, rb;
   {
   register nodeptr t, t2;
   if (e->n_type == N_Elist) {
      t2 = int_leaf(lb->n_type, lb, (int)lb->n_field[0].n_val);
      t = tree5(N_Binop, t2, t2, buildarray(a,lb,e->n_field[0].n_ptr,rb),
		e->n_field[1].n_ptr);
      free(e);
      }
   else
      t = tree5(N_Binop, lb, lb, a, e);
   return t;
   }

nodeptr int_leaf(type, loc_model, c)
int type;
nodeptr loc_model;
int c;
   {
   register nodeptr t;

   t = NewNode(1);
   t->n_type = type;
   t->n_file = loc_model->n_file;
   t->n_line = loc_model->n_line;
   t->n_col = loc_model->n_col;
   t->n_field[0].n_val = c;
   return t;
   }

nodeptr c_str_leaf(type, loc_model, c)
int type;
nodeptr loc_model;
char *c;
   {
   register nodeptr t;

   t = NewNode(1);
   t->n_type = type;
   t->n_file = loc_model->n_file;
   t->n_line = loc_model->n_line;
   t->n_col = loc_model->n_col;
   t->n_field[0].n_str = c;
   return t;
   }

nodeptr i_str_leaf(type, loc_model, c, d)
int type;
nodeptr loc_model;
char *c;
int d;
   {
   register nodeptr t;

   t = NewNode(2);
   t->n_type = type;
   t->n_file = loc_model->n_file;
   t->n_line = loc_model->n_line;
   t->n_col = loc_model->n_col;
   t->n_field[0].n_str = c;
   t->n_field[1].n_val = d;
   return t;
   }

