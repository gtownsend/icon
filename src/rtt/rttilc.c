/*
 * rttilc.c - routines to construct pieces of C code to put in the data base
 *  as in-line code.
 *
 * In-line C code is represented internally as a linked list of structures.
 * The information contained in each structure depends on the type of code
 * being represented. Some structures contain other fragments of C code.
 * Code that does not require special processing is stored as strings. These
 * strings are accumulated in a buffer until it is full or code that cannot
 * be represented as a string must be produced. At that point, the string
 * in placed in a structure and put on the list.
 */
#include "rtt.h"

#ifndef Rttx

/*
 * prototypes for static functions.
 */
static void add_ptr   (struct node *dcltor);
static void alloc_ilc (int il_c_type);
static void flush_str (void);
static void ilc_chnl  (struct token *t);
static void ilc_cnv   (struct node *cnv_typ, struct node *src,
                           struct node *dflt, struct node *dest);
static void ilc_cgoto (int neg, struct node *cond, word lbl);
static void ilc_goto  (word lbl);
static void ilc_lbl   (word lbl);
static void ilc_ret   (struct token *t, int ilc_typ, struct node *n);
static void ilc_str   (char *s);
static void ilc_tok   (struct token *t);
static void ilc_var   (struct sym_entry *sym, int just_desc, int may_mod);
static void ilc_walk  (struct node *n, int may_mod, int const_cast);
static void init_ilc  (void);
static void insrt_str (void);
static void new_ilc   (int il_c_type);
static	struct	il_c	*sep_ilc	(char *s1,struct node *n,char *s2);

#define SBufSz 256

static char sbuf[SBufSz];      /* buffer for constructing fragments of code */
static int nxt_char;           /* next position in sbuf */
static struct token *line_ref; /* "recent" token for comparing line number */
static struct il_c ilc_base;   /* base for list of in-line C code */
static struct il_c *ilc_cur;   /* current end of list of in-line C code */
static int insert_nl;          /* flag: new-line should be inserted in code */
static word cont_lbl = 0;      /* destination label for C continue statement */
static word brk_lbl = 0;       /* destination label for C break statement */

/*
 * inlin_c - Create a self-contained piece of in-line C code from a syntax
 *   sub-tree.
 */
struct il_c *inlin_c(n, may_mod)
struct node *n;
int may_mod;
   {
   init_ilc();              /* initialize code list and string buffer */
   ilc_walk(n, may_mod, 0); /* translate the syntax sub-tree */
   flush_str();            /* flush string buffer to code list */
   return ilc_base.next;
   }

/*
 * simpl_dcl - produce a simple declaration both in the output file and as
 *   in-line C code.
 */
struct il_c *simpl_dcl(tqual, addr_of, sym)
char *tqual;
int addr_of;
struct sym_entry *sym;
   {
   init_ilc();             /* initialize code list and string buffer */
   prt_str(tqual, 0);
   ilc_str(tqual);
   if (addr_of) {
      prt_str("*", 0);
      ilc_str("*");
      }
   prt_str(sym->image, 0);
   ilc_str(sym->image);
   prt_str(";", 0);
   ForceNl();
   flush_str();            /* flush string buffer to code list */
   return ilc_base.next;
   }

/*
 * parm_dcl - produce the declaration for a parameter to a body function.
 *   Print it in the output file and proceduce in-line C code for it.
 */
struct il_c *parm_dcl(addr_of, sym)
int addr_of;
struct sym_entry *sym;
   {
   init_ilc();        /* initialize code list and string buffer */

   /*
    * Produce type-qualifier list, but without non-type information.
    */
   just_type(sym->u.declare_var.tqual, 0, 1);
   prt_str(" ", 0);
   ilc_str(" ");

   /*
    * If the caller requested another level of indirection on the
    *  declaration add it.
    */
   if (addr_of)
      add_ptr(sym->u.declare_var.dcltor);
   else {
      c_walk(sym->u.declare_var.dcltor, 0, 0);
      ilc_walk(sym->u.declare_var.dcltor, 0, 0);
      }
   prt_str(";", 0);
   ForceNl();
   flush_str();       /* flush string buffer to code list */
   return ilc_base.next;
   }

/*
 * add_ptr - add another level of indirection to a declarator. Print it in
 *   the output file and proceduce in-line C code.
 */
static void add_ptr(dcltor)
struct node *dcltor;
   {
   while (dcltor->nd_id == ConCatNd) {
      c_walk(dcltor->u[0].child, IndentInc, 0);
      ilc_walk(dcltor->u[0].child, 0, 0);
      dcltor = dcltor->u[1].child;
      }
   switch (dcltor->nd_id) {
      case PrimryNd:
         /*
          * We have reached the name, add a level of indirection.
          */
         prt_str("(*", IndentInc);
         ilc_str("(*");
         prt_str(dcltor->tok->image, IndentInc);
         ilc_str(dcltor->tok->image);
         prt_str(")", IndentInc);
         ilc_str(")");
         break;
      case PrefxNd:
         /*
          * (...)
          */
         prt_str("(", IndentInc);
         ilc_str("(");
         add_ptr(dcltor->u[0].child);
         prt_str(")", IndentInc);
         ilc_str(")");
         break;
      case BinryNd:
         if (dcltor->tok->tok_id == ')') {
            /*
             * Function declaration.
             */
            add_ptr(dcltor->u[0].child);
            prt_str("(", IndentInc);
            ilc_str("(");
            c_walk(dcltor->u[1].child, IndentInc, 0);
            ilc_walk(dcltor->u[1].child, 0, 0);
            prt_str(")", IndentInc);
            ilc_str(")");
            }
         else {
            /*
             * Array.
             */
            add_ptr(dcltor->u[0].child);
            prt_str("[", IndentInc);
            ilc_str("[");
            c_walk(dcltor->u[1].child, IndentInc, 0);
            ilc_walk(dcltor->u[1].child, 0, 0);
            prt_str("]", IndentInc);
            ilc_str("]");
            }
      }
   }

/*
 * bdy_prm - produce the code that must be be supplied as the argument
 *  to the call of a body function.
 */
struct il_c *bdy_prm(addr_of, just_desc, sym, may_mod)
int addr_of;
int just_desc;
struct sym_entry *sym;
int may_mod;
   {
   init_ilc();              /* initialize code list and string buffer */
   if (addr_of)
      ilc_str("&(");                  /* call-by-reference parameter */
   ilc_var(sym, just_desc, may_mod);  /* variable to pass as argument */
   if (addr_of)
      ilc_str(")");
   flush_str();             /* flush string buffer to code list */
   return ilc_base.next;
   }

/*
 * ilc_dcl - produce in-line code for a C declaration.
 */
struct il_c *ilc_dcl(tqual, dcltor, init)
struct node *tqual;
struct node *dcltor;
struct node *init;
   {
   init_ilc();              /* initialize code list and string buffer */
   ilc_walk(tqual, 0, 0);
   ilc_str(" ");
   ilc_walk(dcltor, 0, 0);
   if (init != NULL) {
      ilc_str(" = ");
      ilc_walk(init, 0, 0);
      }
   ilc_str(";");
   flush_str();             /* flush string buffer to code list */
   return ilc_base.next;
   }


/*
 *  init_ilc - initialize the code list by pointing to ilc_base. Initialize
 *   the string buffer.
 */
static void init_ilc()
   {
   nxt_char = 0;
   line_ref = NULL;
   insert_nl = 0;
   ilc_base.il_c_type = 0;
   ilc_base.next = NULL;
   ilc_cur = &ilc_base;
   }


/*
 * - ilc_chnl - check for new-line.
 */
static void ilc_chnl(t)
struct token *t;
   {
   /*
    * See if this is a reasonable place to put a newline.
    */
   if (t->flag & LineChk) {
      if (line_ref != NULL &&
         (t->fname != line_ref->fname || t->line != line_ref->line))
            insert_nl = 1;
      line_ref = t;
      }
   }

/*
 * ilc_tok - convert a token to its string representation, quoting it
 *  if it is a string or character literal.
 */
static void ilc_tok(t)
struct token *t;
   {
   char *s;

   ilc_chnl(t);
   s = t->image;
   switch (t->tok_id) {
      case StrLit:
         ilc_str("\"");
         ilc_str(s);
         ilc_str("\"");
         break;
      case LStrLit:
         ilc_str("L\"");
         ilc_str(s);
         ilc_str("\"");
         break;
      case CharConst:
         ilc_str("'");
         ilc_str(s);
         ilc_str("'");
         break;
      case LCharConst:
         ilc_str("L'");
         ilc_str(s);
         ilc_str("'");
         break;
      default:
         ilc_str(s);
      }
   }

/*
 * ilc_str - append a string to the string buffer.
 */
static void ilc_str(s)
char *s;
   {
   /*
    * see if a new-line is needed before the string
    */
   if (insert_nl && (nxt_char == 0 || sbuf[nxt_char - 1] != '\n')) {
      insert_nl = 0;
      ilc_str("\n");
      }

   /*
    * Put the string in the buffer. If the buffer is full, flush it
    *  to an element in the in-line code list.
    */
   while (*s != '\0') {
      if (nxt_char >= SBufSz - 1)
         insrt_str();
      sbuf[nxt_char++] = *s++;
      }
   }

/*
 * insrt_str - insert the string in the buffer into the list of in-line
 *  code.
 */
static void insrt_str()
   {
   alloc_ilc(ILC_Str);
   sbuf[nxt_char] = '\0';
   ilc_cur->s = salloc(sbuf);
   nxt_char = 0;
   }

/*
 * flush_str - if the string buffer is not empty, flush it to the list
 *  of in-line code.
 */
static void flush_str()
   {
   if (insert_nl)
      ilc_str("");
   if (nxt_char != 0)
      insrt_str();
   }

/*
 * new_ilc - create a new element for the list of in-line C code. This
 *   is called for non-string elements. If necessary it flushes the
 *   string buffer to another element first.
 */
static void new_ilc(il_c_type)
int il_c_type;
   {
   flush_str();
   alloc_ilc(il_c_type);
   }

/*
 * alloc_ilc - allocate a new element for the list of in-line C code
 *   and add it to the list.
 */
static void alloc_ilc(il_c_type)
int il_c_type;
   {
   int i;
   ilc_cur->next = NewStruct(il_c);
   ilc_cur = ilc_cur->next;
   ilc_cur->next = NULL;
   ilc_cur->il_c_type = il_c_type;
   for (i = 0; i < 3; ++i)
      ilc_cur->code[i] = NULL;
   ilc_cur->n = 0;
   ilc_cur->s = NULL;
   }

/*
 * sep_ilc - translate the syntax tree, n, (possibly surrounding it by
 *  strings) into a sub-list of in-line C code, remove the sub-list from
 *  the main list, and return it.
 */
static struct il_c *sep_ilc(s1, n, s2)
char *s1;
struct node *n;
char *s2;
   {
   struct il_c *ilc;

   ilc = ilc_cur;     /* remember the starting point in the main list */
   if (s1 != NULL)
      ilc_str(s1);
   ilc_walk(n, 0, 0);
   if (s2 != NULL)
      ilc_str(s2);
   flush_str();

   /*
    * Reset the main list to its condition upon entry, and return the sublist
    *   created from s1, n, and s2.
    */
   ilc_cur = ilc;
   ilc = ilc_cur->next;
   ilc_cur->next = NULL;
   return ilc;
   }

/*
 * ilc_var - create in-line C code for a variable in the symbol table.
 */
static void ilc_var(sym, just_desc, may_mod)
struct sym_entry *sym;
int just_desc;
int may_mod;
   {
   if (sym->il_indx >= 0) {
      /*
       * This symbol will be in symbol table iconc builds from the
       *   data base entry. iconc needs to know if this is a modifying
       *   reference so it can perform optimizations. This is indicated by
       *   may_mod. Some variables are implemented as the vword of a
       *   descriptor. Sometime the entire descriptor must be accessed.
       *   This is indicated by just_desc.
       */
      if (may_mod) {
         new_ilc(ILC_Mod);
         if (sym->id_type & DrfPrm)
            sym->u.param_info.parm_mod |= 1;
         }
      else
         new_ilc(ILC_Ref);
      ilc_cur->n = sym->il_indx;
      if (just_desc)
         ilc_cur->s = "d";
      }
   else switch (sym->id_type) {
      case TndDesc:
         /*
          * variable declared: tended struct descrip ...
          */
         new_ilc(ILC_Tend);
         ilc_cur->n = sym->t_indx;   /* index into tended variables */
         break;
      case TndStr:
         /*
          * variable declared: tended char *...
          */
         new_ilc(ILC_Tend);
         ilc_cur->n = sym->t_indx;   /* index into tended variables */
         ilc_str(".vword.sptr");     /* get string pointer from vword union */
         break;
      case TndBlk:
         /*
          * If blk_name field is null, this variable was declared:
          *    tended union block *...
          *  otherwise it was declared:
          *    tended struct <blk_name> *...
          */
         if (sym->u.tnd_var.blk_name != NULL) {
            /*
             * Cast the "union block *" from the vword to the correct
             *   struct pointer. This cast can be used as an r-value or
             *   an l-value.
             */
            ilc_str("(*(struct ");
            ilc_str(sym->u.tnd_var.blk_name);
            ilc_str("**)&");
            }
         new_ilc(ILC_Tend);
         ilc_cur->n = sym->t_indx;  /* index into tended variables */
         ilc_str(".vword.bptr");    /* get block pointer from vword union */
         if (sym->u.tnd_var.blk_name != NULL)
            ilc_str(")");
         break;
      case RsltLoc:
         /*
          * This is the special variable for the result of the operation.
          *  iconc needs to know if this is a modifying reference so it
          *  can perform optimizations.
          */
         if (may_mod)
            new_ilc(ILC_Mod);
         else
            new_ilc(ILC_Ref);
         ilc_cur->n = RsltIndx;
         break;
      default:
         /*
          * This is a variable with an ordinary declaration. Access it by
          *   its identifier.
          */
         ilc_str(sym->image);
      }
   }

/*
 * ilc_walk - walk the syntax tree for C code producing a list of "in-line"
 *   code. This function needs to know if the code is in a modifying context,
 *   such as the left-hand-side of an assignment.
 */
static void ilc_walk(n, may_mod, const_cast)
struct node *n;
int may_mod;
int const_cast;
   {
   struct token *t;
   struct node *n1;
   struct node *n2;
   struct sym_entry *sym;
   word cont_sav;
   word brk_sav;
   word l1, l2;
   int typcd;

   if (n == NULL)
      return;

   t =  n->tok;

   switch (n->nd_id) {
      case PrimryNd:
         /*
          * Primary expressions consisting of a single token.
          */
         switch (t->tok_id) {
            case Fail:
               /*
                * fail statement. Note that this operaion can fail, output
                *   the corresponding "in-line" code, and make sure we have
                *   seen an abstract clause of some kind.
                */
               cur_impl->ret_flag |= DoesFail;
               insert_nl = 1;
               new_ilc(ILC_Fail);
               insert_nl = 1;
               line_ref = NULL;
               chkabsret(t, SomeType);
               break;
            case Errorfail:
               /*
                * errorfail statement. Note that this operaion can do error
                *   conversion and output the corresponding "in-line" code.
                */
               cur_impl->ret_flag |= DoesEFail;
               insert_nl = 1;
               new_ilc(ILC_EFail);
               insert_nl = 1;
               line_ref = NULL;
               break;
            case Break:
               /*
                * iconc can only handle gotos for transfer of control in
                *  in-line code. A break label has been established for
                *  the current loop; transform the "break" into a goto.
                */
               ilc_goto(brk_lbl);
               break;
            case Continue:
               /*
                * iconc can only handle gotos for transfer of control in
                *  in-line code. A continue label has been established for
                *  the current loop; transform the "continue" into a goto.
                */
               ilc_goto(cont_lbl);
               break;
            default:
               /*
                * No special processing is needed for this primary
                *  expression, just output the image of the token.
                */
               ilc_tok(t);
            }
         break;
      case PrefxNd:
         /*
          * Expressions with one operand that are introduced by a token.
          *  Note, "default :" does not appear here because switch
          *  statements are not allowed in in-line code.
          */
         switch (t->tok_id) {
            case Sizeof:
               /*
                * sizeof(...)
                */
               ilc_tok(t);
               ilc_str("(");
               ilc_walk(n->u[0].child, 0, 0);
               ilc_str(")");
               break;
            case '{':
               /*
                * initializer: { ... }
                */
               ilc_tok(t);
               ilc_walk(n->u[0].child, 0, 0);
               ilc_str("}");
               break;
            case Goto:
               /*
                * goto <label>;
                */
               ilc_goto(n->u[0].child->u[0].sym->u.lbl_num);
               break;
            case Return:
               /*
                * return <expression>;
                *  Indicate that this operation can return, then perform
                *  processing to categorize the kind of return statement
                *  and produce appropriate in-line code.
                */
               cur_impl->ret_flag |= DoesRet;
               ilc_ret(t, ILC_Ret, n->u[0].child);
               break;
            case Suspend:
               /*
                * suspend <expression>;
                *  Indicate that this operation can suspend, then perform
                *  processing to categorize the kind of suspend statement
                *  and produce appropriate in-line code.
                */
               cur_impl->ret_flag |= DoesSusp;
               ilc_ret(t, ILC_Susp, n->u[0].child);
               break;
            case '(':
               /*
                * ( ... )
                */
               ilc_tok(t);
               ilc_walk(n->u[0].child, may_mod, const_cast);
               ilc_str(")");
               break;
            case Incr:
            case Decr:
               /*
                * The operand might be modified, otherwise nothing special
                *   is needed.
                */
               ilc_tok(t);
               ilc_walk(n->u[0].child, 1, 0);
               break;
            case '&':
               /*
                * Unless the address is cast to a const pointer, this
                *  might be a modifiying reference.
                */
               ilc_tok(t);
               if (const_cast)
                  ilc_walk(n->u[0].child, 0, 0);
               else
                  ilc_walk(n->u[0].child, 1, 0);
               break;
            default:
               /*
                * Nothing special is needed, just output the image of
                *   the prefix operation followed by its operand.
                */
               ilc_tok(t);
               ilc_walk(n->u[0].child, 0, 0);
            }
         break;
      case PstfxNd:
         /*
          * postfix notation: ';', '++', and '--'. The later two
          *   modify their operands.
          */
         if (t->tok_id == ';')
            ilc_walk(n->u[0].child, 0, 0);
         else
            ilc_walk(n->u[0].child, 1, 0);
         ilc_tok(t);
         break;
      case PreSpcNd:
         /*
          * Prefix notation that needs a space after the expression;
          *   used for pointer/type qualifier lists.
          */
         ilc_tok(t);
         ilc_walk(n->u[0].child, 0, 0);
         ilc_str(" ");
         break;
      case SymNd:
         /*
          * Identifier in symbol table. See if it start a new line. Note
          *   that we need to know whether this is a modifying reference.
          */
         ilc_chnl(n->tok);
         ilc_var(n->u[0].sym, 0, may_mod);
         break;
      case BinryNd:
         switch (t->tok_id) {
            case '[':
               /*
                * Expression or declaration:
                *   <expr1> [ <expr2> ]
                */
               ilc_walk(n->u[0].child, may_mod, 0);
               ilc_str("[");
               ilc_walk(n->u[1].child, 0, 0);
               ilc_str("]");
               break;
            case '(':
               /*
                * ( <type> ) expr
                */
               ilc_tok(t);
               ilc_walk(n->u[0].child, 0, 0);
               ilc_str(")");
               /*
                * See if the is a const cast.
                */
               for (n1 = n->u[0].child; n1->nd_id == LstNd; n1 = n1->u[0].child)
                  ;
               if (n1->nd_id == PrimryNd && n1->tok->tok_id == Const)
                  ilc_walk(n->u[1].child, 0, 1);
               else
                  ilc_walk(n->u[1].child, 0, 0);
               break;
            case ')':
               /*
                * Expression or declaration:
                *   <expr> ( <arg-list> )
                */
               ilc_walk(n->u[0].child, 0, 0);
               ilc_str("(");
               ilc_walk(n->u[1].child, 0, 0);
               ilc_tok(t);
               break;
            case Struct:
            case Union:
            case TokEnum:
               /*
                * <struct-union-enum> <identifier>
                * <struct-union-enum> { <component-list> }
                * <struct-union-enum> <identifier> { <component-list> }
                */
               ilc_tok(t);
               ilc_str(" ");
               ilc_walk(n->u[0].child, 0, 0);
               if (n->u[1].child != NULL) {
                  ilc_str(" {");
                  ilc_walk(n->u[1].child, 0, 0);
                  ilc_str("}");
                  }
               break;
            case ';':
               /*
                * <type specifiers> <declarator> ;
                */
               ilc_walk(n->u[0].child, 0, 0);
               ilc_str(" ");
               ilc_walk(n->u[1].child, 0, 0);
               ilc_tok(t);
               break;
            case ':':
               /*
                * <label> : <statement>
                */
               ilc_lbl(n->u[0].child->u[0].sym->u.lbl_num);
               ilc_walk(n->u[1].child, 0, 0);
               break;
            case Switch:
               errt1(t, "switch statement not supported in in-line code");
               break;
            case While:
               /*
                * Convert "while (c) s" into [conditional] gotos and labels.
                *   Establish labels for break and continue statements
                *   within s.
                */
               brk_sav = brk_lbl;
               cont_sav = cont_lbl;
               cont_lbl = lbl_num++;
               brk_lbl = lbl_num++;
               ilc_lbl(cont_lbl);                      /* L1: */
               ilc_cgoto(1, n->u[0].child, brk_lbl);   /* if (!(c)) goto L2; */
               ilc_walk(n->u[1].child, 0, 0);          /* s */
               ilc_goto(cont_lbl);                     /* goto L1; */
               ilc_lbl(brk_lbl);                       /* L2: */
               brk_lbl = brk_sav;
               cont_lbl = cont_sav;
               break;
            case Do:
               /*
                * Convert "do s while (c);" loop into a conditional goto and
                *  label. Establish labels for break and continue statements
                *  within s.
                */
               brk_sav = brk_lbl;
               cont_sav = cont_lbl;
               cont_lbl = lbl_num++;
               brk_lbl = lbl_num++;
               ilc_lbl(cont_lbl);                        /* L1: */
               ilc_walk(n->u[0].child, 0, 0);            /* s */
               ilc_cgoto(0, n->u[1].child, cont_lbl);    /* if (c) goto L1 */
               ilc_lbl(brk_lbl);
               brk_lbl = brk_sav;
               cont_lbl = cont_sav;
               break;
            case '.':
               /*
                * <expr1> . <expr2>
                */
               ilc_walk(n->u[0].child, may_mod, 0);
               ilc_tok(t);
               ilc_walk(n->u[1].child, 0, 0);
               break;
            case Arrow:
               /*
                * <expr1> -> <expr2>
                */
               ilc_walk(n->u[0].child, 0, 0);
               ilc_tok(t);
               ilc_walk(n->u[1].child, 0, 0);
               break;
            case Runerr:
               /*
                * runerr ( <expr> ) ;
                * runerr ( <expr> , <expr> ) ;
                */
               ilc_str("err_msg(");
               ilc_walk(n->u[0].child, 0, 0);
               if (n->u[1].child == NULL)
                  ilc_str(", NULL);");
               else {
                  ilc_str(", &(");
                  ilc_walk(n->u[1].child, 0, 0);
                  ilc_str("));");
                  }
               /*
                * Handle error conversion.
                */
               cur_impl->ret_flag |= DoesEFail;
               insert_nl = 1;
               new_ilc(ILC_EFail);
               insert_nl = 1;
               break;
            case Is:
               /*
                * is : <type-name> ( <expr> )
                */
               typcd = icn_typ(n->u[0].child);
               n1 =  n->u[1].child;
               if (typcd == str_typ) {
                  ilc_str("(!((");
                  ilc_walk(n1, 0, 0);
                  ilc_str(").dword & F_Nqual))");
                  }
               else if (typcd == Variable) {
                  ilc_str("(((");
                  ilc_walk(n1, 0, 0);
                  ilc_str(").dword & D_Var) == D_Var)");
                  }
              else if (typcd == int_typ) {
                  ForceNl();
                  prt_str("#ifdef LargeInts", 0);
                  ForceNl();

                  ilc_str("(((");
                  ilc_walk(n1, 0, 0);
                  ilc_str(").dword == D_Integer) || ((");
                  ilc_walk(n1, 0, 0);
                  ilc_str(").dword == D_Lrgint))");

                  ForceNl();
                  prt_str("#else /* LargeInts */", 0);
                  ForceNl();

                  ilc_str("((");
                  ilc_walk(n1, 0, 0);
                  ilc_str(").dword == D_Integer)");

                  ForceNl();
                  prt_str("#endif /* LargeInts */", 0);
                  ForceNl();
                  }
              else {
                  ilc_str("((");
                  ilc_walk(n1, 0, 0);
                  ilc_str(").dword == D_");
                  ilc_str(typ_name(typcd, n->u[0].child->tok));
                  ilc_str(")");
                  }
               break;
            case '=':
            case MultAsgn:
            case DivAsgn:
            case ModAsgn:
            case PlusAsgn:
            case MinusAsgn:
            case LShftAsgn:
            case RShftAsgn:
            case AndAsgn:
            case XorAsgn:
            case OrAsgn:
               /*
                * Assignment operation (or initialization or specification
                *   of enumeration value). Left-hand-side may be modified.
                */
               ilc_walk(n->u[0].child, 1, 0);
               ilc_str(" ");
               ilc_tok(t);
               ilc_str(" ");
               ilc_walk(n->u[1].child, 0, 0);
               break;
            default:
               /*
                * Simple binary operator. Nothing special is needed,
                *   just put space around the operator.
                */
               ilc_walk(n->u[0].child, 0, 0);
               ilc_str(" ");
               ilc_tok(t);
               ilc_str(" ");
               ilc_walk(n->u[1].child, 0, 0);
               break;
            }
         break;
      case LstNd:
         /*
          * Consecutive expressions that need a space between them.
          */
         ilc_walk(n->u[0].child, 0, 0);
         ilc_str(" ");
         ilc_walk(n->u[1].child, 0, 0);
         break;
      case ConCatNd:
         /*
          * Consecutive expressions that don't need space between them.
          */
         ilc_walk(n->u[0].child, 0, 0);
         ilc_walk(n->u[1].child, 0, 0);
         break;
      case CommaNd:
         ilc_walk(n->u[0].child, 0, 0);
         ilc_tok(t);
         ilc_str(" ");
         ilc_walk(n->u[1].child, 0, 0);
         break;
      case StrDclNd:
         /*
          * struct field declarator. May be a bit field.
          */
         ilc_walk(n->u[0].child, 0, 0);
         if (n->u[1].child != NULL) {
            ilc_str(": ");
            ilc_walk(n->u[1].child, 0, 0);
            }
         break;
      case CompNd: {
         /*
          * Compound statement. May have declarations including tended
          *   declarations that are separated out.
          */
         struct node *dcls;

         /*
          * If the in-line code has declarations, the block must
          *   be surrounded by braces. Braces are special constructs
          *   because iconc must not delete one without the other
          *   during code optimization.
          */
         dcls = n->u[0].child;
         if (dcls != NULL) {
            insert_nl = 1;
            new_ilc(ILC_LBrc);
            insert_nl = 1;
            line_ref = NULL;
            ilc_walk(dcls, 0, 0);
            }
         /*
          * we are in an inner block. tended locations may need to
          *  be set to values from declaration initializations.
          */
         for (sym = n->u[1].sym; sym!= NULL; sym = sym->u.tnd_var.next) {
            if (sym->u.tnd_var.init != NULL) {
               new_ilc(ILC_Tend);
               ilc_cur->n = sym->t_indx;

               /*
                * See if the variable is just the vword of the descriptor.
                */
               switch (sym->id_type) {
                  case TndDesc:
                     ilc_str(" = ");
                     break;
                  case TndStr:
                     ilc_str(".vword.sptr = ");
                     break;
                  case TndBlk:
                     ilc_str(".vword.bptr = (union block *)");
                     break;
                  }
               ilc_walk(sym->u.tnd_var.init, 0, 0);  /* initial value */
               ilc_str(";");
               }
            }

         ilc_walk(n->u[2].child, 0, 0); /* body of compound statement */

         if (dcls != NULL) {
            insert_nl = 1;
            new_ilc(ILC_RBrc);  /* closing brace */
            insert_nl = 1;
            line_ref = NULL;
            }
         }
         break;
      case TrnryNd:
         switch (t->tok_id) {
            case '?':
               /*
                * <expr> ? <expr> : <expr>
                */
               ilc_walk(n->u[0].child, 0, 0);
               ilc_str(" ");
               ilc_tok(t);
               ilc_str(" ");
               ilc_walk(n->u[1].child, 0, 0);
               ilc_str(" : ");
               ilc_walk(n->u[2].child, 0, 0);
               break;
            case If:
               /*
                * Convert if statement into [conditional] gotos and labels.
                */
               n1 = n->u[1].child;
               n2 = n->u[2].child;
               l1 = lbl_num++;
               if (n2 == NULL) {    /* if (c) then s */
                  ilc_cgoto(1, n->u[0].child, l1);      /* if (!(c)) goto L1; */
                  ilc_walk(n1, 0, 0);                   /* s */
                  ilc_lbl(l1);                          /* L1: */
                  }
               else {               /* if (c) then s1 else s2 */
                  ilc_cgoto(0, n->u[0].child, l1);      /* if (c) goto L1; */
                  ilc_walk(n2, 0, 0);                   /* s2 */
                  l2 = lbl_num++;
                  ilc_goto(l2);                         /* goto L2; */
                  ilc_lbl(l1);                          /* L1: */
                  ilc_walk(n1, 0, 0);                   /* s1 */
                  ilc_lbl(l2);                          /* L2: */
                  }
               break;
            case Type_case:
               errt1(t, "type case statement not supported in in-line code");
               break;
            case Cnv:
               /*
                * cnv : <type> ( <expr> , <expr> )
                */
               ilc_cnv(n->u[0].child, n->u[1].child, NULL, n->u[2].child);
               break;
            }
         break;
      case QuadNd:
         switch (t->tok_id) {
            case For:
               /*
                * convert "for (e1; e2; e3) s" into [conditional] gotos and
                *  labels.
                */
               brk_sav = brk_lbl;
               cont_sav = cont_lbl;
               l1 = lbl_num++;
               cont_lbl = lbl_num++;
               brk_lbl = lbl_num++;
               ilc_walk(n->u[0].child, 0, 0);     /* e1; */
               ilc_str(";");
               ilc_lbl(l1);                       /* L1: */
               n2 = n->u[1].child;
               if (n2 != NULL)
                  ilc_cgoto(1, n2, brk_lbl);      /* if (!(e2)) goto L2; */
               ilc_walk(n->u[3].child, 0, 0);     /* s */
               ilc_lbl(cont_lbl);
               ilc_walk(n->u[2].child, 0, 0);     /* e3; */
               ilc_str(";");
               ilc_goto(l1);                      /* goto L1 */
               ilc_lbl(brk_lbl);                  /* L2: */
               brk_lbl = brk_sav;
               cont_lbl = cont_sav;
               break;
            case Def:
               ilc_cnv(n->u[0].child, n->u[1].child, n->u[2].child,
                  n->u[3].child);
               break;
            }
         break;
      }
   }

/*
 * ilc_cnv - produce code for a cnv: or def: statement.
 */
static void ilc_cnv(cnv_typ, src, dflt, dest)
struct node *cnv_typ;
struct node *src;
struct node *dflt;
struct node *dest;
   {
   int dflt_to_ptr;
   int typcd;

   /*
    * Get the name of the conversion routine for the given type
    *  and determine whether the conversion routine needs a
    *  pointer to the default value (if there is one) rather
    *  the the value itself.
    */
   typcd = icn_typ(cnv_typ);
   ilc_str(cnv_name(typcd, dflt, &dflt_to_ptr));
   ilc_str("(");

   /*
    * If this is a conversion to a temporary string or cset, the
    *  conversion routine needs a temporary buffer in which to
    *  perform the conversion.
    */
   switch (typcd) {
      case TypTStr:
         new_ilc(ILC_SBuf);
         ilc_str(", ");
         break;
      case TypTCset:
         new_ilc(ILC_CBuf);
         ilc_str(", ");
         break;
      }

   /*
    * Produce code for the source expression.
    */
   ilc_str("&(");
   ilc_walk(src, 0, 0);
   ilc_str("), ");

   /*
    * Produce code for the default expression, if there is one.
    */
   if (dflt != NULL) {
      if (dflt_to_ptr)
         ilc_str("&(");
      ilc_walk(dflt, 0, 0);
      if (dflt_to_ptr)
         ilc_str("), ");
      else
         ilc_str(", ");
      }

   /*
    * Produce code for the destination expression.
    */
   ilc_str("&(");
   ilc_walk(dest, 1, 0);
   ilc_str("))");
   }

/*
 * ilc_ret - produce in-line code for suspend/return statement.
 */
static void ilc_ret(t, ilc_typ, n)
struct token *t;
int ilc_typ;
struct node *n;
   {
   struct node *caller;
   struct node *args;
   int typcd;

   insert_nl = 1;
   line_ref = NULL;
   new_ilc(ilc_typ);

   if (n->nd_id == SymNd && n->u[0].sym->id_type == RsltLoc) {
      /*
       * return/suspend result;
       */
      ilc_cur->n = RetNone;
      return;
      }

   if (n->nd_id == PrefxNd && n->tok != NULL) {
      switch (n->tok->tok_id) {
         case C_Integer:
            /*
             * return/suspend C_integer <expr>;
             */
            ilc_cur->n = TypCInt;
            ilc_cur->code[0] = sep_ilc(NULL, n->u[0].child, NULL);
            chkabsret(t, int_typ);
            return;
         case C_Double:
            /*
             * return/suspend C_double <expr>;
             */
            ilc_cur->n = TypCDbl;
            ilc_cur->code[0] = sep_ilc(NULL, n->u[0].child, NULL);
            chkabsret(t, real_typ);
            return;
         case C_String:
            /*
             * return/suspend C_string <expr>;
             */
            ilc_cur->n = TypCStr;
            ilc_cur->code[0] = sep_ilc(NULL, n->u[0].child, NULL);
            chkabsret(t, str_typ);
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
               ilc_cur->n = typcd;
               switch (icontypes[typcd].rtl_ret) {
                  case TRetBlkP:
                  case TRetDescP:
                  case TRetCharP:
                  case TRetCInt:
                     /*
                      * return/suspend <type>(<value>);
                      */
                     ilc_cur->code[0] = sep_ilc(NULL, args, NULL);
                     break;
                  case TRetSpcl:
                     if (typcd == str_typ) {
                        /*
                         * return/suspend string(<len>, <char-pntr>);
                         */
                        ilc_cur->code[0] = sep_ilc(NULL, args->u[0].child,NULL);
                        ilc_cur->code[1] = sep_ilc(NULL, args->u[1].child,NULL);
                        }
                     else if (typcd == stv_typ) {
                        /*
                         * return/suspend tvsubs(<desc-pntr>, <start>, <len>);
                         */
                        ilc_cur->n = stv_typ;
                        ilc_cur->code[0] = sep_ilc(NULL,
                            args->u[0].child->u[0].child, NULL);
                        ilc_cur->code[1] = sep_ilc(NULL,
                            args->u[0].child->u[1].child, NULL);
                        ilc_cur->code[2] = sep_ilc(NULL, args->u[1].child,
                            NULL);
                        chkabsret(t, stv_typ);
                                 }
                     break;
                  }
               chkabsret(t, typcd);
               return;
            case Named_var:
               /*
                * return/suspend named_var(<desc-pntr>);
                */
               ilc_cur->n = RetNVar;
               ilc_cur->code[0] = sep_ilc(NULL, args, NULL);
               chkabsret(t, TypVar);
               return;
            case Struct_var:
               /*
                * return/suspend struct_var(<desc-pntr>, <block_pntr>);
                */
               ilc_cur->n = RetSVar;
               ilc_cur->code[0] = sep_ilc(NULL, args->u[0].child, NULL);
               ilc_cur->code[1] = sep_ilc(NULL, args->u[1].child, NULL);
               chkabsret(t, TypVar);
               return;
            }
         }
      }

   /*
    * If it is not one of the special returns, it is just a return of
    *  a descriptor.
    */
   ilc_cur->n = RetDesc;
   ilc_cur->code[0] = sep_ilc(NULL, n, NULL);
   chkabsret(t, SomeType);
   }

/*
 * ilc_goto - produce in-line C code for a goto to a numbered label.
 */
static void ilc_goto(lbl)
word lbl;
   {
   insert_nl = 1;
   new_ilc(ILC_Goto);
   ilc_cur->n = lbl;
   insert_nl = 1;
   line_ref = NULL;
   }

/*
 * ilc_cgoto - produce in-line C code for a conditional goto to a numbered
 *  label. The condition may be negated.
 */
static void ilc_cgoto(neg, cond, lbl)
int neg;
struct node *cond;
word lbl;
   {
   insert_nl = 1;
   line_ref = NULL;
   new_ilc(ILC_CGto);
   if (neg)
      ilc_cur->code[0] = sep_ilc("!(", cond, ")");
   else
      ilc_cur->code[0] = sep_ilc(NULL, cond, NULL);
   ilc_cur->n = lbl;
   insert_nl = 1;
   line_ref = NULL;
   }

/*
 * ilc_lbl - produce in-line C code for a numbered label.
 */
static void ilc_lbl(lbl)
word lbl;
   {
   insert_nl = 1;
   new_ilc(ILC_Lbl);
   ilc_cur->n = lbl;
   insert_nl = 1;
   line_ref = NULL;
   }
#endif					/* Rttx */

/*
 * chkabsret - make sure a previous abstract return statement
 *  was encountered and that it is consistent with this return,
 *  suspend, or fail.
 */
void chkabsret(tok, ret_typ)
struct token *tok;
int ret_typ;
   {
   if (abs_ret == NoAbstr)
      errt2(tok, tok->image, " with no preceding abstract return");

   /*
    * We only check for type consistency when it is easy, otherwise
    *   we don't bother.
    */
   if (abs_ret == SomeType || ret_typ == SomeType || abs_ret == TypAny)
      return;

   /*
    * Some return types match the generic "variable" type.
    */
   if (abs_ret == TypVar && ret_typ >= 0 && icontypes[ret_typ].deref != DrfNone)
      return;

   /*
    * Otherwise the abstract return must match the real one.
    */
   if (abs_ret != ret_typ)
      errt2(tok, tok->image,  " is inconsistent with abstract return");
   }

/*
 * just_type - strip non-type information from a type-qualifier list. Print
 *   it in the output file and if ilc is set, produce in-line C code.
 */
void just_type(typ, indent, ilc)
struct node *typ;
int indent;
int ilc;
   {
   if (typ->nd_id == LstNd) {
      /*
       * Simple list of type-qualifier elements - concatenate them.
       */
      just_type(typ->u[0].child, indent, ilc);
      just_type(typ->u[1].child, indent, ilc);
      }
   else if (typ->nd_id == PrimryNd) {
      switch (typ->tok->tok_id) {
         case Typedef:
         case Extern:
         case Static:
         case Auto:
         case TokRegister:
         case Const:
         case Volatile:
            return;         /* Don't output these declaration elements */
         default:
            c_walk(typ, indent, 0);
            #ifndef Rttx
               if (ilc)
                  ilc_walk(typ, 0, 0);
            #endif			/* Rttx */
         }
      }
   else {
      c_walk(typ, indent, 0);
      #ifndef Rttx
         if (ilc)
            ilc_walk(typ, 0, 0);
      #endif				/* Rttx */
      }
   }
