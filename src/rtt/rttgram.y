/*
 * Grammar for RTL. The C portion of the grammar is based on
 *  the ANSI Draft Standard - 3rd review.
 */

%{
#include "rtt1.h"
#define YYMAXDEPTH 250
%}

%union {
   struct token *t;
   struct node *n;
   long i;
   }

%token <t> Identifier StrLit LStrLit FltConst DblConst LDblConst
%token <t> CharConst LCharConst IntConst UIntConst LIntConst ULIntConst
%token <t> Arrow Incr Decr LShft RShft Leq Geq Equal Neq
%token <t> And Or MultAsgn DivAsgn ModAsgn PlusAsgn
%token <t> MinusAsgn LShftAsgn RShftAsgn AndAsgn
%token <t> XorAsgn OrAsgn Sizeof Intersect OpSym

%token <t> Typedef Extern Static Auto Register Tended
%token <t> Char Short Int Long Signed Unsigned Float Doubl Const Volatile
%token <t> Void TypeDefName Struct Union Enum Ellipsis

%token <t> Case Default If Else Switch While Do For Goto Continue Break Return

%token <t> '%' '&' '(' ')' '*' '+' ',' '-' '.' '/' '{' '|' '}' '~' '[' ']'
%token <t> '^' ':' ';' '<' '=' '>' '?' '!' '@' '\\'

%token <t> Runerr Is Cnv Def Exact Empty_type IconType Component Variable
%token <t> Any_value Named_var Struct_var C_Integer Arith_case
%token <t> C_Double C_String Tmp_string Tmp_cset Body End Function Keyword
%token <t> Operator Underef Declare Suspend Fail Inline Abstract Store
%token <t> Type New All_fields Then Type_case Of Len_case Constant Errorfail

%type <t> unary_op assign_op struct_or_union typedefname
%type <t> identifier op_name key_const union attrb_name

%type <n> any_ident storage_class_spec type_qual
%type <n> primary_expr postfix_expr arg_expr_lst unary_expr cast_expr
%type <n> multiplicative_expr additive_expr shift_expr relational_expr
%type <n> equality_expr and_expr exclusive_or_expr inclusive_or_expr
%type <n> logical_and_expr logical_or_expr conditional_expr assign_expr
%type <n> expr opt_expr constant_expr opt_constant_expr dcltion
%type <n> typ_dcltion_specs dcltion_specs type_ind type_storcl_tqual_lst
%type <n> storcl_tqual_lst init_dcltor_lst no_tdn_init_dcltor_lst init_dcltor
%type <n> no_tdn_init_dcltor type_spec stnd_type struct_or_union_spec
%type <n> struct_dcltion_lst struct_dcltion struct_dcltion_specs struct_type_ind
%type <n> struct_type_lst struct_dcltor_lst struct_dcltor
%type <n> struct_no_tdn_dcltor_lst struct_no_tdn_dcltor enum_spec enumerator_lst
%type <n> enumerator dcltor no_tdn_dcltor direct_dcltor no_tdn_direct_dcltor
%type <n> pointer opt_pointer tqual_lst param_type_lst opt_param_type_lst
%type <n> param_lst param_dcltion ident_lst type_tqual_lst type_name
%type <n> abstract_dcltor direct_abstract_dcltor initializer initializer_lst
%type <n> stmt labeled_stmt compound_stmt dcltion_lst opt_dcltion_lst stmt_lst
%type <n> expr_stmt selection_stmt iteration_stmt jump_stmt parm_dcls_or_ids
%type <n> func_head opt_stmt_lst local_dcls local_dcl
%type <n> dest_type i_type_name opt_actions actions action ret_val detail_code
%type <n> runerr variable checking_conversions label
%type <n> type_check type_select_lst opt_default type_select selector_lst
%type <n> c_opt_default c_type_select c_type_select_lst non_lbl_stmt
%type <n> simple_check_conj simple_check len_select_lst len_select
%type <n> type_computations side_effect_lst side_effect
%type <n> type basic_type type_lst

%type <i> opt_plus length

/* Get rid of shift/reduce conflict on Else. Use precedence to force shift of
   Else rather than reduction of if-cond-expr. This insures that the Else
   is always paired with innermost If. Note, IfStmt is a dummy token. */
%nonassoc IfStmt
%nonassoc Else

%start translation_unit
%%

primary_expr
   : identifier   {$$ = sym_node($1);}
   | StrLit       {$$ = node0(PrimryNd, $1);}
   | LStrLit      {$$ = node0(PrimryNd, $1);}
   | FltConst     {$$ = node0(PrimryNd, $1);}
   | DblConst     {$$ = node0(PrimryNd, $1);}
   | LDblConst    {$$ = node0(PrimryNd, $1);}
   | CharConst    {$$ = node0(PrimryNd, $1);}
   | LCharConst   {$$ = node0(PrimryNd, $1);}
   | IntConst     {$$ = node0(PrimryNd, $1);}
   | UIntConst    {$$ = node0(PrimryNd, $1);}
   | LIntConst    {$$ = node0(PrimryNd, $1);}
   | ULIntConst   {$$ = node0(PrimryNd, $1);}
   | '(' expr ')' {$$ = node1(PrefxNd, $1, $2); free_t($3);}
   ;

postfix_expr
   : primary_expr
   | postfix_expr '[' expr ']'         {$$ = node2(BinryNd, $2, $1, $3);
                                        free_t($4);}
   | postfix_expr '(' ')'              {$$ = node2(BinryNd, $3, $1, NULL);
                                        free_t($2);}
   | postfix_expr '(' arg_expr_lst ')' {$$ = node2(BinryNd, $4, $1, $3);
                                        free_t($2);}
   | postfix_expr '.' any_ident        {$$ = node2(BinryNd, $2, $1, $3);}
   | postfix_expr Arrow any_ident      {$$ = node2(BinryNd, $2, $1, $3);}
   | postfix_expr Incr                 {$$ = node1(PstfxNd, $2, $1);}
   | postfix_expr Decr                 {$$ = node1(PstfxNd, $2, $1);}
   | Is  ':' i_type_name '(' assign_expr ')'
      {$$ = node2(BinryNd, $1, $3, $5); free_t($2); free_t($4); free_t($6);}
   | Cnv ':' dest_type   '(' assign_expr ',' assign_expr ')'
      {$$ = node3(TrnryNd, $1, $3, $5, $7), free_t($2); free_t($4); free_t($6);
       free_t($8);}
   | Def ':' dest_type   '(' assign_expr ',' assign_expr ',' assign_expr ')'
      {$$ = node4(QuadNd, $1, $3, $5, $7, $9), free_t($2); free_t($4);
       free_t($6); free_t($8); free_t($10);}
   ;

arg_expr_lst
   : assign_expr
   | arg_expr_lst ',' assign_expr {$$ = node2(CommaNd, $2, $1, $3);}
   ;

unary_expr
   : postfix_expr
   | Incr unary_expr          {$$ = node1(PrefxNd, $1, $2);}
   | Decr unary_expr          {$$ = node1(PrefxNd, $1, $2);}
   | unary_op cast_expr       {$$ = node1(PrefxNd, $1, $2);}
   | Sizeof unary_expr        {$$ = node1(PrefxNd, $1, $2);}
   | Sizeof '(' type_name ')' {$$ = node1(PrefxNd, $1, $3);
                               free_t($2); free_t($4);}
   ;

unary_op
   : '&'
   | '*'
   | '+'
   | '-'
   | '~'
   | '!'
   ;

cast_expr
   : unary_expr
   | '(' type_name ')' cast_expr {$$ = node2(BinryNd, $1, $2, $4); free_t($3);}
   ;

multiplicative_expr
   : cast_expr
   | multiplicative_expr '*' cast_expr {$$ = node2(BinryNd, $2, $1, $3);}
   | multiplicative_expr '/' cast_expr {$$ = node2(BinryNd, $2, $1, $3);}
   | multiplicative_expr '%' cast_expr {$$ = node2(BinryNd, $2, $1, $3);}
   ;

additive_expr
   : multiplicative_expr
   | additive_expr '+' multiplicative_expr {$$ = node2(BinryNd, $2, $1, $3);}
   | additive_expr '-' multiplicative_expr {$$ = node2(BinryNd, $2, $1, $3);}
   ;

shift_expr
   : additive_expr
   | shift_expr LShft additive_expr {$$ = node2(BinryNd, $2, $1, $3);}
   | shift_expr RShft additive_expr {$$ = node2(BinryNd, $2, $1, $3);}
   ;

relational_expr
   : shift_expr
   | relational_expr '<' shift_expr {$$ = node2(BinryNd, $2, $1, $3);}
   | relational_expr '>' shift_expr {$$ = node2(BinryNd, $2, $1, $3);}
   | relational_expr Leq shift_expr {$$ = node2(BinryNd, $2, $1, $3);}
   | relational_expr Geq shift_expr {$$ = node2(BinryNd, $2, $1, $3);}
   ;

equality_expr
   : relational_expr
   | equality_expr Equal relational_expr {$$ = node2(BinryNd, $2, $1, $3);}
   | equality_expr Neq   relational_expr {$$ = node2(BinryNd, $2, $1, $3);}
   ;

and_expr
   : equality_expr
   | and_expr '&' equality_expr {$$ = node2(BinryNd, $2, $1, $3);}
   ;

exclusive_or_expr
   : and_expr
   | exclusive_or_expr '^' and_expr {$$ = node2(BinryNd, $2, $1, $3);}
   ;

inclusive_or_expr
   : exclusive_or_expr
   | inclusive_or_expr '|' exclusive_or_expr {$$ = node2(BinryNd, $2, $1, $3);}
   ;

logical_and_expr
   : inclusive_or_expr
   | logical_and_expr And inclusive_or_expr {$$ = node2(BinryNd, $2, $1, $3);}
   ;

logical_or_expr
   : logical_and_expr
   | logical_or_expr Or logical_and_expr {$$ = node2(BinryNd, $2, $1, $3);}
   ;

conditional_expr
   : logical_or_expr
   | logical_or_expr '?' expr ':' conditional_expr
                                         {$$ = node3(TrnryNd, $2, $1, $3, $5);
                                          free_t($4);}
   ;

assign_expr
   : conditional_expr
   | unary_expr assign_op assign_expr {$$ = node2(BinryNd, $2, $1, $3);}
   ;

assign_op
   : '='
   | MultAsgn
   | DivAsgn
   | ModAsgn
   | PlusAsgn
   | MinusAsgn
   | LShftAsgn
   | RShftAsgn
   | AndAsgn
   | XorAsgn
   | OrAsgn
   ;

expr
   : assign_expr
   | expr ',' assign_expr {$$ = node2(BinryNd, $2, $1, $3);}
   ;

opt_expr
   : {$$ = NULL;}
   | expr
   ;

constant_expr
   : conditional_expr
   ;

opt_constant_expr
   : {$$ = NULL;}
   | constant_expr
   ;

dcltion
   :  typ_dcltion_specs ';'                 {$$ = node2(BinryNd, $2, $1, NULL);
                                             dcl_stk->kind_dcl = OtherDcl;}
   |  typ_dcltion_specs init_dcltor_lst ';' {$$ = node2(BinryNd, $3, $1, $2);
                                             dcl_stk->kind_dcl = OtherDcl;}
   |  storcl_tqual_lst no_tdn_init_dcltor_lst ';'
                                            {$$ = node2(BinryNd, $3, $1, $2);
                                             dcl_stk->kind_dcl = OtherDcl;}
   ;

typ_dcltion_specs
   :                  type_ind
   | storcl_tqual_lst type_ind {$$ = node2(LstNd, NULL, $1, $2);}
   ;

dcltion_specs
   : typ_dcltion_specs
   | storcl_tqual_lst
   ;

type_ind
   : typedefname             {$$ = node0(PrimryNd, $1);}
   | typedefname storcl_tqual_lst
                             {$$ = node2(LstNd, NULL, node0(PrimryNd, $1), $2);}
   | type_storcl_tqual_lst
   ;

type_storcl_tqual_lst
   : stnd_type
   | type_storcl_tqual_lst stnd_type          {$$ = node2(LstNd, NULL, $1, $2);}
   | type_storcl_tqual_lst storage_class_spec {$$ = node2(LstNd, NULL, $1, $2);}
   | type_storcl_tqual_lst type_qual          {$$ = node2(LstNd, NULL, $1, $2);}
   ;

storcl_tqual_lst
   : storage_class_spec
   | type_qual
   | storcl_tqual_lst storage_class_spec {$$ = node2(LstNd, NULL, $1, $2);}
   | storcl_tqual_lst type_qual          {$$ = node2(LstNd, NULL, $1, $2);}
   ;

init_dcltor_lst
   : init_dcltor
   | init_dcltor_lst ',' init_dcltor {$$ = node2(CommaNd, $2, $1, $3);}
   ;

no_tdn_init_dcltor_lst
   : no_tdn_init_dcltor
   | no_tdn_init_dcltor_lst ',' no_tdn_init_dcltor
                                              {$$ = node2(CommaNd, $2, $1, $3);}
   ;

init_dcltor
   : dcltor                 {$$ = $1; id_def($1, NULL);}
   | dcltor '=' initializer {$$ = node2(BinryNd, $2, $1, $3); id_def($1, $3);}
   ;

no_tdn_init_dcltor
   : no_tdn_dcltor            {$$ = $1; id_def($1, NULL);}
   | no_tdn_dcltor '=' initializer
                              {$$ = node2(BinryNd, $2, $1, $3); id_def($1, $3);}
   ;

storage_class_spec
   : Typedef  {$$ = node0(PrimryNd, $1); dcl_stk->kind_dcl = IsTypedef;}
   | Extern   {$$ = node0(PrimryNd, $1);}
   | Static   {$$ = node0(PrimryNd, $1);}
   | Auto     {$$ = node0(PrimryNd, $1);}
   | Register {$$ = node0(PrimryNd, $1);}
   ;

type_spec
   : stnd_type
   | typedefname {$$ = node0(PrimryNd, $1);}
   ;

stnd_type
   : Void                {$$ = node0(PrimryNd, $1);}
   | Char                {$$ = node0(PrimryNd, $1);}
   | Short               {$$ = node0(PrimryNd, $1);}
   | Int                 {$$ = node0(PrimryNd, $1);}
   | Long                {$$ = node0(PrimryNd, $1);}
   | Float               {$$ = node0(PrimryNd, $1);}
   | Doubl               {$$ = node0(PrimryNd, $1);}
   | Signed              {$$ = node0(PrimryNd, $1);}
   | Unsigned            {$$ = node0(PrimryNd, $1);}
   | struct_or_union_spec
   | enum_spec
   ;

struct_or_union_spec
   : struct_or_union any_ident '{' struct_dcltion_lst '}'
                                            {$$ = node2(BinryNd, $1, $2, $4);
                                             free_t($3); free_t($5);}
   | struct_or_union '{' struct_dcltion_lst '}'
                                            {$$ = node2(BinryNd, $1, NULL, $3);
                                             free_t($2); free_t($4);}
   | struct_or_union any_ident              {$$ = node2(BinryNd, $1, $2, NULL);}
   ;

struct_or_union
   : Struct
   | Union
   ;

struct_dcltion_lst
   : struct_dcltion
   | struct_dcltion_lst struct_dcltion {$$ = node2(LstNd, NULL, $1, $2);}
   ;

struct_dcltion
   : struct_dcltion_specs struct_dcltor_lst ';'
                                              {$$ = node2(BinryNd, $3, $1, $2);}
   | tqual_lst struct_no_tdn_dcltor_lst ';'   {$$ = node2(BinryNd, $3, $1, $2);}
   ;

struct_dcltion_specs
   :           struct_type_ind
   | tqual_lst struct_type_ind  {$$ = node2(LstNd, NULL, $1, $2);}
   ;

struct_type_ind
   : typedefname            {$$ = node0(PrimryNd, $1);}
   | typedefname tqual_lst  {$$ = node2(LstNd, NULL, node0(PrimryNd, $1), $2);}
   | struct_type_lst
   ;

struct_type_lst
   : stnd_type
   | struct_type_lst stnd_type {$$ = node2(LstNd, NULL, $1, $2);}
   | struct_type_lst type_qual {$$ = node2(LstNd, NULL, $1, $2);} ;

struct_dcltor_lst
   : struct_dcltor
   | struct_dcltor_lst ',' struct_dcltor {$$ = node2(CommaNd, $2, $1, $3);}
   ;

struct_dcltor
   : dcltor                   {$$ = node2(StrDclNd, NULL, $1, NULL);
                               if (dcl_stk->parms_done) pop_cntxt();}
   |        ':' constant_expr {$$ = node2(StrDclNd, $1, NULL, $2);}
   | dcltor ':' {if (dcl_stk->parms_done) pop_cntxt();} constant_expr
                              {$$ = node2(StrDclNd, $2, $1, $4);}
   ;

struct_no_tdn_dcltor_lst
   : struct_no_tdn_dcltor
   | struct_no_tdn_dcltor_lst ',' struct_no_tdn_dcltor
                                              {$$ = node2(CommaNd, $2, $1, $3);}
   ;

struct_no_tdn_dcltor
   : no_tdn_dcltor                   {$$ = node2(StrDclNd, NULL, $1, NULL);
                                      if (dcl_stk->parms_done) pop_cntxt();}
   |               ':' constant_expr {$$ = node2(StrDclNd, $1, NULL, $2);}
   | no_tdn_dcltor ':' {if (dcl_stk->parms_done) pop_cntxt();} constant_expr
                                     {$$ = node2(StrDclNd, $2, $1, $4);}
   ;

enum_spec
   : Enum {push_cntxt(0);} '{' enumerator_lst '}'
       {$$ = node2(BinryNd, $1, NULL, $4); pop_cntxt(); free_t($3); free_t($5);}
   | Enum any_ident {push_cntxt(0);} '{' enumerator_lst '}'
       {$$ = node2(BinryNd, $1, $2,  $5); pop_cntxt(); free_t($4); free_t($6);}
   | Enum any_ident {$$ = node2(BinryNd, $1, $2,  NULL);}
   ;

enumerator_lst
   : enumerator
   | enumerator_lst ',' enumerator {$$ = node2(CommaNd, $2, $1, $3);}
   ;

enumerator
   : any_ident                {$$ = $1; id_def($1, NULL);}
   | any_ident '=' constant_expr
                              {$$ = node2(BinryNd, $2, $1, $3); id_def($1, $3);}
   ;

type_qual
   : Const    {$$ = node0(PrimryNd, $1);}
   | Volatile {$$ = node0(PrimryNd, $1);}
   ;


dcltor
   : opt_pointer direct_dcltor  {$$ = node2(ConCatNd, NULL, $1, $2);}
   ;

no_tdn_dcltor
   : opt_pointer no_tdn_direct_dcltor {$$ = node2(ConCatNd, NULL, $1, $2);}
   ;

direct_dcltor
   : any_ident
   | '(' dcltor ')'                           {$$ = node1(PrefxNd, $1, $2);
                                               free_t($3);}
   | direct_dcltor '[' opt_constant_expr  ']' {$$ = node2(BinryNd, $2, $1, $3);
                                               free_t($4);}
   | direct_dcltor '(' {push_cntxt(1);} parm_dcls_or_ids ')'
                                              {$$ = node2(BinryNd, $5, $1, $4);
                                               if (dcl_stk->nest_lvl == 2)
                                                  dcl_stk->parms_done = 1;
                                                else
                                                  pop_cntxt();
                                               free_t($2);}
   ;

no_tdn_direct_dcltor
   : identifier                               {$$ = node0(PrimryNd, $1);}
   | '(' no_tdn_dcltor ')'                    {$$ = node1(PrefxNd, $1, $2);
                                               free_t($3);}
   | no_tdn_direct_dcltor '[' opt_constant_expr  ']'
                                              {$$ = node2(BinryNd, $2, $1, $3);
                                               free_t($4);}
   | no_tdn_direct_dcltor '(' {push_cntxt(1);} parm_dcls_or_ids ')'
                                              {$$ = node2(BinryNd, $5, $1, $4);
                                               if (dcl_stk->nest_lvl == 2)
                                                  dcl_stk->parms_done = 1;
                                                else
                                                  pop_cntxt();
                                               free_t($2);}
   ;

parm_dcls_or_ids
   : opt_param_type_lst
   | ident_lst
   ;

pointer
   : '*'                   {$$ = node0(PrimryNd, $1);}
   | '*' tqual_lst         {$$ = node1(PreSpcNd, $1, $2);}
   | '*' pointer           {$$ = node1(PrefxNd, $1, $2);}
   | '*' tqual_lst pointer {$$ = node1(PrefxNd, $1, node2(LstNd, NULL, $2,$3));}
   ;

opt_pointer
   : {$$ = NULL;}
   | pointer
   ;

tqual_lst
   : type_qual
   | tqual_lst type_qual {$$ = node2(LstNd, NULL, $1, $2);}
   ;

param_type_lst
   : param_lst
   | param_lst ',' Ellipsis {$$ = node2(CommaNd, $2, $1, node0(PrimryNd, $3));}
   ;

opt_param_type_lst
   : {$$ = NULL;}
   | param_type_lst
   ;

param_lst
   : param_dcltion
   | param_lst ',' param_dcltion {$$ = node2(CommaNd, $2, $1, $3);}
   ;

param_dcltion
   : dcltion_specs no_tdn_dcltor   {$$ = node2(LstNd, NULL, $1, $2);
                                    id_def($2, NULL);}
   | dcltion_specs
   | dcltion_specs abstract_dcltor {$$ = node2(LstNd, NULL, $1, $2);}
   ;

ident_lst
   : identifier               {$$ = node0(PrimryNd, $1);}
   | ident_lst ',' identifier {$$ = node2(CommaNd, $2, $1, node0(PrimryNd,$3));}
   ;

type_tqual_lst
   : type_spec
   | type_qual
   | type_spec type_tqual_lst {$$ = node2(LstNd, NULL, $1, $2);}
   | type_qual type_tqual_lst {$$ = node2(LstNd, NULL, $1, $2);}
   ;

type_name
   : type_tqual_lst
   | type_tqual_lst abstract_dcltor {$$ = node2(LstNd, NULL, $1, $2);}
   ;

abstract_dcltor
   : pointer
   | opt_pointer direct_abstract_dcltor {$$ = node2(ConCatNd, NULL, $1, $2);}
   ;

direct_abstract_dcltor
   : '(' abstract_dcltor ')'                {$$ = node1(PrefxNd, $1, $2);
                                             free_t($3);}
   |                        '[' opt_constant_expr  ']'
                                            {$$ = node2(BinryNd, $1, NULL, $2);
                                             free_t($3);}
   | direct_abstract_dcltor '[' opt_constant_expr  ']'
                                            {$$ = node2(BinryNd, $2, $1, $3);
                                             free_t($4);}
   |                        '(' {push_cntxt(1);} opt_param_type_lst ')'
                                            {$$ = node2(BinryNd, $4, NULL, $3);
                                             pop_cntxt();
                                             free_t($1);}
   | direct_abstract_dcltor '(' {push_cntxt(1);} opt_param_type_lst ')'
                                            {$$ = node2(BinryNd, $5, $1, $4);
                                             pop_cntxt();
                                             free_t($2);}
   ;

initializer
   : assign_expr
   | '{' initializer_lst '}'
                        {$$ = node1(PrefxNd, $1, $2); free_t($3);}
   | '{' initializer_lst ',' '}'
                        {$$ = node1(PrefxNd, $1, node2(CommaNd, $3, $2, NULL));
                          free_t($4);}
   ;

initializer_lst
   : initializer
   | initializer_lst ',' initializer {$$ = node2(CommaNd, $2, $1, $3);}
   ;

stmt
   : labeled_stmt
   | non_lbl_stmt
   ;

non_lbl_stmt
   : {push_cntxt(1);} compound_stmt {$$ = $2; pop_cntxt();}
   | expr_stmt
   | selection_stmt
   | iteration_stmt
   | jump_stmt
   | Runerr '(' assign_expr ')' ';'
      {$$ = node2(BinryNd, $1, $3, NULL); free_t($2); free_t($4);}
   | Runerr '(' assign_expr ',' assign_expr ')' ';'
      {$$ = node2(BinryNd, $1, $3, $5); free_t($2); free_t($4); free_t($6);}
   ;

labeled_stmt
   : label ':' stmt              {$$ = node2(BinryNd, $2, $1, $3);}
   | Case constant_expr ':' stmt {$$ = node2(BinryNd, $1, $2, $4); free_t($3);}
   | Default ':' stmt            {$$ = node1(PrefxNd, $1, $3); free_t($2);}
   ;

compound_stmt
   : '{'            opt_stmt_lst '}' {$$ = comp_nd($1, NULL, $2); free_t($3);}
   | '{' local_dcls opt_stmt_lst '}' {$$ = comp_nd($1, $2,   $3); free_t($4);}
   ;

dcltion_lst
   : dcltion
   | dcltion_lst dcltion {$$ = node2(LstNd, NULL, $1, $2);}
   ;

opt_dcltion_lst
   : {$$ = NULL;}
   | dcltion_lst
   ;

local_dcls
   : local_dcl
   | local_dcls local_dcl {$$ = ($2 == NULL ? $1 : node2(LstNd, NULL, $1, $2));}
   ;

local_dcl
   : dcltion
   | Tended tended_type init_dcltor_lst ';'
             {$$ = NULL; free_t($1); free_t($4); dcl_stk->kind_dcl = OtherDcl;}
   ;

tended_type
   : Char		{tnd_char(); free_t($1);}
   | Struct identifier  {tnd_strct($2); free_t($1);}
   | Struct TypeDefName {tnd_strct($2); free_t($1);}
   | Union  identifier  {tnd_union($2); free_t($1);}
   ;

stmt_lst
   : stmt
   | stmt_lst stmt {$$ = node2(ConCatNd, NULL, $1, $2);}
   ;

opt_stmt_lst
   : {$$ = NULL;}
   | stmt_lst
   ;
expr_stmt
   : opt_expr ';' {$$ = node1(PstfxNd, $2, $1);}
   ;

selection_stmt
   : If '(' expr ')' stmt   %prec IfStmt {$$ = node3(TrnryNd, $1, $3, $5,NULL);
                                          free_t($2); free_t($4);}
   | If '(' expr ')' stmt Else stmt      {$$ = node3(TrnryNd, $1, $3, $5, $7);
                                          free_t($2); free_t($4); free_t($6);}
   | Switch '(' expr ')' stmt            {$$ = node2(BinryNd, $1, $3, $5);
                                          free_t($2); free_t($4);}
   | Type_case expr Of '{' c_type_select_lst c_opt_default '}'
      {$$ = node3(TrnryNd, $1, $2, $5, $6); free_t($3); free_t($4); free_t($7);}
   ;

c_type_select_lst
   :                   c_type_select {$$ = node2(ConCatNd, NULL, NULL, $1);}
   | c_type_select_lst c_type_select {$$ = node2(ConCatNd, NULL,   $1, $2);}
   ;

c_type_select
   : selector_lst non_lbl_stmt {$$ = node2(ConCatNd, NULL, $1, $2);}
   ;

c_opt_default
   : {$$ = NULL;}
   | Default ':' non_lbl_stmt {$$ = $3; free_t($1); free_t($2);}
   ;

iteration_stmt
   : While '(' expr ')' stmt           {$$ = node2(BinryNd, $1, $3, $5);
                                        free_t($2); free_t($4);}
   | Do stmt While '(' expr ')' ';'    {$$ = node2(BinryNd, $1, $2, $5);
                                        free_t($3); free_t($4); free_t($6);
                                        free_t($7);}
   | For '(' opt_expr ';' opt_expr ';' opt_expr ')' stmt
                                       {$$ = node4(QuadNd, $1, $3, $5, $7, $9);
                                        free_t($2); free_t($4); free_t($6);
                                        free_t($8);}
   ;

jump_stmt
   : Goto label';'       {$$ = node1(PrefxNd, $1, $2); free_t($3);}
   | Continue ';'        {$$ = node0(PrimryNd, $1); free_t($2);}
   | Break ';'           {$$ = node0(PrimryNd, $1); free_t($2);}
   | Return ret_val ';'  {$$ = node1(PrefxNd, $1, $2); free_t($3);}
   | Suspend ret_val ';' {$$ = node1(PrefxNd, $1, $2); free_t($3);}
   | Fail ';'            {$$ = node0(PrimryNd, $1); free_t($2);}
   | Errorfail ';'       {$$ = node0(PrimryNd, $1); free_t($2);}
   ;

translation_unit
   :
   | extrn_decltn_lst
   ;

extrn_decltn_lst
   : external_dcltion
   | extrn_decltn_lst external_dcltion
   ;

external_dcltion
   : function_definition
   | dcltion                {dclout($1);}
   | definition
   ;

function_definition
   : func_head {func_def($1);} opt_dcltion_lst compound_stmt
                                                          {fncout($1, $3, $4);}
   ;

func_head
   :                   no_tdn_dcltor {$$ = node2(LstNd, NULL, NULL, $1);}
   | storcl_tqual_lst  no_tdn_dcltor {$$ = node2(LstNd, NULL, $1, $2);}
   | typ_dcltion_specs dcltor        {$$ = node2(LstNd, NULL, $1, $2);}
   ;

any_ident
   : identifier  {$$ = node0(PrimryNd, $1);}
   | typedefname {$$ = node0(PrimryNd, $1);}
   ;

label
   : identifier  {$$ = lbl($1);}
   | typedefname {$$ = lbl($1);}
   ;

typedefname
   : TypeDefName
   | C_Integer /* hack to allow C_integer to be defined with typedef */
   | C_Double  /* for consistency with C_integer */
   | C_String  /* for consistency with C_integer */
   ;

/*
 * The rest of the grammar implements the interface portion of the language.
 */

definition
   : {strt_def();} description operation
   ;

operation
   : fnc_oper op_declare actions End {defout($3); free_t($4);}
   | keyword             actions End {defout($2); free_t($3);}
   | keyword Constant key_const  End {keyconst($3); free_t($2); free_t($4);}
   ;

description
   :         {comment = NULL;}
   | StrLit  {comment = $1;}
   ;

fnc_oper
   : Function '{' result_seq '}' op_name '(' opt_s_parm_lst ')'
      {impl_fnc($5); free_t($1); free_t($2); free_t($4); free_t($6);
       free_t($8);}
   | Operator '{' result_seq {lex_state = OpHead;} '}' OpSym
      {lex_state = DfltLex;} op_name '(' opt_s_parm_lst ')'
      {impl_op($6, $8); free_t($1); free_t($2); free_t($5); free_t($9);
       free_t($11);}

keyword
   : Keyword  '{' result_seq '}' op_name
       {impl_key($5); free_t($1); free_t($2); free_t($4);}
   ;

key_const
   : StrLit
   | CharConst
   | DblConst
   | IntConst
   ;

/*
 * Allow as many special names to be identifiers as possible
 */
identifier
   : Abstract
   | All_fields
   | Any_value
   | Body
   | Component
   | Declare
   | Empty_type
   | End
   | Exact
   | IconType
   | Identifier
   | Inline
   | Named_var
   | New
   | Of
   | Store
   | Struct_var
   | Then
   | Tmp_cset
   | Tmp_string
   | Type
   | Underef
   | Variable
   ;

/*
 * an operation may be given any name.
 */
op_name
   : identifier
   | typedefname
   | Auto
   | Break
   | Case
   | Char
   | Cnv
   | Const
   | Continue
   | Def
   | Default
   | Do
   | Doubl
   | Else
   | Enum
   | Errorfail
   | Extern
   | Fail
   | Float
   | For
   | Function
   | Goto
   | If
   | Int
   | Is
   | Keyword
   | Long
   | Operator
   | Register
   | Return
   | Runerr
   | Short
   | Signed
   | Sizeof
   | Static
   | Struct
   | Suspend
   | Switch
   | Tended
   | Typedef
   | Union
   | Unsigned
   | Void
   | Volatile
   | While
   ;

result_seq
   :                            {set_r_seq(NoRsltSeq, NoRsltSeq, 0);}
   | length            opt_plus {set_r_seq($1, $1, (int)$2);}
   | length ',' length opt_plus {set_r_seq($1, $3, (int)$4); free_t($2);}
   ;

length
   : IntConst {$$ = ttol($1); free_t($1);}
   | '*'      {$$ = UnbndSeq; free_t($1);}
   ;

opt_plus
   :     {$$ = 0;}
   | '+' {$$ = 1; free_t($1);}
   ;

opt_s_parm_lst
   :
   | s_parm_lst
   | s_parm_lst '[' identifier ']' {var_args($3); free_t($2); free_t($4);}
   ;

s_parm_lst
   : s_parm
   | s_parm_lst ',' s_parm {free_t($2);}
   ;

s_parm
   :                          identifier {s_prm_def(NULL, $1);}
   | Underef identifier                  {s_prm_def($2, NULL); free_t($1);}
   | Underef identifier Arrow identifier {s_prm_def($2, $4);   free_t($1);
                                          free_t($3);}
   ;

op_declare
   : {}
   | Declare '{' local_dcls '}' {d_lst_typ($3); free_t($1); free_t($2);
                                 free_t($4);}
   ;

opt_actions
   : {$$ = NULL;}
   | actions
   ;

actions
   : action
   | actions action {$$ = node2(ConCatNd, NULL, $1, $2);}
   ;

action
   : checking_conversions
   | detail_code
   | runerr
   | '{' opt_actions '}' {$$ = node1(PrefxNd, $1, $2); free_t($3);}
   | Abstract {lex_state = TypeComp;} '{' type_computations
         {lex_state = DfltLex;} '}'
         {$$ = $4; free_t($1); free_t($3); free_t($6);}
   ;

checking_conversions
   : If type_check Then action %prec IfStmt
      {$$ = node3(TrnryNd, $1, $2, $4, NULL); free_t($3);}
   | If type_check Then action Else action
      {$$ = node3(TrnryNd, $1, $2, $4, $6); free_t($3); free_t($5);}
   | Type_case variable Of '{' type_select_lst opt_default '}'
      {$$ = node3(TrnryNd, $1, $2, $5, $6); free_t($3); free_t($4); free_t($7);}
   | Len_case identifier Of '{' len_select_lst Default ':' action '}'
      {$$ = node3(TrnryNd, $1, sym_node($2), $5, $8); free_t($3), free_t($4);
       free_t($6); free_t($7); free_t($9);}
   | Arith_case '(' variable ',' variable ')' Of '{'
      dest_type ':' action dest_type ':' action dest_type ':' action '}'
      {$$ = arith_nd($1, $3, $5, $9, $11, $12, $14, $15, $17); free_t($2);
       free_t($4), free_t($6); free_t($7); free_t($8); free_t($10);
       free_t($13); free_t($16); free_t($18);}
   ;

type_select_lst
   :                 type_select {$$ = node2(ConCatNd, NULL, NULL, $1);}
   | type_select_lst type_select {$$ = node2(ConCatNd, NULL,   $1, $2);}
   ;

type_select
   : selector_lst action {$$ = node2(ConCatNd, NULL, $1, $2);}
   ;

opt_default
   : {$$ = NULL;}
   | Default ':' action {$$ = $3; free_t($1); free_t($2);}
   ;

selector_lst
   :              i_type_name ':' {$$ = node2(ConCatNd, NULL, NULL, $1);
                                   free_t($2);}
   | selector_lst i_type_name ':' {$$ = node2(ConCatNd, NULL,   $1, $2);
                                   free_t($3);}
   ;

len_select_lst
   : len_select
   | len_select_lst len_select {$$ = node2(ConCatNd, NULL, $1, $2);}
   ;

len_select
   : IntConst ':' action {$$ = node1(PrefxNd, $1, $3); free_t($2);}
   ;

type_check
   : simple_check_conj
   | '!' simple_check  {$$ = node1(PrefxNd, $1, $2);}
   ;

simple_check_conj
   : simple_check
   | simple_check_conj And simple_check {$$ = node2(BinryNd, $2, $1, $3);}
   ;

simple_check
   : Is ':' i_type_name '(' variable ')'
      {$$ = node2(BinryNd, $1, $3, $5); free_t($2); free_t($4); free_t($6);}
   | Cnv ':' dest_type '(' variable ')'
      {$$ = node3(TrnryNd, $1, $3, $5, NULL), dst_alloc($3, $5); free_t($2);
       free_t($4); free_t($6);}
   | Cnv ':' dest_type '(' variable  ',' assign_expr ')'
      {$$ = node3(TrnryNd, $1, $3, $5, $7), free_t($2); free_t($4); free_t($6);
       free_t($8);}
   | Def ':' dest_type '(' variable  ',' assign_expr ')'
      {$$ = node4(QuadNd, $1, $3, $5, $7, NULL), dst_alloc($3, $5); free_t($2);
       free_t($4); free_t($6); free_t($8);}
   | Def ':' dest_type '(' variable  ',' assign_expr ',' assign_expr ')'
      {$$ = node4(QuadNd, $1, $3, $5, $7, $9), free_t($2); free_t($4);
       free_t($6); free_t($8); free_t($10);}
   ;

detail_code
   : Body   {push_cntxt(1);} compound_stmt
                        {$$ = node1(PrefxNd, $1, $3); pop_cntxt();}
   | Inline {push_cntxt(1);} compound_stmt
                        {$$ = node1(PrefxNd, $1, $3); pop_cntxt();}
   ;

runerr
   : Runerr '(' IntConst ')' opt_semi
                    {$$ = node2(BinryNd, $1, node0(PrimryNd, $3), NULL);
                     free_t($2); free_t($4);}
   | Runerr '(' IntConst ',' variable ')' opt_semi
                    {$$ = node2(BinryNd, $1, node0(PrimryNd, $3), $5);
                     free_t($2); free_t($4); free_t($6);}
   ;

opt_semi
   :
   | ';' {free_t($1);}
   ;

variable
   : identifier                  {$$ = sym_node($1);}
   | identifier '[' IntConst ']' {$$ = node2(BinryNd, $2, sym_node($1),
                                    node0(PrimryNd, $3));
                                  free_t($4);}

dest_type
   : IconType                {$$ = dest_node($1);}
   | C_Integer               {$$ = node0(PrimryNd, $1);}
   | C_Double                {$$ = node0(PrimryNd, $1);}
   | C_String                {$$ = node0(PrimryNd, $1);}
   | Tmp_string              {$$ = node0(PrimryNd, $1); ++n_tmp_str;}
   | Tmp_cset                {$$ = node0(PrimryNd, $1); ++n_tmp_cset;}
   | '(' Exact ')' IconType  {$$ = node0(ExactCnv, chk_exct($4)); free_t($1);
                              free_t($2); free_t($3);}
   | '(' Exact ')' C_Integer {$$ = node0(ExactCnv, $4); free_t($1); free_t($2);
                              free_t($3);}
   ;

i_type_name
   : Any_value  {$$ = node0(PrimryNd, $1);}
   | Empty_type {$$ = node0(PrimryNd, $1);}
   | IconType   {$$ = sym_node($1);}
   | Variable   {$$ = node0(PrimryNd, $1);}
   ;

ret_val
   : opt_expr
   | C_Integer assign_expr             {$$ = node1(PrefxNd, $1, $2);}
   | C_Double assign_expr              {$$ = node1(PrefxNd, $1, $2);}
   | C_String assign_expr              {$$ = node1(PrefxNd, $1, $2);}
   ;

type_computations
   : side_effect_lst Return type opt_semi {$$ = node2(AbstrNd, $2,   $1,   $3);}
   |                 Return type opt_semi {$$ = node2(AbstrNd, $1,   NULL, $2);}
   | side_effect_lst                      {$$ = node2(AbstrNd, NULL, $1, NULL);}
   ;

side_effect_lst
   : side_effect
   | side_effect_lst side_effect {$$ = node2(ConCatNd, NULL, $1, $2);}
   ;

side_effect
   : Store '[' type ']' '=' type opt_semi {$$ = node2(BinryNd, $5, $3, $6);
                                           free_t($1); free_t($2); free_t($4);}
   ;

type
   : basic_type
   | type union basic_type     {$$ = node2(BinryNd, $2, $1, $3);}
   | type Intersect basic_type {$$ = node2(BinryNd, $2, $1, $3);}

basic_type
   : i_type_name                        {$$ = node1(IcnTypNd,
                                         copy_t($1->tok), $1);}
   | Type '(' variable ')'              {$$ = node1(PrefxNd, $1, $3);
                                         free_t($2); free_t($4);}
   | New i_type_name '(' type_lst ')'   {$$ = node2(BinryNd, $1, $2, $4);
                                         free_t($3); free_t($5);}
   | Store '[' type ']'                 {$$ = node1(PrefxNd, $1, $3);
                                         free_t($2); free_t($4);}
   | basic_type '.' attrb_name          {$$ = node1(PstfxNd, $3, $1);
                                         free_t($2);}
   | '(' type ')'                       {$$ = $2; free_t($1); free_t($3);}
   ;

union
   : Incr
   ;

type_lst
   : type
   | type_lst ',' type {$$ = node2(CommaNd, $2, $1, $3);}
   ;

attrb_name
   : Component
   | All_fields
   ;

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

#define free(p) xfree((char*)p)
