/*
 * Routines to read a data base of run-time information.
 */
#include "../h/gsupport.h"
#include "../h/version.h"
#include "icontype.h"

/*
 * GetInt - the next thing in the data base is an integer. Get it.
 */
#define GetInt(n, c)\
   n = 0;\
   while (isdigit(c)) {\
      n = n * 10 + (c - '0');\
      c = getc(db);\
      }

/*
 * SkipWhSp - skip white space characters in the data base.
 */
#define SkipWhSp(c)\
   while (isspace(c)) {\
      if (c == '\n')\
         ++dbline;\
      c = getc(db);\
      }

/*
 * prototypes for static functions.
 */
static int            cmp_1_pre  (int p1, int p2);
static struct il_code *db_abstr  (void);
static void         db_case   (struct il_code *il, int num_cases);
static void         db_err3   (int fatal,char *s1,char *s2,char *s3);
static int             db_icntyp (void);
static struct il_c    *db_ilc    (void);
static struct il_c    *db_ilcret (int il_c_type);
static struct il_code *db_inlin  (void);
static struct il_code *db_ilvar  (void);
static int             db_rtflg  (void);
static int             db_tndtyp (void);
static struct il_c    *new_ilc   (int il_c_type);
static void          quoted   (int delim);

extern char *progname;   /* name of program using this module */

static char *dbname;           /* data base name */
static FILE *db;               /* data base  file */
static int dbline;             /* line number current position in data base */
static struct str_buf db_sbuf; /* string buffer */
static int *type_map;          /* map data base type codes to internal ones */
static int *compnt_map;        /* map data base component codes to internal */

/*
 * opendb - open data base and do other house keeping.
 */
int db_open(s, lrgintflg)
char *s;
char **lrgintflg;
   {
   char *msg_buf;
   char *id;
   int i, n;
   register int c;
   static int first_time = 1;

   if (first_time) {
      first_time = 0;
      init_sbuf(&db_sbuf);
      }
   dbname = s;
   dbline = 0;
   *lrgintflg = NULL;
   db = fopen(dbname, "rb");
   if (db == NULL)
      return 0;
   ++dbline;

   /*
    * Make sure the version number in the data base is what is expected.
    */
   s = db_string();
   if (strcmp(s, DVersion) != 0) {
      msg_buf = alloc(35 + strlen(s) + strlen(progname) + strlen(DVersion));
      sprintf(msg_buf, "found version %s, %s requires version %s",
           s, progname, DVersion);
      db_err1(1, msg_buf);
      }

   *lrgintflg = db_string();  /* large integer flag */

   /*
    * Create tables for mapping type codes and type component codes in
    *  the data base to those compiled into this program. The codes may
    *  be different if types have been added to the program since the
    *  data base was created.
    */
   type_map = alloc(num_typs * sizeof(int));
   db_chstr("", "types");   /* verify section header */
   c = getc(db);
   SkipWhSp(c)
   while (c == 'T') {
      c = getc(db);
      if (!isdigit(c))
         db_err1(1, "expected type code");
      GetInt(n, c)
      if (n >= num_typs)
         db_err1(1, "data base inconsistant with program, rebuild data base");
      SkipWhSp(c)
      if (c != ':')
         db_err1(1, "expected ':'");
      id = db_string();
      for (i = 0; strcmp(id, icontypes[i].id) != 0; ++i)
         if (i >= num_typs)
            db_err2(1, "unknown type:", id);
      type_map[n] = i;
      c = getc(db);
      SkipWhSp(c)
      }
   db_chstr("", "endsect");

   compnt_map = alloc(num_cmpnts * sizeof(int));
   db_chstr("", "components");   /* verify section header */
   c = getc(db);
   SkipWhSp(c)
   while (c == 'C') {
      c = getc(db);
      if (!isdigit(c))
         db_err1(1, "expected type component code");
      GetInt(n, c)
      if (n >= num_cmpnts)
         db_err1(1, "data base inconsistant with program, rebuild data base");
      SkipWhSp(c)
      if (c != ':')
         db_err1(1, "expected ':'");
      id = db_string();
      for (i = 0; strcmp(id, typecompnt[i].id) != 0; ++i)
         if (i >= num_cmpnts)
            db_err2(1, "unknown type component:", id);
      compnt_map[n] = i;
      c = getc(db);
      SkipWhSp(c)
      }
   db_chstr("", "endsect");

   return 1;
   }

/*
 * db_close - close data base.
 */
void db_close()
   {
   if (fclose(db) != 0)
      db_err2(0, "cannot close", dbname);
   }

/*
 * db_string - get a white-space delimited string from the data base.
 */
char *db_string()
   {
   register int c;

   /*
    * Look for the start of the string; '$' starts a special indicator.
    *  Copy characters into string buffer until white space is found.
    */
   c = getc(db);
   SkipWhSp(c);
   if (c == EOF)
      db_err1(1, "unexpected EOF");
   if (c == '$')
      return NULL;
   while (!isspace(c) && c != EOF) {
      AppChar(db_sbuf, c);
      c = getc(db);
      }
   if (c == '\n')
      ++dbline;
   return str_install(&db_sbuf); /* put string in string table */
   }

/*
 * db_impl - read basic header information for an operation into a structure
 *   and return it.
 */
struct implement *db_impl(oper_typ)
int oper_typ;
   {
   register struct implement *ip;
   register int c;
   int i;
   char *name;
   long n;

   /*
    * Get operation name.
    */
   if ((name = db_string()) == NULL)
      return NULL;

   /*
    * Create an internal structure to hold the data base entry.
    */
   ip = NewStruct(implement);
   ip->blink = NULL;
   ip->iconc_flgs = 0;         /* reserved for internal use by compiler */
   ip->oper_typ = oper_typ;
   ip->name = name;
   ip->op = NULL;

   /*
    * Get the function name prefix assigned to this operation.
    */
   c = getc(db);
   SkipWhSp(c)
   if (isalpha(c) || isdigit(c))
      ip->prefix[0] = c;
   else
     db_err2(1, "invalid prefix for", ip->name);
   c = getc(db);
   if (isalpha(c) || isdigit(c))
      ip->prefix[1] = c;
   else
     db_err2(1, "invalid prefix for", ip->name);

   /*
    * Get the number of parameters.
    */
   c = getc(db);
   SkipWhSp(c)
   if (!isdigit(c))
     db_err2(1, "number of parameters missing for", ip->name);
   GetInt(n, c)
   ip->nargs = n;

   /*
    * Get the flags that indicate whether each parameter requires a dereferenced
    *  and/or undereferenced value, and whether the last parameter represents
    *  the end of a varargs list. Store the flags in an array.
    */
   if (n == 0)
      ip->arg_flgs = NULL;
   else
      ip->arg_flgs = alloc(n * sizeof(int));
   if (c != '(')
      db_err2(1, "parameter flags missing for", ip->name);
   c = getc(db);
   for (i = 0; i < n; ++i) {
      if (c == ',' || c == ')')
         db_err2(1, "parameter flag missing for", ip->name);
      ip->arg_flgs[i] = 0;
      while (c != ',' && c != ')') {
          switch (c) {
             case 'u':
                ip->arg_flgs[i] |= RtParm;
                break;
             case 'd':
                ip->arg_flgs[i] |= DrfPrm;
                break;
             case 'v':
                ip->arg_flgs[i] |= VarPrm;
                break;
             default:
                db_err2(1, "invalid parameter flag for", ip->name);
             }
         c = getc(db);
         }
      if (c == ',')
         c = getc(db);
      }
   if (c != ')')
     db_err2(1, "invalid parameter flag list for", ip->name);

   /*
    * Get the result sequence indicator for the operation.
    */
   c = getc(db);
   SkipWhSp(c)
   if (c != '{')
     db_err2(1, "result sequence missing for", ip->name);
   c = getc(db);
   ip->resume = 0;
   if (c == '}') {
      ip->min_result = NoRsltSeq;
      ip->max_result = NoRsltSeq;
      }
   else {
      if (!isdigit(c))
        db_err2(1, "invalid result sequence for", ip->name);
      GetInt(n, c)
      ip->min_result = n;
      if (c != ',')
        db_err2(1, "invalid result sequence for", ip->name);
      c = getc(db);
      if (c == '*') {
         ip->max_result = UnbndSeq;
         c = getc(db);
         }
      else if (isdigit(c)) {
         GetInt(n, c)
         ip->max_result = n;
         }
      else
        db_err2(1, "invalid result sequence for", ip->name);
      if (c == '+') {
         ip->resume = 1;
         c = getc(db);
         }
      if (c != '}')
        db_err2(1, "invalid result sequence for", ip->name);
      }

   /*
    * Get the flag indicating whether the operation contains returns, fails,
    *  or suspends.
    */
   ip->ret_flag = db_rtflg();

   /*
    * Get the t/f flag that indicates whether the operation explicitly
    *  uses the 'result' location.
    */
   c = getc(db);
   SkipWhSp(c)
   switch (c) {
      case 't':
         ip->use_rslt = 1;
         break;
      case 'f':
         ip->use_rslt = 0;
         break;
      default:
         db_err2(1, "invalid 'result' use indicator for", ip->name);
         }
   return ip;
   }

/*
 * db_code - read the RTL code for the body of an operation.
 */
void db_code(ip)
struct implement *ip;
   {
   register int c;
   char *s;
   word n;
   int var_type;
   int i;

   /*
    * read the descriptive string.
    */
   c = getc(db);
   SkipWhSp(c)
   if (c != '"')
      db_err1(1, "operation description expected");
   for (c = getc(db); c != '"' && c != '\n' && c != EOF; c = getc(db)) {
      if (c == '\\') {
         AppChar(db_sbuf, c);
         c = getc(db);
         }
      AppChar(db_sbuf, c);
      }
   if (c != '"')
      db_err1(1, "expected '\"'");
   ip->comment = str_install(&db_sbuf);

   /*
    * Get the number of tended variables in the declare clause.
    */
   c = getc(db);
   SkipWhSp(c)
   GetInt(n, c)
   ip->ntnds = n;

   /*
    * Read information about the tended variables into an array.
    */
   if (n == 0)
      ip->tnds = NULL;
   else
      ip->tnds = alloc(n * sizeof(struct tend_var));
   for (i = 0; i < n; ++i) {
      var_type = db_tndtyp();  /* type of tended declaration */
      ip->tnds[i].var_type = var_type;
      ip->tnds[i].blk_name = NULL;
      if (var_type == TndBlk) {
         /*
          * Tended block pointer declarations include a block type or '*' to
          *  indicate 'union block *'.
          */
         s = db_string();
         if (s == NULL)
            db_err1(1, "block name expected");
         if (*s != '*')
            ip->tnds[i].blk_name = s;
         }
      ip->tnds[i].init = db_ilc();  /* C code for declaration initializer */
      }

   /*
    * Get the number of non-tended variables in the declare clause.
    */
   c = getc(db);
   SkipWhSp(c)
   GetInt(n, c)
   ip->nvars = n;

   /*
    * Get each non-tended declaration and store it in an array.
    */
   if (n == 0)
      ip->vars = NULL;
   else
      ip->vars = alloc(n * sizeof(struct ord_var));
   for (i = 0; i < n; ++i) {
      s = db_string();             /* variable name */
      if (s == NULL)
         db_err1(1, "variable name expected");
      ip->vars[i].name = s;
      ip->vars[i].dcl = db_ilc();  /* full declaration including name */
      }

   /*
    * Get the executable RTL code.
    */
   ip->in_line = db_inlin();

   /*
    * We should be at the end of the operation.
    */
   c = getc(db);
   SkipWhSp(c)
   if (c != '$')
      db_err1(1, "expected $end");
   }

/*
 * db_inlin - read in the in-line code (executable RTL code) for an operation.
 */
static struct il_code *db_inlin()
   {
   struct il_code *il = NULL;
   register int c;
   int i;
   int indx;
   int fall_thru;
   int n, n1;

   /*
    * The following nested switch statements act as a trie for recognizing
    *  the prefix form of RTL code in the data base.
    */
   c = getc(db);
   SkipWhSp(c)
   switch (c) {
      case 'a':
         switch (getc(db)) {
            case 'b': {
               db_chstr("ab", "str");
               il = new_il(IL_Abstr, 2);        /* abstract type computation */
               il->u[0].fld = db_abstr();       /* side effects */
               il->u[1].fld = db_abstr();       /* return type */
               break;
               }
            case 'c': {
               db_chstr("ac", "ase");
               il = new_il(IL_Acase, 5);        /* arith_case */
               il->u[0].fld = db_ilvar();       /* first variable */
               il->u[1].fld = db_ilvar();       /* second variable */
               il->u[2].fld = db_inlin();       /* C_integer action */
               il->u[3].fld = db_inlin();       /* integer action */
               il->u[4].fld = db_inlin();       /* C_double action */
               break;
               }
            default:
               db_err1(1, "expected abstr or acase");
            }
         break;

      case 'b':
         db_chstr("b", "lock");
         c = getc(db);
         SkipWhSp(c)
         if (c == 't')
            fall_thru = 1;
         else
            fall_thru = 0;
         c = getc(db);
         SkipWhSp(c)
         GetInt(n, c)
         il = new_il(IL_Block, 3 + n);    /* block of in-line C code */
         il->u[0].n = fall_thru;
         il->u[1].n = n;                  /* number of local tended */
         for (i = 2; i - 2 < n; ++i)
             il->u[i].n = db_tndtyp();    /* tended declaration */
         il->u[i].c_cd = db_ilc();        /* C code */
         break;

      case 'c':
         switch (getc(db)) {
            case 'a': {
               char prfx3;
               int ret_val = 0;
               int ret_flag;
               int rslt = 0;
               int num_sbuf;
               int num_cbuf;

               db_chstr("ca", "ll");
               /*
                * Call to body function. Get the letter used as the 3rd
                *  character of the function prefix.
                */
               c = getc(db);
               SkipWhSp(c)
               prfx3 = c;

               /*
                * Determine what the body function returns directly.
                */
               c = getc(db);
               SkipWhSp(c)
               switch (c) {
                  case 'i':
                     ret_val = RetInt;    /* returns C integer */
                     break;
                  case 'd':
                     ret_val = RetDbl;    /* returns C double */
                     break;
                  case 'n':
                     ret_val = RetNoVal;  /* returns nothing directly */
                     break;
                  case 's':
                     ret_val = RetSig;    /* returns a signal */
                     break;
                  default:
                     db_err1(1, "invalid indicator for type of return value");
                  }

              /*
               * Get the return/suspend/fail/fall-through flag.
               */
               c = getc(db);
               ret_flag = db_rtflg();

               /*
                * Get the flag indicating whether the body function expects
                *  to have an explicit result location passed to it.
                */
               c = getc(db);
               SkipWhSp(c)
               switch (c) {
                  case 't':
                     rslt = 1;
                     break;
                  case 'f':
                     rslt = 0;
                     break;
                  default:
                     db_err1(1, "t or f expected");
                  }

               c = getc(db);
               SkipWhSp(c)
               GetInt(num_sbuf, c)  /* number of cset buffers */
               c = getc(db);
               SkipWhSp(c)
               GetInt(num_cbuf, c)  /* number of string buffers */
               c = getc(db);
               SkipWhSp(c)
               GetInt(n, c)         /* num args */

               il = new_il(IL_Call, 8 + n * 2);
               il->u[0].n = 0;      /* reserved for internal use by compiler */
               il->u[1].n = prfx3;
               il->u[2].n = ret_val;
               il->u[3].n = ret_flag;
               il->u[4].n = rslt;
               il->u[5].n = num_sbuf;
               il->u[6].n = num_cbuf;
               il->u[7].n = n;
               indx = 8;

               /*
                * get the prototype parameter declarations and actual arguments.
                */
               n *= 2;
               while (n--)
                  il->u[indx++].c_cd = db_ilc();
               }
               break;

            case 'n':
               if (getc(db) != 'v')
                  db_err1(1, "expected cnv1 or cnv2");
               switch (getc(db)) {
                  case '1':
                     il = new_il(IL_Cnv1, 2);
                     il->u[0].n = db_icntyp();      /* type code */
                     il->u[1].fld = db_ilvar();     /* source */
                     break;
                  case '2':
                     il = new_il(IL_Cnv2, 3);
                     il->u[0].n = db_icntyp();      /* type code */
                     il->u[1].fld = db_ilvar();     /* source */
                     il->u[2].c_cd = db_ilc();      /* destination */
                     break;
                  default:
                     db_err1(1, "expected cnv1 or cnv2");
                  }
               break;

            case 'o':
               db_chstr("co", "nst");
               il = new_il(IL_Const, 2);     /* constant keyword */
               il->u[0].n = db_icntyp();     /* type code */
               c = getc(db);
               SkipWhSp(c)
               if (c == '"' || c == '\'') {
                  quoted(c);
                  c = getc(db);              /* quoted literal without quotes */
                  }
               else
                  while (c != EOF && !isspace(c)) {
                     AppChar(db_sbuf, c);
                     c = getc(db);
                     }
               il->u[1].s = str_install(&db_sbuf); /* non-quoted values */
               break;

            default:
               db_err1(1, "expected call, const, cnv1, or cnv2");
            }
         break;

      case 'd':
         if (getc(db) != 'e' || getc(db) != 'f')
            db_err1(1, "expected def1 or def2");
         switch (getc(db)) {
            case '1':
               il = new_il(IL_Def1, 3);       /* defaulting, no dest. field */
               il->u[0].n = db_icntyp();      /* type code */
               il->u[1].fld = db_ilvar();     /* source */
               il->u[2].c_cd = db_ilc();      /* default value */
               break;
            case '2':
               il = new_il(IL_Def2, 4);       /* defaulting, with dest. field */
               il->u[0].n = db_icntyp();      /* type code */
               il->u[1].fld = db_ilvar();     /* source */
               il->u[2].c_cd = db_ilc();      /* default value */
               il->u[3].c_cd = db_ilc();      /* destination */
               break;
            default:
               db_err1(1, "expected dflt1 or dflt2");
            }
         break;

      case 'r':
         if (getc(db) != 'u' || getc(db) != 'n' || getc(db) != 'e' ||
            getc(db) != 'r' || getc(db) != 'r')
            db_err1(1, "expected runerr1 or runerr2");
         switch (getc(db)) {
            case '1':
               il = new_il(IL_Err1, 1);       /* runerr, no offending value */
               c = getc(db);
               SkipWhSp(c)
               GetInt(n, c)
               il->u[0].n = n;                /* error number */
               break;
            case '2':
               il = new_il(IL_Err2, 2);       /* runerr, with offending value */
               c = getc(db);
               SkipWhSp(c)
               GetInt(n, c)
               il->u[0].n = n;                /* error number */
               il->u[1].fld = db_ilvar();     /* variable */
               break;
            default:
               db_err1(1, "expected runerr1 or runerr2");
            }
         break;

      case 'i':
         switch (getc(db)) {
            case 'f':
               switch (getc(db)) {
                  case '1':
                     il = new_il(IL_If1, 2);    /* if-then */
                     il->u[0].fld = db_inlin(); /* condition */
                     il->u[1].fld = db_inlin(); /* then clause */
                     break;
                  case '2':
                     il = new_il(IL_If2, 3);     /* if-then-else */
                     il->u[0].fld = db_inlin(); /* condition */
                     il->u[1].fld = db_inlin(); /* then clause */
                     il->u[2].fld = db_inlin(); /* else clause */
                     break;
                  default:
                     db_err1(1, "expected if1 or if2");
                  }
               break;
            case 's':
               il = new_il(IL_Is, 2);         /* type check */
               il->u[0].n = db_icntyp();      /* type code */
               il->u[1].fld = db_ilvar();     /* variable */
               break;
            default:
               db_err1(1, "expected if1, if2, or is");
            }
         break;

      case 'l':
         switch (getc(db)) {
            case 'c':
               db_chstr("lc", "ase");
               c = getc(db);
               SkipWhSp(c)
               GetInt(n, c)
               il = new_il(IL_Lcase, 2 + 2 * n); /* length case */
               il->u[0].n = n;                   /* number of cases */
               indx = 1;
               while (n--) {
                  c = getc(db);
                  SkipWhSp(c)
                  GetInt(n1, c)
                  il->u[indx++].n = n1;           /* selection number */
                  il->u[indx++].fld = db_inlin(); /* action */
                  }
               il->u[indx].fld = db_inlin();      /* default */
               break;

            case 's':
               if (getc(db) != 't')
                  db_err1(1, "expected lst");
               il = new_il(IL_Lst, 2);            /* sequence of code parts */
               il->u[0].fld = db_inlin();         /* 1st part */
               il->u[1].fld = db_inlin();         /* 2nd part */
               break;

            default:
               db_err1(1, "expected lcase or lst");
            }
         break;

      case 'n':
         db_chstr("n", "il");
         il = NULL;
         break;

      case 't': {
         struct il_code *var;

         if (getc(db) != 'c' || getc(db) != 'a' || getc(db) != 's' ||
            getc(db) != 'e')
               db_err1(1, "expected tcase1 or tcase2");
         switch (getc(db)) {
            case '1':
               var = db_ilvar();
               c = getc(db);
               SkipWhSp(c)
               GetInt(n, c)
               il = new_il(IL_Tcase1, 3 * n + 2); /* type case, no default */
               il->u[0].fld = var;                /* variable */
               db_case(il, n);                    /* get cases */
               break;

            case '2':
               var = db_ilvar();
               c = getc(db);
               SkipWhSp(c)
               GetInt(n, c)
               il = new_il(IL_Tcase2, 3 * n + 3);  /* type case, with default */
               il->u[0].fld = var;                 /* variable */
               db_case(il, n);                     /* get cases */
               il->u[3 * n + 2].fld = db_inlin();  /* default */
               break;

            default:
               db_err1(1, "expected tcase1 or tcase2");
            }
         }
         break;

      case '!':
         il = new_il(IL_Bang, 1);                   /* negated condition */
         il->u[0].fld = db_inlin();                 /* condition */
         break;

      case '&':
         if (getc(db) != '&')
            db_err1(1, "expected &&");
         il = new_il(IL_And, 2);                    /* && (conjunction) */
         il->u[0].fld = db_inlin();                 /* 1st operand */
         il->u[1].fld = db_inlin();                 /* 2nd operand */
         break;

      default:
         db_err1(1, "syntax error");
      }
   return il;
   }

/*
 * db_rtflg - get the sequence of 4 [or 5] flags that indicate whether code
 *  for a operation [or body function] returns, fails, suspends, has error
 *  failure, [or execution falls through the code].
 */
static int db_rtflg()
   {
   register int c;
   int ret_flag;

   /*
    * The presence of each flag is indicated by a unique character. Its absence
    *  indicated by '_'.
    */
   ret_flag = 0;
   c = getc(db);
   SkipWhSp(c)
   if (c == 'f')
      ret_flag |= DoesFail;
   else if (c != '_')
     db_err1(1, "invalid return indicator");
   c = getc(db);
   if (c == 'r')
      ret_flag |= DoesRet;
   else if (c != '_')
     db_err1(1, "invalid return indicator");
   c = getc(db);
   if (c == 's')
      ret_flag |= DoesSusp;
   else if (c != '_')
     db_err1(1, "invalid return indicator");
   c = getc(db);
   if (c == 'e')
      ret_flag |= DoesEFail;
   else if (c != '_')
     db_err1(1, "invalid return indicator");
   c = getc(db);
   if (c == 't')
      ret_flag |= DoesFThru;
   else if (c != '_' && c != ' ')
     db_err1(1, "invalid return indicator");
   return ret_flag;
   }

/*
 * db_case - get the cases for a type_case statement from the data base.
 */
static void db_case(il, num_cases)
struct il_code *il;
int num_cases;
   {
   register int c;
   int *typ_vect;
   int i, j;
   int num_types;
   int indx;

   il->u[1].n = num_cases;    /* number of cases */
   indx = 2;
   for (i = 0; i < num_cases; ++i) {
      /*
       * Determine the number of types in this case then store the
       *  type codes in an array.
       */
      c = getc(db);
      SkipWhSp(c)
      GetInt(num_types, c)
      il->u[indx++].n = num_types;
      typ_vect = alloc(num_types * sizeof(int));
      il->u[indx++].vect = typ_vect;
      for (j = 0; j < num_types; ++j)
         typ_vect[j] = db_icntyp();           /* type code */

      il->u[indx++].fld = db_inlin();         /* action */
      }
   }

/*
 * db_ilvar - get a symbol table index for a simple variable or a
 *  subscripted variable from the data base.
 */
static struct il_code *db_ilvar()
   {
   struct il_code *il;
   register int c;
   int n;

   c = getc(db);
   SkipWhSp(c)

   if (isdigit(c)) {
      /*
       * Simple variable: just a symbol table index.
       */
      il = new_il(IL_Var, 1);
      GetInt(n, c)
      il->u[0].n = n;    /* symbol table index */
      }
   else {
      if (c != '[')
         db_err1(1, "expected symbol table index or '['");
      /*
       * Subscripted variable: symbol table index and subscript.
       */
      il = new_il(IL_Subscr, 2);
      c = getc(db);
      SkipWhSp(c);
      GetInt(n, c)
      il->u[0].n = n;    /* symbol table index */
      c = getc(db);
      SkipWhSp(c)
      GetInt(n, c)
      il->u[1].n = n;    /* subscripting index */
      }
   return il;
   }

/*
 * db_abstr - get abstract type computations from the data base.
 */
static struct il_code *db_abstr()
   {
   struct il_code *il = NULL;
   register int c;
   word typcd;
   word indx;
   int n;
   int nargs;

   c = getc(db);
   SkipWhSp(c)
   switch (c) {
      case 'l':
         db_chstr("l", "st");
         il = new_il(IL_Lst, 2);        /* sequence of code parts */
         il->u[0].fld = db_abstr();     /* 1st part */
         il->u[1].fld = db_abstr();     /* 2nd part */
         break;

      case 'n':
         switch (getc(db)) {
            case 'e':
               if (getc(db) != 'w')
                  db_err1(1, "expected new");
               typcd = db_icntyp();
               c = getc(db);
               SkipWhSp(c)
               GetInt(nargs, c)
               il = new_il(IL_New, 2 + nargs);  /* new structure create here */
               il->u[0].n = typcd;              /* type code */
               il->u[1].n = nargs;              /* number of args */
               indx = 2;
               while (nargs--)
                  il->u[indx++].fld = db_abstr(); /* argument for component */
               break;
            case 'i':
               if (getc(db) != 'l')
                  db_err1(1, "expected nil");
               il = NULL;
               break;
            default:
               db_err1(1, "expected new or nil");
            }
       break;

      case 's':
         db_chstr("s", "tore");
         il = new_il(IL_Store, 1);  /* abstract store */
         il->u[0].fld = db_abstr(); /* type to "dereference" */
         break;

      case 't':
         db_chstr("t", "yp");
         il = new_il(IL_IcnTyp, 1);  /* explicit type */
         il->u[0].n = db_icntyp();   /* type code */
         break;

      case 'v':
         db_chstr("v", "artyp");
         il = new_il(IL_VarTyp, 1);        /* variable */
         il->u[0].fld = db_ilvar();        /* symbol table index, etc */
         break;

      case '.':
         il = new_il(IL_Compnt, 2);        /* component access */
         il->u[0].fld = db_abstr();        /* type being accessed */
         c = getc(db);
         SkipWhSp(c)
         switch (c) {
            case 'f':
               il->u[1].n = CM_Fields;
               break;
            case 'C':
               c = getc(db);
               GetInt(n, c)
               il->u[1].n = compnt_map[n];
               break;
            default:
               db_err1(1, "expected component code");
            }
         break;

      case '=':
         il = new_il(IL_TpAsgn, 2);        /* assignment (side effect) */
         il->u[0].fld = db_abstr();        /* left-hand-side */
         il->u[1].fld = db_abstr();        /* right-hand-side */
         break;

      case '+':
         if (getc(db) != '+')
            db_err1(1, "expected ++");
         il = new_il(IL_Union, 2);         /* ++ (union) */
         il->u[0].fld = db_abstr();        /* 1st operand */
         il->u[1].fld = db_abstr();        /* 2nd operand */
         break;

      case '*':
         if (getc(db) != '*')
            db_err1(1, "expected **");
         il = new_il(IL_Inter, 2);         /* ** (intersection) */
         il->u[0].fld = db_abstr();        /* 1st operand */
         il->u[1].fld = db_abstr();        /* 2nd operand */
         break;
      }
   return il;
   }

/*
 * db_ilc - read a piece of in-line C code.
 */
static struct il_c *db_ilc()
   {
   register int c;
   int old_c;
   word n;
   struct il_c *base = NULL;
   struct il_c **nxtp = &base;

   c = getc(db);
   SkipWhSp(c)
   switch (c) {
      case '$':
         /*
          * This had better be the starting $c.
          */
         c = getc(db);
         if (c == 'c') {
            c = getc(db);
            for (;;) {
               SkipWhSp(c)
               if (c == '$') {
                  c = getc(db);
                  switch (c) {
                     case 'c':             /* $cb or $cgoto <cond> <lbl num> */
                        c = getc(db);
                        switch (c) {
                           case 'b':
                              *nxtp = new_ilc(ILC_CBuf);
                              c = getc(db);
                              break;
                           case 'g':
                              db_chstr("$cg", "oto");
                              *nxtp = new_ilc(ILC_CGto);
#ifdef MultiThread
   #undef code
#endif					/* MultiThead */
                              (*nxtp)->code[0] = db_ilc();
                              c = getc(db);
                              SkipWhSp(c);
                              if (!isdigit(c))
                                 db_err1(1, "$cgoto: expected label number");
                              GetInt(n, c);
                              (*nxtp)->n = n;
                              break;
                           default:
                             db_err1(1, "expected $cb or $cgoto");
                           }
                        break;
                     case 'e':
                        c = getc(db);
                        if (c == 'f') {             /* $efail */
                            db_chstr("$ef", "ail");
                            *nxtp = new_ilc(ILC_EFail);
                            c = getc(db);
                            break;
                            }
                        else
                           return base;            /* $e */
                     case 'f':                     /* $fail */
                        db_chstr("$f", "ail");
                        *nxtp = new_ilc(ILC_Fail);
                        c = getc(db);
                        break;
                     case 'g':                     /* $goto <lbl num> */
                        db_chstr("$g", "oto");
                        *nxtp = new_ilc(ILC_Goto);
                        c = getc(db);
                        SkipWhSp(c);
                        if (!isdigit(c))
                           db_err1(1, "$goto: expected label number");
                        GetInt(n, c);
                        (*nxtp)->n = n;
                        break;
                     case 'l':                     /* $lbl <lbl num> */
                        db_chstr("$l", "bl");
                        *nxtp = new_ilc(ILC_Lbl);
                        c = getc(db);
                        SkipWhSp(c);
                        if (!isdigit(c))
                           db_err1(1, "$lbl: expected label number");
                        GetInt(n, c);
                        (*nxtp)->n = n;
                        break;
                     case 'm':                     /* $m[d]<indx> */
                        *nxtp = new_ilc(ILC_Mod);
                        c = getc(db);
                        if (c == 'd') {
                           (*nxtp)->s = "d";
                           c = getc(db);
                           }
                        if (isdigit(c)) {
                           GetInt(n, c);
                           (*nxtp)->n = n;
                           }
                        else if (c == 'r') {
                           (*nxtp)->n = RsltIndx;
                           c = getc(db);
                           }
                        else
                           db_err1(1, "$m: expected symbol table index");
                        break;
                     case 'r':                     /* $r[d]<indx> or $ret ... */
                        c = getc(db);
                        if (isdigit(c) || c == 'd') {
                           *nxtp = new_ilc(ILC_Ref);
                           if (c == 'd') {
                              (*nxtp)->s = "d";
                              c = getc(db);
                              }
                           GetInt(n, c);
                           (*nxtp)->n = n;
                           }
                        else if (c == 'r') {
                           *nxtp = new_ilc(ILC_Ref);
                           (*nxtp)->n = RsltIndx;
                           c = getc(db);
                           }
                        else {
                           if (c != 'e' || getc(db) != 't')
                              db_err1(1, "expected $ret");
                           *nxtp = db_ilcret(ILC_Ret);
                           c = getc(db);
                           }
                        break;
                     case 's':                     /* $sb or $susp ... */
                        c = getc(db);
                        switch (c) {
                           case 'b':
                              *nxtp = new_ilc(ILC_SBuf);
                              c = getc(db);
                              break;
                           case 'u':
                              db_chstr("$su", "sp");
                              *nxtp = db_ilcret(ILC_Susp);
                              c = getc(db);
                              break;
                           default:
                             db_err1(1, "expected $sb or $susp");
                           }
                        break;
                     case 't':                     /* $t[d]<indx> */
                        *nxtp = new_ilc(ILC_Tend);
                        c = getc(db);
                        if (!isdigit(c))
                           db_err1(1, "$t: expected index");
                        GetInt(n, c);
                        (*nxtp)->n = n;
                        break;
                     case '{':
                        *nxtp = new_ilc(ILC_LBrc);
                        c = getc(db);
                        break;
                     case '}':
                        *nxtp = new_ilc(ILC_RBrc);
                        c = getc(db);
                        break;
                     default:
                        db_err1(1, "invalid $ escape in C code");
                     }
                  }
               else {
                  /*
                   * Arbitrary code - gather into a string.
                   */
                  while (c != '$') {
                     if (c == '"' || c == '\'') {
                        quoted(c);
                        c = getc(db);
                        }
                     if (c == '\n')
                        ++dbline;
                     if (c == EOF)
                        db_err1(1, "unexpected EOF in C code");
                     old_c = c;
                     AppChar(db_sbuf, c);
                     c = getc(db);
                     if (old_c == ' ')
                        while (c == ' ')
                           c = getc(db);
                     }
                  *nxtp = new_ilc(ILC_Str);
                  (*nxtp)->s = str_install(&db_sbuf);
                  }
               nxtp = &(*nxtp)->next;
               }
            }
         break;
      case 'n':
         db_chstr("n", "il");
         return NULL;
      }
   db_err1(1, "expected C code of the form $c ... $e or nil");
   /*NOTREACHED*/
   return 0;	/* avoid gcc warning */
   }

/*
 * quoted - get the string for a quoted literal. The first quote mark
 *  has been read.
 */
static void quoted(delim)
int delim;
   {
   register int c;

   AppChar(db_sbuf, delim);
   c = getc(db);
   while (c != delim && c != EOF) {
      if (c == '\\') {
         AppChar(db_sbuf, c);
         c = getc(db);
         if (c == EOF)
            db_err1(1, "unexpected EOF in quoted literal");
         }
      AppChar(db_sbuf, c);
      c = getc(db);
      }
   if (c == EOF)
      db_err1(1, "unexpected EOF in quoted literal");
   AppChar(db_sbuf, c);
   }

/*
 * db_ilcret - get the in-line C code on a return or suspend statement.
 */
static struct il_c *db_ilcret(il_c_type)
int il_c_type;
   {
   struct il_c *ilc;
   int c;
   int n;
   int i;

   ilc = new_ilc(il_c_type);
   ilc->n = db_icntyp();       /* kind of return expression */
   c = getc(db);
   SkipWhSp(c)
   GetInt(n, c)                /* number of arguments in this expression */
   for (i = 0; i < n; ++i)
      ilc->code[i] = db_ilc(); /* an argument to the return expression */
   return ilc;
   }

/*
 * db_tndtyp - get the indication for the type of a tended declaration.
 */
static int db_tndtyp()
   {
   int c;

   c = getc(db);
   SkipWhSp(c)
   switch (c) {
      case 'b':
         db_chstr("b", "lkptr");
         return TndBlk;          /* tended block pointer */
      case 'd':
         db_chstr("d", "esc");
         return TndDesc;         /* tended descriptor */
      case 's':
         db_chstr("s", "tr");
         return TndStr;          /* tended string */
      default:
         db_err1(1, "expected blkptr, desc, or str");
         /* NOTREACHED */
      }
   /* NOTREACHED */
   return 0;	/* avoid gcc warning */
   }

/*
 * db_icntyp - get a type code from the data base.
 */
static int db_icntyp()
   {
   int c;
   int n;

   c = getc(db);
   SkipWhSp(c)
   switch (c) {
      case 'T':
         c = getc(db);
         GetInt(n, c)
         if (n < num_typs)
            return type_map[n];       /* type code from specification system */
         break;
      case 'a':
         return TypAny;               /* a - any type */
      case 'c':
         switch (getc(db)) {
            case 'i':
               return TypCInt;        /* ci - C integer */
            case 'd':
               return TypCDbl;        /* cd - C double */
            case 's':
               return TypCStr;        /* cs - C string */
            }
         break;
      case 'd':
         return RetDesc;              /* d - descriptor on return statement */
      case 'e':
         switch (getc(db)) {
            case 'c':
               if (getc(db) == 'i')
                  return TypECInt;    /* eci - exact C integer */
               break;
            case 'i':
               return TypEInt;        /* ei - exact integer */
            case ' ':
            case '\n':
            case '\t':
                return TypEmpty;      /* e - empty  type */
            }
         break;
      case 'n':
         if (getc(db) == 'v')
            return RetNVar;           /* nv - named variable on return */
         break;
      case 'r':
         if (getc(db) == 'n')
            return RetNone;           /* rn - nothing explicitly returned */
         break;
      case 's':
         if (getc(db) == 'v')
            return RetSVar;           /* sv - structure variable on return */
         break;
      case 't':
         switch (getc(db)) {
            case 'c':
               return TypTCset;       /* tc - temporary cset */
            case 's':
               return TypTStr;        /* ts - temporary string */
            }
         break;
      case 'v':
         return TypVar;               /* v - variable */
      }
   db_err1(1, "invalid type code");
   /* NOTREACHED */
   return 0;	/* avoid gcc warning */
   }

/*
 * new_ilc - allocate a new structure to hold a piece of in-line C code.
 */
static struct il_c *new_ilc(il_c_type)
int il_c_type;
   {
   struct il_c *ilc;
   int i;

   ilc = NewStruct(il_c);
   ilc->next = NULL;
   ilc->il_c_type = il_c_type;
   for (i = 0; i < 3; ++i)
      ilc->code[i] = NULL;
   ilc->n = 0;
   ilc->s = NULL;
   return ilc;
   }

/*
 * new_il - allocate a new structure with "size" fields to hold a piece of
 *   RTL code.
 */
struct il_code *new_il(il_type, size)
int il_type;
int size;
   {
   struct il_code *il;

   il = alloc(sizeof(struct il_code) + (size-1) * sizeof(union il_fld));
   il->il_type = il_type;
   return il;
   }

/*
 * db_dscrd - discard an implementation up to $end, skipping the in-line
 *   RTL code.
 */
void db_dscrd(ip)
struct implement *ip;
   {
   char state;  /* how far along we are at recognizing $end */

   free(ip);
   state = '\0';
   for (;;) {
      switch (getc(db)) {
         case '$':
            state = '$';
            continue;
         case 'e':
            if (state == '$') {
               state = 'e';
               continue;
               }
            break;
         case 'n':
            if (state == 'e') {
               state = 'n';
               continue;
               }
            break;
         case 'd':
            if (state == 'n')
               return;
            break;
         case '\n':
            ++dbline;
            break;
         case EOF:
            db_err1(1, "unexpected EOF");
         }
      state = '\0';
      }
   }

/*
 * db_chstr - we are expecting a specific string. We may already have
 *   read a prefix of it.
 */
void db_chstr(prefix, suffix)
char *prefix;
char *suffix;
   {
   int c;

   c = getc(db);
   SkipWhSp(c)

   for (;;) {
      if (*suffix == '\0' && (isspace(c) || c == EOF)) {
         if (c == '\n')
            ++dbline;
         return;
         }
      else if (*suffix != c)
         break;
      c = getc(db);
      ++suffix;
      }
   db_err3(1, "expected:", prefix, suffix);
   }

/*
 * db_tbl - fill in a hash table of implementation information for the
 *  given section.
 */
int db_tbl(section, tbl)
char *section;
struct implement **tbl;
   {
   struct implement *ip;
   int num_added = 0;
   unsigned hashval;

   /*
    * Get past the section header.
    */
   db_chstr("", section);

   /*
    * Create an entry in the hash table for each entry in the data base.
    *  If multiple data bases are loaded into one hash table, use the
    *  first entry encountered for each operation.
    */
   while ((ip = db_impl(toupper(section[0]))) != NULL) {
      if (db_ilkup(ip->name, tbl) == NULL) {
         db_code(ip);
         hashval = IHasher(ip->name);
         ip->blink = tbl[hashval];
         tbl[hashval] = ip;
         ++num_added;
         db_chstr("", "end");
         }
      else
         db_dscrd(ip);
      }
   db_chstr("", "endsect");
   return num_added;
   }

/*
 * db_ilkup - look up id in a table of implementation information and return
 *  pointer it or NULL if it is not there.
 */
struct implement *db_ilkup(id, tbl)
char *id;
struct implement **tbl;
   {
   register struct implement *ptr;

   ptr = tbl[IHasher(id)];
   while (ptr != NULL && ptr->name != id)
      ptr = ptr->blink;
   return ptr;
   }

/*
 * nxt_pre - assign next prefix. A prefix consists of n characters each from
 *   the range 0-9 and a-z, at least one of which is a digit.
 *
 */
void nxt_pre(pre, nxt, n)
char *pre;
char *nxt;
int n;
   {
   int i, num_dig;

   if (nxt[0] == '\0') {
      fprintf(stderr, "out of unique prefixes\n");
      exit(EXIT_FAILURE);
      }

   /*
    * copy the next prefix into the output string.
    */
   for (i = 0; i < n; ++i)
      pre[i] = nxt[i];

   /*
    * Increment next prefix. First, determine how many digits there are in
    *  the current prefix.
    */
   num_dig = 0;
   for (i = 0; i < n; ++i)
      if (isdigit(nxt[i]))
         ++num_dig;

   for (i = n - 1; i >= 0; --i) {
      switch (nxt[i]) {
         case '9':
            /*
             * If there is at least one other digit, increment to a letter.
             *  Otherwise, start over at zero and continue to the previous
             *  character in the prefix.
             */
            if (num_dig > 1) {
               nxt[i] = 'a';
               return;
               }
            else
               nxt[i] = '0';
            break;

         case 'z':
            /*
             * Start over at zero and continue to previous character in the
             *  prefix.
             */
            nxt[i] = '0';
            ++num_dig;
            break;
         default:
            ++nxt[i];
            return;
         }
      }

   /*
    * Indicate that there are no more prefixes.
    */
   nxt[0] = '\0';
   }

/*
 * cmp_pre - lexically compare 2-character prefixes.
 */
int cmp_pre(pre1, pre2)
char *pre1;
char *pre2;
   {
   int cmp;

   cmp = cmp_1_pre(pre1[0], pre2[0]);
   if (cmp == 0)
      return cmp_1_pre(pre1[1], pre2[1]);
   else
      return cmp;
   }

/*
 * cmp_1_pre - lexically compare 1 character of a prefix.
 */
static int cmp_1_pre(p1, p2)
int p1;
int p2;
   {
   if (isdigit(p1)) {
      if (isdigit(p2))
         return p1 - p2;
      else
         return -1;
      }
    else {
       if (isdigit(p2))
          return 1;
       else
         return p1 - p2;
      }
   }

/*
 * db_err1 - print a data base error message in the form of 1 string.
 */
void db_err1(fatal, s)
int fatal;
char *s;
   {
   if (fatal)
      fprintf(stderr, "error, ");
   else
      fprintf(stderr, "warning, ");
   fprintf(stderr, "data base \"%s\", line %d - %s\n", dbname, dbline, s);
   if (fatal)
      exit(EXIT_FAILURE);
   }

/*
 * db_err2 - print a data base error message in the form of 2 strings.
 */
void db_err2(fatal, s1, s2)
int fatal;
char *s1;
char *s2;
   {
   if (fatal)
      fprintf(stderr, "error, ");
   else
      fprintf(stderr, "warning, ");
   fprintf(stderr, "data base \"%s\", line %d - %s %s\n", dbname, dbline, s1,
      s2);
   if (fatal)
      exit(EXIT_FAILURE);
   }

/*
 * db_err3 - print a data base error message in the form of 3 strings.
 */
static void db_err3(fatal, s1, s2, s3)
int fatal;
char *s1;
char *s2;
char *s3;
   {
   if (fatal)
      fprintf(stderr, "error, ");
   else
      fprintf(stderr, "warning, ");
   fprintf(stderr, "data base \"%s\", line %d - %s %s%s\n", dbname, dbline, s1,
      s2, s3);
   if (fatal)
      exit(EXIT_FAILURE);
   }
