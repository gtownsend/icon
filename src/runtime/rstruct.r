/*
 * File: rstruct.r
 *  Contents: addmem, cpslots, cplist, cpset, hmake, hchain, hfirst, hnext,
 *  hgrow, hshrink, memb
 */

/*
 * addmem - add a new set element block in the correct spot in
 *  the bucket chain.
 */

void addmem(ps,pe,pl)
union block **pl;
struct b_set *ps;
struct b_selem *pe;
   {
   ps->size++;
   if (*pl != NULL )
      pe->clink = *pl;
   *pl = (union block *) pe;
   }

/*
 * cpslots(dp1, slotptr, i, j) - copy elements of sublist dp1[i:j]
 *  into an array of descriptors.
 */

void cpslots(dp1, slotptr, i, j)
dptr dp1, slotptr;
word i, j;
   {
   word size;
   tended struct b_list *lp1;
   tended struct b_lelem *bp1;
   /*
    * Get pointers to the list and list elements for the source list
    *  (bp1, lp1).
    */
   lp1 = (struct b_list *) BlkLoc(*dp1);
   bp1 = (struct b_lelem *) lp1->listhead;
   size = j - i;

   /*
    * Locate the block containing element i in the source list.
    */
   if (size > 0) {
      while (i > bp1->nused) {
         i -= bp1->nused;
         bp1 = (struct b_lelem *) bp1->listnext;
         }
      }

   /*
    * Copy elements from the source list into the sublist, moving to
    *  the next list block in the source list when all elements in a
    *  block have been copied.
    */
   while (size > 0) {
      j = bp1->first + i - 1;
      if (j >= bp1->nslots)
         j -= bp1->nslots;
      *slotptr++ = bp1->lslots[j];
      if (++i > bp1->nused) {
         i = 1;
         bp1 = (struct b_lelem *) bp1->listnext;
         }
      size--;
      }
   }


/*
 * cplist(dp1,dp2,i,j) - copy sublist dp1[i:j] into dp2.
 */

int cplist(dp1, dp2, i, j)
dptr dp1, dp2;
word i, j;
   {
   word size, nslots;
   tended struct b_list *lp2;
   tended struct b_lelem *bp2;

   /*
    * Calculate the size of the sublist.
    */
   size = nslots = j - i;
   if (nslots == 0)
      nslots = MinListSlots;

   Protect(lp2 = (struct b_list *) alclist(size), return Error);
   Protect(bp2 = (struct b_lelem *)alclstb(nslots,(word)0,size), return Error);
   lp2->listhead = lp2->listtail = (union block *) bp2;
#ifdef ListFix
   bp2->listprev = bp2->listnext = (union block *) lp2;
#endif					/* ListFix */

   cpslots(dp1, bp2->lslots, i, j);

   /*
    * Fix type and location fields for the new list.
    */
   dp2->dword = D_List;
   BlkLoc(*dp2) = (union block *) lp2;
   EVValD(dp2, E_Lcreate);
   return Succeeded;
   }

#ifdef TableFix
/*
 * cpset(dp1,dp2,n) - copy set dp1 to dp2, reserving memory for n entries.
 */
int cpset(dp1, dp2, n)
dptr dp1, dp2;
word n;
   {
   int i = cphash(dp1, dp2, n, T_Set);
   EVValD(dp2, E_Screate);
   return i;
   }

int cptable(dp1, dp2, n)
dptr dp1, dp2;
word n;
   {
   int i = cphash(dp1, dp2, n, T_Table);
   BlkLoc(*dp2)->table.defvalue = BlkLoc(*dp1)->table.defvalue;
   EVValD(dp2, E_Tcreate);
   return i;
   }

int cphash(dp1, dp2, n, tcode)
dptr dp1, dp2;
word n;
int tcode;
   {
   union block *src;
   tended union block *dst;
   tended struct b_slots *seg;
   tended struct b_selem *ep, *prev;
   struct b_selem *se;
   register word slotnum;
   register int i;

   /*
    * Make a new set organized like dp1, with room for n elements.
    */
   dst = hmake(tcode, BlkLoc(*dp1)->set.mask + 1, n);
   if (dst == NULL)
      return Error;
   /*
    * Copy the header and slot blocks.
    */
   src = BlkLoc(*dp1);
   dst->set.size = src->set.size;	/* actual set size */
   dst->set.mask = src->set.mask;	/* hash mask */
   for (i = 0; i < HSegs && src->set.hdir[i] != NULL; i++)
      memcpy((char *)dst->set.hdir[i], (char *)src->set.hdir[i],
         src->set.hdir[i]->blksize);
   /*
    * Work down the chain of element blocks in each bucket
    *	and create identical chains in new set.
    */
   for (i = 0; i < HSegs && (seg = dst->set.hdir[i]) != NULL; i++)
      for (slotnum = segsize[i] - 1; slotnum >= 0; slotnum--)  {
	 prev = NULL;
         for (ep = (struct b_selem *)seg->hslots[slotnum];
	      ep != NULL && BlkType(ep) != T_Table;
	      ep = (struct b_selem *)ep->clink) {
	    if (tcode == T_Set) {
               Protect(se = alcselem(&ep->setmem, ep->hashnum), return Error);
               se->clink = ep->clink;
	       }
	    else {
	       Protect(se = (struct b_selem *)alctelem(), return Error);
	       *(struct b_telem *)se = *(struct b_telem *)ep; /* copy table entry */
	       if (BlkType(se->clink) == T_Table)
		  se->clink = dst;
	       }
	    if (prev == NULL)
		seg->hslots[slotnum] = (union block *)se;
	    else
		prev->clink = (union block *)se;
	    prev = se;
            }
         }
   dp2->dword = tcode | D_Typecode | F_Ptr;
   BlkLoc(*dp2) = dst;
   if (TooSparse(dst))
      hshrink(dst);
   return Succeeded;
   }
#else					/* TableFix */
/*
 * cpset(dp1,dp2,n) - copy set dp1 to dp2, reserving memory for n entries.
 */
int cpset(dp1, dp2, n)
dptr dp1, dp2;
word n;
   {
   union block *src;
   tended union block *dst;
   tended struct b_slots *seg;
   tended struct b_selem *ep, *prev;
   struct b_selem *se;
   register word slotnum;
   register int i;

   /*
    * Make a new set organized like dp1, with room for n elements.
    */
   dst = hmake(T_Set, BlkLoc(*dp1)->set.mask + 1, n);
   if (dst == NULL)
      return Error;
   /*
    * Copy the header and slot blocks.
    */
   src = BlkLoc(*dp1);
   dst->set.size = src->set.size;	/* actual set size */
   dst->set.mask = src->set.mask;	/* hash mask */
   for (i = 0; i < HSegs && src->set.hdir[i] != NULL; i++)
      memcpy((char *)dst->set.hdir[i], (char *)src->set.hdir[i],
         src->set.hdir[i]->blksize);
   /*
    * Work down the chain of element blocks in each bucket
    *	and create identical chains in new set.
    */
   for (i = 0; i < HSegs && (seg = dst->set.hdir[i]) != NULL; i++)
      for (slotnum = segsize[i] - 1; slotnum >= 0; slotnum--)  {
	 prev = NULL;
         for (ep = (struct b_selem *)seg->hslots[slotnum];
	       ep != NULL; ep = (struct b_selem *)ep->clink) {
            Protect(se = alcselem(&ep->setmem, ep->hashnum), return Error);
	    if (prev == NULL)
		seg->hslots[slotnum] = (union block *)se;
	    else
		prev->clink = (union block *)se;
            se->clink = ep->clink;
	    prev = se;
            }
         }
   dp2->dword = D_Set;
   BlkLoc(*dp2) = dst;
   if (TooSparse(dst))
      hshrink(dst);
   Desc_EVValD(dst, E_Screate, D_Set);
   return Succeeded;
   }
#endif					/* TableFix */

/*
 * hmake - make a hash structure (Set or Table) with a given number of slots.
 *  If *nslots* is zero, a value appropriate for *nelem* elements is chosen.
 *  A return of NULL indicates allocation failure.
 */
union block *hmake(tcode, nslots, nelem)
int tcode;
word nslots, nelem;
   {
   word seg, t, blksize, elemsize;
   tended union block *blk;
   struct b_slots *segp;

   if (nslots == 0)
      nslots = (nelem + MaxHLoad - 1) / MaxHLoad;
   for (seg = t = 0; seg < (HSegs - 1) && (t += segsize[seg]) < nslots; seg++)
      ;
   nslots = ((word)HSlots) << seg;	/* ensure legal power of 2 */
   if (tcode == T_Table) {
      blksize = sizeof(struct b_table);
      elemsize = sizeof(struct b_telem);
      }
   else {	/* T_Set */
      blksize = sizeof(struct b_set);
      elemsize = sizeof(struct b_selem);
      }
   if (!reserve(Blocks, (word)(blksize + (seg + 1) * sizeof(struct b_slots)
      + (nslots - HSlots * (seg + 1)) * sizeof(union block *)
      + nelem * elemsize))) return NULL;
   Protect(blk = alchash(tcode), return NULL);
   for (; seg >= 0; seg--) {
      Protect(segp = alcsegment(segsize[seg]), return NULL);
      blk->set.hdir[seg] = segp;
#ifdef TableFix
      if (tcode == T_Table) {
	 int j;
	 for (j = 0; j < segsize[seg]; j++)
	    segp->hslots[j] = blk;
         }
#endif					/* TableFix */
      }
   blk->set.mask = nslots - 1;
   return blk;
   }

/*
 * hchain - return a pointer to the word that points to the head of the hash
 *  chain for hash number hn in hashed structure s.
 */

/*
 * lookup table for log to base 2; must have powers of 2 through (HSegs-1)/2.
 */
static unsigned char log2h[] = {
   0,1,2,2, 3,3,3,3, 4,4,4,4, 4,4,4,4, 5,5,5,5, 5,5,5,5, 5,5,5,5, 5,5,5,5,
   6,6,6,6, 6,6,6,6, 6,6,6,6, 6,6,6,6, 6,6,6,6, 6,6,6,6, 6,6,6,6, 6,6,6,6,
   7,7,7,7, 7,7,7,7, 7,7,7,7, 7,7,7,7, 7,7,7,7, 7,7,7,7, 7,7,7,7, 7,7,7,7,
   7,7,7,7, 7,7,7,7, 7,7,7,7, 7,7,7,7, 7,7,7,7, 7,7,7,7, 7,7,7,7, 7,7,7,7,
   8,8,8,8, 8,8,8,8, 8,8,8,8, 8,8,8,8, 8,8,8,8, 8,8,8,8, 8,8,8,8, 8,8,8,8,
   8,8,8,8, 8,8,8,8, 8,8,8,8, 8,8,8,8, 8,8,8,8, 8,8,8,8, 8,8,8,8, 8,8,8,8,
   8,8,8,8, 8,8,8,8, 8,8,8,8, 8,8,8,8, 8,8,8,8, 8,8,8,8, 8,8,8,8, 8,8,8,8,
   8,8,8,8, 8,8,8,8, 8,8,8,8, 8,8,8,8, 8,8,8,8, 8,8,8,8, 8,8,8,8, 8,8,8,8,
   9,9,9,9, 9,9,9,9, 9,9,9,9, 9,9,9,9, 9,9,9,9, 9,9,9,9, 9,9,9,9, 9,9,9,9,
   9,9,9,9, 9,9,9,9, 9,9,9,9, 9,9,9,9, 9,9,9,9, 9,9,9,9, 9,9,9,9, 9,9,9,9,
   9,9,9,9, 9,9,9,9, 9,9,9,9, 9,9,9,9, 9,9,9,9, 9,9,9,9, 9,9,9,9, 9,9,9,9,
   9,9,9,9, 9,9,9,9, 9,9,9,9, 9,9,9,9, 9,9,9,9, 9,9,9,9, 9,9,9,9, 9,9,9,9,
   9,9,9,9, 9,9,9,9, 9,9,9,9, 9,9,9,9, 9,9,9,9, 9,9,9,9, 9,9,9,9, 9,9,9,9,
   9,9,9,9, 9,9,9,9, 9,9,9,9, 9,9,9,9, 9,9,9,9, 9,9,9,9, 9,9,9,9, 9,9,9,9,
   9,9,9,9, 9,9,9,9, 9,9,9,9, 9,9,9,9, 9,9,9,9, 9,9,9,9, 9,9,9,9, 9,9,9,9,
   9,9,9,9, 9,9,9,9, 9,9,9,9, 9,9,9,9, 9,9,9,9, 9,9,9,9, 9,9,9,9, 9,9,9,9,
   };

union block **hchain(pb, hn)
union block *pb;
register uword hn;
   {
   register struct b_set *ps;
   register word slotnum, segnum, segslot;

   ps = (struct b_set *)pb;
   slotnum = hn & ps->mask;
   if (slotnum >= HSlots * sizeof(log2h))
      segnum = log2h[slotnum >> (LogHSlots + HSegs/2)] + HSegs/2;
   else
      segnum = log2h[slotnum >> LogHSlots];
   segslot = hn & (segsize[segnum] - 1);
   return &ps->hdir[segnum]->hslots[segslot];
   }

/*
 * hgfirst - initialize for generating set or table, and return first element.
 */

union block *hgfirst(bp, s)
union block *bp;
struct hgstate *s;
   {
   int i;

   s->segnum = 0;				/* set initial state */
   s->slotnum = -1;
   s->tmask = bp->table.mask;
   for (i = 0; i < HSegs; i++)
      s->sghash[i] = s->sgmask[i] = 0;
   return hgnext(bp, s, (union block *)0);	/* get and return first value */
   }

/*
 * hgnext - return the next element of a set or table generation sequence.
 *
 *  We carefully generate each element exactly once, even if the hash chains
 *  are split between calls.  We do this by recording the state of things at
 *  the time of the split and checking past history when starting to process
 *  a new chain.
 *
 *  Elements inserted or deleted between calls may or may not be generated.
 *
 *  We assume that no structure *shrinks* after its initial creation; they
 *  can only *grow*.
 */

union block *hgnext(bp, s, ep)
union block *bp;
struct hgstate *s;
union block *ep;
   {
   int i;
   word d, m;
   uword hn;

   /*
    * Check to see if the set or table's hash buckets were split (once or
    *  more) since the last call.  We notice this unless the next entry
    *  has same hash value as the current one, in which case we defer it
    *  by doing nothing now.
    */
#ifdef TableFix
   if (bp->table.mask != s->tmask &&
	  (ep->selem.clink == NULL || BlkType(ep->telem.clink) == T_Table ||
	  ep->telem.clink->telem.hashnum != ep->telem.hashnum)) {
#else					/* TableFix */
   if (bp->table.mask != s->tmask &&
	  (ep->selem.clink == NULL ||
	  ep->telem.clink->telem.hashnum != ep->telem.hashnum)) {
#endif					/* TableFix */
      /*
       * Yes, they did split.  Make a note of the current state.
       */
      hn = ep->telem.hashnum;
      for (i = 1; i < HSegs; i++)
         if ((((word)HSlots) << (i - 1)) > s->tmask) {
	 /*
	  * For the newly created segments only, save the mask and
	  *  hash number being processed at time of creation.
	  */
	 s->sgmask[i] = s->tmask;
	 s->sghash[i] = hn;
         }
      s->tmask = bp->table.mask;
      /*
       * Find the next element in our original segment by starting
       *  from the beginning and skipping through the current hash
       *  number.  We can't just follow the link from the current
       *  element, because it may have moved to a new segment.
       */
      ep = bp->table.hdir[s->segnum]->hslots[s->slotnum];
#ifdef TableFix
      while (ep != NULL && BlkType(ep) != T_Table &&
	     ep->telem.hashnum <= hn)
#else					/* TableFix */
      while (ep != NULL && ep->telem.hashnum <= hn)
#endif					/* TableFix */
         ep = ep->telem.clink;
      }

   else {
      /*
       * There was no split, or else if there was we're between items
       *  that have identical hash numbers.  Find the next element in
       *  the current hash chain.
       */
#ifdef TableFix
      if (ep != NULL && BlkType(ep) != T_Table)	/* NULL on very first call */
#else					/* TableFix */
      if (ep != NULL)			/* already NULL on very first call */
#endif					/* TableFix */
         ep = ep->telem.clink;		/* next element in chain, if any */
   }

   /*
    * If we don't yet have an element, search successive slots.
    */
#ifdef TableFix
   while (ep == NULL || BlkType(ep) == T_Table) {
#else					/* TableFix */
   while (ep == NULL) {
#endif					/* TableFix */
      /*
       * Move to the next slot and pick the first entry.
       */
      s->slotnum++;
      if (s->slotnum >= segsize[s->segnum]) {
	 s->slotnum = 0;		/* need to move to next segment */
	 s->segnum++;
	 if (s->segnum >= HSegs || bp->table.hdir[s->segnum] == NULL)
	    return 0;			/* return NULL at end of set/table */
         }
      ep = bp->table.hdir[s->segnum]->hslots[s->slotnum];
      /*
       * Check to see if parts of this hash chain were already processed.
       *  This could happen if the elements were in a different chain,
       *  but a split occurred while we were suspended.
       */
      for (i = s->segnum; (m = s->sgmask[i]) != 0; i--) {
         d = (word)(m & s->slotnum) - (word)(m & s->sghash[i]);
         if (d < 0)			/* if all elements processed earlier */
            ep = NULL;			/* skip this slot */
         else if (d == 0) {
            /*
             * This chain was split from its parent while the parent was
             *  being processed.  Skip past elements already processed.
             */
#ifdef TableFix
            while (ep != NULL && BlkType(ep) != T_Table &&
		   ep->telem.hashnum <= s->sghash[i])
#else					/* TableFix */
            while (ep != NULL && ep->telem.hashnum <= s->sghash[i])
#endif					/* TableFix */
               ep = ep->telem.clink;
            }
         }
      }

   /*
    * Return the element.
    */
#ifdef TableFix
   if (ep && BlkType(ep) == T_Table) ep = NULL;
#endif					/* TableFix */
   return ep;
   }

/*
 * hgrow - split a hashed structure (doubling the buckets) for faster access.
 */

void hgrow(bp)
union block *bp;
   {
   register union block **tp0, **tp1, *ep;
   register word newslots, slotnum, segnum;
   tended struct b_set *ps;
   struct b_slots *seg, *newseg;
   union block **curslot;

   ps = (struct b_set *) bp;
   if (ps->hdir[HSegs-1] != NULL)
      return;				/* can't split further */
   newslots = ps->mask + 1;
   Protect(newseg = alcsegment(newslots), return);
#ifdef TableFix
   if (BlkType(bp) == T_Table) {
      int j;
      for(j=0; j<newslots; j++) newseg->hslots[j] = bp;
      }
#endif					/* TableFix */

   curslot = newseg->hslots;
   for (segnum = 0; (seg = ps->hdir[segnum]) != NULL; segnum++)
      for (slotnum = 0; slotnum < segsize[segnum]; slotnum++)  {
         tp0 = &seg->hslots[slotnum];	/* ptr to tail of old slot */
         tp1 = curslot++;		/* ptr to tail of new slot */
#ifdef TableFix
         for (ep = *tp0;
	      ep != NULL && BlkType(ep) != T_Table;
	      ep = ep->selem.clink) {
#else					/* TableFix */
         for (ep = *tp0; ep != NULL; ep = ep->selem.clink) {
#endif					/* TableFix */
            if ((ep->selem.hashnum & newslots) == 0) {
               *tp0 = ep;		/* element does not move */
               tp0 = &ep->selem.clink;
               }
            else {
               *tp1 = ep;		/* element moves to new slot */
               tp1 = &ep->selem.clink;
               }
            }
#ifdef TableFix
         if ( BlkType(bp) == T_Table )
	    *tp0 = *tp1 = bp;
         else
            *tp0 = *tp1 = NULL;
#else					/* TableFix */
         *tp0 = *tp1 = NULL;
#endif					/* TableFix */
         }
   ps->hdir[segnum] = newseg;
   ps->mask = (ps->mask << 1) | 1;
   }

/*
 * hshrink - combine buckets in a set or table that is too sparse.
 *
 *  Call this only for newly created structures.  Shrinking an active structure
 *  can wreak havoc on suspended generators.
 */
void hshrink(bp)
union block *bp;
   {
   register union block **tp, *ep0, *ep1;
   int topseg, curseg;
   word slotnum;
   tended struct b_set *ps;
   struct b_slots *seg;
   union block **uppslot;

   ps = (struct b_set *)bp;
   topseg = 0;
   for (topseg = 1; topseg < HSegs && ps->hdir[topseg] != NULL; topseg++)
      ;
   topseg--;
   while (TooSparse(ps)) {
      uppslot = ps->hdir[topseg]->hslots;
      ps->hdir[topseg--] = NULL;
      for (curseg = 0; (seg = ps->hdir[curseg]) != NULL; curseg++)
         for (slotnum = 0; slotnum < segsize[curseg]; slotnum++)  {
            tp = &seg->hslots[slotnum];		/* tail pointer */
            ep0 = seg->hslots[slotnum];		/* lower slot entry pointer */
            ep1 = *uppslot++;			/* upper slot entry pointer */
#ifdef TableFix
            while (ep0 != NULL && BlkType(ep0) != T_Table &&
		   ep1 != NULL && BlkType(ep1) != T_Table)
#else					/* TableFix */
            while (ep0 != NULL && ep1 != NULL)
#endif					/* TableFix */
               if (ep0->selem.hashnum < ep1->selem.hashnum) {
                  *tp = ep0;
                  tp = &ep0->selem.clink;
                  ep0 = ep0->selem.clink;
                  }
               else {
                  *tp = ep1;
                  tp = &ep1->selem.clink;
                  ep1 = ep1->selem.clink;
                  }
#ifdef TableFix
            while (ep0 != NULL && BlkType(ep0) != T_Table) {
#else					/* TableFix */
            while (ep0 != NULL) {
#endif					/* TableFix */
               *tp = ep0;
               tp = &ep0->selem.clink;
               ep0 = ep0->selem.clink;
               }
#ifdef TableFix
            while (ep1 != NULL && BlkType(ep1) != T_Table) {
#else					/* TableFix */
            while (ep1 != NULL) {
#endif					/* TableFix */
               *tp = ep1;
               tp = &ep1->selem.clink;
               ep1 = ep1->selem.clink;
               }
            }
      ps->mask >>= 1;
      }
   }

/*
 * memb - sets res flag to 1 if x is a member of a set or table, or to 0 if not.
 *  Returns a pointer to the word which points to the element, or which
 *  would point to it if it were there.
 */

union block **memb(pb, x, hn, res)
union block *pb;
dptr x;
register uword hn;
int *res;				/* pointer to integer result flag */
   {
   struct b_set *ps;
   register union block **lp;
   register struct b_selem *pe;
   register uword eh;

   ps = (struct b_set *)pb;
   lp = hchain(pb, hn);
   /*
    * Look for x in the hash chain.
    */
   *res = 0;
#ifdef TableFix
   while ((pe = (struct b_selem *)*lp) != NULL && BlkType(pe) != T_Table) {
#else					/* TableFix */
   while ((pe = (struct b_selem *)*lp) != NULL) {
#endif					/* TableFix */
      eh = pe->hashnum;
      if (eh > hn)			/* too far - it isn't there */
         return lp;
      else if ((eh == hn) && (equiv(&pe->setmem, x)))  {
         *res = 1;
         return lp;
         }
      /*
       * We haven't reached the right hashnumber yet or
       *  the element isn't the right one so keep looking.
       */
      lp = &(pe->clink);
      }
   /*
    *  At end of chain - not there.
    */
   return lp;
   }
