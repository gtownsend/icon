/*
 * tsym.c -- functions for symbol table management.
 */

#include "../h/gsupport.h"
#include "tproto.h"
#include "tglobals.h"
#include "ttoken.h"
#include "tsym.h"
#include "keyword.h"
#include "lfile.h"

/*
 * Prototypes.
 */

static struct	tgentry *alcglob
   (struct tgentry *blink, char *name,int flag,int nargs);
static struct	tcentry *alclit
   (struct tcentry *blink, char *name, int len,int flag);
static struct	tlentry *alcloc
   (struct tlentry *blink, char *name,int flag);
static struct	tcentry *clookup	(char *id,int flag);
static struct	tgentry *glookup	(char *id);
static struct	tlentry *llookup	(char *id);
static void	putglob
   (char *id,int id_type, int n_args);

#ifdef DeBugTrans
   void	cdump	(void);
   void	gdump	(void);
   void	ldump	(void);
#endif					/* DeBugTrans */


/*
 * Keyword table.
 */

struct keyent {
   char *keyname;
   int keyid;
   };

#define KDef(p,n) Lit(p), n,
static struct keyent keytab[] = {
#include "../h/kdefs.h"
   NULL, -1
};

/*
 * loc_init - clear the local and constant symbol tables.
 */

void loc_init()
   {
   struct tlentry *lptr, *lptr1;
   struct tcentry *cptr, *cptr1;
   int i;

   /*
    * Clear local table, freeing entries.
    */
   for (i = 0; i < lhsize; i++) {
      for (lptr = lhash[i]; lptr != NULL; lptr = lptr1) {
          lptr1 = lptr->l_blink;
          free((char *)lptr);
          }
       lhash[i] = NULL;
       }
   lfirst = NULL;
   llast = NULL;

   /*
    * Clear constant table, freeing entries.
    */
   for (i = 0; i < lchsize; i++) {
      for (cptr = chash[i]; cptr != NULL; cptr = cptr1) {
          cptr1 = cptr->c_blink;
          free((char *)cptr);
          }
       chash[i] = NULL;
       }
   cfirst = NULL;
   clast = NULL;
   }

/*
 * install - put an identifier into the global or local symbol table.
 *  The basic idea here is to look in the right table and install
 *  the identifier if it isn't already there.  Some semantic checks
 *  are performed.
 */
void install(name, flag, argcnt)
char *name;
int flag, argcnt;
   {
   union {
      struct tgentry *gp;
      struct tlentry *lp;
      } p;

   switch (flag) {
      case F_Global:	/* a variable in a global declaration */
         if ((p.gp = glookup(name)) == NULL)
            putglob(name, flag, argcnt);
         else
            p.gp->g_flag |= flag;
         break;

      case F_Proc|F_Global:	/* procedure declaration */
      case F_Record|F_Global:	/* record declaration */
      case F_Builtin|F_Global:	/* external declaration */
         if ((p.gp = glookup(name)) == NULL)
            putglob(name, flag, argcnt);
         else if ((p.gp->g_flag & (~F_Global)) == 0) { /* superfluous global
							   declaration for
							   record or proc */
            p.gp->g_flag |= flag;
            p.gp->g_nargs = argcnt;
            }
         else			/* the user can't make up his mind */
            tfatal("inconsistent redeclaration", name);
         break;

      case F_Static:	/* static declaration */
      case F_Dynamic:	/* local declaration (possibly implicit?) */
      case F_Argument:	/* formal parameter */
         if ((p.lp = llookup(name)) == NULL)
            putloc(name,flag);
         else if (p.lp->l_flag == flag) /* previously declared as same type */
            tfatal("redeclared identifier", name);
         else		/* previously declared as different type */
            tfatal("inconsistent redeclaration", name);
         break;

      default:
         tsyserr("install: unrecognized symbol table flag.");
      }
   }

/*
 * putloc - make a local symbol table entry and return the index
 *  of the entry in lhash.  alcloc does the work if there is a collision.
 */
int putloc(id,id_type)
char *id;
int id_type;
   {
   register struct tlentry *ptr;

   if ((ptr = llookup(id)) == NULL) {	/* add to head of hash chain */
      ptr = lhash[lhasher(id)];
      lhash[lhasher(id)] = alcloc(ptr, id, id_type);
      return lhash[lhasher(id)]->l_index;
      }
   return ptr->l_index;
   }

/*
 * putglob makes a global symbol table entry. alcglob does the work if there
 *  is a collision.
 */

static void putglob(id, id_type, n_args)
char *id;
int id_type, n_args;
   {
   register struct tgentry *ptr;

   if ((ptr = glookup(id)) == NULL) {	 /* add to head of hash chain */
      ptr = ghash[ghasher(id)];
      ghash[ghasher(id)] = alcglob(ptr, id, id_type, n_args);
      }
   }

/*
 * putlit makes a constant symbol table entry and returns the table "index"
 *  of the constant.  alclit does the work if there is a collision.
 */
int putlit(id, idtype, len)
char *id;
int len, idtype;
   {
   register struct tcentry *ptr;

   if ((ptr = clookup(id,idtype)) == NULL) {   /* add to head of hash chain */
      ptr = chash[chasher(id)];
      chash[chasher(id)] = alclit(ptr, id, len, idtype);
      return chash[chasher(id)]->c_index;
      }
   return ptr->c_index;
   }

/*
 * llookup looks up id in local symbol table and returns pointer to
 *  to it if found or NULL if not present.
 */

static struct tlentry *llookup(id)
char *id;
   {
   register struct tlentry *ptr;

   ptr = lhash[lhasher(id)];
   while (ptr != NULL && ptr->l_name != id)
      ptr = ptr->l_blink;
   return ptr;
   }

/*
 * glookup looks up id in global symbol table and returns pointer to
 *  to it if found or NULL if not present.
 */
static struct tgentry *glookup(id)
char *id;
   {
   register struct tgentry *ptr;

   ptr = ghash[ghasher(id)];
   while (ptr != NULL && ptr->g_name != id) {
      ptr = ptr->g_blink;
      }
   return ptr;
   }

/*
 * clookup looks up id in constant symbol table and returns pointer to
 *  to it if found or NULL if not present.
 */
static struct tcentry *clookup(id,flag)
char *id;
int flag;
   {
   register struct tcentry *ptr;

   ptr = chash[chasher(id)];
   while (ptr != NULL && (ptr->c_name != id || ptr->c_flag != flag))
      ptr = ptr->c_blink;

   return ptr;
   }

/*
 * klookup looks up keyword named by id in keyword table and returns
 *  its number (keyid).
 */
int klookup(id)
register char *id;
   {
   register struct keyent *kp;

   for (kp = keytab; kp->keyid >= 0; kp++)
      if (strcmp(kp->keyname,id) == 0)
         return (kp->keyid);

   return 0;
   }

#ifdef DeBugTrans
/*
 * ldump displays local symbol table to stdout.
 */

void ldump()
   {
   register int i;
   register struct tlentry *lptr;
   int n;

   if (llast == NULL)
      n = 0;
   else
      n = llast->l_index + 1;
   fprintf(stderr,"Dump of local symbol table (%d entries)\n", n);
   fprintf(stderr," loc   blink   id		  (name)      flags\n");
   for (i = 0; i < lhsize; i++)
      for (lptr = lhash[i]; lptr != NULL; lptr = lptr->l_blink)
         fprintf(stderr,"%5d  %5d  %5d	%20s  %7o\n", lptr->l_index,
		lptr->l_blink, lptr->l_name, lptr->l_name, lptr->l_flag);
   fflush(stderr);

   }

/*
 * gdump displays global symbol table to stdout.
 */

void gdump()
   {
   register int i;
   register struct tgentry *gptr;
   int n;

   if (glast == NULL)
      n = 0;
   else
      n = glast->g_index + 1;
   fprintf(stderr,"Dump of global symbol table (%d entries)\n", n);
   fprintf(stderr," loc   blink   id		  (name)      flags	  nargs\n");
   for (i = 0; i < ghsize; i++)
      for (gptr = ghash[i]; gptr != NULL; gptr = gptr->g_blink)
         fprintf(stderr,"%5d  %5d  %5d	%20s  %7o   %8d\n", gptr->g_index,
		gptr->g_blink, gptr->g_name, gptr->g_name,
		gptr->g_flag, gptr->g_nargs);
   fflush(stderr);
   }

/*
 * cdump displays constant symbol table to stdout.
 */

void cdump()
   {
   register int i;
   register struct tcentry *cptr;
   int n;

   if (clast == NULL)
      n = 0;
   else
      n = clast->c_index + 1;
   fprintf(stderr,"Dump of constant symbol table (%d entries)\n", n);
   fprintf(stderr," loc   blink   id		  (name)      flags\n");
   for (i = 0; i < lchsize; i++)
      for (cptr = chash[i]; cptr != NULL; cptr = cptr->c_blink)
         fprintf(stderr,"%5d  %5d  %5d	%20s  %7o\n", cptr->c_index,
		cptr->c_blink, cptr->c_name, cptr->c_name, cptr->c_flag);
   fflush(stderr);
   }
#endif					/* DeBugTrans */

/*
 * alcloc allocates a local symbol table entry, fills in fields with
 *  specified values and returns the new entry.
 */
static struct tlentry *alcloc(blink, name, flag)
struct tlentry *blink;
char *name;
int flag;
   {
   register struct tlentry *lp;

   lp = NewStruct(tlentry);
   lp->l_blink = blink;
   lp->l_name = name;
   lp->l_flag = flag;
   lp->l_next = NULL;
   if (lfirst == NULL) {
      lfirst = lp;
      lp->l_index = 0;
      }
   else {
      llast->l_next = lp;
      lp->l_index = llast->l_index + 1;
      }
   llast = lp;
   return lp;
   }

/*
 * alcglob allocates a global symbol table entry, fills in fields with
 *  specified values and returns offset of new entry.
 */
static struct tgentry *alcglob(blink, name, flag, nargs)
struct tgentry *blink;
char *name;
int flag, nargs;
   {
   register struct tgentry *gp;

   gp = NewStruct(tgentry);
   gp->g_blink = blink;
   gp->g_name = name;
   gp->g_flag = flag;
   gp->g_nargs = nargs;
   gp->g_next = NULL;
   if (gfirst == NULL) {
      gfirst = gp;
      gp->g_index = 0;
      }
   else {
      glast->g_next = gp;
      gp->g_index = glast->g_index + 1;
      }
   glast = gp;
   return gp;
   }

/*
 * alclit allocates a constant symbol table entry, fills in fields with
 *  specified values and returns the new entry.
 */
static struct tcentry *alclit(blink, name, len, flag)
struct tcentry *blink;
char *name;
int len, flag;
   {
   register struct tcentry *cp;

   cp = NewStruct(tcentry);
   cp->c_blink = blink;
   cp->c_name = name;
   cp->c_length = len;
   cp->c_flag = flag;
   cp->c_next = NULL;
   if (cfirst == NULL) {
      cfirst = cp;
      cp->c_index = 0;
      }
   else {
      clast->c_next = cp;
      cp->c_index = clast->c_index + 1;
      }
   clast = cp;
   return cp;
   }

/*
 * lout dumps local symbol table to fd, which is a .u1 file.
 */
void lout(fd)
FILE *fd;
   {
   register struct tlentry *lp;

   for (lp = lfirst; lp != NULL; lp = lp->l_next)
      writecheck(fprintf(fd, "\tlocal\t%d,%06o,%s\n",
         lp->l_index, lp->l_flag, lp->l_name));
   }

/*
 * constout dumps constant symbol table to fd, which is a .u1 file.
 */
void constout(fd)
FILE *fd;
   {
   register int l;
   register char *c;
   register struct tcentry *cp;

   for (cp = cfirst; cp != NULL; cp = cp->c_next) {
      writecheck(fprintf(fd, "\tcon\t%d,%06o", cp->c_index, cp->c_flag));
      if (cp->c_flag & F_IntLit)
         writecheck(fprintf(fd,",%d,%s\n",(int)strlen(cp->c_name),cp->c_name));
      else if (cp->c_flag & F_RealLit)
         writecheck(fprintf(fd, ",%s\n", cp->c_name));
      else {
         c = cp->c_name;
         l = cp->c_length;
         writecheck(fprintf(fd, ",%d", l));
         while (l--)
            writecheck(fprintf(fd, ",%03o", *c++ & 0377));
         writecheck(putc('\n', fd));
         }
      }
   }

/*
 * rout dumps a record declaration for name to file fd, which is a .u2 file.
 */
void rout(fd,name)
FILE *fd;
char *name;
   {
   register struct tlentry *lp;
   int n;

   if (llast == NULL)
      n = 0;
   else
      n = llast->l_index + 1;
   writecheck(fprintf(fd, "record\t%s,%d\n", name, n));
   for (lp = lfirst; lp != NULL; lp = lp->l_next)
      writecheck(fprintf(fd, "\t%d,%s\n", lp->l_index, lp->l_name));
   }

/*
 * gout writes various items to fd, which is a .u2 file.  These items
 *  include: implicit status, tracing activation, link directives,
 *  invocable directives, and the global table.
 */
void gout(fd)
FILE *fd;
   {
   register char *name;
   register struct tgentry *gp;
   int n;
   struct lfile *lfl;
   struct invkl *ivl;

   if (uwarn)
      name = "error";
   else
      name = "local";
   writecheck(fprintf(fd, "impl\t%s\n", name));
   if (trace)
      writecheck(fprintf(fd, "trace\n"));

   lfl = lfiles;
   while (lfl) {
      writecheck(fprintf(fd,"link\t%s.u1\n",lfl->lf_name));
      lfl = lfl->lf_link;
      }
   lfiles = 0;

   for (ivl = invkls; ivl != NULL; ivl = ivl->iv_link)
      writecheck(fprintf(fd, "invocable\t%s\n", ivl->iv_name));
   invkls = NULL;

   if (glast == NULL)
      n = 0;
   else
      n = glast->g_index + 1;
   writecheck(fprintf(fd, "global\t%d\n", n));
   for (gp = gfirst; gp != NULL; gp = gp->g_next)
      writecheck(fprintf(fd, "\t%d,%06o,%s,%d\n", gp->g_index, gp->g_flag,
         gp->g_name, gp->g_nargs));
   }
