/*
 * cgrammar.c - includes and macros for building the parse tree.
 */
#include "../h/define.h"
#include "../common/yacctok.h"

%{
/*
 * These commented directives are passed through the first application
 * of cpp, then turned into real directives in cgram.g by fixgram.icn.
 */
/*#include "../h/gsupport.h"*/
/*#include "../h/lexdef.h"*/
/*#include "ctrans.h"*/
/*#include "csym.h"*/
/*#include "ctree.h"*/
/*#include "ccode.h" */
/*#include "cproto.h"*/
/*#undef YYSTYPE*/
/*#define YYSTYPE nodeptr*/
/*#define YYMAXDEPTH 500*/

int idflag;

#define EmptyNode tree1(N_Empty) 

#define Alt(x1,x2,x3)		$$ = tree4(N_Alt,x2,x1,x3) 
#define Apply(x1,x2,x3)		$$ = tree4(N_Apply,x2,x1,x3) 
#define Arglist1()		/* empty */
#define Arglist2(x)		/* empty */
#define Arglist3(x1,x2,x3)	proc_lst->nargs = -proc_lst->nargs
#define Bact(x1,x2,x3)		$$ = tree5(N_Activat,x2,x2,x1,x3) 
#define Bamper(x1,x2,x3)	$$ = binary_nd(x2,x1,x3) 
#define Bassgn(x1,x2,x3)	$$ = binary_nd(x2,x1,x3) 
#define Baugact(x1,x2,x3)	$$ = tree5(N_Activat,x2,x2,x1,x3) 
#define Baugamper(x1,x2,x3)	$$ = aug_nd(x2,x1,x3) 
#define Baugcat(x1,x2,x3)	$$ = aug_nd(x2,x1,x3)
#define Baugeq(x1,x2,x3)	$$ = aug_nd(x2,x1,x3)
#define Baugeqv(x1,x2,x3)	$$ = aug_nd(x2,x1,x3)
#define Baugge(x1,x2,x3)	$$ = aug_nd(x2,x1,x3)
#define Bauggt(x1,x2,x3)	$$ = aug_nd(x2,x1,x3)
#define Bauglcat(x1,x2,x3)	$$ = aug_nd(x2,x1,x3)
#define Baugle(x1,x2,x3)	$$ = aug_nd(x2,x1,x3)
#define Bauglt(x1,x2,x3)	$$ = aug_nd(x2,x1,x3)
#define Baugne(x1,x2,x3)	$$ = aug_nd(x2,x1,x3)
#define Baugneqv(x1,x2,x3)	$$ = aug_nd(x2,x1,x3)
#define Baugques(x1,x2,x3)	$$ = tree5(N_Scan,x2,x2,x1,x3) 
#define Baugseq(x1,x2,x3)	$$ = aug_nd(x2,x1,x3)
#define Baugsge(x1,x2,x3)	$$ = aug_nd(x2,x1,x3)
#define Baugsgt(x1,x2,x3)	$$ = aug_nd(x2,x1,x3)
#define Baugsle(x1,x2,x3)	$$ = aug_nd(x2,x1,x3)
#define Baugslt(x1,x2,x3)	$$ = aug_nd(x2,x1,x3)
#define Baugsne(x1,x2,x3)	$$ = aug_nd(x2,x1,x3)
#define Bcaret(x1,x2,x3)	$$ = binary_nd(x2,x1,x3) 
#define Bcareta(x1,x2,x3)	$$ = aug_nd(x2,x1,x3)
#define Bcat(x1,x2,x3)		$$ = binary_nd(x2,x1,x3) 
#define Bdiff(x1,x2,x3)		$$ = binary_nd(x2,x1,x3) 
#define Bdiffa(x1,x2,x3)	$$ = aug_nd(x2,x1,x3)
#define Beq(x1,x2,x3)		$$ = binary_nd(x2,x1,x3)
#define Beqv(x1,x2,x3)		$$ = binary_nd(x2,x1,x3)
#define Bge(x1,x2,x3)		$$ = binary_nd(x2,x1,x3)
#define Bgt(x1,x2,x3)		$$ = binary_nd(x2,x1,x3)
#define Binter(x1,x2,x3)	$$ = binary_nd(x2,x1,x3)
#define Bintera(x1,x2,x3)	$$ = aug_nd(x2,x1,x3)
#define Blcat(x1,x2,x3)		$$ = binary_nd(x2,x1,x3)
#define Ble(x1,x2,x3)		$$ = binary_nd(x2,x1,x3)
#define Blim(x1,x2,x3)		$$ = tree4(N_Limit,x2,x1,x3) 
#define Blt(x1,x2,x3)		$$ = binary_nd(x2,x1,x3)
#define Bminus(x1,x2,x3)	$$ = binary_nd(x2,x1,x3)
#define Bminusa(x1,x2,x3)	$$ = aug_nd(x2,x1,x3)
#define Bmod(x1,x2,x3)		$$ = binary_nd(x2,x1,x3)
#define Bmoda(x1,x2,x3)		$$ = aug_nd(x2,x1,x3)
#define Bne(x1,x2,x3)		$$ = binary_nd(x2,x1,x3)
#define Bneqv(x1,x2,x3)		$$ = binary_nd(x2,x1,x3)
#define Bplus(x1,x2,x3)		$$ = binary_nd(x2,x1,x3)
#define Bplusa(x1,x2,x3)	$$ = aug_nd(x2,x1,x3)
#define Bques(x1,x2,x3)		$$ = tree5(N_Scan,x2,x2,x1,x3) 
#define Brace(x1,x2,x3)		$$ = x2
#define Brack(x1,x2,x3)		$$ = list_nd(x1,x2) 
#define Brassgn(x1,x2,x3)	$$ = binary_nd(x2,x1,x3)
#define Break(x1,x2)		$$ = tree3(N_Break,x1,x2) 
#define Brswap(x1,x2,x3)	$$ = binary_nd(x2,x1,x3)
#define Bseq(x1,x2,x3)		$$ = binary_nd(x2,x1,x3)
#define Bsge(x1,x2,x3)		$$ = binary_nd(x2,x1,x3)
#define Bsgt(x1,x2,x3)		$$ = binary_nd(x2,x1,x3)
#define Bslash(x1,x2,x3)	$$ = binary_nd(x2,x1,x3)
#define Bslasha(x1,x2,x3)	$$ = aug_nd(x2,x1,x3)
#define Bsle(x1,x2,x3)		$$ = binary_nd(x2,x1,x3)
#define Bslt(x1,x2,x3)		$$ = binary_nd(x2,x1,x3)
#define Bsne(x1,x2,x3)		$$ = binary_nd(x2,x1,x3)
#define Bstar(x1,x2,x3)		$$ = binary_nd(x2,x1,x3)
#define Bstara(x1,x2,x3)	$$ = aug_nd(x2,x1,x3)
#define Bswap(x1,x2,x3)		$$ = binary_nd(x2,x1,x3)
#define Bunion(x1,x2,x3)	$$ = binary_nd(x2,x1,x3)
#define Buniona(x1,x2,x3)	$$ = aug_nd(x2,x1,x3)
#define Case(x1,x2,x3,x4,x5,x6) $$ = case_nd(x1,x2,x5) 
#define Caselist(x1,x2,x3)	$$ = tree4(N_Clist,x2,x1,x3) 
#define Cclause0(x1,x2,x3)	$$ = tree4(N_Ccls,x2,x1,x3) 
#define Cclause1(x1,x2,x3)	$$ = tree4(N_Ccls,x2,x1,x3) 
#define Cliter(x)		CSym0(x) = putlit(Str0(x),F_CsetLit,(int)Val1(x))
#define Colon(x)		$$ = x
#define Compound(x1,x2,x3)	$$ = tree4(N_Slist,x2,x1,x3) 
#define Create(x1,x2)		$$ = tree3(N_Create,x1,x2);\
				 proc_lst->has_coexpr = 1;
#define Elst0(x)                $$ = x;
#define Elst1(x1,x2,x3)	        $$ = tree4(N_Elist,x2,x1,x3);
#define Every0(x1,x2)		$$ = tree5(N_Loop,x1,x1,x2,EmptyNode) 
#define Every1(x1,x2,x3,x4)	$$ = tree5(N_Loop,x1,x1,x2,x4) 
#define Fail(x)			$$ = tree4(N_Ret,x,x,EmptyNode) 
#define Field(x1,x2,x3)		$$ = tree4(N_Field,x2,x1,x3) 
#define Global0(x)		idflag = F_Global
#define Global1(x1,x2,x3)	/* empty */
#define Globdcl(x)		/* empty */
#define Ident(x)		install(Str0(x),idflag)
#define Idlist(x1,x2,x3)	install(Str0(x3),idflag)
#define If0(x1,x2,x3,x4)	$$ = tree5(N_If,x1,x2,x4,EmptyNode) 
#define If1(x1,x2,x3,x4,x5,x6)	$$ = tree5(N_If,x1,x2,x4,x6) 
#define Iliter(x)		CSym0(x) = putlit(Str0(x),F_IntLit,0)
#define Initial1()		$$ = EmptyNode
#define Initial2(x1,x2,x3)	$$ = x2
#define Invocdcl(x)             /* empty */
#define Invocable(x1,x2)        /* empty */
#define Invoclist(x1,x2, x3)    /* empty */
#define Invocop1(x)             invoc_grp(Str0(x));
#define Invocop2(x)             invocbl(x, -1);
#define Invocop3(x1,x2,x3)      invocbl(x1, atoi(Str0(x3)));
#define Invoke(x1,x2,x3,x4)	$$ = invk_nd(x2,x1,x3)
#define Keyword(x1,x2)		$$ = key_leaf(x1,Str0(x2))
#define Kfail(x1,x2)		$$ = key_leaf(x1,spec_str("fail")) 
#define Link(x1,x2)		/* empty */
#define Linkdcl(x)		/* empty */
#define Lnkfile1(x)		lnkdcl(Str0(x));
#define Lnkfile2(x)		lnkdcl(Str0(x));
#define Lnklist(x1,x2,x3)	/* empty */
#define Local(x)		idflag = F_Dynamic
#define Locals1()		/* empty */
#define Locals2(x1,x2,x3,x4)	/* empty */
#define Mcolon(x)		$$ = x
#define Nexpr()			$$ = EmptyNode
#define Next(x)			$$ = tree2(N_Next,x) 
#define Paren(x1,x2,x3)		if ((x2)->n_type == N_Elist)\
                                   $$ = invk_nd(x1,EmptyNode,x2);\
 				else\
 				   $$ = x2
#define Pcolon(x)		$$ = x
#define Pdco0(x1,x2,x3)		$$ = invk_nd(x2,x1,list_nd(x2,EmptyNode))
#define Pdco1(x1,x2,x3,x4)	$$ = invk_nd(x2,x1,list_nd(x2,x3)) 
#define Pdcolist0(x)		$$ = tree3(N_Create,x,x);\
				 proc_lst->has_coexpr = 1;
#define Pdcolist1(x1,x2,x3)	$$ =tree4(N_Elist,x2,x1,tree3(N_Create,x2,x3));\
				 proc_lst->has_coexpr = 1;
#define Proc1(x1,x2,x3,x4,x5,x6) $$ = tree6(N_Proc,x1,x1,x4,x5,x6)
#define Procbody1()		$$ = EmptyNode
#define Procbody2(x1,x2,x3)	$$ = tree4(N_Slist,x2,x1,x3) 
#define Procdcl(x)		proc_lst->tree = x
#define Prochead1(x1,x2)	init_proc(Str0(x2));\
				idflag = F_Argument
#define Prochead2(x1,x2,x3,x4,x5,x6) /* empty */
#define Progend(x1,x2)		/* empty */
#define Recdcl(x)		/* empty */
#define Record1(x1, x2)		init_rec(Str0(x2));\
                                idflag = F_Field
#define Record2(x1,x2,x3,x4,x5,x6) /* empty */
#define Repeat(x1,x2)		$$ = tree5(N_Loop,x1,x1,x2,EmptyNode) 
#define Return(x1,x2)		$$ = tree4(N_Ret,x1,x1,x2) 
#define Rliter(x)		CSym0(x) = putlit(Str0(x),F_RealLit,0)
#define Section(x1,x2,x3,x4,x5,x6) $$ = sect_nd(x4,x1,x3,x5) 
#define Sliter(x)		CSym0(x) = putlit(Str0(x),F_StrLit,(int)Val1(x))
#define Static(x)		idflag = F_Static
#define Subscript(x1,x2,x3,x4)	$$ = buildarray(x1,x2,x3)
#define Suspend0(x1,x2)		$$ = tree5(N_Loop,x1,x1,x2,EmptyNode) 
#define Suspend1(x1,x2,x3,x4)	$$ = tree5(N_Loop,x1,x1,x2,x4) 
#define To0(x1,x2,x3)		$$ = to_nd(x2,x1,x3) 
#define To1(x1,x2,x3,x4,x5)	$$ = toby_nd(x2,x1,x3,x5) 
#define Uat(x1,x2)		$$ = tree5(N_Activat,x1,x1,EmptyNode,x2) 
#define Ubackslash(x1,x2)	$$ = unary_nd(x1,x2)
#define Ubang(x1,x2)		$$ = unary_nd(x1,x2)
#define Ubar(x1,x2)		$$ = tree3(N_Bar,x2,x2) 
#define Ucaret(x1,x2)		$$ = unary_nd(x1,x2)
#define Uconcat(x1,x2)		$$ = tree3(N_Bar,x2,x2) 
#define Udiff(x1,x2)		$$ = MultiUnary(x1,x2)
#define Udot(x1,x2)		$$ = unary_nd(x1,x2)
#define Uequiv(x1,x2)		$$ = MultiUnary(x1,x2)
#define Uinter(x1,x2)		$$ = MultiUnary(x1,x2)
#define Ulconcat(x1,x2)		$$ = tree3(N_Bar,x2,x2) 
#define Ulexeq(x1,x2)		$$ = MultiUnary(x1,x2)
#define Ulexne(x1,x2)		$$ = MultiUnary(x1,x2)
#define Uminus(x1,x2)		$$ = unary_nd(x1,x2)
#define Unot(x1,x2)		$$ = tree3(N_Not,x2,x2) 
#define Unotequiv(x1,x2)	$$ = MultiUnary(x1,x2)
#define Until0(x1,x2)		$$ = tree5(N_Loop,x1,x1,x2,EmptyNode) 
#define Until1(x1,x2,x3,x4)	$$ = tree5(N_Loop,x1,x1,x2,x4) 
#define Unumeq(x1,x2)		$$ = unary_nd(x1,x2)
#define Unumne(x1,x2)		$$ = MultiUnary(x1,x2)
#define Uplus(x1,x2)		$$ = unary_nd(x1,x2)
#define Uqmark(x1,x2)		$$ = unary_nd(x1,x2)
#define Uslash(x1,x2)		$$ = unary_nd(x1,x2)
#define Ustar(x1,x2)		$$ = unary_nd(x1,x2)
#define Utilde(x1,x2)		$$ = unary_nd(x1,x2)
#define Uunion(x1,x2)		$$ = MultiUnary(x1,x2)
#define Var(x)			LSym0(x) = putloc(Str0(x),0)
#define While0(x1,x2)		$$ = tree5(N_Loop,x1,x1,x2,EmptyNode)
#define While1(x1,x2,x3,x4)	$$ = tree5(N_Loop,x1,x1,x2,x4) 
%}

%%
#include "../h/grammar.h"
%%

/*
 * xfree(p) -- used with free(p) macro to avoid compiler errors from
 *  miscast free calls generated by Yacc.
 */
#undef free
static void xfree(p)
char *p;
{
   free(p);
}

/*#define free(p) xfree((char*)p)*/
