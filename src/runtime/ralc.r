/*
 * File: ralc.r
 *  Contents: allocation routines
 */

/*
 * Prototypes.
 */
static struct region *findgap	(struct region *curr, word nbytes);
static struct region *newregion	(word nbytes, word stdsize);

extern word alcnum;

#ifndef MultiThread
word coexp_ser = 2;	/* serial numbers for co-expressions; &main is 1 */
word list_ser = 1;	/* serial numbers for lists */
word set_ser = 1;	/* serial numbers for sets */
word table_ser = 1;	/* serial numbers for tables */
#endif					/* MultiThread */


/*
 * AlcBlk - allocate a block.
 */
#begdef AlcBlk(var, struct_nm, t_code, nbytes)
{
#ifdef MultiThread
   EVVal((word)nbytes, typech[t_code]);
#endif					/* MultiThread */

   /*
    * Ensure that there is enough room in the block region.
    */
   if (DiffPtrs(blkend,blkfree) < nbytes && !reserve(Blocks, nbytes))
      return NULL;

   /*
    * If monitoring, show the allocation.
    */
#ifndef MultiThread
   EVVal((word)nbytes, typech[t_code]);
#endif

   /*
    * Decrement the free space in the block region by the number of bytes
    *  allocated and return the address of the first byte of the allocated
    *  block.
    */
   blktotal += nbytes;
   var = (struct struct_nm *)blkfree;
   blkfree += nbytes;
   var->title = t_code;
}
#enddef

/*
 * AlcFixBlk - allocate a fixed length block.
 */
#define AlcFixBlk(var, struct_nm, t_code)\
   AlcBlk(var, struct_nm, t_code, sizeof(struct struct_nm))

/*
 * AlcVarBlk - allocate a variable-length block.
 */
#begdef AlcVarBlk(var, struct_nm, t_code, n_desc)
   {
#ifdef EventMon
   uword size;
#else					/* EventMon */
   register uword size;
#endif					/* EventMon */

   /*
    * Variable size blocks are declared with one descriptor, thus
    *  we need add in only n_desc - 1 descriptors.
    */
   size = sizeof(struct struct_nm) + (n_desc - 1) * sizeof(struct descrip);
   AlcBlk(var, struct_nm, t_code, size)
   var->blksize = size;
   }
#enddef

/*
 * alcactiv - allocate a co-expression activation block.
 */

struct astkblk *alcactiv()
   {
   struct astkblk *abp;

   abp = (struct astkblk *)malloc(sizeof(struct astkblk));

   /*
    * If malloc failed, attempt to free some co-expression blocks and retry.
    */
   if (abp == NULL) {
      collect(Static);
      abp = (struct astkblk *)malloc(sizeof(struct astkblk));
      }

   if (abp == NULL)
      ReturnErrNum(305, NULL);
   abp->nactivators = 0;
   abp->astk_nxt = NULL;
   return abp;
   }

#ifdef LargeInts
/*
 * alcbignum - allocate an n-digit bignum in the block region
 */

struct b_bignum *alcbignum(n)
word n;
   {
   register struct b_bignum *blk;
   register uword size;

   size = sizeof(struct b_bignum) + ((n - 1) * sizeof(DIGIT));
   /* ensure whole number of words allocated */
   size = (size + WordSize - 1) & -WordSize;
   AlcBlk(blk, b_bignum, T_Lrgint, size);
   blk->blksize = size;
   blk->msd = blk->sign = 0;
   blk->lsd = n - 1;
   return blk;
   }
#endif					/* LargeInts */

/*
 * alccoexp - allocate a co-expression stack block.
 */

#if COMPILER
struct b_coexpr *alccoexp()
   {
   struct b_coexpr *ep;
   static int serial = 2; /* main co-expression is allocated elsewhere */
   ep = (struct b_coexpr *)malloc(stksize);

   /*
    * If malloc failed or if there have been too many co-expression allocations
    * since a collection, attempt to free some co-expression blocks and retry.
    */

   if (ep == NULL || alcnum > AlcMax) {
      collect(Static);
      ep = (struct b_coexpr *)malloc(stksize);
      }

   if (ep == NULL)
      ReturnErrNum(305, NULL);

   alcnum++;                    /* increment allocation count since last g.c. */

   ep->title = T_Coexpr;
   ep->size = 0;
   ep->id = serial++;
   ep->nextstk = stklist;
   ep->es_tend = NULL;
   ep->file_name = "";
   ep->line_num = 0;
   ep->freshblk = nulldesc;
   ep->es_actstk = NULL;
   ep->cstate[0] = 0;		/* zero the first two cstate words as a flag */
   ep->cstate[1] = 0;
   stklist = ep;
   return ep;
   }
#else					/* COMPILER */
#ifdef MultiThread
/*
 * If this is a new program being loaded, an icodesize>0 gives the
 * hdr.hsize and a stacksize to use; allocate
 * sizeof(progstate) + icodesize + mstksize
 * Otherwise (icodesize==0), allocate a normal stksize...
 */
struct b_coexpr *alccoexp(icodesize, stacksize)
long icodesize, stacksize;
#else					/* MultiThread */
struct b_coexpr *alccoexp()
#endif					/* MultiThread */

   {
   struct b_coexpr *ep;

#ifdef MultiThread
   if (icodesize > 0) {
      ep = (struct b_coexpr *)
	calloc(1, stacksize+
		       icodesize+
		       sizeof(struct progstate)+
		       sizeof(struct b_coexpr));
      }
   else
#endif					/* MultiThread */

   ep = (struct b_coexpr *)malloc(stksize);

   /*
    * If malloc failed or if there have been too many co-expression allocations
    * since a collection, attempt to free some co-expression blocks and retry.
    */

   if (ep == NULL || alcnum > AlcMax) {

      collect(Static);

#ifdef MultiThread
      if (icodesize>0) {
         ep = (struct b_coexpr *)
	    malloc(mstksize+icodesize+sizeof(struct progstate));
         }
      else
#endif					/* MultiThread */

         ep = (struct b_coexpr *)malloc(stksize);
      }
      if (ep == NULL)
         ReturnErrNum(305, NULL);

   alcnum++;		/* increment allocation count since last g.c. */

   ep->title = T_Coexpr;
   ep->es_actstk = NULL;
   ep->size = 0;
#ifdef MultiThread
   ep->es_pfp = NULL;
   ep->es_gfp = NULL;
   ep->es_argp = NULL;
   ep->tvalloc = NULL;

   if (icodesize > 0)
      ep->id = 1;
   else
#endif					/* MultiThread */
   ep->id = coexp_ser++;
   ep->nextstk = stklist;
   ep->es_tend = NULL;
   ep->cstate[0] = 0;		/* zero the first two cstate words as a flag */
   ep->cstate[1] = 0;

#ifdef MultiThread
   /*
    * Initialize program state to self for &main; curpstate for others.
    */
   if(icodesize>0) ep->program = (struct progstate *)(ep+1);
   else ep->program = curpstate;
#endif					/* MultiThread */

   stklist = ep;
   return ep;
   }
#endif					/* COMPILER */

/*
 * alccset - allocate a cset in the block region.
 */

struct b_cset *alccset()
   {
   register struct b_cset *blk;
   register int i;

   AlcFixBlk(blk, b_cset, T_Cset)
   blk->size = -1;              /* flag size as not yet computed */

   /*
    * Zero the bit array.
    */
   for (i = 0; i < CsetSize; i++)
     blk->bits[i] = 0;
   return blk;
   }

/*
 * alcfile - allocate a file block in the block region.
 */

struct b_file *alcfile(fd, status, name)
FILE *fd;
int status;
dptr name;
   {
   tended struct descrip tname = *name;
   register struct b_file *blk;

   AlcFixBlk(blk, b_file, T_File)
   blk->fd = fd;
   blk->status = status;
   blk->fname = tname;
   return blk;
   }

/*
 * alchash - allocate a hashed structure (set or table header) in the block
 *  region.
 */
union block *alchash(tcode)
int tcode;
   {
   register int i;
   register struct b_set *ps;
   register struct b_table *pt;

   if (tcode == T_Table) {
      AlcFixBlk(pt, b_table, T_Table);
      ps = (struct b_set *)pt;
      ps->id = table_ser++;
      }
   else {	/* tcode == T_Set */
      AlcFixBlk(ps, b_set, T_Set);
      ps->id = set_ser++;
      }
   ps->size = 0;
   ps->mask = 0;
   for (i = 0; i < HSegs; i++)
      ps->hdir[i] = NULL;
   return (union block *)ps;
   }

/*
 * alcsegment - allocate a slot block in the block region.
 */

struct b_slots *alcsegment(nslots)
word nslots;
   {
   uword size;
   register struct b_slots *blk;

   size = sizeof(struct b_slots) + WordSize * (nslots - HSlots);
   AlcBlk(blk, b_slots, T_Slots, size);
   blk->blksize = size;
   while (--nslots >= 0)
      blk->hslots[nslots] = NULL;
   return blk;
   }

/*
 * alclist - allocate a list header block in the block region.
 *
 *  Forces a g.c. if there's not enough room for the whole list.
 */

struct b_list *alclist(size)
uword size;
   {
   register struct b_list *blk;

   if (!reserve(Blocks, (word)(sizeof(struct b_list) + sizeof (struct b_lelem)
      + (size - 1) * sizeof(struct descrip)))) return NULL;
   AlcFixBlk(blk, b_list, T_List)
   blk->size = size;
   blk->id = list_ser++;
   blk->listhead = NULL;
   blk->listtail = NULL;
   return blk;
   }

/*
 * alclstb - allocate a list element block in the block region.
 */

struct b_lelem *alclstb(nslots, first, nused)
uword nslots, first, nused;
   {
   register struct b_lelem *blk;
   register word i;

   AlcVarBlk(blk, b_lelem, T_Lelem, nslots)
   blk->nslots = nslots;
   blk->first = first;
   blk->nused = nused;
   blk->listprev = NULL;
   blk->listnext = NULL;
   /*
    * Set all elements to &null.
    */
   for (i = 0; i < nslots; i++)
      blk->lslots[i] = nulldesc;
   return blk;
   }

/*
 * alcreal - allocate a real value in the block region.
 */

struct b_real *alcreal(val)
double val;
   {
   register struct b_real *blk;

   AlcFixBlk(blk, b_real, T_Real)

#ifdef Double
/* access real values one word at a time */
   { int *rp, *rq;
     rp = (int *) &(blk->realval);
     rq = (int *) &val;
     *rp++ = *rq++;
     *rp   = *rq;
   }
#else					/* Double */
   blk->realval = val;
#endif					/* Double */

   return blk;
   }

/*
 * alcrecd - allocate record with nflds fields in the block region.
 */

struct b_record *alcrecd(nflds, recptr)
int nflds;
union block *recptr;
   {
   tended union block *trecptr = recptr;
   register struct b_record *blk;

   AlcVarBlk(blk, b_record, T_Record, nflds)
   blk->recdesc = trecptr;
   blk->id = (((struct b_proc *)recptr)->recid)++;
   return blk;
   }

/*
 * alcrefresh - allocate a co-expression refresh block.
 */

#if COMPILER
struct b_refresh *alcrefresh(na, nl, nt, wrk_sz)
int na;
int nl;
int nt;
int wrk_sz;
   {
   struct b_refresh *blk;

   AlcVarBlk(blk, b_refresh, T_Refresh, na + nl)
   blk->nlocals = nl;
   blk->nargs = na;
   blk->ntemps = nt;
   blk->wrk_size = wrk_sz;
   return blk;
   }
#else					/* COMPILER */
struct b_refresh *alcrefresh(entryx, na, nl)
word *entryx;
int na, nl;
   {
   struct b_refresh *blk;

   AlcVarBlk(blk, b_refresh, T_Refresh, na + nl);
   blk->ep = entryx;
   blk->numlocals = nl;
   return blk;
   }
#endif					/* COMPILER */

/*
 * alcselem - allocate a set element block.
 */

struct b_selem *alcselem(mbr,hn)
uword hn;
dptr mbr;

   {
   tended struct descrip tmbr = *mbr;
   register struct b_selem *blk;

   AlcFixBlk(blk, b_selem, T_Selem)
   blk->clink = NULL;
   blk->setmem = tmbr;
   blk->hashnum = hn;
   return blk;
   }

/*
 * alcstr - allocate a string in the string space.
 */

char *alcstr(s, slen)
register char *s;
register word slen;
   {
   tended struct descrip ts;
   register char *d;
   char *ofree;

#ifdef MultiThread
   StrLen(ts) = slen;
   StrLoc(ts) = s;
#ifdef EventMon
   if (!noMTevents)
#endif					/* EventMon */
      EVVal(slen, E_String);
   s = StrLoc(ts);
#endif					/* MultiThread */

   /*
    * Make sure there is enough room in the string space.
    */
   if (DiffPtrs(strend,strfree) < slen) {
      StrLen(ts) = slen;
      StrLoc(ts) = s;
      if (!reserve(Strings, slen))
         return NULL;
      s = StrLoc(ts);
      }

   strtotal += slen;

   /*
    * Copy the string into the string space, saving a pointer to its
    *  beginning.  Note that s may be null, in which case the space
    *  is still to be allocated but nothing is to be copied into it.
    */
   ofree = d = strfree;
   if (s) {
      while (slen-- > 0)
         *d++ = *s++;
      }
   else
      d += slen;

   strfree = d;
   return ofree;
   }

/*
 * alcsubs - allocate a substring trapped variable in the block region.
 */

struct b_tvsubs *alcsubs(len, pos, var)
word len, pos;
dptr var;
   {
   tended struct descrip tvar = *var;
   register struct b_tvsubs *blk;

   AlcFixBlk(blk, b_tvsubs, T_Tvsubs)
   blk->sslen = len;
   blk->sspos = pos;
   blk->ssvar = tvar;
   return blk;
   }

/*
 * alctelem - allocate a table element block in the block region.
 */

struct b_telem *alctelem()
   {
   register struct b_telem *blk;

   AlcFixBlk(blk, b_telem, T_Telem)
   blk->hashnum = 0;
   blk->clink = NULL;
   blk->tref = nulldesc;
   return blk;
   }

/*
 * alctvtbl - allocate a table element trapped variable block in the block
 *  region.
 */

struct b_tvtbl *alctvtbl(tbl, ref, hashnum)
register dptr tbl, ref;
uword hashnum;
   {
   tended struct descrip ttbl = *tbl;
   tended struct descrip tref = *ref;
   register struct b_tvtbl *blk;

   AlcFixBlk(blk, b_tvtbl, T_Tvtbl)
   blk->hashnum = hashnum;
   blk->clink = BlkLoc(ttbl);
   blk->tref = tref;
   return blk;
   }

/*
 * deallocate - return a block to the heap.
 *
 *  The block must be the one that is at the very end of a block region.
 */
void deallocate (bp)
union block *bp;
{
   word nbytes;
   struct region *rp;

   nbytes = BlkSize(bp);
   for (rp = curblock; rp; rp = rp->next)
      if ((char *)bp + nbytes == rp->free)
         break;
   if (!rp)
      for (rp = curblock->prev; rp; rp = rp->prev)
	 if ((char *)bp + nbytes == rp->free)
            break;
   if (!rp)
      syserr ("deallocation botch");
   rp->free = (char *)bp;
   blktotal -= nbytes;
   EVVal(nbytes, E_BlkDeAlc);
}

/*
 * reserve -- ensure space in either string or block region.
 *
 *   1. check for space in current region.
 *   2. check for space in older regions.
 *   3. check for space in newer regions.
 *   4. set goal of 10% of size of newest region.
 *   5. collect regions, newest to oldest, until goal met.
 *   6. allocate new region at 200% the size of newest existing.
 *   7. reset goal back to original request.
 *   8. collect regions that were too small to bother with before.
 *   9. search regions, newest to oldest.
 *  10. give up and signal error.
 */

char *reserve(region, nbytes)
int region;
word nbytes;
{
   struct region **pcurr, *curr, *rp;
   word want, newsize;
   extern int qualfail;

   if (region == Strings)
      pcurr = &curstring;
   else
      pcurr = &curblock;
   curr = *pcurr;

   /*
    * Check for space available now.
    */
   if (DiffPtrs(curr->end, curr->free) >= nbytes)
      return curr->free;		/* quick return: current region is OK */

   if ((rp = findgap(curr, nbytes)) != 0) {    /* check all regions on chain */
      *pcurr = rp;			/* switch regions */
      return rp->free;
      }

   /*
    * Set "curr" to point to newest region.
    */
   while (curr->next)
      curr = curr->next;

   /*
    * Need to collect garbage.  To reduce thrashing, set a minimum requirement
    *  of 10% of the size of the newest region, and collect regions until that
    *  amount of free space appears in one of them.
    */
   want = (curr->size / 100) * memcushion;
   if (want < nbytes)
      want = nbytes;

   for (rp = curr; rp; rp = rp->prev)
      if (rp->size >= want) {	/* if large enough to possibly succeed */
         *pcurr = rp;
         collect(region);
         if (DiffPtrs(rp->end,rp->free) >= want)
            return rp->free;
         }

   /*
    * That didn't work.  Allocate a new region with a size based on the
    * newest previous region.
    */
   newsize = (curr->size / 100) * memgrowth;
   if (newsize < nbytes)
      newsize = nbytes;
   if (newsize < MinAbrSize)
      newsize = MinAbrSize;

   if ((rp = newregion(nbytes, newsize)) != 0) {
      rp->prev = curr;
      rp->next = NULL;
      curr->next = rp;
      rp->Gnext = curr;
      rp->Gprev = curr->Gprev;
      if (curr->Gprev) curr->Gprev->Gnext = rp;
      curr->Gprev = rp;
      *pcurr = rp;
#ifdef EventMon
      if (!noMTevents) {
         if (region == Strings) {
            EVVal(rp->size, E_TenureString);
            }
         else {
            EVVal(rp->size, E_TenureBlock);
            }
         }
#endif					/* EventMon */
      return rp->free;
      }

   /*
    * Allocation failed.  Try to continue, probably thrashing all the way.
    *  Collect the regions that weren't collected before and see if any
    *  region has enough to satisfy the original request.
    */
   for (rp = curr; rp; rp = rp->prev)
      if (rp->size < want) {		/* if not collected earlier */
         *pcurr = rp;
         collect(region);
         if (DiffPtrs(rp->end,rp->free) >= want)
            return rp->free;
         }
   if ((rp = findgap(curr, nbytes)) != 0) {
      *pcurr = rp;
      return rp->free;
      }

   /*
    * All attempts failed.
    */
   if (region == Blocks)
      ReturnErrNum(307, 0);
   else if (qualfail)
      ReturnErrNum(304, 0);
   else
      ReturnErrNum(306, 0);
}

/*
 * findgap - search region chain for a region having at least nbytes available
 */
static struct region *findgap(curr, nbytes)
struct region *curr;
word nbytes;
   {
   struct region *rp;

   for (rp = curr; rp; rp = rp->prev)
      if (DiffPtrs(rp->end, rp->free) >= nbytes)
         return rp;
   for (rp = curr->next; rp; rp = rp->next)
      if (DiffPtrs(rp->end, rp->free) >= nbytes)
         return rp;
   return NULL;
   }

/*
 * newregion - try to malloc a new region and tenure the old one,
 *  backing off if the requested size fails.
 */
static struct region *newregion(nbytes,stdsize)
word nbytes,stdsize;
{
   uword minSize = MinAbrSize;
   struct region *rp;

   if ((uword)nbytes > minSize)
      minSize = (uword)nbytes;
   rp = (struct region *)malloc(sizeof(struct region));
   if (rp) {
      rp->size = stdsize;
      if (rp->size < nbytes)
         rp->size = Max(nbytes+stdsize, nbytes);
      do {
         rp->free = rp->base = (char *)AllocReg(rp->size);
         if (rp->free != NULL) {
            rp->end = rp->base + rp->size;
            return rp;
            }
         else {
            }
         rp->size = (rp->size + nbytes)/2 - 1;
         }
      while (rp->size >= minSize);
      free((char *)rp);
      }
   return NULL;
}
