/*
 * tgrammar.c - includes and macros for building the parse tree
 */

#include "../h/define.h"
#include "../common/yacctok.h"

%{
/*
 * These commented directives are passed through the first application
 * of cpp, then turned into real includes in tgram.g by fixgram.icn.
 */
/*#include "../h/gsupport.h"*/
/*#include "../h/lexdef.h"*/
/*#include "tproto.h"*/
/*#include "tglobals.h"*/
/*#include "tsym.h"*/
/*#include "tree.h"*/
/*#include "keyword.h"*/
/*#undef YYSTYPE*/
/*#define YYSTYPE nodeptr*/
/*#define YYMAXDEPTH 500*/

extern int fncargs[];
int idflag;
int id_cnt;

#define EmptyNode tree1(N_Empty)

#define Alt(x1,x2,x3)		$$ = tree4(N_Alt,x2,x1,x3)
#define Apply(x1,x2,x3)		$$ = tree4(N_Apply,x2,x1,x3)
#define Arglist1()		id_cnt = 0
#define Arglist2(x)		/* empty */
#define Arglist3(x,y,z)		id_cnt = -id_cnt
#define Bact(x1,x2,x3)		$$ = tree5(N_Activat,x2,x2,x3,x1)
#define Bamper(x1,x2,x3)	$$ = tree5(N_Conj,x2,x2,x1,x3)
#define Bassgn(x1,x2,x3)	$$ = tree5(N_Binop,x2,x2,x1,x3)
#define Baugact(x1,x2,x3)	$$ = tree5(N_Activat,x2,x2,x3,x1)
#define Baugamper(x1,x2,x3)	$$ = tree5(N_Conj,x2,x2,x1,x3)
#define Baugcat(x1,x2,x3)	$$ = tree5(N_Augop,x2,x2,x1,x3)
#define Baugeq(x1,x2,x3)	$$ = tree5(N_Augop,x2,x2,x1,x3)
#define Baugeqv(x1,x2,x3)	$$ = tree5(N_Augop,x2,x2,x1,x3)
#define Baugge(x1,x2,x3)	$$ = tree5(N_Augop,x2,x2,x1,x3)
#define Bauggt(x1,x2,x3)	$$ = tree5(N_Augop,x2,x2,x1,x3)
#define Bauglcat(x1,x2,x3)	$$ = tree5(N_Augop,x2,x2,x1,x3)
#define Baugle(x1,x2,x3)	$$ = tree5(N_Augop,x2,x2,x1,x3)
#define Bauglt(x1,x2,x3)	$$ = tree5(N_Augop,x2,x2,x1,x3)
#define Baugne(x1,x2,x3)	$$ = tree5(N_Augop,x2,x2,x1,x3)
#define Baugneqv(x1,x2,x3)	$$ = tree5(N_Augop,x2,x2,x1,x3)
#define Baugques(x1,x2,x3)	$$ = tree5(N_Scan,x2,x2,x1,x3)
#define Baugseq(x1,x2,x3)	$$ = tree5(N_Augop,x2,x2,x1,x3)
#define Baugsge(x1,x2,x3)	$$ = tree5(N_Augop,x2,x2,x1,x3)
#define Baugsgt(x1,x2,x3)	$$ = tree5(N_Augop,x2,x2,x1,x3)
#define Baugsle(x1,x2,x3)	$$ = tree5(N_Augop,x2,x2,x1,x3)
#define Baugslt(x1,x2,x3)	$$ = tree5(N_Augop,x2,x2,x1,x3)
#define Baugsne(x1,x2,x3)	$$ = tree5(N_Augop,x2,x2,x1,x3)
#define Bcaret(x1,x2,x3)	$$ = tree5(N_Binop,x2,x2,x1,x3)
#define Bcareta(x1,x2,x3)	$$ = tree5(N_Augop,x2,x2,x1,x3)
#define Bcat(x1,x2,x3)		$$ = tree5(N_Binop,x2,x2,x1,x3)
#define Bdiff(x1,x2,x3)		$$ = tree5(N_Binop,x2,x2,x1,x3)
#define Bdiffa(x1,x2,x3)	$$ = tree5(N_Augop,x2,x2,x1,x3)
#define Beq(x1,x2,x3)		$$ = tree5(N_Binop,x2,x2,x1,x3)
#define Beqv(x1,x2,x3)		$$ = tree5(N_Binop,x2,x2,x1,x3)
#define Bge(x1,x2,x3)		$$ = tree5(N_Binop,x2,x2,x1,x3)
#define Bgt(x1,x2,x3)		$$ = tree5(N_Binop,x2,x2,x1,x3)
#define Binter(x1,x2,x3)	$$ = tree5(N_Binop,x2,x2,x1,x3)
#define Bintera(x1,x2,x3)	$$ = tree5(N_Augop,x2,x2,x1,x3)
#define Blcat(x1,x2,x3)		$$ = tree5(N_Binop,x2,x2,x1,x3)
#define Ble(x1,x2,x3)		$$ = tree5(N_Binop,x2,x2,x1,x3)
#define Blim(x1,x2,x3)		$$ = tree4(N_Limit,x1,x1,x3)
#define Blt(x1,x2,x3)		$$ = tree5(N_Binop,x2,x2,x1,x3)
#define Bminus(x1,x2,x3)	$$ = tree5(N_Binop,x2,x2,x1,x3)
#define Bminusa(x1,x2,x3)	$$ = tree5(N_Augop,x2,x2,x1,x3)
#define Bmod(x1,x2,x3)		$$ = tree5(N_Binop,x2,x2,x1,x3)
#define Bmoda(x1,x2,x3)		$$ = tree5(N_Augop,x2,x2,x1,x3)
#define Bne(x1,x2,x3)		$$ = tree5(N_Binop,x2,x2,x1,x3)
#define Bneqv(x1,x2,x3)		$$ = tree5(N_Binop,x2,x2,x1,x3)
#define Bplus(x1,x2,x3)		$$ = tree5(N_Binop,x2,x2,x1,x3)
#define Bplusa(x1,x2,x3)	$$ = tree5(N_Augop,x2,x2,x1,x3)
#define Bques(x1,x2,x3)		$$ = tree5(N_Scan,x2,x2,x1,x3)
#define Brace(x1,x2,x3)		$$ = x2
#define Brack(x1,x2,x3)		$$ = tree3(N_List,x1,x2)
#define Brassgn(x1,x2,x3)	$$ = tree5(N_Binop,x2,x2,x1,x3)
#define Break(x1,x2)		$$ = tree3(N_Break,x1,x2)
#define Brswap(x1,x2,x3)	$$ = tree5(N_Binop,x2,x2,x1,x3)
#define Bseq(x1,x2,x3)		$$ = tree5(N_Binop,x2,x2,x1,x3)
#define Bsge(x1,x2,x3)		$$ = tree5(N_Binop,x2,x2,x1,x3)
#define Bsgt(x1,x2,x3)		$$ = tree5(N_Binop,x2,x2,x1,x3)
#define Bslash(x1,x2,x3)	$$ = tree5(N_Binop,x2,x2,x1,x3)
#define Bslasha(x1,x2,x3)	$$ = tree5(N_Augop,x2,x2,x1,x3)
#define Bsle(x1,x2,x3)		$$ = tree5(N_Binop,x2,x2,x1,x3)
#define Bslt(x1,x2,x3)		$$ = tree5(N_Binop,x2,x2,x1,x3)
#define Bsne(x1,x2,x3)		$$ = tree5(N_Binop,x2,x2,x1,x3)
#define Bstar(x1,x2,x3)		$$ = tree5(N_Binop,x2,x2,x1,x3)
#define Bstara(x1,x2,x3)	$$ = tree5(N_Augop,x2,x2,x1,x3)
#define Bswap(x1,x2,x3)		$$ = tree5(N_Binop,x2,x2,x1,x3)
#define Bunion(x1,x2,x3)	$$ = tree5(N_Binop,x2,x2,x1,x3)
#define Buniona(x1,x2,x3)	$$ = tree5(N_Augop,x2,x2,x1,x3)
#define Call(x1,x2,x3,x4)	if (Val2(x1) = blocate(Str0(x1))) {\
                                   Val4(x1) = fncargs[Val2(x1)-1]; \
				   $$ = tree4(N_Call,x2,x1,x3);} \
				else { \
				   Val0(x1) = putloc(Str0(x1),0); \
				   $$ = tree4(N_Invok,x2,x1,x3); \
				   }
#define Case(x1,x2,x3,x4,x5,x6) $$ = tree4(N_Case,x1,x2,x5)
#define Caselist(x1,x2,x3)	$$ = tree4(N_Clist,x2,x1,x3)
#define Cclause0(x1,x2,x3)	$$ = tree4(N_Ccls,x2,x1,x3)
#define Cclause1(x1,x2,x3)	$$ = tree4(N_Ccls,x2,x1,x3)
#define Cliter(x)		Val0(x) = putlit(Str0(x),F_CsetLit,(int)Val1(x))
#define Colon(x)		$$ = x
#define Compound(x1,x2,x3)	$$ = tree4(N_Slist,x2,x1,x3)
#define Create(x1,x2)		$$ = tree3(N_Create,x1,x2)
#define Elst0(x1)		/* empty */
#define Elst1(x1,x2,x3)		$$ = tree4(N_Elist,x2,x1,x3)
#define Every0(x1,x2)		$$ = tree5(N_Loop,x1,x1,x2,EmptyNode)
#define Every1(x1,x2,x3,x4)	$$ = tree5(N_Loop,x1,x1,x2,x4)
#define Fail(x)			$$ = tree4(N_Ret,x,x,EmptyNode)
#define Field(x1,x2,x3)		$$ = tree4(N_Field,x2,x1,x3)
#define Global0(x)		idflag = F_Global
#define Global1(x1,x2,x3)	/* empty */
#define Globdcl(x)		/* empty */
#define Ident(x)		install(Str0(x),idflag,0);\
				id_cnt = 1
#define Idlist(x1,x2,x3)	install(Str0(x3),idflag,0);\
				++id_cnt
#define If0(x1,x2,x3,x4)	$$ = tree5(N_If,x1,x2,x4,EmptyNode)
#define If1(x1,x2,x3,x4,x5,x6)	$$ = tree5(N_If,x1,x2,x4,x6)
#define Iliter(x)		Val0(x) = putlit(Str0(x),F_IntLit,0)
#define Initial1()		$$ = EmptyNode
#define Initial2(x1,x2,x3)	$$ = x2
#define Invocable(x1,x2)	/* empty */
#define Invocdcl(x1)		/* empty */
#define Invoclist(x1,x2,x3)	/* empty */
#define Invocop1(x1)		addinvk(Str0(x1),1)
#define Invocop2(x1)		addinvk(Str0(x1),2)
#define Invocop3(x1,x2,x3)	addinvk(Str0(x1),3)
#define Invoke(x1,x2,x3,x4)	$$ = tree4(N_Invok,x2,x1,x3)
#define Keyword(x1,x2)		if (klookup(Str0(x2)) == 0)\
				   tfatal("invalid keyword",Str0(x2));\
				$$ = c_str_leaf(N_Key,x1,Str0(x2))
#define Kfail(x1,x2)		$$ = c_str_leaf(N_Key,x1,"fail")
#define Link(x1,x2)		/* empty */
#define Linkdcl(x)		/* empty */
#define Lnkfile1(x)		addlfile(Str0(x))
#define Lnkfile2(x)		addlfile(Str0(x))
#define Lnklist(x1,x2,x3)	/* empty */
#define Local(x)		idflag = F_Dynamic
#define Locals1()		/* empty */
#define Locals2(x1,x2,x3,x4)	/* empty */
#define Mcolon(x)		$$ = x
#define Nexpr()			$$ = EmptyNode
#define Next(x)			$$ = tree2(N_Next,x)
#define Paren(x1,x2,x3)		if ((x2)->n_type == N_Elist)\
				   $$ = tree4(N_Invok,x1,EmptyNode,x2);\
				else\
				   $$ = x2
#define Pcolon(x)		$$ = x
#define Pdco0(x1,x2,x3)		$$ = tree4(N_Invok,x2,x1,\
				      tree3(N_List,x2,EmptyNode))
#define Pdco1(x1,x2,x3,x4)	$$ = tree4(N_Invok,x2,x1,tree3(N_List,x2,x3))
#define Pdcolist0(x)		$$ = tree3(N_Create,x,x)
#define Pdcolist1(x1,x2,x3)	$$ = tree4(N_Elist,x2,x1,tree3(N_Create,x2,x3))
#define Proc1(x1,x2,x3,x4,x5,x6) $$ = tree6(N_Proc,x1,x1,x4,x5,x6)
#define Procbody1()		$$ = EmptyNode
#define Procbody2(x1,x2,x3)	$$ = tree4(N_Slist,x2,x1,x3)
#define Procdcl(x)		if (!nocode)\
				   codegen(x);\
				nocode = 0;\
				loc_init()
#define Prochead1(x1,x2)	idflag = F_Argument
#define Prochead2(x1,x2,x3,x4,x5,x6)\
				$$ = x2;\
				install(Str0(x2),F_Proc|F_Global,id_cnt)
#define Progend(x1,x2)		gout(globfile)
#define Recdcl(x)		if (!nocode)\
				   rout(globfile, Str0(x));\
				nocode = 0;\
				loc_init()
#define Record1(x1,x2)		idflag = F_Argument
#define Record2(x1,x2,x3,x4,x5,x6) install(Str0(x2),F_Record|F_Global,id_cnt); \
				    $$ = x2
#define Repeat(x1,x2)		$$ = tree5(N_Loop,x1,x1,x2,EmptyNode)
#define Return(x1,x2)		$$ = tree4(N_Ret,x1,x1,x2)
#define Rliter(x)		Val0(x) = putlit(Str0(x),F_RealLit,0)
#define Section(x1,x2,x3,x4,x5,x6) $$ = tree6(N_Sect,x4,x4,x1,x3,x5)
#define Sliter(x)		Val0(x) = putlit(Str0(x),F_StrLit,(int)Val1(x))
#define Static(x)		idflag = F_Static
#define Subscript(x1,x2,x3,x4)	$$ = buildarray(x1,x2,x3,x4)
#define Suspend0(x1,x2)		$$ = tree5(N_Loop,x1,x1,x2,EmptyNode)
#define Suspend1(x1,x2,x3,x4)	$$ = tree5(N_Loop,x1,x1,x2,x4)
#define To0(x1,x2,x3)		$$ = tree4(N_To,x2,x1,x3)
#define To1(x1,x2,x3,x4,x5)	$$ = tree5(N_ToBy,x2,x1,x3,x5)
#define Uat(x1,x2)		$$ = tree5(N_Activat,x1,x1,x2,EmptyNode)
#define Ubackslash(x1,x2)	$$ = tree4(N_Unop,x1,x1,x2)
#define Ubang(x1,x2)		$$ = tree4(N_Unop,x1,x1,x2)
#define Ubar(x1,x2)		$$ = tree3(N_Bar,x2,x2)
#define Ucaret(x1,x2)		$$ = tree4(N_Unop,x1,x1,x2)
#define Uconcat(x1,x2)		$$ = tree3(N_Bar,x2,x2)
#define Udiff(x1,x2)		$$ = tree4(N_Unop,x1,x1,x2)
#define Udot(x1,x2)		$$ = tree4(N_Unop,x1,x1,x2)
#define Uequiv(x1,x2)		$$ = tree4(N_Unop,x1,x1,x2)
#define Uinter(x1,x2)		$$ = tree4(N_Unop,x1,x1,x2)
#define Ulconcat(x1,x2)		$$ = tree3(N_Bar,x2,x2)
#define Ulexeq(x1,x2)		$$ = tree4(N_Unop,x1,x1,x2)
#define Ulexne(x1,x2)		$$ = tree4(N_Unop,x1,x1,x2)
#define Uminus(x1,x2)		$$ = tree4(N_Unop,x1,x1,x2)
#define Unot(x1,x2)		$$ = tree3(N_Not,x2,x2)
#define Unotequiv(x1,x2)	$$ = tree4(N_Unop,x1,x1,x2)
#define Until0(x1,x2)		$$ = tree5(N_Loop,x1,x1,x2,EmptyNode)
#define Until1(x1,x2,x3,x4)	$$ = tree5(N_Loop,x1,x1,x2,x4)
#define Unumeq(x1,x2)		$$ = tree4(N_Unop,x1,x1,x2)
#define Unumne(x1,x2)		$$ = tree4(N_Unop,x1,x1,x2)
#define Uplus(x1,x2)		$$ = tree4(N_Unop,x1,x1,x2)
#define Uqmark(x1,x2)		$$ = tree4(N_Unop,x1,x1,x2)
#define Uslash(x1,x2)		$$ = tree4(N_Unop,x1,x1,x2)
#define Ustar(x1,x2)		$$ = tree4(N_Unop,x1,x1,x2)
#define Utilde(x1,x2)		$$ = tree4(N_Unop,x1,x1,x2)
#define Uunion(x1,x2)		$$ = tree4(N_Unop,x1,x1,x2)
#define Var(x)			Val0(x) = putloc(Str0(x),0)
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
static void xfree(p)
char *p;
{
   free(p);
}

/*#define free(p) xfree((char*)p)*/
