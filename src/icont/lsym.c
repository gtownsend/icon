/*
 * lsym.c -- functions for symbol table manipulation.
 */

#include "link.h"
#include "tproto.h"
#include "tglobals.h"

/*
 * Prototypes.
 */

static struct	fentry *alcfhead
   (struct fentry *blink, word name, int fid, struct rentry *rlist);
static struct	rentry *alcfrec
   (struct rentry *link, struct gentry *gp, int fnum);
static struct	gentry *alcglobal
   (struct gentry *blink, word name, int flag, int nargs, int procid);
static struct	ientry *alcident	(char *nam, int len);

int dynoff;			/* stack offset counter for locals */
int argoff;			/* stack offset counter for arguments */
int static1;			/* first static in procedure */
int lstatics = 0;		/* static variable counter */

int nlocal;			/* number of locals in local table */
int nconst;			/* number of constants in constant table */
int nfields = 0;		/* number of fields in field table */

/*
 * instid - copy the string s to the start of the string free space
 *  and call putident with the length of the string.
 */
word instid(s)
char *s;
   {
   register int l;
   register word indx;
   register char *p;

   indx = lsfree;
   p = s;
   l = 0;
   do {
      if (indx >= stsize)
         lsspace = (char *)trealloc(lsspace, NULL, &stsize, 1, 1,
            "string space");
      l++;
      } while ((lsspace[indx++] = *p++) != 0);

   return putident(l, 1);
   }

/*
 * putident - install the identifier named by the string starting at lsfree
 *  and extending for len bytes.  The installation entails making an
 *  entry in the identifier hash table and then making an identifier
 *  table entry for it with alcident.  A side effect of installation
 *  is the incrementing of lsfree by the length of the string, thus
 *  "saving" it.
 *
 * Nothing is changed if the identifier has already been installed.
 *
 * If "install" is 0, putident returns -1 for a nonexistent identifier,
 * and does not install it.
 */
word putident(len, install)
int len, install;
   {
   register int hash;
   register char *s;
   register struct ientry *ip;
   int l;

   /*
    * Compute hash value by adding bytes and masking result with imask.
    *  (Recall that imask is ihsize-1.)
    */
   s = &lsspace[lsfree];
   hash = 0;
   l = len;
   while (l--)
      hash += *s++;
   l = len;
   s = &lsspace[lsfree];
   hash &= imask;
   /*
    * If the identifier hasn't been installed, install it.
    */
   if ((ip = lihash[hash]) != NULL) {	 /* collision */
      for (;;) {
         /*
          * follow i_blink chain until id is found or end of chain reached
          */
         if (l == ip->i_length && lexeql(l, s, &lsspace[ip->i_name]))
            return ip->i_name;		/* id is already installed, return it */
         if (ip->i_blink == NULL) {	/* end of chain */
            if (install == 0)
               return -1;
            ip->i_blink = alcident(s, l);
            lsfree += l;
            return ip->i_blink->i_name;
            }
         ip = ip->i_blink;
         }
      }
   /*
    * Hashed to an empty slot.
    */
   if (install == 0)
      return -1;
   lihash[hash] = alcident(s, l);
   lsfree += l;
   return lihash[hash]->i_name;
   }

/*
 * lexeql - compare two strings of given length.  Returns non-zero if
 *  equal, zero if not equal.
 */
int lexeql(l, s1, s2)
register int l;
register char *s1, *s2;
   {
   while (l--)
      if (*s1++ != *s2++)
         return 0;
   return 1;
   }

/*
 * alcident - get the next free identifier table entry, and fill it in with
 *  the specified values.
 */
static struct ientry *alcident(nam, len)
char *nam;
int len;
   {
   register struct ientry *ip;

   ip = NewStruct(ientry);
   ip->i_blink = NULL;
   ip->i_name = (word)(nam - lsspace);
   ip->i_length = len;
   return ip;
   }

/*
 * locinit -  clear local symbol table.
 */
void locinit()
   {
   dynoff = 0;
   argoff = 0;
   nlocal = -1;
   nconst = -1;
   static1 = lstatics;
   }

/*
 * putlocal - make a local symbol table entry.
 */
void putlocal(n, id, flags, imperror, procname)
int n;
word id;
register int flags;
int imperror;
word procname;
   {
   register struct lentry *lp;
   union {
      struct gentry *gp;
      int bn;
      } p;

   if (n >= lsize)
      lltable  = (struct lentry *)trealloc(lltable, NULL, &lsize,
         sizeof(struct lentry), 1, "local symbol table");
   if (n > nlocal)
      nlocal = n;
   lp = &lltable[n];
   lp->l_name = id;
   lp->l_flag = flags;
   if (flags == 0) {				/* undeclared */
      if ((p.gp = glocate(id)) != NULL) {	/* check global */
         lp->l_flag = F_Global;
         lp->l_val.global = p.gp;
         }

      else if ((p.bn = blocate(id)) != 0) {	/* check for function */
         lp->l_flag = F_Builtin | F_Global;
         lp->l_val.global = putglobal(id, F_Builtin | F_Proc, -1, p.bn);
         }

      else {					/* implicit local */
         if (imperror)
            lwarn(&lsspace[id], "undeclared identifier, procedure ",
               &lsspace[procname]);
         lp->l_flag = F_Dynamic;
         lp->l_val.offset = ++dynoff;
         }
      }
   else if (flags & F_Global) {			/* global variable */
      if ((p.gp = glocate(id)) == NULL)
         quit("putlocal: global not in global table");
      lp->l_val.global = p.gp;
      }
   else if (flags & F_Argument)			/* procedure argument */
      lp->l_val.offset = ++argoff;
   else if (flags & F_Dynamic)			/* local dynamic */
      lp->l_val.offset = ++dynoff;
   else if (flags & F_Static)			/* local static */
      lp->l_val.staticid = ++lstatics;
   else
      quit("putlocal: unknown flags");
   }

/*
 * putglobal - make a global symbol table entry.
 */
struct gentry *putglobal(id, flags, nargs, procid)
word id;
int flags;
int nargs;
int procid;
   {
   register struct gentry *p;

   flags |= F_Global;
   if ((p = glocate(id)) == NULL) {	/* add to head of hash chain */
      p = lghash[ghasher(id)];
      lghash[ghasher(id)] = alcglobal(p, id, flags, nargs, procid);
      return lghash[ghasher(id)];
      }
   p->g_flag |= flags;
   p->g_nargs = nargs;
   p->g_procid = procid;
   return p;
   }

/*
 * putconst - make a constant symbol table entry.
 */
void putconst(n, flags, len, pc, valp)
int n;
int flags, len;
word pc;
union xval *valp;

   {
   register struct centry *p;
   if (n >= csize)
      lctable  = (struct centry *)trealloc(lctable, NULL, &csize,
         sizeof(struct centry), 1, "constant table");
   if (nconst < n)
      nconst = n;
   p = &lctable[n];
   p->c_flag = flags;
   p->c_pc = pc;
   if (flags & F_IntLit) {
      p->c_val.ival = valp->ival;
      }
   else if (flags & F_StrLit) {
      p->c_val.sval = valp->sval;
      p->c_length = len;
      }
   else if (flags & F_CsetLit) {
      p->c_val.sval = valp->sval;
      p->c_length = len;
      }
   else	if (flags & F_RealLit) {
      #ifdef Double
         /*
          *  Access real values one word at a time.
          */
         int *rp, *rq;
         rp = (int *) &(p->c_val.rval);
         rq = (int *) &(valp->rval);
         *rp++ = *rq++;
         *rp   = *rq;
      #else				/* Double */
         p->c_val.rval = valp->rval;
      #endif				/* Double */
      }
   else
      fprintf(stderr, "putconst: bad flags: %06o %011lo\n", flags, valp->ival);
   }

/*
 * putfield - make a record/field table entry.
 */
void putfield(fname, gp, fnum)
word fname;
struct gentry *gp;
int fnum;
   {
   register struct fentry *fp;
   register struct rentry *rp, *rp2;
   word hash;

   fp = flocate(fname);
   if (fp == NULL) {		/* create a field entry */
      nfields++;
      hash = fhasher(fname);
      fp = lfhash[hash];
      lfhash[hash] = alcfhead(fp, fname, nfields, alcfrec((struct rentry *)NULL,
         gp, fnum));
      return;
      }
   rp = fp->f_rlist;				/* found field entry; */
   if (rp->r_gp->g_procid > gp->g_procid) {	/* find spot in record list */
      fp->f_rlist = alcfrec(rp, gp, fnum);
      return;
      }
   while (rp->r_gp->g_procid < gp->g_procid) {	/* keep record list ascending */
      if (rp->r_link == NULL) {
         rp->r_link = alcfrec((struct rentry *)NULL, gp, fnum);
         return;
         }
      rp2 = rp;
      rp = rp->r_link;
      }
   rp2->r_link = alcfrec(rp, gp, fnum);
   }

/*
 * glocate - lookup identifier in global symbol table, return NULL
 *  if not present.
 */
struct gentry *glocate(id)
word id;
   {
   register struct gentry *p;

   p = lghash[ghasher(id)];
   while (p != NULL && p->g_name != id)
      p = p->g_blink;
   return p;
   }

/*
 * flocate - lookup identifier in field table.
 */
struct fentry *flocate(id)
word id;
   {
   register struct fentry *p;

   p = lfhash[fhasher(id)];
   while (p != NULL && p->f_name != id)
      p = p->f_blink;
   return p;
   }

/*
 * alcglobal - create a new global symbol table entry.
 */
static struct gentry *alcglobal(blink, name, flag, nargs, procid)
struct gentry *blink;
word name;
int flag;
int nargs;
int procid;
   {
   register struct gentry *gp;

   gp = NewStruct(gentry);
   gp->g_blink = blink;
   gp->g_name = name;
   gp->g_flag = flag;
   gp->g_nargs = nargs;
   gp->g_procid = procid;
   gp->g_next = NULL;
   if (lgfirst == NULL) {
      lgfirst = gp;
      gp->g_index = 0;
      }
   else {
      lglast->g_next = gp;
      gp->g_index = lglast->g_index + 1;
      }
   lglast = gp;
   return gp;
   }

/*
 * alcfhead - allocate a field table header.
 */
static struct fentry *alcfhead(blink, name, fid, rlist)
struct fentry *blink;
word name;
int fid;
struct rentry *rlist;
   {
   register struct fentry *fp;

   fp = NewStruct(fentry);
   fp->f_blink = blink;
   fp->f_name = name;
   fp->f_fid = fid;
   fp->f_rlist = rlist;
   fp->f_nextentry = NULL;
   if (lffirst == NULL)
      lffirst = fp;
   else
      lflast->f_nextentry = fp;
   lflast = fp;
   return fp;
   }

/*
 * alcfrec - allocate a field table record list element.
 */
static struct rentry *alcfrec(link, gp, fnum)
struct rentry *link;
struct gentry *gp;
int fnum;
   {
   register struct rentry *rp;

   rp = NewStruct(rentry);
   rp->r_link = link;
   rp->r_gp = gp;
   rp->r_fnum = fnum;
   return rp;
   }

/*
 * blocate - search for a function. The search is linear to make
 *  it easier to add/delete functions. If found, returns index+1 for entry.
 */

int blocate(s_indx)
word s_indx;
   {
register char *s;
   register int i;
   extern char *ftable[];
   extern int ftbsize;

   s = &lsspace[s_indx];
   for (i = 0; i < ftbsize; i++)
      if (strcmp(ftable[i], s) == 0)
         return i + 1;
   return 0;
   }
