/*
 * This lexical analyzer uses the preprocessor to convert text into tokens.
 *  The lexical anayser discards white space, checks to see if identifiers
 *  are reserved words or typedef names, makes sure single characters
 *  are valid tokens, and converts preprocessor constants into the
 *  various C constants.
 */
#include "rtt.h"

/*
 * Prototype for static function.
 */
static int int_suffix (char *s);

int lex_state = DfltLex;

char *ident = "ident";

/*
 * Characters are used as token id's for single character tokens. The
 *  following table indicates which ones can be valid for RTL.
 */

#define GoodChar(c) ((c) < 127 && good_char[c])
static int good_char[128] = {
   0  /* \000 */,   0  /* \001 */,   0  /* \002 */,   0  /* \003 */,
   0  /* \004 */,   0  /* \005 */,   0  /* \006 */,   0  /* \007 */,
   0  /*  \b  */,   0  /*  \t  */,   0  /*  \n  */,   0  /*  \v  */,
   0  /*  \f  */,   0  /*  \r  */,   0  /* \016 */,   0  /* \017 */,
   0  /* \020 */,   0  /* \021 */,   0  /* \022 */,   0  /* \023 */,
   0  /* \024 */,   0  /* \025 */,   0  /* \026 */,   0  /* \027 */,
   0  /* \030 */,   0  /* \031 */,   0  /* \032 */,   0  /*  \e  */,
   0  /* \034 */,   0  /* \035 */,   0  /* \036 */,   0  /* \037 */,
   0  /*      */,   1  /*  !   */,   0  /*  \   */,   0  /*  #   */,
   0  /*  $   */,   1  /*  %   */,   1  /*  &   */,   0  /*  '   */,
   1  /*  (   */,   1  /*  )   */,   1  /*  *   */,   1  /*  +   */,
   1  /*  ,   */,   1  /*  -   */,   1  /*  .   */,   1  /*  /   */,
   0  /*  0   */,   0  /*  1   */,   0  /*  2   */,   0  /*  3   */,
   0  /*  4   */,   0  /*  5   */,   0  /*  6   */,   0  /*  7   */,
   0  /*  8   */,   0  /*  9   */,   1  /*  :   */,   1  /*  ;   */,
   1  /*  <   */,   1  /*  =   */,   1  /*  >   */,   1  /*  ?   */,
   0  /*  @   */,   0  /*  A   */,   0  /*  B   */,   0  /*  C   */,
   0  /*  D   */,   0  /*  E   */,   0  /*  F   */,   0  /*  G   */,
   0  /*  H   */,   0  /*  I   */,   0  /*  J   */,   0  /*  K   */,
   0  /*  L   */,   0  /*  M   */,   0  /*  N   */,   0  /*  O   */,
   0  /*  P   */,   0  /*  Q   */,   0  /*  R   */,   0  /*  S   */,
   0  /*  T   */,   0  /*  U   */,   0  /*  V   */,   0  /*  W   */,
   0  /*  X   */,   0  /*  Y   */,   0  /*  Z   */,   1  /*  [   */,
   1  /*  \\  */,   1  /*  ]   */,   1  /*  ^   */,   0  /*  _   */,
   0  /*  `   */,   0  /*  a   */,   0  /*  b   */,   0  /*  c   */,
   0  /*  d   */,   0  /*  e   */,   0  /*  f   */,   0  /*  g   */,
   0  /*  h   */,   0  /*  i   */,   0  /*  j   */,   0  /*  k   */,
   0  /*  l   */,   0  /*  m   */,   0  /*  n   */,   0  /*  o   */,
   0  /*  p   */,   0  /*  q   */,   0  /*  r   */,   0  /*  s   */,
   0  /*  t   */,   0  /*  u   */,   0  /*  v   */,   0  /*  w   */,
   0  /*  x   */,   0  /*  y   */,   0  /*  z   */,   1  /*  {   */,
   1  /*  |   */,   1  /*  }   */,   1  /*  ~   */,   0  /*  \d  */
   };

/*
 * init_lex - initialize lexical analyzer.
 */
void init_lex()
   {
   struct sym_entry *sym;
   int i;
   static int first_time = 1;

   if (first_time) {
      first_time = 0;
      ident = spec_str(ident);  /* install ident in string table */
      /*
       * install C keywords into the symbol table
       */
      sym_add(Auto,          spec_str("auto"),          OtherDcl, 0);
      sym_add(Break,         spec_str("break"),         OtherDcl, 0);
      sym_add(Case,          spec_str("case"),          OtherDcl, 0);
      sym_add(TokChar,       spec_str("char"),          OtherDcl, 0);
      sym_add(Const,         spec_str("const"),         OtherDcl, 0);
      sym_add(Continue,      spec_str("continue"),      OtherDcl, 0);
      sym_add(Default,       spec_str("default"),       OtherDcl, 0);
      sym_add(Do,            spec_str("do"),            OtherDcl, 0);
      sym_add(Doubl,         spec_str("double"),        OtherDcl, 0);
      sym_add(Else,          spec_str("else"),          OtherDcl, 0);
      sym_add(TokEnum,       spec_str("enum"),          OtherDcl, 0);
      sym_add(Extern,        spec_str("extern"),        OtherDcl, 0);
      sym_add(Float,         spec_str("float"),         OtherDcl, 0);
      sym_add(For,           spec_str("for"),           OtherDcl, 0);
      sym_add(Goto,          spec_str("goto"),          OtherDcl, 0);
      sym_add(If,            spec_str("if"),            OtherDcl, 0);
      sym_add(Int,           spec_str("int"),           OtherDcl, 0);
      sym_add(TokLong,       spec_str("long"),          OtherDcl, 0);
      sym_add(TokRegister,   spec_str("register"),      OtherDcl, 0);
      sym_add(Return,        spec_str("return"),        OtherDcl, 0);
      sym_add(TokShort,      spec_str("short"),         OtherDcl, 0);
      sym_add(Signed,        spec_str("signed"),        OtherDcl, 0);
      sym_add(Sizeof,        spec_str("sizeof"),        OtherDcl, 0);
      sym_add(Static,        spec_str("static"),        OtherDcl, 0);
      sym_add(Struct,        spec_str("struct"),        OtherDcl, 0);
      sym_add(Switch,        spec_str("switch"),        OtherDcl, 0);
      sym_add(Typedef,       spec_str("typedef"),       OtherDcl, 0);
      sym_add(Union,         spec_str("union"),         OtherDcl, 0);
      sym_add(Unsigned,      spec_str("unsigned"),      OtherDcl, 0);
      sym_add(Void,          spec_str("void"),          OtherDcl, 0);
      sym_add(Volatile,      spec_str("volatile"),      OtherDcl, 0);
      sym_add(While,         spec_str("while"),         OtherDcl, 0);

      /*
       * Install keywords from run-time interface language.
       */
      sym_add(Abstract,      spec_str("abstract"),      OtherDcl, 0);
      sym_add(All_fields,    spec_str("all_fields"),    OtherDcl, 0);
      sym_add(Any_value,     spec_str("any_value"),     OtherDcl, 0);
      sym_add(Arith_case,    spec_str("arith_case"),    OtherDcl, 0);
      sym_add(Body,          spec_str("body"),          OtherDcl, 0);
      sym_add(C_Double,      spec_str("C_double"),      OtherDcl, 0);
      sym_add(C_Integer,     spec_str("C_integer"),     OtherDcl, 0);
      sym_add(C_String,      spec_str("C_string"),      OtherDcl, 0);
      sym_add(Cnv,           spec_str("cnv"),           OtherDcl, 0);
      sym_add(Constant,      spec_str("constant"),      OtherDcl, 0);
      sym_add(Declare,       spec_str("declare"),       OtherDcl, 0);
      sym_add(Def,           spec_str("def"),           OtherDcl, 0);
      sym_add(Empty_type,    spec_str("empty_type"),    OtherDcl, 0);
      sym_add(End,           spec_str("end"),           OtherDcl, 0);
      sym_add(Errorfail,     spec_str("errorfail"),     OtherDcl, 0);
      sym_add(Exact,         spec_str("exact"),         OtherDcl, 0);
      sym_add(Fail,          spec_str("fail"),          OtherDcl, 0);
      sym_add(TokFunction,   spec_str("function"),      OtherDcl, 0);
      sym_add(Inline,        spec_str("inline"),        OtherDcl, 0);
      sym_add(Is,            spec_str("is"),            OtherDcl, 0);
      sym_add(Keyword,       spec_str("keyword"),       OtherDcl, 0);
      sym_add(Len_case,      spec_str("len_case"),      OtherDcl, 0);
      sym_add(Named_var,     spec_str("named_var"),     OtherDcl, 0);
      sym_add(New,           spec_str("new"),           OtherDcl, 0);
      sym_add(Of,            spec_str("of"),            OtherDcl, 0);
      sym_add(Operator,      spec_str("operator"),      OtherDcl, 0);
      str_rslt = spec_str("result");
      sym_add(Runerr,        spec_str("runerr"),        OtherDcl, 0);
      sym_add(Store,         spec_str("store"),         OtherDcl, 0);
      sym_add(Struct_var,    spec_str("struct_var"),    OtherDcl, 0);
      sym_add(Suspend,       spec_str("suspend"),       OtherDcl, 0);
      sym_add(Tended,        spec_str("tended"),        OtherDcl, 0);
      sym_add(Then,          spec_str("then"),          OtherDcl, 0);
      sym_add(Tmp_cset,      spec_str("tmp_cset"),      OtherDcl, 0);
      sym_add(Tmp_string,    spec_str("tmp_string"),    OtherDcl, 0);
      sym_add(TokType,       spec_str("type"),          OtherDcl, 0);
      sym_add(Type_case,     spec_str("type_case"),     OtherDcl, 0);
      sym_add(Underef,       spec_str("underef"),       OtherDcl, 0);
      sym_add(Variable,      spec_str("variable"),      OtherDcl, 0);

      for (i = 0; i < num_typs; ++i) {
         icontypes[i].id = spec_str(icontypes[i].id);
         sym = sym_add(IconType, icontypes[i].id, OtherDcl, 0);
         sym->u.typ_indx = i;
         }

      for (i = 0; i < num_cmpnts; ++i) {
         typecompnt[i].id = spec_str(typecompnt[i].id);
         sym = sym_add(Component, typecompnt[i].id, OtherDcl, 0);
         sym->u.typ_indx = i;
         }
      }
   }

/*
 * int_suffix - we have reached the end of what seems to be an integer
 *  constant. check for a valid suffix.
 */
static int int_suffix(s)
char *s;
   {
   int tok_id;

   if (*s == 'u' || *s == 'U') {
      ++s;
      if (*s == 'l' || *s == 'L') {
         ++s;
         tok_id = ULIntConst;  /* unsigned long */
         }
      else
         tok_id  = UIntConst;  /* unsigned */
      }
   else if (*s == 'l' || *s == 'L') {
      ++s;
      if (*s == 'u' || *s == 'U') {
         ++s;
         tok_id = ULIntConst;  /* unsigned long */
         }
      else
         tok_id = LIntConst;   /* long */
      }
   else
      tok_id = IntConst;       /* plain int */
   if (*s != '\0')
      errt2(yylval.t, "invalid integer constant: ", yylval.t->image);
   return tok_id;
   }

/*
 * yylex - lexical analyzer, called by yacc-generated parser.
 */
int yylex()
   {
   register char *s;
   struct sym_entry *sym;
   struct token *lk_ahead = NULL;
   int is_float;
   struct str_buf *sbuf;

   /*
    * See if the last call to yylex() left a token from looking ahead.
    */
   if (lk_ahead == NULL)
      yylval.t = preproc();
   else {
      yylval.t = lk_ahead;
      lk_ahead = NULL;
      }

   /*
    * Skip white space, then check for end-of-input.
    */
   while (yylval.t != NULL && yylval.t->tok_id == WhiteSpace) {
      free_t(yylval.t);
      yylval.t = preproc();
      }
   if (yylval.t == NULL)
      return 0;

   /*
    * The rtt recognizes ** as an operator in abstract type computations.
    *  The parsing context is indicated by lex_state.
    */
   if (lex_state == TypeComp && yylval.t->tok_id == '*') {
      lk_ahead = preproc();
      if (lk_ahead != NULL && lk_ahead->tok_id == '*') {
         free_t(lk_ahead);
         lk_ahead = NULL;
         yylval.t->tok_id = Intersect;
         yylval.t->image = spec_str("**");
         }
      }

   /*
    * Some tokens are passed along without change, but some need special
    *  processing: identifiers, numbers, PpKeep tokens, and single
    *  character tokens.
    */
   if (yylval.t->tok_id == Identifier) {
      /*
       * See if this is an identifier, a reserved word, or typedef name.
       */
      sym = sym_lkup(yylval.t->image);
      if (sym != NULL)
         yylval.t->tok_id = sym->tok_id;
      }
   else if (yylval.t->tok_id == PpNumber) {
      /*
       * Determine what kind of numeric constant this is.
       */
      s = yylval.t->image;
      if (*s == '0' && (*++s == 'x' || *s == 'X')) {
         /*
          * Hex integer constant.
          */
         ++s;
         while (isxdigit(*s))
            ++s;
         yylval.t->tok_id = int_suffix(s);
         }
      else {
         is_float = 0;
         while (isdigit(*s))
             ++s;
         if (*s == '.') {
            is_float = 1;
            ++s;
            while (isdigit(*s))
               ++s;
            }
         if (*s == 'e' || *s == 'E') {
            is_float = 1;
            ++s;
            if (*s == '+' || *s == '-')
               ++s;
            while (isdigit(*s))
               ++s;
            }
         if (is_float) {
            switch (*s) {
               case '\0':
                  yylval.t->tok_id = DblConst;   /* double */
                  break;
               case 'f': case 'F':
                   yylval.t->tok_id = FltConst;  /* float */
                   break;
               case 'l': case 'L':
                   yylval.t->tok_id = LDblConst; /* long double */
                   break;
               default:
                   errt2(yylval.t, "invalid float constant: ", yylval.t->image);
               }
            }
         else {
            /*
             * This appears to be an integer constant. If it starts
             *  with '0', it should be an octal constant.
             */
            if (yylval.t->image[0] == '0') {
               s = yylval.t->image;
               while (*s >= '0' && *s <= '7')
                  ++s;
               }
            yylval.t->tok_id = int_suffix(s);
            }
         }
      }
   else if (yylval.t->tok_id == PpKeep) {
      /*
       * This is a non-standard preprocessor directive that must be
       *  passed on to the output.
       */
      keepdir(yylval.t);
      return yylex();
      }
   else if (lex_state == OpHead && yylval.t->tok_id != '}' &&
     GoodChar((int)yylval.t->image[0])) {
      /*
       * This should be the operator symbol in the header of an operation
       *  declaration. Concatenate all operator symbols into one token
       *  of type OpSym.
       */
      sbuf = get_sbuf();
      for (s = yylval.t->image; *s != '\0'; ++s)
         AppChar(*sbuf, *s);
      lk_ahead = preproc();
      while (lk_ahead != NULL && GoodChar((int)lk_ahead->image[0])) {
         for (s = lk_ahead->image; *s != '\0'; ++s)
            AppChar(*sbuf, *s);
         free_t(lk_ahead);
         lk_ahead = preproc();
         }
      yylval.t->tok_id = OpSym;
      yylval.t->image = str_install(sbuf);
      rel_sbuf(sbuf);
      }
   else if (yylval.t->tok_id < 256) {
      /*
       * This is a one-character token, make sure it is valid.
       */
      if (!GoodChar(yylval.t->tok_id))
         errt2(yylval.t, "invalid character: ", yylval.t->image);
      }

   return yylval.t->tok_id;
   }
