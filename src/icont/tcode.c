/*
 * tcode.c -- translator functions for traversing parse trees and generating
 *  code.
 */

#include "../h/gsupport.h"
#include "tproto.h"
#include "tglobals.h"
#include "tree.h"
#include "ttoken.h"
#include "tsym.h"

/*
 * Prototypes.
 */

static int	alclab		(int n);
static void	binop		(int op);
static void	emit		(char *s);
static void	emitl		(char *s,int a);
static void	emitlab		(int l);
static void	emitn		(char *s,int a);
static void	emits		(char *s,char *a);
static void	emitfile	(nodeptr n);
static void	emitline	(nodeptr n);
static void	setloc		(nodeptr n);
static int	traverse	(nodeptr t);
static void	unopa		(int op, nodeptr t);
static void	unopb		(int op);

extern int tfatals;
extern int nocode;

/*
 * Code generator parameters.
 */

#define LoopDepth   20		/* max. depth of nested loops */
#define CaseDepth   10		/* max. depth of nested case statements */
#define CreatDepth  10		/* max. depth of nested create statements */

/*
 * loopstk structures hold information about nested loops.
 */
struct loopstk {
   int nextlab;			/* label for next exit */
   int breaklab;		/* label for break exit */
   int markcount;		/* number of marks */
   int ltype;			/* loop type */
   };

/*
 * casestk structure hold information about case statements.
 */
struct casestk {
   int endlab;			/* label for exit from case statement */
   nodeptr deftree;		/* pointer to tree for default clause */
   };

/*
 * creatstk structures hold information about create statements.
 */
struct creatstk {
   int nextlab;			/* previous value of nextlab */
   int breaklab;		/* previous value of breaklab */
   };
static int nextlab;		/* next label allocated by alclab() */

/*
 * codegen - traverse tree t, generating code.
 */

void codegen(t)
nodeptr t;
   {
   nextlab = 1;
   traverse(t);
   }

/*
 * traverse - traverse tree rooted at t and generate code.  This is just
 *  plug and chug code for each of the node types.
 */

static int traverse(t)
register nodeptr t;
   {
   register int lab, n, i;
   struct loopstk loopsave;
   static struct loopstk loopstk[LoopDepth];	/* loop stack */
   static struct loopstk *loopsp;
   static struct casestk casestk[CaseDepth];	/* case stack */
   static struct casestk *casesp;
   static struct creatstk creatstk[CreatDepth]; /* create stack */
   static struct creatstk *creatsp;

   n = 1;
   switch (TType(t)) {

      case N_Activat:			/* co-expression activation */
	 if (Val0(Tree0(t)) == AUGAT) {
	    emit("pnull");
	    }
	 traverse(Tree2(t));		/* evaluate result expression */
	 if (Val0(Tree0(t)) == AUGAT)
	    emit("sdup");
	 traverse(Tree1(t));		/* evaluate activate expression */
	 setloc(t);
	 emit("coact");
	 if (Val0(Tree0(t)) == AUGAT)
	    emit("asgn");
         free(Tree0(t));
	 break;

      case N_Alt:			/* alternation */
	 lab = alclab(2);
	 emitl("mark", lab);
	 loopsp->markcount++;
	 traverse(Tree0(t));		/* evaluate first alternative */
	 loopsp->markcount--;
         #ifdef EventMon
            setloc(t);
         #endif				/* EventMon */
	 emit("esusp");                 /*  and suspend with its result */
	 emitl("goto", lab+1);
	 emitlab(lab);
	 traverse(Tree1(t));		/* evaluate second alternative */
	 emitlab(lab+1);
	 break;

      case N_Augop:			/* augmented assignment */
      case N_Binop:			/*  or a binary operator */
	 emit("pnull");
	 traverse(Tree1(t));
	 if (TType(t) == N_Augop)
	    emit("dup");
	 traverse(Tree2(t));
	 setloc(t);
	 binop((int)Val0(Tree0(t)));
	 free(Tree0(t));
	 break;

      case N_Bar:			/* repeated alternation */
	 lab = alclab(1);
	 emitlab(lab);
	 emit("mark0");         /* fail if expr fails first time */
	 loopsp->markcount++;
	 traverse(Tree0(t));		/* evaluate first alternative */
	 loopsp->markcount--;
	 emitl("chfail", lab);          /* change to loop on failure */
	 emit("esusp");                 /* suspend result */
	 break;

      case N_Break:			/* break expression */
	 if (loopsp->breaklab <= 0)
	    nfatal(t, "invalid context for break", NULL);
	 else {
	    for (i = 0; i < loopsp->markcount; i++)
	       emit("unmark");
	    loopsave = *loopsp--;
	    traverse(Tree0(t));
	    *++loopsp = loopsave;
	    emitl("goto", loopsp->breaklab);
	    }
	 break;

      case N_Case:			/* case expression */
	 lab = alclab(1);
	 casesp++;
	 casesp->endlab = lab;
	 casesp->deftree = NULL;
	 emit("mark0");
	 loopsp->markcount++;
	 traverse(Tree0(t));		/* evaluate control expression */
	 loopsp->markcount--;
	 emit("eret");
	 traverse(Tree1(t));		/* do rest of case (CLIST) */
	 if (casesp->deftree != NULL) { /* evaluate default clause */
	    emit("pop");
	    traverse(casesp->deftree);
	    }
	 else
	    emit("efail");
	 emitlab(lab);			/* end label */
	 casesp--;
	 break;

      case N_Ccls:			/* case expression clause */
	 if (TType(Tree0(t)) == N_Res && /* default clause */
	     Val0(Tree0(t)) == DEFAULT) {
	    if (casesp->deftree != NULL)
	       nfatal(t, "more than one default clause", NULL);
	    else
	       casesp->deftree = Tree1(t);
            free(Tree0(t));
	    }
	 else {				/* case clause */
	    lab = alclab(1);
	    emitl("mark", lab);
	    loopsp->markcount++;
	    emit("ccase");
	    traverse(Tree0(t));		/* evaluate selector */
	    setloc(t);
	    emit("eqv");
	    loopsp->markcount--;
	    emit("unmark");
	    emit("pop");
	    traverse(Tree1(t));		/* evaluate expression */
	    emitl("goto", casesp->endlab); /* goto end label */
	    emitlab(lab);		/* label for next clause */
	    }
	 break;

      case N_Clist:			/* list of case clauses */
	 traverse(Tree0(t));
	 traverse(Tree1(t));
	 break;

      case N_Conj:			/* conjunction */
	 if (Val0(Tree0(t)) == AUGAND) {
	    emit("pnull");
	    }
	 traverse(Tree1(t));
	 if (Val0(Tree0(t)) != AUGAND)
	    emit("pop");
	 traverse(Tree2(t));
	 if (Val0(Tree0(t)) == AUGAND) {
	    setloc(t);
	    emit("asgn");
	    }
	 free(Tree0(t));
	 break;

      case N_Create:			/* create expression */
	 creatsp++;
	 creatsp->nextlab = loopsp->nextlab;
	 creatsp->breaklab = loopsp->breaklab;
	 loopsp->nextlab = 0;		/* make break and next illegal */
	 loopsp->breaklab = 0;
	 lab = alclab(3);
	 emitl("goto", lab+2);          /* skip over code for co-expression */
	 emitlab(lab);			/* entry point */
	 emit("pop");                   /* pop the result from activation */
	 emitl("mark", lab+1);
	 loopsp->markcount++;
	 traverse(Tree0(t));		/* traverse code for co-expression */
	 loopsp->markcount--;
	 setloc(t);
	 emit("coret");                 /* return to activator */
	 emit("efail");                 /* drive co-expression */
	 emitlab(lab+1);		/* loop on exhaustion */
	 emit("cofail");                /* and fail each time */
	 emitl("goto", lab+1);
	 emitlab(lab+2);
	 emitl("create", lab);          /* create entry block */
	 loopsp->nextlab = creatsp->nextlab;   /* legalize break and next */
	 loopsp->breaklab = creatsp->breaklab;
	 creatsp--;
	 break;

      case N_Cset:			/* cset literal */
	 emitn("cset", (int)Val0(t));
	 break;

      case N_Elist:			/* expression list */
	 n = traverse(Tree0(t));
	 n += traverse(Tree1(t));
	 break;

      case N_Empty:			/* a missing expression */
	 emit("pnull");
	 break;

      case N_Field:			/* field reference */
	 emit("pnull");
	 traverse(Tree0(t));
	 setloc(t);
	 emits("field", Str0(Tree1(t)));
	 free(Tree1(t));
	 break;

      case N_Id:			/* identifier */
	 emitn("var", (int)Val0(t));
	 break;

      case N_If:			/* if expression */
	 if (TType(Tree2(t)) == N_Empty) {
	    lab = 0;
	    emit("mark0");
	    }
	 else {
	    lab = alclab(2);
	    emitl("mark", lab);
	    }
	 loopsp->markcount++;
	 traverse(Tree0(t));
	 loopsp->markcount--;
	 emit("unmark");
	 traverse(Tree1(t));
	 if (lab > 0) {
	    emitl("goto", lab+1);
	    emitlab(lab);
	    traverse(Tree2(t));
	    emitlab(lab+1);
	    }
         else
	    free(Tree2(t));
	 break;

      case N_Int:			/* integer literal */
	 emitn("int", (int)Val0(t));
	 break;

      case N_Apply:			/* application */
         traverse(Tree0(t));
         traverse(Tree1(t));
         emitn("invoke", -1);
         break;

      case N_Invok:			/* invocation */
	 if (TType(Tree0(t)) != N_Empty) {
	    traverse(Tree0(t));
	     }
	 else {
	    emit("pushn1");             /* default to -1(e1,...,en) */
	    free(Tree0(t));
	    }
	 if (TType(Tree1(t)) == N_Empty) {
            n = 0;
	    free(Tree1(t));
            }
         else
	    n = traverse(Tree1(t));
	 setloc(t);
	 emitn("invoke", n);
	 n = 1;
	 break;

      case N_Key:			/* keyword reference */
	 setloc(t);
	 emits("keywd", Str0(t));
	 break;

      case N_Limit:			/* limitation */
	 traverse(Tree1(t));
	 setloc(t);
	 emit("limit");
	 loopsp->markcount++;
	 traverse(Tree0(t));
	 loopsp->markcount--;
	 emit("lsusp");
	 break;

      case N_List:			/* list construction */
	 emit("pnull");
	 if (TType(Tree0(t)) == N_Empty) {
	    n = 0;
	    free(Tree0(t));
            }
	 else
	    n = traverse(Tree0(t));
	 setloc(t);
	 emitn("llist", n);
	 n = 1;
	 break;

      case N_Loop:			/* loop */
	 switch ((int)Val0(Tree0(t))) {
	    case EVERY:
	       lab = alclab(2);
	       loopsp++;
	       loopsp->ltype = EVERY;
	       loopsp->nextlab = lab;
	       loopsp->breaklab = lab + 1;
	       loopsp->markcount = 1;
	       emit("mark0");
	       traverse(Tree1(t));
	       emit("pop");
	       if (TType(Tree2(t)) != N_Empty) {   /* every e1 do e2 */
		  emit("mark0");
		  loopsp->ltype = N_Loop;
		  loopsp->markcount++;
		  traverse(Tree2(t));
		  loopsp->markcount--;
		  emit("unmark");
		  }
               else
		  free(Tree2(t));
	       emitlab(loopsp->nextlab);
	       emit("efail");
	       emitlab(loopsp->breaklab);
	       loopsp--;
	       break;

	    case REPEAT:
	       lab = alclab(3);
	       loopsp++;
	       loopsp->ltype = N_Loop;
	       loopsp->nextlab = lab + 1;
	       loopsp->breaklab = lab + 2;
	       loopsp->markcount = 1;
	       emitlab(lab);
	       emitl("mark", lab);
	       traverse(Tree1(t));
	       emitlab(loopsp->nextlab);
	       emit("unmark");
	       emitl("goto", lab);
	       emitlab(loopsp->breaklab);
	       loopsp--;
               free(Tree2(t));
	       break;

	    case SUSPEND:			/* suspension expression */
	       if (creatsp > creatstk)
		  nfatal(t, "invalid context for suspend", NULL);
	       lab = alclab(2);
	       loopsp++;
	       loopsp->ltype = EVERY;		/* like every ... do for next */
	       loopsp->nextlab = lab;
	       loopsp->breaklab = lab + 1;
	       loopsp->markcount = 1;
	       emit("mark0");
	       traverse(Tree1(t));
	       setloc(t);
	       emit("psusp");
	       emit("pop");
	       if (TType(Tree2(t)) != N_Empty) { /* suspend e1 do e2 */
		  emit("mark0");
		  loopsp->ltype = N_Loop;
		  loopsp->markcount++;
		  traverse(Tree2(t));
		  loopsp->markcount--;
		  emit("unmark");
		  }
               else
		  free(Tree2(t));
	       emitlab(loopsp->nextlab);
	       emit("efail");
	       emitlab(loopsp->breaklab);
	       loopsp--;
	       break;

	    case WHILE:
	       lab = alclab(3);
	       loopsp++;
	       loopsp->ltype = N_Loop;
	       loopsp->nextlab = lab + 1;
	       loopsp->breaklab = lab + 2;
	       loopsp->markcount = 1;
	       emitlab(lab);
	       emit("mark0");
	       traverse(Tree1(t));
	       if (TType(Tree2(t)) != N_Empty) {
		  emit("unmark");
		  emitl("mark", lab);
		  traverse(Tree2(t));
		  }
               else
		  free(Tree2(t));
	       emitlab(loopsp->nextlab);
	       emit("unmark");
	       emitl("goto", lab);
	       emitlab(loopsp->breaklab);
	       loopsp--;
	       break;

	    case UNTIL:
	       lab = alclab(4);
	       loopsp++;
	       loopsp->ltype = N_Loop;
	       loopsp->nextlab = lab + 2;
	       loopsp->breaklab = lab + 3;
	       loopsp->markcount = 1;
	       emitlab(lab);
	       emitl("mark", lab+1);
	       traverse(Tree1(t));
	       emit("unmark");
	       emit("efail");
	       emitlab(lab+1);
	       emitl("mark", lab);
	       traverse(Tree2(t));
	       emitlab(loopsp->nextlab);
	       emit("unmark");
	       emitl("goto", lab);
	       emitlab(loopsp->breaklab);
	       loopsp--;
	       break;
	    }
	 free(Tree0(t));
	 break;

      case N_Next:			/* next expression */
	 if (loopsp < loopstk || loopsp->nextlab <= 0)
	    nfatal(t, "invalid context for next", NULL);
	 else {
	    if (loopsp->ltype != EVERY && loopsp->markcount > 1)
	       for (i = 0; i < loopsp->markcount - 1; i++)
		  emit("unmark");
	    emitl("goto", loopsp->nextlab);
	    }
	 break;

      case N_Not:			/* not expression */
	 lab = alclab(1);
	 emitl("mark", lab);
	 loopsp->markcount++;
	 traverse(Tree0(t));
	 loopsp->markcount--;
	 emit("unmark");
	 emit("efail");
	 emitlab(lab);
	 emit("pnull");
	 break;

      case N_Proc:			/* procedure */
	 loopsp = loopstk;
	 loopsp->nextlab = 0;
	 loopsp->breaklab = 0;
	 loopsp->markcount = 0;
	 casesp = casestk;
	 creatsp = creatstk;

	 writecheck(fprintf(codefile, "proc %s\n", Str0(Tree0(t))));
	 emitfile(t);
	 lout(codefile);
	 constout(codefile);
	 emit("declend");
	 emitline(t);

	 if (TType(Tree1(t)) != N_Empty) {
	    lab = alclab(1);
	    emitl("init", lab);
	    emitl("mark", lab);
	    traverse(Tree1(t));
	    emit("unmark");
	    emitlab(lab);
	    }
         else
	    free(Tree1(t));
	 if (TType(Tree2(t)) != N_Empty)
	    traverse(Tree2(t));
         else
	    free(Tree2(t));
	 setloc(Tree3(t));
	 emit("pfail");
	 emit("end");
	 if (!silent)
	    fprintf(stderr, "  %s\n", Str0(Tree0(t)));
	 free(Tree0(t));
	 free(Tree3(t));
	 break;

      case N_Real:			/* real literal */
	 emitn("real", (int)Val0(t));
	 break;

      case N_Ret:			/* return expression */
	 if (creatsp > creatstk)
	    nfatal(t, "invalid context for return or fail", NULL);
	 if (Val0(Tree0(t)) == FAIL)
	    free(Tree1(t));
         else {
	    lab = alclab(1);
	    emitl("mark", lab);
	    loopsp->markcount++;
	    traverse(Tree1(t));
	    loopsp->markcount--;
	    setloc(t);
	    emit("pret");
	    emitlab(lab);
	    }
	 setloc(t);
	 emit("pfail");
         free(Tree0(t));
	 break;

      case N_Scan:			/* scanning expression */
	 if (Val0(Tree0(t)) == AUGQMARK)
	    emit("pnull");
	 traverse(Tree1(t));
	 if (Val0(Tree0(t)) == AUGQMARK)
	    emit("sdup");
	 setloc(t);
	 emit("bscan");
	 traverse(Tree2(t));
	 setloc(t);
	 emit("escan");
	 if (Val0(Tree0(t)) == AUGQMARK)
	    emit("asgn");
	 free(Tree0(t));
	 break;

      case N_Sect:			/* section operation */
	 emit("pnull");
	 traverse(Tree1(t));
	 traverse(Tree2(t));
	 if (Val0(Tree0(t)) == PCOLON || Val0(Tree0(t)) == MCOLON)
	    emit("dup");
	 traverse(Tree3(t));
	 setloc(Tree0(t));
	 if (Val0(Tree0(t)) == PCOLON)
	    emit("plus");
	 else if (Val0(Tree0(t)) == MCOLON)
	    emit("minus");
	 setloc(t);
	 emit("sect");
	 free(Tree0(t));
	 break;

      case N_Slist:			/* semicolon-separated expr list */
	 lab = alclab(1);
	 emitl("mark", lab);
	 loopsp->markcount++;
	 traverse(Tree0(t));
	 loopsp->markcount--;
	 emit("unmark");
	 emitlab(lab);
	 traverse(Tree1(t));
	 break;

      case N_Str:			/* string literal */
	 emitn("str", (int)Val0(t));
	 break;

      case N_To:			/* to expression */
	 emit("pnull");
	 traverse(Tree0(t));
	 traverse(Tree1(t));
	 emit("push1");
	 setloc(t);
	 emit("toby");
	 break;

      case N_ToBy:			/* to-by expression */
	 emit("pnull");
	 traverse(Tree0(t));
	 traverse(Tree1(t));
	 traverse(Tree2(t));
	 setloc(t);
	 emit("toby");
	 break;

      case N_Unop:			/* unary operator */
	 unopa((int)Val0(Tree0(t)),t);
	 traverse(Tree1(t));
	 setloc(t);
	 unopb((int)Val0(Tree0(t)));
	 free(Tree0(t));
	 break;

      default:
	 emitn("?????", TType(t));
	 tsyserr("traverse: undefined node type");
      }
   free(t);
   return n;
   }

/*
 * binop emits code for binary operators.  For non-augmented operators,
 *  the name of operator is emitted.  For augmented operators, an "asgn"
 *  is emitted after the name of the operator.
 */
static void binop(op)
int op;
   {
   register int asgn;
   register char *name;

   asgn = 0;
   switch (op) {

      case ASSIGN:
	 name = "asgn";
	 break;

      case AUGCARET:
	 asgn++;
      case CARET:
	 name = "power";
	 break;

      case AUGCONCAT:
	 asgn++;
      case CONCAT:
	 name = "cat";
	 break;

      case AUGDIFF:
	 asgn++;
      case DIFF:
	 name = "diff";
	 break;

      case AUGEQUIV:
	 asgn++;
      case EQUIV:
	 name = "eqv";
	 break;

      case AUGINTER:
	 asgn++;
      case INTER:
	 name = "inter";
	 break;

      case LBRACK:
	 name = "subsc";
	 break;

      case AUGLCONCAT:
	 asgn++;
      case LCONCAT:
	 name = "lconcat";
	 break;

      case AUGSEQ:
	 asgn++;
      case SEQ:
	 name = "lexeq";
	 break;

      case AUGSGE:
	 asgn++;
      case SGE:
	 name = "lexge";
	 break;

      case AUGSGT:
	 asgn++;
      case SGT:
	 name = "lexgt";
	 break;

      case AUGSLE:
	 asgn++;
      case SLE:
	 name = "lexle";
	 break;

      case AUGSLT:
	 asgn++;
      case SLT:
	 name = "lexlt";
	 break;

      case AUGSNE:
	 asgn++;
      case SNE:
	 name = "lexne";
	 break;

      case AUGMINUS:
	 asgn++;
      case MINUS:
	 name = "minus";
	 break;

      case AUGMOD:
	 asgn++;
      case MOD:
	 name = "mod";
	 break;

      case AUGNEQUIV:
	 asgn++;
      case NEQUIV:
	 name = "neqv";
	 break;

      case AUGNMEQ:
	 asgn++;
      case NMEQ:
	 name = "numeq";
	 break;

      case AUGNMGE:
	 asgn++;
      case NMGE:
	 name = "numge";
	 break;

      case AUGNMGT:
	 asgn++;
      case NMGT:
	 name = "numgt";
	 break;

      case AUGNMLE:
	 asgn++;
      case NMLE:
	 name = "numle";
	 break;

      case AUGNMLT:
	 asgn++;
      case NMLT:
	 name = "numlt";
	 break;

      case AUGNMNE:
	 asgn++;
      case NMNE:
	 name = "numne";
	 break;

      case AUGPLUS:
	 asgn++;
      case PLUS:
	 name = "plus";
	 break;

      case REVASSIGN:
	 name = "rasgn";
	 break;

      case REVSWAP:
	 name = "rswap";
	 break;

      case AUGSLASH:
	 asgn++;
      case SLASH:
	 name = "div";
	 break;

      case AUGSTAR:
	 asgn++;
      case STAR:
	 name = "mult";
	 break;

      case SWAP:
	 name = "swap";
	 break;

      case AUGUNION:
	 asgn++;
      case UNION:
	 name = "unions";
	 break;

      default:
	 emitn("?binop", op);
	 tsyserr("binop: undefined binary operator");
      }
   emit(name);
   if (asgn)
      emit("asgn");

   }
/*
 * unopa and unopb handle code emission for unary operators. unary operator
 *  sequences that are the same as binary operator sequences are recognized
 *  by the lexical analyzer as binary operators.  For example, ~===x means to
 *  do three tab(match(...)) operations and then a cset complement, but the
 *  lexical analyzer sees the operator sequence as the "neqv" binary
 *  operation.	unopa and unopb unravel tokens of this form.
 *
 * When a N_Unop node is encountered, unopa is called to emit the necessary
 *  number of "pnull" operations to receive the intermediate results.  This
 *  amounts to a pnull for each operation.
 */
static void unopa(op,t)
int op;
nodeptr t;
   {
   switch (op) {
      case NEQUIV:		/* unary ~ and three = operators */
	 emit("pnull");
      case SNE:		/* unary ~ and two = operators */
      case EQUIV:		/* three unary = operators */
	 emit("pnull");
      case NMNE:		/* unary ~ and = operators */
      case UNION:		/* two unary + operators */
      case DIFF:		/* two unary - operators */
      case SEQ:		/* two unary = operators */
      case INTER:		/* two unary * operators */
	 emit("pnull");
      case BACKSLASH:		/* unary \ operator */
      case BANG:		/* unary ! operator */
      case CARET:		/* unary ^ operator */
      case PLUS:		/* unary + operator */
      case TILDE:		/* unary ~ operator */
      case MINUS:		/* unary - operator */
      case NMEQ:		/* unary = operator */
      case STAR:		/* unary * operator */
      case QMARK:		/* unary ? operator */
      case SLASH:		/* unary / operator */
      case DOT:			/* unary . operator */
         emit("pnull");
         break;
      default:
	 tsyserr("unopa: undefined unary operator");
      }
   }

/*
 * unopb is the back-end code emitter for unary operators.  It emits
 *  the operations represented by the token op.  For tokens representing
 *  a single operator, the name of the operator is emitted.  For tokens
 *  representing a sequence of operators, recursive calls are used.  In
 *  such a case, the operator sequence is "scanned" from right to left
 *  and unopb is called with the token for the appropriate operation.
 *
 * For example, consider the sequence of calls and code emission for "~===":
 *	unopb(NEQUIV)		~===
 *	    unopb(NMEQ)	=
 *		emits "tabmat"
 *	    unopb(NMEQ)	=
 *		emits "tabmat"
 *	    unopb(NMEQ)	=
 *		emits "tabmat"
 *	    emits "compl"
 */
static void unopb(op)
int op;
   {
   register char *name;

   switch (op) {

      case DOT:			/* unary . operator */
	 name = "value";
	 break;

      case BACKSLASH:		/* unary \ operator */
	 name = "nonnull";
	 break;

      case BANG:		/* unary ! operator */
	 name = "bang";
	 break;

      case CARET:		/* unary ^ operator */
	 name = "refresh";
	 break;

      case UNION:		/* two unary + operators */
	 unopb(PLUS);
      case PLUS:		/* unary + operator */
	 name = "number";
	 break;

      case NEQUIV:		/* unary ~ and three = operators */
	 unopb(NMEQ);
      case SNE:		/* unary ~ and two = operators */
	 unopb(NMEQ);
      case NMNE:		/* unary ~ and = operators */
	 unopb(NMEQ);
      case TILDE:		/* unary ~ operator (cset compl) */
	 name = "compl";
	 break;

      case DIFF:		/* two unary - operators */
	 unopb(MINUS);
      case MINUS:		/* unary - operator */
	 name = "neg";
	 break;

      case EQUIV:		/* three unary = operators */
	 unopb(NMEQ);
      case SEQ:		/* two unary = operators */
	 unopb(NMEQ);
      case NMEQ:		/* unary = operator */
	 name = "tabmat";
	 break;

      case INTER:		/* two unary * operators */
	 unopb(STAR);
      case STAR:		/* unary * operator */
	 name = "size";
	 break;

      case QMARK:		/* unary ? operator */
	 name = "random";
	 break;

      case SLASH:		/* unary / operator */
	 name = "null";
	 break;

      default:
	 emitn("?unop", op);
	 tsyserr("unopb: undefined unary operator");
      }
   emit(name);
   }

/*
 * emitfile(n) emits "filen" directives for node n's source location.
 * emitline(n) emits "line" and possibly "colm" directives.
 * setloc(n) does both.
 *  A directive is only emitted if the corresponding value
 *  has changed since the previous call.
 *
 */
static char *lastfiln = NULL;
static int lastlin = 0;

static void setloc(n)
nodeptr n;
   {
   emitfile(n);
   emitline(n);
   }

static void emitfile(n)
nodeptr n;
   {
   if ((n != NULL) &&
      (TType(n) != N_Empty) &&
      (File(n) != NULL) &&
      (lastfiln == NULL || strcmp(File(n), lastfiln) != 0)) {
         lastfiln = File(n);
         emits("filen", lastfiln);
         }
   }

static void emitline(n)
nodeptr n;
   {
   #ifdef SrcColumnInfo
      /*
       * if either line or column has changed, emit location information
       */
      if (((Col(n) << 16) + Line(n)) != lastlin) {
         lastlin = (Col(n) << 16) + Line(n);
         emitn("line",Line(n));
         emitn("colm",Col(n));
         }
   #else				/* SrcColumnInfo */
      /*
       * if line has changed, emit line information
       */
      if (Line(n) != lastlin) {
         lastlin = Line(n);
         emitn("line", lastlin);
         }
   #endif				/* SrcColumnInfo */
   }

/*
 * The emit* routines output ucode to codefile.  The various routines are:
 *
 *  emitlab(l) - emit "lab" instruction for label l.
 *  emit(s) - emit instruction s.
 *  emitl(s,a) - emit instruction s with reference to label a.
 *  emitn(s,n) - emit instruction s with numeric argument a.
 *  emits(s,a) - emit instruction s with string argument a.
 */
static void emitlab(l)
int l;
   {
   writecheck(fprintf(codefile, "lab L%d\n", l));
   }

static void emit(s)
char *s;
   {
   writecheck(fprintf(codefile, "\t%s\n", s));
   }

static void emitl(s, a)
char *s;
int a;
   {
   writecheck(fprintf(codefile, "\t%s\tL%d\n", s, a));
   }

static void emitn(s, a)
char *s;
int a;
   {
   writecheck(fprintf(codefile, "\t%s\t%d\n", s, a));
   }

static void emits(s, a)
char *s, *a;
   {
   writecheck(fprintf(codefile, "\t%s\t%s\n", s, a));
   }

/*
 * alclab allocates n labels and returns the first.  For the interpreter,
 *  labels are restarted at 1 for each procedure, while in the compiler,
 *  they start at 1 and increase throughout the entire compilation.
 */
static int alclab(n)
int n;
   {
   register int lab;

   lab = nextlab;
   nextlab += n;
   return lab;
   }
