#include "rtt.h"

/*
 * node0 - create a syntax tree leaf node.
 */
struct node *node0(id, tok)
int id;
struct token *tok;
   {
   struct node *n;

   n = NewNode(0);
   n->nd_id = id;
   n->tok = tok;
   return n;
   }

/*
 * node1 - create a syntax tree node with one child.
 */
struct node *node1(id, tok, n1)
int id;
struct token *tok;
struct node *n1;
   {
   struct node *n;

   n = NewNode(1);
   n->nd_id = id;
   n->tok = tok;
   n->u[0].child = n1;
   return n;
   }

/*
 * node2 - create a syntax tree node with two children.
 */
struct node *node2(id, tok, n1, n2)
int id;
struct token *tok;
struct node *n1;
struct node *n2;
   {
   struct node *n;

   n = NewNode(2);
   n->nd_id = id;
   n->tok = tok;
   n->u[0].child = n1;
   n->u[1].child = n2;
   return n;
   }

/*
 * node3 - create a syntax tree node with three children.
 */
struct node *node3(id, tok, n1, n2, n3)
int id;
struct token *tok;
struct node *n1;
struct node *n2;
struct node *n3;
   {
   struct node *n;

   n = NewNode(3);
   n->nd_id = id;
   n->tok = tok;
   n->u[0].child = n1;
   n->u[1].child = n2;
   n->u[2].child = n3;
   return n;
   }

/*
 * node4 - create a syntax tree node with four children.
 */
struct node *node4(id, tok, n1, n2, n3, n4)
int id;
struct token *tok;
struct node *n1;
struct node *n2;
struct node *n3;
struct node *n4;
   {
   struct node *n;

   n = NewNode(4);
   n->nd_id = id;
   n->tok = tok;
   n->u[0].child = n1;
   n->u[1].child = n2;
   n->u[2].child = n3;
   n->u[3].child = n4;
   return n;
   }

/*
 * sym_node - create a syntax tree node for a variable. If the identifier
 *  is in the symbol table, create a node that references the entry,
 *  otherwise create a simple leaf node.
 */
struct node *sym_node(tok)
struct token *tok;
   {
   struct sym_entry *sym;
   struct node *n;

   sym = sym_lkup(tok->image);
   if (sym != NULL) {
      n = NewNode(1);
      n->nd_id = SymNd;
      n->tok = tok;
      n->u[0].sym = sym;
      ++sym->ref_cnt;
      /*
       * If this is the result location of an operation, note that it
       *  is explicitly referenced.
       */
      if (sym->id_type == RsltLoc)
         sym->u.referenced = 1;
      return n;
      }
   else
      return node0(PrimryNd, tok);
   }

/*
 * comp_nd - create a node for a compound statement.
 */
struct node *comp_nd(tok, dcls, stmts)
struct token *tok;
struct node *dcls;
struct node *stmts;
   {
   struct node *n;

   n = NewNode(3);
   n->nd_id = CompNd;
   n->tok = tok;
   n->u[0].child = dcls;
   n->u[1].sym = dcl_stk->tended; /* tended declarations are not in dcls */
   n->u[2].child = stmts;
   return n;
   }

/*
 * arith_nd - create a node for an arith_case statement.
 */
struct node *arith_nd(tok, p1, p2, c_int, ci_act, intgr, i_act, dbl, d_act)
struct token *tok;
struct node *p1;
struct node *p2;
struct node *c_int;
struct node *ci_act;
struct node *intgr;
struct node *i_act;
struct node *dbl;
struct node *d_act;
   {
   struct node *n;

   /*
    * Insure the cases are what we expect.
    */
   if (c_int->tok->tok_id != C_Integer)
      errt3(c_int->tok, "expected \"C_integer\", found \"", c_int->tok->image,
         "\"");
   if (intgr->tok->image != icontypes[int_typ].id)
      errt3(intgr->tok, "expected \"integer\", found \"", intgr->tok->image,
         "\"");
   if (dbl->tok->tok_id != C_Double)
      errt3(dbl->tok, "expected \"C_double\", found \"", dbl->tok->image,
         "\"");

   /*
    * Indicate in the symbol table that the arguments are converted to C
    *  values.
    */
   dst_alloc(c_int, p1);
   dst_alloc(c_int, p2);
   dst_alloc(dbl, p1);
   dst_alloc(dbl, p2);

   free_tree(c_int);
   free_tree(intgr);
   free_tree(dbl);

   n = node3(TrnryNd, NULL, ci_act, i_act, d_act);
   return node3(TrnryNd, tok, p1, p2, n);
   }

struct node *dest_node(tok)
struct token *tok;
   {
   struct node *n;
   int typcd;

   n = sym_node(tok);
   typcd = n->u[0].sym->u.typ_indx;
   if (typcd != int_typ && typcd != str_typ && typcd != cset_typ &&
      typcd != real_typ)
      errt2(tok, "cannot convert to ", tok->image);
   return n;
   }


/*
 * free_tree - free storage for a syntax tree.
 */
void free_tree(n)
struct node *n;
   {
   struct sym_entry *sym, *sym1;

   if (n == NULL)
      return;

   /*
    * Free any subtrees and other referenced storage.
    */
   switch (n->nd_id) {
      case SymNd:
         free_sym(n->u[0].sym); /* Indicate one less reference to symbol */
         break;

      case CompNd:
         /*
          * Compound node. Free ordinary declarations, tended declarations,
          *  and executable code.
          */
         free_tree(n->u[0].child);
         sym = n->u[1].sym;
         while (sym != NULL) {
            sym1 = sym;
            sym = sym->u.tnd_var.next;
            free_sym(sym1);
            }
         free_tree(n->u[2].child);
         break;

      case QuadNd:
         free_tree(n->u[3].child);
         /* fall thru to next case */
      case TrnryNd:
         free_tree(n->u[2].child);
         /* fall thru to next case */
      case AbstrNd: case BinryNd: case CommaNd: case ConCatNd: case LstNd:
      case StrDclNd:
         free_tree(n->u[1].child);
         /* fall thru to next case */
      case IcnTypNd: case PstfxNd: case PreSpcNd: case PrefxNd:
         free_tree(n->u[0].child);
         /* fall thru to next case */
      case ExactCnv: case PrimryNd:
         break;

      default:
         fprintf(stdout, "rtt internal error: unknown node type\n");
         exit(EXIT_FAILURE);
         }
   free_t(n->tok);             /* free token */
   free((char *)n);
   }
