/*
 * NOTE: this file is generated automatically by mktoktab
 *  from tokens.txt and op.txt.
 */

/*
 * Token table - contains an entry for each token type
 * with printable name of token, token type, and flags
 * for semicolon insertion.
 */

struct toktab toktab[] = {
/*  token		token type	flags */

   /* primitives */
   "identifier",      IDENT,         Beginner+Ender,    /*   0 */
   "integer-literal", INTLIT,        Beginner+Ender,    /*   1 */
   "real-literal",    REALLIT,       Beginner+Ender,    /*   2 */
   "string-literal",  STRINGLIT,     Beginner+Ender,    /*   3 */
   "cset-literal",    CSETLIT,       Beginner+Ender,    /*   4 */
   "end-of-file",     EOFX,          0,                 /*   5 */

   /* reserved words */
   "break",           BREAK,         Beginner+Ender,    /*   6 */
   "by",              BY,            0,                 /*   7 */
   "case",            CASE,          Beginner,          /*   8 */
   "create",          CREATE,        Beginner,          /*   9 */
   "default",         DEFAULT,       Beginner,          /*  10 */
   "do",              DO,            0,                 /*  11 */
   "else",            ELSE,          0,                 /*  12 */
   "end",             END,           Beginner,          /*  13 */
   "every",           EVERY,         Beginner,          /*  14 */
   "fail",            FAIL,          Beginner+Ender,    /*  15 */
   "global",          GLOBAL,        0,                 /*  16 */
   "if",              IF,            Beginner,          /*  17 */
   "initial",         INITIAL,       Beginner,          /*  18 */
   "invocable",       INVOCABLE,     0,                 /*  19 */
   "link",            LINK,          0,                 /*  20 */
   "local",           LOCAL,         Beginner,          /*  21 */
   "next",            NEXT,          Beginner+Ender,    /*  22 */
   "not",             NOT,           Beginner,          /*  23 */
   "of",              OF,            0,                 /*  24 */
   "procedure",       PROCEDURE,     0,                 /*  25 */
   "record",          RECORD,        0,                 /*  26 */
   "repeat",          REPEAT,        Beginner,          /*  27 */
   "return",          RETURN,        Beginner+Ender,    /*  28 */
   "static",          STATIC,        Beginner,          /*  29 */
   "suspend",         SUSPEND,       Beginner+Ender,    /*  30 */
   "then",            THEN,          0,                 /*  31 */
   "to",              TO,            0,                 /*  32 */
   "until",           UNTIL,         Beginner,          /*  33 */
   "while",           WHILE,         Beginner,          /*  34 */
   "end-of-file",     0,             0,
   };

/*
 * restab[c] points to the first reserved word in toktab which
 * begins with the letter c.
 */

struct toktab *restab[] = {
   NULL,        &toktab[ 6], &toktab[ 8], &toktab[10], /* 61-64 abcd */
   &toktab[12], &toktab[15], &toktab[16], NULL,        /* 65-68 efgh */
   &toktab[17], NULL,        NULL,        &toktab[20], /* 69-6C ijkl */
   NULL,        &toktab[22], &toktab[24], &toktab[25], /* 6D-70 mnop */
   NULL,        &toktab[26], &toktab[29], &toktab[31], /* 71-74 qrst */
   &toktab[33], NULL,        &toktab[34], NULL,        /* 75-78 uvwx */
   NULL,        NULL,                                  /* 79-7A yz */
   };

/*
 * The operator table acts to extend the token table, it
 *  indicates what implementations are expected from rtt,
 *  and it has pointers for the implementation information.
 */

struct optab optab[] = {
   {{"!",      BANG,       Beginner}, Unary,          NULL, NULL}, /* 0 */
   {{"%",      MOD,        0},        Binary,         NULL, NULL}, /* 1 */
   {{"%:=",    AUGMOD,     0},        0,              NULL, NULL}, /* 2 */
   {{"&",      AND,        Beginner}, Binary,         NULL, NULL}, /* 3 */
   {{"&:=",    AUGAND,     0},        0,              NULL, NULL}, /* 4 */
   {{"*",      STAR,       Beginner}, Unary | Binary, NULL, NULL}, /* 5 */
   {{"*:=",    AUGSTAR,    0},        0,              NULL, NULL}, /* 6 */
   {{"**",     INTER,      Beginner}, Binary,         NULL, NULL}, /* 7 */
   {{"**:=",   AUGINTER,   0},        0,              NULL, NULL}, /* 8 */
   {{"+",      PLUS,       Beginner}, Unary | Binary, NULL, NULL}, /* 9 */
   {{"+:=",    AUGPLUS,    0},        0,              NULL, NULL}, /* 10 */
   {{"++",     UNION,      Beginner}, Binary,         NULL, NULL}, /* 11 */
   {{"++:=",   AUGUNION,   0},        0,              NULL, NULL}, /* 12 */
   {{"-",      MINUS,      Beginner}, Unary | Binary, NULL, NULL}, /* 13 */
   {{"-:=",    AUGMINUS,   0},        0,              NULL, NULL}, /* 14 */
   {{"--",     DIFF,       Beginner}, Binary,         NULL, NULL}, /* 15 */
   {{"--:=",   AUGDIFF,    0},        0,              NULL, NULL}, /* 16 */
   {{".",      DOT,        Beginner}, Unary,          NULL, NULL}, /* 17 */
   {{"/",      SLASH,      Beginner}, Unary | Binary, NULL, NULL}, /* 18 */
   {{"/:=",    AUGSLASH,   0},        0,              NULL, NULL}, /* 19 */
   {{":=",     ASSIGN,     0},        Binary,         NULL, NULL}, /* 20 */
   {{":=:",    SWAP,       0},        Binary,         NULL, NULL}, /* 21 */
   {{"<",      NMLT,       0},        Binary,         NULL, NULL}, /* 22 */
   {{"<:=",    AUGNMLT,    0},        0,              NULL, NULL}, /* 23 */
   {{"<-",     REVASSIGN,  0},        Binary,         NULL, NULL}, /* 24 */
   {{"<->",    REVSWAP,    0},        Binary,         NULL, NULL}, /* 25 */
   {{"<<",     SLT,        0},        Binary,         NULL, NULL}, /* 26 */
   {{"<<:=",   AUGSLT,     0},        0,              NULL, NULL}, /* 27 */
   {{"<<=",    SLE,        0},        Binary,         NULL, NULL}, /* 28 */
   {{"<<=:=",  AUGSLE,     0},        0,              NULL, NULL}, /* 29 */
   {{"<=",     NMLE,       0},        Binary,         NULL, NULL}, /* 30 */
   {{"<=:=",   AUGNMLE,    0},        0,              NULL, NULL}, /* 31 */
   {{"=",      NMEQ,       Beginner}, Unary | Binary, NULL, NULL}, /* 32 */
   {{"=:=",    AUGNMEQ,    0},        0,              NULL, NULL}, /* 33 */
   {{"==",     SEQ,        Beginner}, Binary,         NULL, NULL}, /* 34 */
   {{"==:=",   AUGSEQ,     0},        0,              NULL, NULL}, /* 35 */
   {{"===",    EQUIV,      Beginner}, Binary,         NULL, NULL}, /* 36 */
   {{"===:=",  AUGEQUIV,   0},        0,              NULL, NULL}, /* 37 */
   {{">",      NMGT,       0},        Binary,         NULL, NULL}, /* 38 */
   {{">:=",    AUGNMGT,    0},        0,              NULL, NULL}, /* 39 */
   {{">=",     NMGE,       0},        Binary,         NULL, NULL}, /* 40 */
   {{">=:=",   AUGNMGE,    0},        0,              NULL, NULL}, /* 41 */
   {{">>",     SGT,        0},        Binary,         NULL, NULL}, /* 42 */
   {{">>:=",   AUGSGT,     0},        0,              NULL, NULL}, /* 43 */
   {{">>=",    SGE,        0},        Binary,         NULL, NULL}, /* 44 */
   {{">>=:=",  AUGSGE,     0},        0,              NULL, NULL}, /* 45 */
   {{"?",      QMARK,      Beginner}, Unary,          NULL, NULL}, /* 46 */
   {{"?:=",    AUGQMARK,   0},        0,              NULL, NULL}, /* 47 */
   {{"@",      AT,         Beginner}, 0,              NULL, NULL}, /* 48 */
   {{"@:=",    AUGAT,      0},        0,              NULL, NULL}, /* 49 */
   {{"\\",     BACKSLASH,  Beginner}, Unary,          NULL, NULL}, /* 50 */
   {{"^",      CARET,      Beginner}, Unary | Binary, NULL, NULL}, /* 51 */
   {{"^:=",    AUGCARET,   0},        0,              NULL, NULL}, /* 52 */
   {{"|",      BAR,        Beginner}, 0,              NULL, NULL}, /* 53 */
   {{"||",     CONCAT,     Beginner}, Binary,         NULL, NULL}, /* 54 */
   {{"||:=",   AUGCONCAT,  0},        0,              NULL, NULL}, /* 55 */
   {{"|||",    LCONCAT,    Beginner}, Binary,         NULL, NULL}, /* 56 */
   {{"|||:=",  AUGLCONCAT, 0},        0,              NULL, NULL}, /* 57 */
   {{"~",      TILDE,      Beginner}, Unary,          NULL, NULL}, /* 58 */
   {{"~=",     NMNE,       Beginner}, Binary,         NULL, NULL}, /* 59 */
   {{"~=:=",   AUGNMNE,    0},        0,              NULL, NULL}, /* 60 */
   {{"~==",    SNE,        Beginner}, Binary,         NULL, NULL}, /* 61 */
   {{"~==:=",  AUGSNE,     0},        0,              NULL, NULL}, /* 62 */
   {{"~===",   NEQUIV,     Beginner}, Binary,         NULL, NULL}, /* 63 */
   {{"~===:=", AUGNEQUIV,  0},        0,              NULL, NULL}, /* 64 */
   {{"(",      LPAREN,     Beginner}, 0,              NULL, NULL}, /* 65 */
   {{")",      RPAREN,     Ender},    0,              NULL, NULL}, /* 66 */
   {{"+:",     PCOLON,     0},        0,              NULL, NULL}, /* 67 */
   {{",",      COMMA,      0},        0,              NULL, NULL}, /* 68 */
   {{"-:",     MCOLON,     0},        0,              NULL, NULL}, /* 69 */
   {{":",      COLON,      0},        0,              NULL, NULL}, /* 70 */
   {{";",      SEMICOL,    0},        0,              NULL, NULL}, /* 71 */
   {{"[",      LBRACK,     Beginner}, 0,              NULL, NULL}, /* 72 */
   {{"]",      RBRACK,     Ender},    0,              NULL, NULL}, /* 73 */
   {{"{",      LBRACE,     Beginner}, 0,              NULL, NULL}, /* 74 */
   {{"}",      RBRACE,     Ender},    0,              NULL, NULL}, /* 75 */
   {{"$(",     LBRACE,     Beginner}, 0,              NULL, NULL}, /* 76 */
   {{"$)",     RBRACE,     Ender},    0,              NULL, NULL}, /* 77 */
   {{"$<",     LBRACK,     Beginner}, 0,              NULL, NULL}, /* 78 */
   {{"$>",     RBRACK,     Ender},    0,              NULL, NULL}, /* 79 */
   {{NULL,          0,     0},        0,              NULL, NULL}
   };

int asgn_loc = 20;
int semicol_loc = 71;
int plus_loc = 9;
int minus_loc = 13;

/*
 * getopr - find the longest legal operator and return the
 *  index to its entry in the operator table.
 */

int getopr(ac, cc)
int ac;
int *cc;
   {
   register char c;

   *cc = ' ';
   switch (c = ac) {
      case '!':
         return 0;   /* ! */
      case '$':
         switch (c = NextChar) {
            case '(':
               return 76;   /* $( */
            case ')':
               return 77;   /* $) */
            case '<':
               return 78;   /* $< */
            case '>':
               return 79;   /* $> */
            }
         break;
      case '%':
         if ((c = NextChar) == ':') {
            if ((c = NextChar) == '=') {
               return 2;   /* %:= */
               }
            }
         else {
            *cc = c;
            return 1;   /* % */
            }
         break;
      case '&':
         if ((c = NextChar) == ':') {
            if ((c = NextChar) == '=') {
               return 4;   /* &:= */
               }
            }
         else {
            *cc = c;
            return 3;   /* & */
            }
         break;
      case '(':
         return 65;   /* ( */
      case ')':
         return 66;   /* ) */
      case '*':
         switch (c = NextChar) {
            case '*':
               if ((c = NextChar) == ':') {
                  if ((c = NextChar) == '=') {
                     return 8;   /* **:= */
                     }
                  }
               else {
                  *cc = c;
                  return 7;   /* ** */
                  }
               break;
            case ':':
               if ((c = NextChar) == '=') {
                  return 6;   /* *:= */
                  }
               break;
            default:
               *cc = c;
               return 5;   /* * */
            }
         break;
      case '+':
         switch (c = NextChar) {
            case '+':
               if ((c = NextChar) == ':') {
                  if ((c = NextChar) == '=') {
                     return 12;   /* ++:= */
                     }
                  }
               else {
                  *cc = c;
                  return 11;   /* ++ */
                  }
               break;
            case ':':
               if ((c = NextChar) == '=') {
                  return 10;   /* +:= */
                  }
               else {
                  *cc = c;
                  return 67;   /* +: */
                  }
            default:
               *cc = c;
               return 9;   /* + */
            }
         break;
      case ',':
         return 68;   /* , */
      case '-':
         switch (c = NextChar) {
            case '-':
               if ((c = NextChar) == ':') {
                  if ((c = NextChar) == '=') {
                     return 16;   /* --:= */
                     }
                  }
               else {
                  *cc = c;
                  return 15;   /* -- */
                  }
               break;
            case ':':
               if ((c = NextChar) == '=') {
                  return 14;   /* -:= */
                  }
               else {
                  *cc = c;
                  return 69;   /* -: */
                  }
            default:
               *cc = c;
               return 13;   /* - */
            }
         break;
      case '.':
         return 17;   /* . */
      case '/':
         if ((c = NextChar) == ':') {
            if ((c = NextChar) == '=') {
               return 19;   /* /:= */
               }
            }
         else {
            *cc = c;
            return 18;   /* / */
            }
         break;
      case ':':
         if ((c = NextChar) == '=') {
            if ((c = NextChar) == ':') {
               return 21;   /* :=: */
               }
            else {
               *cc = c;
               return 20;   /* := */
               }
            }
         else {
            *cc = c;
            return 70;   /* : */
            }
      case ';':
         return 71;   /* ; */
      case '<':
         switch (c = NextChar) {
            case '-':
               if ((c = NextChar) == '>') {
                  return 25;   /* <-> */
                  }
               else {
                  *cc = c;
                  return 24;   /* <- */
                  }
            case ':':
               if ((c = NextChar) == '=') {
                  return 23;   /* <:= */
                  }
               break;
            case '<':
               switch (c = NextChar) {
                  case ':':
                     if ((c = NextChar) == '=') {
                        return 27;   /* <<:= */
                        }
                     break;
                  case '=':
                     if ((c = NextChar) == ':') {
                        if ((c = NextChar) == '=') {
                           return 29;   /* <<=:= */
                           }
                        }
                     else {
                        *cc = c;
                        return 28;   /* <<= */
                        }
                     break;
                  default:
                     *cc = c;
                     return 26;   /* << */
                  }
               break;
            case '=':
               if ((c = NextChar) == ':') {
                  if ((c = NextChar) == '=') {
                     return 31;   /* <=:= */
                     }
                  }
               else {
                  *cc = c;
                  return 30;   /* <= */
                  }
               break;
            default:
               *cc = c;
               return 22;   /* < */
            }
         break;
      case '=':
         switch (c = NextChar) {
            case ':':
               if ((c = NextChar) == '=') {
                  return 33;   /* =:= */
                  }
               break;
            case '=':
               switch (c = NextChar) {
                  case ':':
                     if ((c = NextChar) == '=') {
                        return 35;   /* ==:= */
                        }
                     break;
                  case '=':
                     if ((c = NextChar) == ':') {
                        if ((c = NextChar) == '=') {
                           return 37;   /* ===:= */
                           }
                        }
                     else {
                        *cc = c;
                        return 36;   /* === */
                        }
                     break;
                  default:
                     *cc = c;
                     return 34;   /* == */
                  }
               break;
            default:
               *cc = c;
               return 32;   /* = */
            }
         break;
      case '>':
         switch (c = NextChar) {
            case ':':
               if ((c = NextChar) == '=') {
                  return 39;   /* >:= */
                  }
               break;
            case '=':
               if ((c = NextChar) == ':') {
                  if ((c = NextChar) == '=') {
                     return 41;   /* >=:= */
                     }
                  }
               else {
                  *cc = c;
                  return 40;   /* >= */
                  }
               break;
            case '>':
               switch (c = NextChar) {
                  case ':':
                     if ((c = NextChar) == '=') {
                        return 43;   /* >>:= */
                        }
                     break;
                  case '=':
                     if ((c = NextChar) == ':') {
                        if ((c = NextChar) == '=') {
                           return 45;   /* >>=:= */
                           }
                        }
                     else {
                        *cc = c;
                        return 44;   /* >>= */
                        }
                     break;
                  default:
                     *cc = c;
                     return 42;   /* >> */
                  }
               break;
            default:
               *cc = c;
               return 38;   /* > */
            }
         break;
      case '?':
         if ((c = NextChar) == ':') {
            if ((c = NextChar) == '=') {
               return 47;   /* ?:= */
               }
            }
         else {
            *cc = c;
            return 46;   /* ? */
            }
         break;
      case '@':
         if ((c = NextChar) == ':') {
            if ((c = NextChar) == '=') {
               return 49;   /* @:= */
               }
            }
         else {
            *cc = c;
            return 48;   /* @ */
            }
         break;
      case '[':
         return 72;   /* [ */
      case '\\':
         return 50;   /* \ */
      case ']':
         return 73;   /* ] */
      case '^':
         if ((c = NextChar) == ':') {
            if ((c = NextChar) == '=') {
               return 52;   /* ^:= */
               }
            }
         else {
            *cc = c;
            return 51;   /* ^ */
            }
         break;
      case '{':
         return 74;   /* { */
      case '|':
         if ((c = NextChar) == '|') {
            switch (c = NextChar) {
               case ':':
                  if ((c = NextChar) == '=') {
                     return 55;   /* ||:= */
                     }
                  break;
               case '|':
                  if ((c = NextChar) == ':') {
                     if ((c = NextChar) == '=') {
                        return 57;   /* |||:= */
                        }
                     }
                  else {
                     *cc = c;
                     return 56;   /* ||| */
                     }
                  break;
               default:
                  *cc = c;
                  return 54;   /* || */
               }
            }
         else {
            *cc = c;
            return 53;   /* | */
            }
         break;
      case '}':
         return 75;   /* } */
      case '~':
         if ((c = NextChar) == '=') {
            switch (c = NextChar) {
               case ':':
                  if ((c = NextChar) == '=') {
                     return 60;   /* ~=:= */
                     }
                  break;
               case '=':
                  switch (c = NextChar) {
                     case ':':
                        if ((c = NextChar) == '=') {
                           return 62;   /* ~==:= */
                           }
                        break;
                     case '=':
                        if ((c = NextChar) == ':') {
                           if ((c = NextChar) == '=') {
                              return 64;   /* ~===:= */
                              }
                           }
                        else {
                           *cc = c;
                           return 63;   /* ~=== */
                           }
                        break;
                     default:
                        *cc = c;
                        return 61;   /* ~== */
                     }
                  break;
               default:
                  *cc = c;
                  return 59;   /* ~= */
               }
            }
         else {
            *cc = c;
            return 58;   /* ~ */
            }
         break;
      }
   tfatal("invalid character", (char *)NULL);
   return -1;
   }
