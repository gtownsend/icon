/*
 * dbase.c - routines to access data base of implementation information
 *  produced by rtt.
 */
#include "../h/gsupport.h"
#include "../h/lexdef.h"
#include "ctrans.h"
#include "csym.h"
#include "ctree.h"
#include "ccode.h"
#include "cproto.h"
#include "cglobals.h"

/*
 * Prototypes.
 */
static int chck_spec (struct implement *ip);
static int acpt_op   (struct implement *ip);


static struct optab *optr; /* pointer into operator table */

/*
 * readdb - read data base produced by rtt.
 */
void readdb(db_name)
char *db_name;
   {
   char *op, *s;
   int i;
   struct implement *ip;
   char buf[MaxPath];			/* file name construction buffer */
   struct fileparts *fp;
   unsigned hashval;

   fp = fparse(db_name);
   if (*fp->ext == '\0')
      db_name = salloc(makename(buf, NULL, db_name, DBSuffix));
   else if (!smatch(fp->ext, DBSuffix))
      quitf("bad data base name: %s", db_name);

   if (!db_open(db_name, &s))
        db_err1(1, "cannot open data base");

   if (largeints && (*s == 'N')) {
       twarn("Warning, run-time system does not support large integers", NULL);
       largeints = 0;
       }

   /*
    * Read information about functions.
    */
   db_tbl("functions", bhash);

   /*
    * Read information about operators.
    */
   optr = optab;

   /*
    * read past operators header.
    */
   db_chstr("operators", "operators");

   while ((op = db_string()) != NULL) {
      if ((ip = db_impl('O')) == NULL)
         db_err2(1, "no implementation information for operator", op);
      ip->op = op;
      if (acpt_op(ip)) {
         db_code(ip);
         hashval = IHasher(op);
         ip->blink = ohash[hashval];
         ohash[hashval] = ip;
         db_chstr("end", "end");
         }
      else
         db_dscrd(ip);
      }
   db_chstr("endsect", "endsect");

   /*
    * Read information about keywords.
    */
   db_tbl("keywords", khash);

   db_close();

   /*
    * If error conversion is supported, make sure it is reflected in
    *   the minimum result sequence of operations.
    */
   if (err_conv) {
      for (i = 0; i < IHSize; ++i)
         for (ip = bhash[i]; ip != NULL; ip = ip->blink)
            if (ip->ret_flag & DoesEFail)
               ip->min_result = 0;
      for (i = 0; i < IHSize; ++i)
         for (ip = ohash[i]; ip != NULL; ip = ip->blink)
            if (ip->ret_flag & DoesEFail)
               ip->min_result = 0;
      for (i = 0; i < IHSize; ++i)
         for (ip = khash[i]; ip != NULL; ip = ip->blink)
            if (ip->ret_flag & DoesEFail)
               ip->min_result = 0;
      }
   }

/*
 * acpt_opt - given a data base entry for an operator determine if it
 *  is in iconc's operator table.
 */
static int acpt_op(ip)
struct implement *ip;
   {
   register char *op;
   register int opcmp;

   /*
    * Calls to this function are in lexical order by operator symbol continue
    *  searching operator table  from where we left off.
    */
   op = ip->op;
   for (;;) {
      /*
       * optab has augmented assignments out of lexical order. Skip anything
       *  which does not expect an implementation. This gets augmented
       *  assignments out of the way.
       */
      while (optr->expected == 0 && optr->tok.t_word != NULL)
         ++optr;
      if (optr->tok.t_word == NULL)
         return chck_spec(ip);
      opcmp = strcmp(op, optr->tok.t_word);
      if (opcmp > 0)
         ++optr;
      else if (opcmp < 0)
         return chck_spec(ip);
      else {
         if (ip->nargs == 1 && (optr->expected & Unary)) {
            if (optr->unary == NULL) {
               optr->unary = ip;
               return 1;
               }
            else
               return 0;
            }
         else if (ip->nargs == 2 && (optr->expected & Binary)) {
            if (optr->binary == NULL) {
               optr->binary = ip;
               return 1;
               }
            else
               return 0;
            }
         else
            return chck_spec(ip);
         }
      }
   }

/*
 * chck_spec - check whether the operator is one that does not use standard
 *    unary or binary syntax.
 */
static int chck_spec(ip)
struct implement *ip;
   {
   register char *op;
   int indx;

   indx = -1;
   op = ip->op;
   if (strcmp(op, "...") == 0) {
      if (ip->nargs == 2)
         indx = ToOp;
      else
         indx = ToByOp;
      }
   else if (strcmp(op, "[:]") == 0)
      indx = SectOp;
   else if (strcmp(op, "[]") == 0)
      indx = SubscOp;
   else if (strcmp(op, "[...]") == 0)
      indx = ListOp;

   if (indx == -1) {
      db_err2(0, "unexpected operator (or arity),", op);
      return 0;
      }
   if (spec_op[indx] == NULL) {
      spec_op[indx] = ip;
      return 1;
      }
   else
      return 0;
   }
