/*
 * rttdb.c - routines to read, manipulate, and write the data base of
 *  information about run-time routines.
 */

#include "rtt.h"
#include "../h/version.h"

#define DHSize 47
#define MaxLine 80

/*
 * prototypes for static functions.
 */
static void max_pre   (struct implement **tbl, char *pre);
static int     name_cmp  (char *p1, char *p2);
static int     op_cmp    (char *p1, char *p2);
static void prt_dpnd  (FILE *db);
static void prt_impls (FILE *db, char *sect, struct implement **tbl,
                           int num, struct implement **sort_ary, int (*com)());
static int     prt_c_fl  (FILE *db, struct cfile *clst, int line_left);
static int     put_case  (FILE *db, struct il_code *il);
static void put_ilc   (FILE *db, struct il_c *ilc);
static void put_inlin (FILE *db, struct il_code *il);
static void put_ret   (FILE *db, struct il_c *ilc);
static void put_typcd (FILE *db, int typcd);
static void put_var   (FILE *db, int code, struct il_c *ilc);
static void ret_flag  (FILE *db, int flag, int may_fthru);
static int     set_impl  (struct token *name, struct implement **tbl,
                           int num_impl, char *pre);
static void set_prms  (struct implement *ptr);
static int     src_cmp   (char *p1, char *p2);

static struct implement *bhash[IHSize];	/* hash area for built-in func table */
static struct implement *ohash[IHSize]; /* hash area for operator table */
static struct implement *khash[IHSize];	/* hash area for keyword table */

static struct srcfile *dhash[DHSize];	/* hash area for file dependencies */

static int num_fnc;		/* number of function in data base */
static int num_op = 0;		/* number of operators in data base */
static int num_key;		/* number of keywords in data base */
static int num_src = 0;		/* number of source files in dependencies */

static char fnc_pre[2];		/* next prefix available for functions */
static char op_pre[2];		/* next prefix available for operators */
static char key_pre[2];		/* next prefix available for keywords */

static long min_rs;		/* min result sequence of current operation */
static long max_rs;		/* max result sequence of current operation */
static int rsm_rs;		/* '+' at end of result sequencce of cur. oper. */

static int newdb = 0;		/* flag: this is a new data base */
struct token *comment;		/* comment associated with current operation */
struct implement *cur_impl;	/* data base entry for current operation */

/*
 * loaddb - load data base.
 */
void loaddb(dbname)
char *dbname;
   {
   char *op;
   struct implement *ip;
   unsigned hashval;
   int i;
   char *srcname;
   char *c_name;
   struct srcfile *sfile;


   /*
    * Initialize internal data base.
    */
   for (i = 0; i < IHSize; i++) {
       bhash[i] = NULL;   /* built-in function table */
       ohash[i] = NULL;   /* operator table */
       khash[i] = NULL;   /* keyword table */
       }
   for (i = 0; i < DHSize; i++)
       dhash[i] = NULL;   /* dependency table */

   /*
    * Determine if this is a new data base or an existing one.
    */
   if (iconx_flg || !db_open(dbname, &largeints))
      newdb = 1;
   else {

      /*
       * Read information about built-in functions.
       */
      num_fnc = db_tbl("functions", bhash);

      /*
       * Read information about operators.
       */
      db_chstr("", "operators");    /* verify and skip "operators" */

      while ((op = db_string()) != NULL) {
         /*
          * Read header information for the operator.
           */
         if ((ip = db_impl('O')) == NULL)
            db_err2(1, "no implementation information for operator", op);
         ip->op = op;

         /*
          * Read the descriptive comment and in-line code for the operator,
          *  then put the entry in the hash table.
          */
         db_code(ip);
         hashval = (int)IHasher(op);
         ip->blink = ohash[hashval];
         ohash[hashval] = ip;
         db_chstr("", "end");         /* verify and skip "end" */
         ++num_op;
         }
      db_chstr("", "endsect");       /* verify and skip "endsect" */

      /*
       * Read information about keywords.
       */
      num_key = db_tbl("keywords", khash);

      /*
       * Read C file/source dependency information.
       */
      db_chstr("", "dependencies");  /* verify and skip "dependencies" */

      while ((srcname = db_string()) != NULL) {
         sfile = src_lkup(srcname);
         while ((c_name = db_string()) != NULL)
            add_dpnd(sfile, c_name);
         db_chstr("", "end");         /* verify and skip "end" */
         }
      db_chstr("", "endsect");        /* verify and skip "endsect" */

      db_close();
      }

   /*
    * Determine the next available operation prefixes by finding the
    *  maximum prefixes currently in use.
    */
   max_pre(bhash, fnc_pre);
   max_pre(ohash, op_pre);
   max_pre(khash, key_pre);
   }

/*
 * max_pre - find the maximum prefix in an implemetation table and set the
 *  prefix array to the next value.
 */
static void max_pre(tbl, pre)
struct implement **tbl;
char *pre;
   {
   register struct implement *ptr;
   unsigned hashval;
   int empty = 1;
   char dmy_pre[2];

   pre[0] = '0';
   pre[1] = '0';
   for (hashval = 0; hashval < IHSize; ++hashval)
      for (ptr = tbl[hashval]; ptr != NULL; ptr = ptr->blink) {
         empty = 0;
         /*
          * Determine if this prefix is larger than any found so far.
          */
         if (cmp_pre(ptr->prefix, pre) > 0) {
            pre[0] = ptr->prefix[0];
            pre[1] = ptr->prefix[1];
            }
         }
   if (!empty)
      nxt_pre(dmy_pre, pre, 2);
   }


/*
 * src_lkup - return pointer to dependency information for the given
 *   source file.
 */
struct srcfile *src_lkup(srcname)
char *srcname;
   {
   unsigned hashval;
   struct srcfile *sfile;

   /*
    * See if the source file is already in the dependancy section of
    *  the data base.
    */
   hashval = (unsigned int)(unsigned long)srcname % DHSize;
   for (sfile = dhash[hashval]; sfile != NULL && sfile->name != srcname;
        sfile = sfile->next)
      ;

   /*
    * If an entry for the source file was not found, create one.
    */
   if (sfile == NULL) {
      sfile = NewStruct(srcfile);
      sfile->name = srcname;
      sfile->dependents = NULL;
      sfile->next = dhash[hashval];
      dhash[hashval] = sfile;
      ++num_src;
      }
   return sfile;
   }

/*
 * add_dpnd - add the given source/dependency relation to the dependency
 *   table.
 */
void add_dpnd(sfile, c_name)
struct srcfile *sfile;
char *c_name;
   {
   struct cfile *cf;

   cf = NewStruct(cfile);
   cf->name = c_name;
   cf->next = sfile->dependents;
   sfile->dependents = cf;
   }

/*
 * clr_dpnd - delete all dependencies for the given source file.
 */
void clr_dpnd(srcname)
char *srcname;
   {
   src_lkup(srcname)->dependents = NULL;
   }

/*
 * dumpdb - write the updated data base.
 */
void dumpdb(dbname)
char *dbname;
   {
   #ifdef Rttx
      fprintf(stdout,
         "rtt was compiled to only support the intepreter, use -x\n");
      exit(EXIT_FAILURE);
   #else				/* Rttx */
   FILE *db;
   struct implement **sort_ary;
   int ary_sz;
   int i;

   db = fopen(dbname, "wb");
   if (db == NULL)
      err2("cannot open data base for output:", dbname);
   if(newdb)
      fprintf(stdout, "creating new data base: %s\n", dbname);

   /*
    * The data base starts with a version number associated with this
    *   version of rtt and an indication of whether LargeInts was
    *   defined during the build.
    */
   fprintf(db, "%s %s\n\n", DVersion, largeints);

   fprintf(db, "\ntypes\n\n");          /* start of type code section */
   for (i = 0; i < num_typs; ++i)
      fprintf(db, "   T%d: %s\n", i, icontypes[i].id);
   fprintf(db, "\n$endsect\n\n");       /* end of section for type codes */

   fprintf(db, "\ncomponents\n\n");     /* start of component code section */
   for (i = 0; i < num_cmpnts; ++i)
      fprintf(db, "   C%d: %s\n", i, typecompnt[i].id);
   fprintf(db, "\n$endsect\n\n");       /* end of section for component codes */

   /*
    * Allocate an array for sorting operation entries. It must be
    *   large enough to hold functions, operators, or keywords.
    */
   ary_sz = Max(num_fnc, num_op);
   ary_sz = Max(ary_sz, num_key);
   if (ary_sz > 0)
      sort_ary = alloc(ary_sz * sizeof(struct implement*));
   else
      sort_ary = NULL;

   /*
    * Sort and print to the data base the enties for each of the
    *   three operation sections.
    */
   prt_impls(db, "functions", bhash, num_fnc, sort_ary, name_cmp);
   prt_impls(db, "\noperators", ohash, num_op, sort_ary, op_cmp);
   prt_impls(db, "\nkeywords", khash, num_key, sort_ary, name_cmp);
   if (ary_sz > 0)
      free((char *)sort_ary);

   /*
    * Print the dependancy information to the data base.
    */
   prt_dpnd(db);
   if (fclose(db) != 0)
     err2("cannot close ", dbname);
   #endif				/* Rttx */
   }

#ifndef Rttx
/*
 * prt_impl - sort and print to the data base the enties from one
 *   of the operation tables.
 */
static void prt_impls(db, sect, tbl, num, sort_ary, cmp)
FILE *db;
char *sect;
struct implement **tbl;
int num;
struct implement **sort_ary;
int (*cmp)();
   {
   int i;
   int j;
   unsigned hashval;
   struct implement *ip;

   /*
    * Each operation section begins with the section name.
    */
   fprintf(db, "%s\n\n", sect);

   /*
    * Sort the table entries before printing.
    */
   if (num > 0) {
      i = 0;
      for (hashval = 0; hashval < IHSize; ++hashval)
         for (ip = tbl[hashval]; ip != NULL; ip = ip->blink)
            sort_ary[i++] = ip;
      qsort((char *)sort_ary, num, sizeof(struct implement *), cmp);
      }

   /*
    * Output each entry to the data base.
    */
   for (i = 0; i < num; ++i) {
      ip = sort_ary[i];

      /*
       * Operators have operator symbols.
       */
      if (ip->op != NULL)
         fprintf(db, "%s\t", ip->op);

      /*
       * Print the operation name, the unique prefix used to generate
       *   C function names, and the number of parameters to the operation.
       */
      fprintf(db, "%s\t%c%c %d(", ip->name, ip->prefix[0], ip->prefix[1],
         ip->nargs);

      /*
       * For each parameter, write and indication of whether a dereferenced
       *   value, 'd', and/or and undereferenced value, 'u', is needed.
       */
      for (j = 0; j < ip->nargs; ++j) {
         if (j > 0)
            fprintf(db, ",");
         if (ip->arg_flgs[j] & RtParm)
            fprintf(db, "u");
         if (ip->arg_flgs[j] & DrfPrm)
            fprintf(db, "d");
         }

      /*
       * Indicate if the last parameter represents the tail of a
       *   variable length argument list.
       */
      if (ip->nargs > 0 && ip->arg_flgs[ip->nargs - 1] & VarPrm)
         fprintf(db, "v");
      fprintf(db, ")\t{");

      /*
       * Print the min and max result sequence length.
       */
      if (ip->min_result != NoRsltSeq) {
         fprintf(db, "%ld,", ip->min_result);
         if (ip->max_result == UnbndSeq)
            fprintf(db, "*");
         else
            fprintf(db, "%ld", ip->max_result);
         if (ip->resume)
            fprintf(db, "+");
         }
      fprintf(db, "} ");

      /*
       * Print the return/suspend/fail/fall-through flag and an indication
       *   of whether the operation explicitly uses the result location
       *   (as opposed to an implicit use via return or suspend).
       */
      ret_flag(db, ip->ret_flag, 0);
      if (ip->use_rslt)
         fprintf(db, "t ");
      else
         fprintf(db, "f ");

      /*
       * Print the descriptive comment associated with the operation.
       */
      fprintf(db, "\n\"%s\"\n", ip->comment);

      /*
       * Print information about tended declarations from the declare
       *  statement. The number of tended variables is printed followed
       *  by an entry for each variable. Each entry consists of the
       *  type of the declaration
       *
       *     struct descrip  -> desc
       *     char *          -> str
       *     struct b_xxx *  -> blkptr b_xxx
       *     union block *   -> blkptr *
       *
       *  followed by the C code for the initializer (nil indicates none).
       */
      fprintf(db, "%d ", ip->ntnds);
      for (j = 0; j < ip->ntnds; ++j) {
         switch (ip->tnds[j].var_type) {
            case TndDesc:
               fprintf(db, "desc ");
               break;
            case TndStr:
               fprintf(db, "str ");
               break;
            case TndBlk:
               fprintf(db, "blkptr ");
               if (ip->tnds[j].blk_name == NULL)
                  fprintf(db, "* ");
               else
                  fprintf(db, "%s ", ip->tnds[j].blk_name);
               break;
            }
         put_ilc(db, ip->tnds[j].init);
         }

      /*
       * Print information about non-tended declarations from the declare
       *  statement. The number of variables is printed followed by an
       *  entry for each variable. Each entry consists of the variable
       *  name followed by the complete C code for the declaration.
       */
      fprintf(db, "\n%d ", ip->nvars);
      for (j = 0; j < ip->nvars; ++j) {
         fprintf(db, "%s ", ip->vars[j].name);
         put_ilc(db, ip->vars[j].dcl);
         }
      fprintf(db, "\n");

      /*
       * Output the "executable" code (includes abstract code) for the
       *   operation.
       */
      put_inlin(db, ip->in_line);
      fprintf(db, "\n$end\n\n");    /* end of operation entry */
      }
   fprintf(db, "$endsect\n\n");     /* end of section for operation type */
   }

/*
 * put_inlin - put in-line code into the data base file. This is the
 *   code used by iconc to perform type infernence for the operation
 *   and to generate a tailored version of the operation.
 */
static void put_inlin(db, il)
FILE *db;
struct il_code *il;
   {
   int i;
   int num_cases;
   int indx;

   /*
    * RTL statements are handled by this function. Other functions
    *  are called for C code.
    */
   if (il == NULL) {
      fprintf(db, "nil ");
      return;
      }

   switch (il->il_type) {
      case IL_Const:
         /*
          * Constant keyword.
          */
         fprintf(db, "const ");
         put_typcd(db, il->u[0].n);              /* type  code */
         fputs(il->u[1].s, db); fputc(' ', db);  /* literal */
         break;
      case IL_If1:
         /*
          * if-then statment.
          */
         fprintf(db, "if1 ");
         put_inlin(db, il->u[0].fld);            /* condition */
         fprintf(db, "\n");
         put_inlin(db, il->u[1].fld);            /* then clause */
         break;
      case IL_If2:
         /*
          * if-then-else statment.
          */
         fprintf(db, "if2 ");
         put_inlin(db, il->u[0].fld);            /* condition */
         fprintf(db, "\n");
         put_inlin(db, il->u[1].fld);            /* then clause */
         fprintf(db, "\n");
         put_inlin(db, il->u[2].fld);            /* else clause */
         break;
      case IL_Tcase1:
         /*
          * type_case statement with no default clause.
          */
         fprintf(db, "tcase1 ");
         put_case(db, il);
         break;
      case IL_Tcase2:
         /*
          * type_case statement with a default clause.
          */
         fprintf(db, "tcase2 ");
         indx = put_case(db, il);
         fprintf(db, "\n");
         put_inlin(db, il->u[indx].fld);         /* default */
         break;
      case IL_Lcase:
         /*
          * len_case statement.
          */
         fprintf(db, "lcase ");
         num_cases = il->u[0].n;
         fprintf(db, "%d ", num_cases);
         indx = 1;
         for (i = 0; i < num_cases; ++i) {
            fprintf(db, "\n%d ", il->u[indx++].n);    /* selection number */
            put_inlin(db, il->u[indx++].fld);        /* action */
            }
         fprintf(db, "\n");
         put_inlin(db, il->u[indx].fld);             /* default */
         break;
      case IL_Acase:
         /*
          * arith_case statement.
          */
         fprintf(db, "acase ");
         put_inlin(db, il->u[0].fld);               /* first variable */
         put_inlin(db, il->u[1].fld);               /* second variable */
         fprintf(db, "\n");
         put_inlin(db, il->u[2].fld);               /* C_integer action */
         fprintf(db, "\n");
         put_inlin(db, il->u[3].fld);               /* integer action */
         fprintf(db, "\n");
         put_inlin(db, il->u[4].fld);               /* C_double action */
         break;
      case IL_Err1:
         /*
          * runerr with no value argument.
          */
         fprintf(db, "runerr1 ");
         fprintf(db, "%d ", il->u[0].n);      /* error number */
         break;
      case IL_Err2:
         /*
          * runerr with a value argument.
          */
         fprintf(db, "runerr2 ");
         fprintf(db, "%d ", il->u[0].n);      /* error number */
         put_inlin(db, il->u[1].fld);          /* variable */
         break;
      case IL_Lst:
         /*
          * "glue" to string statements together.
          */
         fprintf(db, "lst ");
         put_inlin(db, il->u[0].fld);
         fprintf(db, "\n");
         put_inlin(db, il->u[1].fld);
         break;
      case IL_Bang:
         /*
          * ! operator from type checking.
          */
         fprintf(db, "! ");
         put_inlin(db, il->u[0].fld);
         break;
      case IL_And:
         /*
          * && operator from type checking.
          */
         fprintf(db, "&& ");
         put_inlin(db, il->u[0].fld);
         put_inlin(db, il->u[1].fld);
         break;
      case IL_Cnv1:
         /*
          * cnv:<dest-type>(<source>)
          */
         fprintf(db, "cnv1 ");
         put_typcd(db, il->u[0].n);      /* type code */
         put_inlin(db, il->u[1].fld);    /* source */
         break;
      case IL_Cnv2:
         /*
          * cnv:<dest-type>(<source>,<destination>)
          */
         fprintf(db, "cnv2 ");
         put_typcd(db, il->u[0].n);      /* type code */
         put_inlin(db, il->u[1].fld);    /* source */
         put_ilc(db, il->u[2].c_cd);     /* destination */
         break;
      case IL_Def1:
         /*
          * def:<dest-type>(<source>,<default-value>)
          */
         fprintf(db, "def1 ");
         put_typcd(db, il->u[0].n);      /* type code */
         put_inlin(db, il->u[1].fld);    /* source */
         put_ilc(db, il->u[2].c_cd);     /* default value */
         break;
      case IL_Def2:
         /*
          * def:<dest-type>(<source>,<default-value>,<destination>)
          */
         fprintf(db, "def2 ");
         put_typcd(db, il->u[0].n);      /* type code */
         put_inlin(db, il->u[1].fld);    /* source */
         put_ilc(db, il->u[2].c_cd);     /* default value */
         put_ilc(db, il->u[3].c_cd);     /* destination */
         break;
      case IL_Is:
         /*
          * is:<type-name>(<variable>)
          */
         fprintf(db, "is ");
         put_typcd(db, il->u[0].n);      /* type code */
         put_inlin(db, il->u[1].fld);    /* variable */
         break;
      case IL_Var:
         /*
          * A variable.
          */
         fprintf(db, "%d ", il->u[0].n);    /* symbol table index */
         break;
      case IL_Subscr:
         /*
          * A subscripted variable.
          */
         fprintf(db, "[ ");
         fprintf(db, "%d ", il->u[0].n);    /* symbol table index */
         fprintf(db, "%d ", il->u[1].n);    /* subscripting index */
         break;
      case IL_Block:
         /*
          * A block of in-line code.
          */
         fprintf(db, "block ");
         if (il->u[0].n)
            fprintf(db, "t ");              /* execution can fall through */
         else
            fprintf(db, "_ ");              /* execution cannot fall through */
          /*
           * Output a symbol table of tended variables.
           */
         fprintf(db, "%d ", il->u[1].n);    /* number of local tended */
         for (i = 2; i - 2 < il->u[1].n; ++i)
             switch (il->u[i].n) {
                case TndDesc:
                   fprintf(db, "desc ");
                   break;
                case TndStr:
                   fprintf(db, "str ");
                   break;
                case TndBlk:
                   fprintf(db, "blkptr ");
                   break;
                }
         put_ilc(db, il->u[i].c_cd);         /* body of block */
         break;
      case IL_Call:
         /*
          * A call to a body function.
          */
         fprintf(db, "call ");

         /*
          * Each body function has a 3rd prefix character to distingish
          *  it from other functions for the operation.
          */
         fprintf(db, "%c ", (char)il->u[1].n);

         /*
          * A body function that would only return one possible signal
          *   need return none. In which case, it can directly return a
          *   C integer or double directly rather than using a result
          *   descriptor location. Indicate what it does.
          */
         switch (il->u[2].n) {
            case RetInt:
               fprintf(db, "i ");  /* directly return integer */
               break;
            case RetDbl:
               fprintf(db, "d ");  /* directly return double */
               break;
            case RetNoVal:
               fprintf(db, "n ");  /* return nothing directly */
               break;
            case RetSig:
               fprintf(db, "s ");  /* return a signal */
               break;
            }

         /*
          * Output the return/suspend/fail/fall-through flag.
          */
         ret_flag(db, il->u[3].n, 1);

         /*
          * Indicate whether the body function expects to have
          *   an explicit result location passed to it.
          */
         if (il->u[4].n)
            fprintf(db, "t ");
         else
            fprintf(db, "f ");

         fprintf(db, "%d ", il->u[5].n);    /* num string bufs */
         fprintf(db, "%d ", il->u[6].n);    /* num cset bufs */
         i = il->u[7].n;
         fprintf(db, "%d ", i);             /* num args */
         indx = 8;
         /*
          * output prototype paramater declarations and actual arguments.
          */
         i *= 2;
         while (i--)
            put_ilc(db, il->u[indx++].c_cd);
         break;
      case IL_Abstr:
         /*
          * Abstract type computation.
          */
         fprintf(db, "abstr ");
         put_inlin(db, il->u[0].fld);    /* side effects */
         put_inlin(db, il->u[1].fld);    /* return type */
         break;
      case IL_VarTyp:
         /*
          * type(<parameter>)
          */
         fprintf(db, "vartyp ");
         put_inlin(db, il->u[0].fld);    /* variable */
         break;
      case IL_Store:
         /*
          * store[<type>]
          */
         fprintf(db, "store ");
         put_inlin(db, il->u[0].fld);    /* type to be "dereferenced "*/
         break;
      case IL_Compnt:
         /*
          * <type>.<component>
          */
         fprintf(db, ". ");
         put_inlin(db, il->u[0].fld);    /* type */
         if (il->u[1].n == CM_Fields)
             fprintf(db, "f ");          /* special case record fields */
         else
             fprintf(db, "C%d ", (int)il->u[1].n); /* component table index */
         break;
      case IL_TpAsgn:
         /*
          * store[<variable-type>] = <value-type>
          */
         fprintf(db, "= ");
         put_inlin(db, il->u[0].fld);    /* variable type */
         put_inlin(db, il->u[1].fld);    /* value type */
         break;
      case IL_Union:
         /*
          * <type 1> ++ <type 2>
          */
         fprintf(db, "++ ");
         put_inlin(db, il->u[0].fld);
         put_inlin(db, il->u[1].fld);
         break;
      case IL_Inter:
         /*
          * <type 1> ** <type 2>
          */
         fprintf(db, "** ");
         put_inlin(db, il->u[0].fld);
         put_inlin(db, il->u[1].fld);
         break;
      case IL_New:
         /*
          * new <type-name>(<type 1> , ...)
          */
         fprintf(db, "new ");
         put_typcd(db, il->u[0].n);      /* type code */
         i = il->u[1].n;
         fprintf(db, "%d ", i);          /* num args */
         indx = 2;
         while (i--)
            put_inlin(db, il->u[indx++].fld);
         break;
      case IL_IcnTyp:
         /*
          * <type-name>
          */
         fprintf(db, "typ ");
         put_typcd(db, il->u[0].n);      /* type code */
         break;
      }
   }

/*
 * put_case - put the cases of a type_case statement into the data base file.
 */
static int put_case(db, il)
FILE *db;
struct il_code *il;
   {
   int *typ_vect;
   int i, j;
   int num_cases;
   int num_types;
   int indx;

   put_inlin(db, il->u[0].fld);               /* expression being checked */
   num_cases = il->u[1].n;                    /* number of cases */
   fprintf(db, "%d ", num_cases);
   indx = 2;
   for (i = 0; i < num_cases; ++i) {
      num_types = il->u[indx++].n;             /* number of types in case */
      fprintf(db, "\n%d ", num_types);
      typ_vect = il->u[indx++].vect;          /* vector of type codes */
      for (j = 0; j < num_types; ++j)
         put_typcd(db, typ_vect[j]);          /* type code */
      put_inlin(db, il->u[indx++].fld);       /* action */
      }
   return indx;
   }

/*
 * put_typcd - convert a numeric type code into an alpha type code and
 *  put it in the data base file.
 */
static void put_typcd(db, typcd)
FILE *db;
int typcd;
   {
   if (typcd >= 0)
      fprintf(db, "T%d ", typcd);
   else {
      switch (typcd) {
         case TypAny:
            fprintf(db, "a ");	   /* any_value */
            break;
         case TypEmpty:
            fprintf(db, "e ");	   /* empty_type */
            break;
         case TypVar:
            fprintf(db, "v ");	   /* variable */
            break;
         case TypCInt:
            fprintf(db, "ci ");    /* C_integer */
            break;
         case TypCDbl:
            fprintf(db, "cd ");    /* C_double */
            break;
         case TypCStr:
            fprintf(db, "cs ");    /* C_string */
            break;
         case TypEInt:
            fprintf(db, "ei ");    /* (exact)integer) */
            break;
         case TypECInt:
            fprintf(db, "eci ");   /* (exact)C_integer */
            break;
         case TypTStr:
            fprintf(db, "ts ");    /* tmp_string */
            break;
         case TypTCset:
            fprintf(db, "tc ");    /* tmp_cset */
            break;
         case RetDesc:
            fprintf(db, "d ");     /* plain descriptor on return/suspend */
            break;
         case RetNVar:
            fprintf(db, "nv ");    /* named_var */
            break;
         case RetSVar:
            fprintf(db, "sv ");    /* struct_var */
            break;
         case RetNone:
            fprintf(db, "rn ");   /* preset result location on return/suspend */
            break;
         }
      }
   }

/*
 * put_ilc - put in-line C code in the data base file.
 */
static void put_ilc(db, ilc)
FILE *db;
struct il_c *ilc;
   {
   /*
    * In-line C code is either "nil" or code bracketed by $c $e.
    *   The bracketed code consists of text for C code plus special
    *   constructs starting with $. Control structures have been
    *   translated into gotos in the form of special constructs
    *   (note that case statements are not supported in in-line code).
    */
   if (ilc == NULL) {
      fprintf(db, "nil ");
      return;
      }
   fprintf(db, "$c ");
   while (ilc != NULL) {
      switch(ilc->il_c_type) {
         case ILC_Ref:
            put_var(db, 'r', ilc);   /* non-modifying reference to variable */
            break;
         case ILC_Mod:
            put_var(db, 'm', ilc);   /* modifying reference to variable */
            break;
         case ILC_Tend:
            put_var(db, 't', ilc);   /* variable declared tended */
            break;
         case ILC_SBuf:
            fprintf(db, "$sb ");     /* string buffer for tmp_string */
            break;
         case ILC_CBuf:
            fprintf(db, "$cb ");     /* cset buffer for tmp_cset */
            break;
         case ILC_Ret:
            fprintf(db, "$ret ");    /* return statement */
            put_ret(db, ilc);
            break;
         case ILC_Susp:
            fprintf(db, "$susp ");   /* suspend statement */
            put_ret(db, ilc);
            break;
         case ILC_Fail:
            fprintf(db, "$fail ");   /* fail statement */
            break;
         case ILC_EFail:
            fprintf(db, "$efail ");  /* errorfail statement */
            break;
         case ILC_Goto:
            fprintf(db, "$goto %d ", ilc->n);  /* goto label */
            break;
         case ILC_CGto:
            fprintf(db, "$cgoto ");            /* conditional goto */
            put_ilc(db, ilc->code[0]);         /* condition (with $c $e) */
            fprintf(db, "%d ", ilc->n);        /* label */
            break;
         case ILC_Lbl:
            fprintf(db, "$lbl %d ", ilc->n);   /* label */
            break;
         case ILC_LBrc:
            fprintf(db, "${ ");                /* start of C block with dcls */
            break;
         case ILC_RBrc:
            fprintf(db, "$} ");                /* end of C block with dcls */
            break;
         case ILC_Str:
            fprintf(db, "%s", ilc->s);         /* C code as plain text */
            break;
         }
      ilc = ilc->next;
      }
   fprintf(db, " $e ");
   }

/*
 * put_var - output in-line C code for a variable.
 */
static void put_var(db, code, ilc)
FILE *db;
int code;
struct il_c *ilc;
   {
   fprintf(db, "$%c", code);  /* 'r': non-mod ref, 'm': mod ref, 't': tended */
   if (ilc->s != NULL)
      fprintf(db, "%s", ilc->s);  /* access into descriptor */
   if (ilc->n == RsltIndx)
      fprintf(db, "r ");          /* this is "result" */
   else
      fprintf(db, "%d ", ilc->n); /* offset into a symbol table */
   }

/*
 * ret_flag - put a return/suspend/fail/fall-through flag in the data base
 *  file.
 */
static void ret_flag(db, flag, may_fthru)
FILE *db;
int flag;
int may_fthru;
   {
   if (flag & DoesFail)
      fprintf(db, "f");      /* can fail */
   else
      fprintf(db, "_");      /* cannot fail */
   if (flag & DoesRet)
      fprintf(db, "r");      /* can return */
   else
      fprintf(db, "_");      /* cannot return */
   if (flag & DoesSusp)
      fprintf(db, "s");      /* can suspend */
   else
      fprintf(db, "_");      /* cannot suspend */
   if (flag & DoesEFail)
      fprintf(db, "e");      /* can do error conversion */
   else
      fprintf(db, "_");      /* cannot do error conversion */
   if (may_fthru)            /* body functions only: */
      if (flag & DoesFThru)
         fprintf(db, "t");      /* can fall through */
      else
         fprintf(db, "_");      /* cannot fall through */
  fprintf(db, " ");
  }

/*
 * put_ret - put the body of a return/suspend statement in the data base.
 */
static void put_ret(db, ilc)
FILE *db;
struct il_c *ilc;
   {
   int i;

   /*
    * Output the type of descriptor constructor on the return/suspend,
    *  then output the the number of arguments to the constructor, and
    *  the arguments themselves.
    */
   put_typcd(db, ilc->n);
   for (i = 0; i < 3 && ilc->code[i] != NULL; ++i)
       ;
   fprintf(db, "%d ", i);
   for (i = 0; i < 3 && ilc->code[i] != NULL; ++i)
       put_ilc(db, ilc->code[i]);
   }

/*
 * name_cmp - compare implementation structs by name; function used as
 *  an argument to qsort().
 */
static int name_cmp(p1, p2)
char *p1;
char *p2;
   {
   register struct implement *ip1;
   register struct implement *ip2;

   ip1 = *(struct implement **)p1;
   ip2 = *(struct implement **)p2;
   return strcmp(ip1->name, ip2->name);
   }

/*
 * op_cmp - compare implementation structs by operator and number of args;
 *   function used as an argument to qsort().
 */
static int op_cmp(p1, p2)
char *p1;
char *p2;
   {
   register int cmp;
   register struct implement *ip1;
   register struct implement *ip2;

   ip1 = *(struct implement **)p1;
   ip2 = *(struct implement **)p2;

   cmp = strcmp(ip1->op, ip2->op);
   if (cmp == 0)
      return ip1->nargs - ip2->nargs;
   else
      return cmp;
   }

/*
 * prt_dpnd - print dependency information to the data base.
 */
static void prt_dpnd(db)
FILE *db;
   {
   struct srcfile **sort_ary;
   struct srcfile *sfile;
   unsigned hashval;
   int line_left;
   int num;
   int i;

   fprintf(db, "\ndependencies\n\n");  /* start of dependency section */

   /*
    * sort the dependency information by source file name.
    */
   num = 0;
   if (num_src > 0) {
      sort_ary = alloc(num_src * sizeof(struct srcfile *));
      for (hashval = 0; hashval < DHSize; ++hashval)
         for (sfile = dhash[hashval]; sfile != NULL; sfile = sfile->next)
            sort_ary[num++] = sfile;
      qsort((char *)sort_ary, num, sizeof(struct srcfile *),
         (int (*)())src_cmp);
      }

   /*
    * For each source file with dependents, output the source file
    *  name followed by the list of dependent files. The list is
    *  terminated with "end".
    */
   for (i = 0; i < num; ++i) {
      sfile = sort_ary[i];
      if (sfile->dependents != NULL) {
         fprintf(db, "%-12s  ", sfile->name);
         line_left = prt_c_fl(db, sfile->dependents, MaxLine - 14);
         if (line_left - 4 < 0)
            fprintf(db, "\n            ");
         fprintf(db, "$end\n");
         }
      }
   fprintf(db, "\n$endsect\n");  /* end of dependency section */
   if (num_src > 0)
      free((char *)sort_ary);
   }

/*
 * src_cmp - compare srcfile structs; function used as an argument to qsort().
 */
static int src_cmp(p1, p2)
char *p1;
char *p2;
   {
   register struct srcfile *sp1;
   register struct srcfile *sp2;

   sp1 = *(struct srcfile **)p1;
   sp2 = *(struct srcfile **)p2;
   return strcmp(sp1->name, sp2->name);
   }

/*
 * prt_c_fl - print list of C files in reverse order.
 */
static int prt_c_fl(db, clst, line_left)
FILE *db;
struct cfile *clst;
int line_left;
   {
   int len;

   if (clst == NULL)
      return line_left;
   line_left = prt_c_fl(db, clst->next, line_left);

   /*
    * If this will exceed the line length, print a new-line and some
    *  leading white space.
    */
   len = strlen(clst->name) + 1;
   if (line_left - len < 0) {
      fprintf(db, "\n              ");
      line_left = MaxLine - 14;
      }
   fprintf(db, "%s ", clst->name);
   return line_left - len;
   }
#endif					/* Rttx */

/*
 * full_lst - print a full list of all files produced by translations
 *  as represented in the dependencies section of the data base.
 */
void full_lst(fname)
char *fname;
   {
   unsigned hashval;
   struct srcfile *sfile;
   struct cfile *clst;
   struct fileparts *fp;
   FILE *f;

   f = fopen(fname, "w");
   if (f == NULL)
      err2("cannot open ", fname);
   for (hashval = 0; hashval < DHSize; ++hashval)
      for (sfile = dhash[hashval]; sfile != NULL; sfile = sfile->next)
         for (clst = sfile->dependents; clst != NULL; clst = clst->next) {
            /*
             * Remove the suffix from the name before printing.
             */
            fp = fparse(clst->name);
            fprintf(f, "%s\n", fp->name);
            }
   if (fclose(f) != 0)
      err2("cannot close ", fname);
   }

/*
 * impl_fnc - find or create implementation struct for function currently
 *  being parsed.
 */
void impl_fnc(name)
struct token *name;
   {
   /*
    * Set the global operation type for later use. If this is a
    *  new function update the number of them.
    */
   op_type = TokFunction;
   num_fnc = set_impl(name, bhash, num_fnc, fnc_pre);
   }

/*
 * impl_key - find or create implementation struct for keyword currently
 *  being parsed.
 */
void impl_key(name)
struct token *name;
   {
   /*
    * Set the global operation type for later use. If this is a
    *  new keyword update the number of them.
    */
   op_type = Keyword;
   num_key = set_impl(name, khash, num_key, key_pre);
   }

/*
 * set_impl - lookup a function or keyword in a hash table and update the
 *  entry, creating the entry if needed.
 */
static int set_impl(name, tbl, num_impl, pre)
struct token *name;
struct implement **tbl;
int num_impl;
char *pre;
   {
   register struct implement *ptr;
   char *name_s;
   unsigned hashval;

   /*
    * we only need the operation name and not the entire token.
    */
   name_s = name->image;
   free_t(name);

   /*
    * If the operation is not in the hash table, put it there.
    */
   if ((ptr = db_ilkup(name_s, tbl)) == NULL) {
      ptr = NewStruct(implement);
      hashval = IHasher(name_s);
      ptr->blink = tbl[hashval];
      ptr->oper_typ = ((op_type == TokFunction) ? 'F' : 'K');
      nxt_pre(ptr->prefix, pre, 2);    /* allocate a unique prefix */
      ptr->name = name_s;
      ptr->op = NULL;
      tbl[hashval] = ptr;
      ++num_impl;
      }

   cur_impl = ptr;   /* put entry in global variable for later access */

   /*
    * initialize the entry based on global information set during parsing.
    */
   set_prms(ptr);
   ptr->min_result = min_rs;
   ptr->max_result = max_rs;
   ptr->resume = rsm_rs;
   ptr->ret_flag = 0;
   if (comment == NULL)
      ptr->comment = "";
   else {
      ptr->comment = comment->image;
      free_t(comment);
      comment = NULL;
      }
   ptr->ntnds = 0;
   ptr->tnds = NULL;
   ptr->nvars = 0;
   ptr->vars = NULL;
   ptr->in_line = NULL;
   ptr->iconc_flgs = 0;
   return num_impl;
   }

/*
 * set_prms - set the parameter information of an implementation based on
 *   the params list constructed during parsing.
 */
static void set_prms(ptr)
struct implement *ptr;
   {
   struct sym_entry *sym;
   int nargs;
   int i;

   /*
    * Create an array of parameter flags for the operation. The flag
    * indicates the deref/underef and varargs status for each parameter.
    */
   if (params == NULL) {
      ptr->nargs = 0;
      ptr->arg_flgs = NULL;
      }
   else {
      /*
       * The parameters are in reverse order, so the number of the parameters
       *  can be determined by the number assigned to the first one on the
       *  list.
       */
      nargs = params->u.param_info.param_num + 1;
      ptr->nargs = nargs;
      ptr->arg_flgs = alloc(nargs * sizeof(int));
      for (i = 0; i < nargs; ++i)
         ptr->arg_flgs[i] = 0;
      for (sym = params; sym != NULL; sym = sym->u.param_info.next)
         ptr->arg_flgs[sym->u.param_info.param_num] |= sym->id_type;
      }
   }

/*
 * impl_op - find or create implementation struct for operator currently
 *  being parsed.
 */
void impl_op(op_sym, name)
struct token *op_sym;
struct token *name;
   {
   register struct implement *ptr;
   char *op;
   int nargs;
   unsigned hashval;

   /*
    * The operator symbol is needed but not the entire token.
    */
   op = op_sym->image;
   free_t(op_sym);

   /*
    * The parameters are in reverse order, so the number of the parameters
    *  can be determined by the number assigned to the first one on the
    *  list.
    */
   if (params == NULL)
      nargs = 0;
   else
      nargs = params->u.param_info.param_num + 1;

   /*
    * Locate the operator in the hash table; it must match both the
    *  operator symbol and the number of arguments. If the operator is
    *  not there, create an entry.
    */
   hashval = IHasher(op);
   ptr = ohash[hashval];
   while (ptr != NULL && (ptr->op != op || ptr->nargs != nargs))
      ptr = ptr->blink;
   if (ptr == NULL) {
      ptr = NewStruct(implement);
      ptr->blink = ohash[hashval];
      ptr->oper_typ = 'O';
      nxt_pre(ptr->prefix, op_pre, 2);   /* allocate a unique prefix */
      ptr->op = op;
      ohash[hashval] = ptr;
      ++num_op;
      }

   /*
    * Put the entry and operation type in global variables for
    *  later access.
    */
   cur_impl = ptr;
   op_type = Operator;

   /*
    * initialize the entry based on global information set during parsing.
    */
   ptr->name = name->image;
   free_t(name);
   set_prms(ptr);
   ptr->min_result = min_rs;
   ptr->max_result = max_rs;
   ptr->resume = rsm_rs;
   ptr->ret_flag = 0;
   if (comment == NULL)
      ptr->comment = "";
   else {
      ptr->comment = comment->image;
      free_t(comment);
      comment = NULL;
      }
   ptr->ntnds = 0;
   ptr->tnds = NULL;
   ptr->nvars = 0;
   ptr->vars = NULL;
   ptr->in_line = NULL;
   ptr->iconc_flgs = 0;
   }

/*
 * set_r_seq - save result sequence information for updating the
 *  operation entry.
 */
void set_r_seq(min, max, resume)
long min;
long max;
int resume;
   {
   if (min == UnbndSeq)
      min = 0;
   min_rs = min;
   max_rs = max;
   rsm_rs = resume;
   }

