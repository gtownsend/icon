/*
 * grammar.h -- Yacc grammar for Icon
 *
 * This file is combined with other files to make the Yacc input for
 * building icont, iconc, and variant translators.
 *
 * Any modifications to this grammar will require corresponding changes to
 * parserr.h, icont/tgrammar.c, iconc/cgrammar.c, and vtran/vtfiles/ident.c.
 */

program	: decls EOFX {Progend($1,$2);} ;

decls	: ;
	| decls decl ;

decl	: record {Recdcl($1);} ;
	| proc {Procdcl($1);} ;
	| global {Globdcl($1);} ;
	| link {Linkdcl($1);} ;
        | invocable {Invocdcl($1);} ;

invocable : INVOCABLE invoclist {Invocable($1, $2);} ;

invoclist : invocop;
	  | invoclist COMMA invocop {Invoclist($1,$2,$3);} ;

invocop  : IDENT {Invocop1($1);} ;
	 | STRINGLIT {Invocop2($1);} ;
	 | STRINGLIT COLON INTLIT {Invocop3($1,$2,$3);} ;

link	: LINK lnklist {Link($1, $2);} ;

lnklist	: lnkfile ;
	| lnklist COMMA lnkfile {Lnklist($1,$2,$3);} ;

lnkfile	: IDENT {Lnkfile1($1);} ;
	| STRINGLIT {Lnkfile2($1);} ;

global	: GLOBAL {Global0($1);} idlist  {Global1($1, $2, $3);} ;

record	: RECORD IDENT {Record1($1,$2);} LPAREN fldlist RPAREN {
		Record2($1,$2,$3,$4,$5,$6);
		} ;

fldlist	: {Arglist1();} ;
	| idlist {Arglist2($1);} ;

proc	: prochead SEMICOL locals initial procbody END {
		Proc1($1,$2,$3,$4,$5,$6);
		} ;

prochead: PROCEDURE IDENT {Prochead1($1,$2);} LPAREN arglist RPAREN {
		Prochead2($1,$2,$3,$4,$5,$6);
		} ;

arglist	: {Arglist1();} ;
	| idlist {Arglist2($1);} ;
	| idlist LBRACK RBRACK {Arglist3($1,$2,$3);} ;


idlist	: IDENT {
		Ident($1);
		} ;
	| idlist COMMA IDENT {
		Idlist($1,$2,$3);
		} ;

locals	: {Locals1();} ;
	| locals retention idlist SEMICOL {Locals2($1,$2,$3,$4);} ;

retention: LOCAL {Local($1);} ;
	| STATIC {Static($1);} ;

initial	: {Initial1();} ;
	| INITIAL expr SEMICOL {Initial2($1,$2,$3);} ;

procbody: {Procbody1();} ;
	| nexpr SEMICOL procbody {Procbody2($1,$2,$3);} ;

nexpr	: {Nexpr();} ;
	| expr ;

expr	: expr1a ;
	| expr AND expr1a	{Bamper($1,$2,$3);} ;

expr1a	: expr1 ;
	| expr1a QMARK expr1	{Bques($1,$2,$3);} ;

expr1	: expr2 ;
	| expr2 SWAP expr1 {Bswap($1,$2,$3);} ;
	| expr2 ASSIGN expr1 {Bassgn($1,$2,$3);} ;
	| expr2 REVSWAP expr1 {Brswap($1,$2,$3);} ;
	| expr2 REVASSIGN expr1 {Brassgn($1,$2,$3);} ;
	| expr2 AUGCONCAT expr1 {Baugcat($1,$2,$3);} ;
	| expr2 AUGLCONCAT expr1 {Bauglcat($1,$2,$3);} ;
	| expr2 AUGDIFF expr1 {Bdiffa($1,$2,$3);} ;
	| expr2 AUGUNION expr1 {Buniona($1,$2,$3);} ;
	| expr2 AUGPLUS expr1 {Bplusa($1,$2,$3);} ;
	| expr2 AUGMINUS expr1 {Bminusa($1,$2,$3);} ;
	| expr2 AUGSTAR expr1 {Bstara($1,$2,$3);} ;
	| expr2 AUGINTER expr1 {Bintera($1,$2,$3);} ;
	| expr2 AUGSLASH expr1 {Bslasha($1,$2,$3);} ;
	| expr2 AUGMOD expr1 {Bmoda($1,$2,$3);} ;
	| expr2 AUGCARET expr1 {Bcareta($1,$2,$3);} ;
	| expr2 AUGNMEQ expr1 {Baugeq($1,$2,$3);} ;
	| expr2 AUGEQUIV expr1 {Baugeqv($1,$2,$3);} ;
	| expr2 AUGNMGE expr1 {Baugge($1,$2,$3);} ;
	| expr2 AUGNMGT expr1 {Bauggt($1,$2,$3);} ;
	| expr2 AUGNMLE expr1 {Baugle($1,$2,$3);} ;
	| expr2 AUGNMLT expr1 {Bauglt($1,$2,$3);} ;
	| expr2 AUGNMNE expr1 {Baugne($1,$2,$3);} ;
	| expr2 AUGNEQUIV expr1 {Baugneqv($1,$2,$3);} ;
	| expr2 AUGSEQ expr1 {Baugseq($1,$2,$3);} ;
	| expr2 AUGSGE expr1 {Baugsge($1,$2,$3);} ;
	| expr2 AUGSGT expr1 {Baugsgt($1,$2,$3);} ;
	| expr2 AUGSLE expr1 {Baugsle($1,$2,$3);} ;
	| expr2 AUGSLT expr1 {Baugslt($1,$2,$3);} ;
	| expr2 AUGSNE expr1 {Baugsne($1,$2,$3);} ;
	| expr2 AUGQMARK expr1 {Baugques($1,$2,$3);} ;
	| expr2 AUGAND expr1 {Baugamper($1,$2,$3);} ;
	| expr2 AUGAT expr1 {Baugact($1,$2,$3);} ;

expr2	: expr3 ;
	| expr2 TO expr3 {To0($1,$2,$3);} ;
	| expr2 TO expr3 BY expr3 {To1($1,$2,$3,$4,$5);} ;

expr3	: expr4 ;
	| expr4 BAR expr3 {Alt($1,$2,$3);} ;

expr4	: expr5 ;
	| expr4 SEQ expr5 {Bseq($1,$2,$3);} ;
	| expr4 SGE expr5 {Bsge($1,$2,$3);} ;
	| expr4 SGT expr5 {Bsgt($1,$2,$3);} ;
	| expr4 SLE expr5 {Bsle($1,$2,$3);} ;
	| expr4 SLT expr5 {Bslt($1,$2,$3);} ;
	| expr4 SNE expr5 {Bsne($1,$2,$3);} ;
	| expr4 NMEQ expr5 {Beq($1,$2,$3);} ;
	| expr4 NMGE expr5 {Bge($1,$2,$3);} ;
	| expr4 NMGT expr5 {Bgt($1,$2,$3);} ;
	| expr4 NMLE expr5 {Ble($1,$2,$3);} ;
	| expr4 NMLT expr5 {Blt($1,$2,$3);} ;
	| expr4 NMNE expr5 {Bne($1,$2,$3);} ;
	| expr4 EQUIV expr5 {Beqv($1,$2,$3);} ;
	| expr4 NEQUIV expr5 {Bneqv($1,$2,$3);} ;

expr5	: expr6 ;
	| expr5 CONCAT expr6 {Bcat($1,$2,$3);} ;
	| expr5 LCONCAT expr6 {Blcat($1,$2,$3);} ;

expr6	: expr7 ;
	| expr6 PLUS expr7 {Bplus($1,$2,$3);} ;
	| expr6 DIFF expr7 {Bdiff($1,$2,$3);} ;
	| expr6 UNION expr7 {Bunion($1,$2,$3);} ;
	| expr6 MINUS expr7 {Bminus($1,$2,$3);} ;

expr7	: expr8 ;
	| expr7 STAR expr8 {Bstar($1,$2,$3);} ;
	| expr7 INTER expr8 {Binter($1,$2,$3);} ;
	| expr7 SLASH expr8 {Bslash($1,$2,$3);} ;
	| expr7 MOD expr8 {Bmod($1,$2,$3);} ;

expr8	: expr9 ;
	| expr9 CARET expr8 {Bcaret($1,$2,$3);} ;

expr9	: expr10 ;
	| expr9 BACKSLASH expr10 {Blim($1,$2,$3);} ;
	| expr9 AT expr10 {Bact($1,$2,$3);};
	| expr9 BANG expr10 {Apply($1,$2,$3);};

expr10	: expr11 ;
	| AT expr10 {Uat($1,$2);} ;
	| NOT expr10 {Unot($1,$2);} ;
	| BAR expr10 {Ubar($1,$2);} ;
	| CONCAT expr10 {Uconcat($1,$2);} ;
	| LCONCAT expr10 {Ulconcat($1,$2);} ;
	| DOT expr10 {Udot($1,$2);} ;
	| BANG expr10 {Ubang($1,$2);} ;
	| DIFF expr10 {Udiff($1,$2);} ;
	| PLUS expr10 {Uplus($1,$2);} ;
	| STAR expr10 {Ustar($1,$2);} ;
	| SLASH expr10 {Uslash($1,$2);} ;
	| CARET expr10 {Ucaret($1,$2);} ;
	| INTER expr10 {Uinter($1,$2);} ;
	| TILDE expr10 {Utilde($1,$2);} ;
	| MINUS expr10 {Uminus($1,$2);} ;
	| NMEQ expr10 {Unumeq($1,$2);} ;
	| NMNE expr10 {Unumne($1,$2);} ;
	| SEQ expr10 {Ulexeq($1,$2);} ;
	| SNE expr10 {Ulexne($1,$2);} ;
	| EQUIV expr10 {Uequiv($1,$2);} ;
	| UNION expr10 {Uunion($1,$2);} ;
	| QMARK expr10 {Uqmark($1,$2);} ;
	| NEQUIV expr10 {Unotequiv($1,$2);} ;
	| BACKSLASH expr10 {Ubackslash($1,$2);} ;

expr11	: literal ;
	| section ;
	| return ;
	| if ;
	| case ;
	| while ;
	| until ;
	| every ;
	| repeat ;
	| CREATE expr {Create($1,$2);} ;
	| IDENT {Var($1);} ;
	| NEXT {Next($1);} ;
	| BREAK nexpr {Break($1,$2);} ;
	| LPAREN exprlist RPAREN {Paren($1,$2,$3);} ;
	| LBRACE compound RBRACE {Brace($1,$2,$3);} ;
	| LBRACK exprlist RBRACK {Brack($1,$2,$3);} ;
	| expr11 LBRACK exprlist RBRACK {Subscript($1,$2,$3,$4);} ;
	| expr11 LBRACE	RBRACE {Pdco0($1,$2,$3);} ;
	| expr11 LBRACE pdcolist RBRACE {Pdco1($1,$2,$3,$4);} ;
	| expr11 LPAREN exprlist RPAREN {Invoke($1,$2,$3,$4);} ;
	| expr11 DOT IDENT {Field($1,$2,$3);} ;
	| AND FAIL {Kfail($1,$2);} ;
	| AND IDENT {Keyword($1,$2);} ;

while	: WHILE expr {While0($1,$2);} ;
	| WHILE expr DO expr {While1($1,$2,$3,$4);} ;

until	: UNTIL expr {Until0($1,$2);} ;
	| UNTIL expr DO expr {Until1($1,$2,$3,$4);} ;

every	: EVERY expr {Every0($1,$2);} ;
	| EVERY expr DO expr {Every1($1,$2,$3,$4);} ;

repeat	: REPEAT expr {Repeat($1,$2);} ;

return	: FAIL {Fail($1);} ;
	| RETURN nexpr {Return($1,$2);} ;
	| SUSPEND nexpr {Suspend0($1,$2);} ;
        | SUSPEND expr DO expr {Suspend1($1,$2,$3,$4);};

if	: IF expr THEN expr {If0($1,$2,$3,$4);} ;
	| IF expr THEN expr ELSE expr {If1($1,$2,$3,$4,$5,$6);} ;

case	: CASE expr OF LBRACE caselist RBRACE {Case($1,$2,$3,$4,$5,$6);} ;

caselist: cclause ;
	| caselist SEMICOL cclause {Caselist($1,$2,$3);} ;

cclause	: DEFAULT COLON expr {Cclause0($1,$2,$3);} ;
	| expr COLON expr {Cclause1($1,$2,$3);} ;

exprlist: nexpr                {Elst0($1);}
	| exprlist COMMA nexpr {Elst1($1,$2,$3);} ;

pdcolist: nexpr {
		Pdcolist0($1);
		} ;
	| pdcolist COMMA nexpr {
		Pdcolist1($1,$2,$3);
		} ;

literal	: INTLIT {Iliter($1);} ;
	| REALLIT {Rliter($1);} ;
	| STRINGLIT {Sliter($1);} ;
	| CSETLIT {Cliter($1);} ;

section	: expr11 LBRACK expr sectop expr RBRACK {Section($1,$2,$3,$4,$5,$6);} ;

sectop	: COLON {Colon($1);} ;
	| PCOLON {Pcolon($1);} ;
	| MCOLON {Mcolon($1);} ;

compound: nexpr ;
	| nexpr SEMICOL compound {Compound($1,$2,$3);} ;

program	: error decls EOFX ;
proc	: prochead error procbody END ;
expr	: error ;
