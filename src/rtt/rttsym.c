/*
 * rttsym.c contains symbol table routines.
 */
#include "rtt.h"

#define HashSize 149

/*
 * Prototype for static function.
 */
static void add_def    (struct node *dcltor);
static void add_s_prm  (struct token *ident, int param_num, int flags);
static void dcl_typ    (struct node *dcl);
static void dcltor_typ (struct node *dcltor, struct node *tqual);

word lbl_num = 0;                   /* next unused label number */
struct lvl_entry *dcl_stk;          /* stack of declaration contexts */

char *str_rslt;                     /* string "result" in string table */
struct init_tend *tend_lst = NULL;  /* list of tended descriptors */
struct sym_entry *decl_lst = NULL;  /* declarations from "declare {...}" */
struct sym_entry *v_len = NULL;     /* entry for length of varargs */
int il_indx = 0;                    /* data base symbol table index */

static struct sym_entry *sym_tbl[HashSize]; /* symbol table */

/*
 * The following strings are put in the string table and used for
 *  recognizing valid tended declarations.
 */
static char *block = "block";
static char *descrip = "descrip";

/*
 * init_sym - initialize  symbol table.
 */
void init_sym()
   {
   static int first_time = 1;
   int hash_val;
   register struct sym_entry *sym;
   int i;

   /*
    * Initialize the symbol table and declaration stack. When called for
    *  the first time, put strings in string table.
    */
   if (first_time) {
      first_time = 0;
      for (i = 0; i < HashSize; ++i)
         sym_tbl[i] = NULL;
      dcl_stk = NewStruct(lvl_entry);
      dcl_stk->nest_lvl = 1;
      dcl_stk->next = NULL;
      block = spec_str(block);
      descrip = spec_str(descrip);
      }
   else {
      for (hash_val = 0; hash_val < HashSize; ++ hash_val) {
         for (sym = sym_tbl[hash_val]; sym != NULL &&
           sym->nest_lvl > 0; sym = sym_tbl[hash_val]) {
            sym_tbl[hash_val] = sym->next;
            free((char *)sym);
            }
         }
      }
   dcl_stk->kind_dcl = OtherDcl;
   dcl_stk->parms_done = 0;
   }

/*
 * sym_lkup - look up a string in the symbol table. Return NULL If it is not
 *  there.
 */
struct sym_entry *sym_lkup(image)
char *image;
   {
   register struct sym_entry *sym;

   for (sym = sym_tbl[(unsigned int)(unsigned long)image % HashSize];
         sym != NULL;
         sym = sym->next)
      if (sym->image == image)
         return sym;
   return NULL;
   }

/*
 * sym_add - add a symbol to the symbol table. For some types of entries
 *  it is illegal to redefine them. In that case, NULL is returned otherwise
 *  the entry is returned.
 */
struct sym_entry *sym_add(tok_id, image, id_type, nest_lvl)
int tok_id;
char *image;
int id_type;
int nest_lvl;
   {
   register struct sym_entry **symp;
   register struct sym_entry *sym;

   symp = &sym_tbl[(unsigned int)(unsigned long)image % HashSize];
   while (*symp != NULL && (*symp)->nest_lvl > nest_lvl)
      symp = &((*symp)->next);
   while (*symp != NULL && (*symp)->nest_lvl == nest_lvl) {
      if ((*symp)->image == image) {
         /*
          * Redeclaration:
          *
          * An explicit typedef may be given for a built-in typedef
          * name. A label appears in multiply gotos and as a label
          * on a statement. Assume a global redeclaration is for an
          * extern. Return the entry for these situations but don't
          * try too hard to detect errors. If actual errors are not
          * caught here, the C compiler will find them.
          */
         if (tok_id == TypeDefName && ((*symp)->tok_id == C_Integer ||
            (*symp)->tok_id == TypeDefName))
            return *symp;
         if (id_type == Label && (*symp)->id_type == Label)
            return *symp;
         if ((*symp)->nest_lvl == 1)
            return *symp;
         return NULL;	/* illegal redeclarations */
         }
      symp = &((*symp)->next);
      }

   /*
    * No entry exists for the symbol, create one, fill in its fields, and add
    *  it to the table.
    */
   sym = NewStruct(sym_entry);
   sym->tok_id = tok_id;
   sym->image = image;
   sym->id_type =  id_type;
   sym->nest_lvl = nest_lvl;
   sym->ref_cnt = 1;
   sym->il_indx = -1;
   sym->may_mod = 0;
   if (id_type == Label)
      sym->u.lbl_num = lbl_num++;
   sym->next = *symp;
   *symp = sym;

   return sym;     /* success */
   }

/*
 * lbl - make sure the label is in the symbol table and return a node
 *  referencing the symbol table entry.
 */
struct node *lbl(t)
struct token *t;
   {
   struct sym_entry *sym;
   struct node *n;

   sym = sym_add(Identifier, t->image, Label, 2);
   if (sym == NULL)
      errt2(t, "conflicting definitions for ", t->image);
   n = sym_node(t);
   if (n->u[0].sym != sym)
      errt2(t, "conflicting definitions for ", t->image);
   return n;
   }

/*
 * push_cntxt - push a level of declaration context (this may or may not
 *  be level of declaration nesting).
 */
void push_cntxt(lvl_incr)
int lvl_incr;
   {
   struct lvl_entry *entry;

   entry = NewStruct(lvl_entry);
   entry->nest_lvl = dcl_stk->nest_lvl + lvl_incr;
   entry->kind_dcl = OtherDcl;
   entry->parms_done = 0;
   entry->tended = NULL;
   entry->next = dcl_stk;
   dcl_stk = entry;
   }

/*
 * pop_cntxt - end a level of declaration context
 */
void pop_cntxt()
   {
   int hash_val;
   int old_lvl;
   int new_lvl;
   register struct sym_entry *sym;
   struct lvl_entry *entry;

   /*
    * Move the top entry of the stack to the free list.
    */
   old_lvl = dcl_stk->nest_lvl;
   entry = dcl_stk;
   dcl_stk = dcl_stk->next;
   free((char *)entry);

   /*
    * If this pop reduced the declaration nesting level, remove obsolete
    *  entries from the symbol table.
    */
   new_lvl = dcl_stk->nest_lvl;
   if (old_lvl > new_lvl) {
      for (hash_val = 0; hash_val < HashSize; ++ hash_val) {
         for (sym = sym_tbl[hash_val]; sym != NULL &&
           sym->nest_lvl > new_lvl; sym = sym_tbl[hash_val]) {
            sym_tbl[hash_val] = sym->next;
            free_sym(sym);
            }
         }
      unuse(tend_lst, old_lvl);
      }
   }

/*
 * unuse - mark tended slots in at the given level of declarations nesting
 *  as being no longer in use, and leave the slots available for reuse
 *  for declarations that occur in pararallel compound statements.
 */
void unuse(t_lst, lvl)
struct init_tend *t_lst;
int lvl;
   {
   while (t_lst != NULL) {
      if (t_lst->nest_lvl >= lvl)
         t_lst->in_use = 0;
      t_lst = t_lst->next;
      }
   }

/*
 * free_sym - remove a reference to a symbol table entry and free storage
 *  related to it if no references remain.
 */
void free_sym(sym)
struct sym_entry *sym;
   {
   if (--sym->ref_cnt <= 0) {
      switch (sym->id_type) {
         case TndDesc:
         case TndStr:
         case TndBlk:
            free_tree(sym->u.tnd_var.init); /* initializer expression */
         }
      free((char *)sym);
      }
   }

/*
 * alloc_tnd - allocated a slot in a tended array for a variable and return
 *  its index.
 */
int alloc_tnd(typ, init, lvl)
int typ;
struct node *init;
int lvl;
   {
   register struct init_tend *tnd;

   if (lvl > 2) {
     /*
      * This declaration occurs in an inner compound statement. There
      *  may be slots created for parallel compound statement, but were
      *  freed and can be reused here.
      */
     tnd = tend_lst;
     while (tnd != NULL && (tnd->in_use || tnd->init_typ != typ))
       tnd = tnd->next;
     if (tnd != NULL) {
         tnd->in_use = 1;
         tnd->nest_lvl = lvl;
         return tnd->t_indx;
         }
      }

   /*
    * Allocate a new tended slot, compute its index in the array, and
    *  set initialization and other information.
    */
   tnd = NewStruct(init_tend);

   if (tend_lst == NULL)
      tnd->t_indx = 0;
   else
      tnd->t_indx = tend_lst->t_indx + 1;
   tnd->init_typ = typ;
   /*
    * The initialization from the declaration will only be used to
    *  set up the tended location if the declaration is in the outermost
    *  "block". Otherwise a generic initialization will be done during
    *  the set up and the one from the declaration will be put off until
    *  the block is entered.
    */
   if (lvl == 2)
      tnd->init = init;
   else
      tnd->init = NULL;
   tnd->in_use = 1;
   tnd->nest_lvl = lvl;
   tnd->next = tend_lst;
   tend_lst = tnd;
   return tnd->t_indx;
   }

/*
 * free_tend - put the list of tended descriptors on the free list.
 */
void free_tend()
   {
   register struct init_tend *tnd, *tnd1;

   for (tnd = tend_lst; tnd != NULL; tnd = tnd1) {
      tnd1 = tnd->next;
      free((char *)tnd);
      }
   tend_lst = NULL;
   }

/*
 * dst_alloc - the conversion of a parameter is encountered during
 *  parsing; make sure a place is allocated to act as the destination.
 */
void dst_alloc(cnv_typ, var)
struct node *cnv_typ;
struct node *var;
   {
   struct sym_entry *sym;

   if (var->nd_id == SymNd) {
      sym = var->u[0].sym;
      if (sym->id_type & DrfPrm) {
         switch (cnv_typ->tok->tok_id) {
            case C_Integer:
               sym->u.param_info.non_tend |= PrmInt;
               break;
            case C_Double:
               sym->u.param_info.non_tend |= PrmDbl;
               break;
            }
         }
      }
   }

/*
 * strt_def - the start of an operation definition is encountered during
 *  parsing; establish an new declaration context and make "result"
 *  a special identifier.
 */
void strt_def()
   {
   struct sym_entry *sym;

   push_cntxt(1);
   sym = sym_add(Identifier, str_rslt, RsltLoc, dcl_stk->nest_lvl);
   sym->u.referenced = 0;
   }

/*
 * add_def - update the symbol table for the given declarator.
 */
static void add_def(dcltor)
struct node *dcltor;
   {
   struct sym_entry *sym;
   struct token *t;
   int tok_id;

   /*
    * find the identifier within the declarator.
    */
   for (;;) {
      switch (dcltor->nd_id) {
         case BinryNd:
            /* ')' or '[' */
            dcltor = dcltor->u[0].child;
            break;
         case ConCatNd:
            /* pointer direct-declarator */
            dcltor = dcltor->u[1].child;
            break;
         case PrefxNd:
            /* ( ... ) */
            dcltor =  dcltor->u[0].child;
            break;
         case PrimryNd:
            t = dcltor->tok;
            if (t->tok_id == Identifier || t->tok_id == TypeDefName) {
               /*
                * We have found the identifier, add an entry to the
                *  symbol table based on information in the declaration
                *  context.
                */
               if (dcl_stk->kind_dcl == IsTypedef)
                  tok_id = TypeDefName;
               else
                  tok_id = Identifier;
               sym = sym_add(tok_id, t->image, OtherDcl, dcl_stk->nest_lvl);
               if (sym == NULL)
                  errt2(t, "redefinition of ", t->image);
               }
            return;
         default:
            return;
         }
      }
   }

/*
 * id_def - a declarator has been parsed. Determine what to do with it
 *  based on information put in the declaration context while parsing
 *  the "storage class type qualifier list".
 */
void id_def(dcltor, init)
struct node *dcltor;
struct node *init;
   {
   struct node *chld0, *chld1;
   struct sym_entry *sym;

   if (dcl_stk->parms_done)
      pop_cntxt();

   /*
    * Look in the declaration context (the top of the declaration stack)
    *  to see if this is a tended declaration.
    */
   switch (dcl_stk->kind_dcl) {
      case TndDesc:
      case TndStr:
      case TndBlk:
         /*
          * Tended variables are either simple identifiers or pointers to
          *  simple identifiers.
          */
         chld0 = dcltor->u[0].child;
         chld1 = dcltor->u[1].child;
         if (chld1->nd_id != PrimryNd || (chld1->tok->tok_id != Identifier &&
           chld1->tok->tok_id != TypeDefName))
             errt1(chld1->tok, "unsupported tended declaration");
         if (dcl_stk->kind_dcl == TndDesc) {
            /*
             * Declared as full tended descriptor - must not be a pointer.
             */
            if (chld0 != NULL)
               errt1(chld1->tok, "unsupported tended declaration");
            }
         else {
            /*
             * Must be a tended pointer.
             */
            if (chld0 == NULL || chld0->nd_id != PrimryNd)
               errt1(chld1->tok, "unsupported tended declaration");
            }

         /*
          * This is a legal tended declaration, make a symbol table entry
          *  for it and allocated a tended slot. Add the symbol table
          *  entry to the list of tended variables in this context.
          */
         sym = sym_add(Identifier, chld1->tok->image, dcl_stk->kind_dcl,
            dcl_stk->nest_lvl);
         if (sym == NULL)
            errt2(chld1->tok, "redefinition of ", chld1->tok->image);
         sym->u.tnd_var.blk_name = dcl_stk->blk_name;
         sym->u.tnd_var.init = init;
         sym->t_indx = alloc_tnd(dcl_stk->kind_dcl, init, dcl_stk->nest_lvl);
         sym->u.tnd_var.next = dcl_stk->tended;
         dcl_stk->tended = sym;
         ++sym->ref_cnt;
         return;
      default:
         add_def(dcltor); /* ordinary declaration */
      }
   }

/*
 * func_def - a function header has been parsed. Add the identifier for
 *  the function to the symbol table.
 */
void func_def(head)
struct node *head;
   {
   /*
    * If this is really a function header, the current declaration
    *  context indicates that a parameter list has been completed.
    *  Parameter lists at other than at nesting level 2 are part of
    *  nested declaration information and do not show up here. The
    *  function parameters must remain in the symbol table, so the
    *  context is just updated, not popped.
    */
   if (!dcl_stk->parms_done)
      yyerror("invalid declaration");
   dcl_stk->parms_done = 0;
   if (dcl_stk->next->kind_dcl == IsTypedef)
      yyerror("a typedef may not be a function definition");
   add_def(head->u[1].child);
   }

/*
 * s_prm_def - add symbol table entries for a parameter to an operation.
 *  Undereferenced and/or dereferenced versions of the parameter may be
 *  specified.
 */
void s_prm_def(u_ident, d_ident)
struct token *u_ident;
struct token *d_ident;
   {
   int param_num;

   if (params == NULL)
      param_num = 0;
   else
      param_num = params->u.param_info.param_num + 1;
   if (u_ident != NULL)
      add_s_prm(u_ident, param_num, RtParm);
   if (d_ident != NULL)
      add_s_prm(d_ident, param_num, DrfPrm);
   }

/*
 * add_s_prm - add a symbol table entry for either a dereferenced or
 *  undereferenced version of a parameter. Put it on the current
 *  list of parameters.
 */
static void add_s_prm(ident, param_num, flags)
struct token *ident;
int param_num;
int flags;
   {
   struct sym_entry *sym;

   sym = sym_add(Identifier, ident->image, flags, dcl_stk->nest_lvl);
   if (sym == NULL)
      errt2(ident, "redefinition of ", ident->image);
   sym->u.param_info.param_num = param_num;
   sym->u.param_info.non_tend = 0;
   sym->u.param_info.cur_loc = PrmTend;
   sym->u.param_info.parm_mod = 0;
   sym->u.param_info.next = params;
   sym->il_indx = il_indx++;
   params = sym;
   ++sym->ref_cnt;
   }

/*
 * var_args - a variable length parameter list for an operation is parsed.
 */
void var_args(ident)
struct token *ident;
   {
   struct sym_entry *sym;

   /*
    * The last parameter processed represents the variable part of the list;
    *  update the symbol table entry. It may be dereferenced or undereferenced
    *  but not both.
    */
   sym = params->u.param_info.next;
   if (sym != NULL && sym->u.param_info.param_num ==
      params->u.param_info.param_num)
         errt1(ident, "only one version of variable parameter list allowed");
   params->id_type |= VarPrm;

   /*
    * Add the identifier for the length of the variable part of the list
    *  to the symbol table.
    */
   sym = sym_add(Identifier, ident->image, VArgLen, dcl_stk->nest_lvl);
   if (sym == NULL)
      errt2(ident, "redefinition of ", ident->image);
   sym->il_indx = il_indx++;
   v_len = sym;
   ++v_len->ref_cnt;
   }

/*
 * d_lst_typ - the end of a "declare {...}" is encountered. Go through a
 *   declaration list adding storage class, type qualifier, declarator
 *   and initializer information to the symbol table entry for each
 *   identifier. Add the entry onto the list associated with the "declare"
 */
void d_lst_typ(dcls)
struct node *dcls;
   {
   if (dcls == NULL)
      return;
   for ( ; dcls != NULL && dcls->nd_id == LstNd; dcls = dcls->u[0].child)
      dcl_typ(dcls->u[1].child);
   dcl_typ(dcls);
   }

/*
 * dcl_typ - go through the declarators of a declaration adding the storage
 *   class, type qualifier, declarator, and initializer information to the
 *   symbol table entry of each identifier. Add the entry onto the list
 *   associated with the current "declare {...}".
 */
static void dcl_typ(dcl)
struct node *dcl;
   {
   struct node *tqual;
   struct node *dcltors;

   if (dcl == NULL)
      return;
   tqual = dcl->u[0].child;
   for (dcltors = dcl->u[1].child; dcltors->nd_id == CommaNd;
      dcltors = dcltors->u[0].child)
         dcltor_typ(dcltors->u[1].child, tqual);
   dcltor_typ(dcltors, tqual);
   }

/*
 * dcltor_typ- find the identifier in the [initialized] declarator and add
 *   the storage class, type qualifer, declarator, and initialization
 *   information to its symbol table entry. Add the entry onto the list
 *   associated with the current "declare {...}".
 */
static void dcltor_typ(dcltor, tqual)
struct node *dcltor;
struct node *tqual;
   {
   struct sym_entry *sym;
   struct node *part_dcltor;
   struct node *init = NULL;
   struct token *t;

   if (dcltor->nd_id == BinryNd && dcltor->tok->tok_id == '=') {
      init = dcltor->u[1].child;
      dcltor = dcltor->u[0].child;
      }
   part_dcltor = dcltor;
   for (;;) {
      switch (part_dcltor->nd_id) {
         case BinryNd:
            /* ')' or '[' */
            part_dcltor = part_dcltor->u[0].child;
            break;
         case ConCatNd:
            /* pointer direct-declarator */
            part_dcltor = part_dcltor->u[1].child;
            break;
         case PrefxNd:
            /* ( ... ) */
            part_dcltor =  part_dcltor->u[0].child;
            break;
         case PrimryNd:
            t = part_dcltor->tok;
            if (t->tok_id == Identifier || t->tok_id == TypeDefName) {
               /*
                * The identifier has been found, update its symbol table
                *  entry.
                */
               sym = sym_lkup(t->image);
               sym->u.declare_var.tqual = tqual;
               sym->u.declare_var.dcltor = dcltor;
               sym->u.declare_var.init = init;
               ++sym->ref_cnt;
               sym->u.declare_var.next = decl_lst;
               decl_lst = sym;
               }
            return;
         default:
            return;
         }
      }
   }

/*
 * tnd_char - indicate in the current declaration context that a tended
 *  character (pointer?) declaration has been found.
 */
void tnd_char()
   {
   dcl_stk->kind_dcl = TndStr;
   dcl_stk->blk_name = NULL;
   }

/*
 * tnd_strct - indicate in the current declaration context that a tended
 *  struct declaration has been found and indicate the struct type.
 */
void tnd_strct(t)
struct token *t;
   {
   char *strct_nm;

   strct_nm = t->image;
   free_t(t);

   if (strct_nm == descrip) {
      dcl_stk->kind_dcl = TndDesc;
      dcl_stk->blk_name = NULL;
      return;
      }
   dcl_stk->kind_dcl = TndBlk;
   dcl_stk->blk_name = strct_nm;
   }

/*
 * tnd_strct - indicate in the current declaration context that a tended
 *  union (pointer?) declaration has been found.
 */
void tnd_union(t)
struct token *t;
   {
   /*
    * Only union block pointers may be tended.
    */
   if (t->image != block)
      yyerror("unsupported tended type");
   free_t(t);
   dcl_stk->kind_dcl = TndBlk;
   dcl_stk->blk_name = NULL;
   }
