/*
 * File: oset.r
 *  Contents: compl, diff, inter, union
 */

"~x - complement cset x."

operator{1} ~ compl(x)
   /*
    * x must be a cset.
    */
   if !cnv:tmp_cset(x) then
      runerr(104, x)

   abstract {
      return cset
      }
   body {
      register int i;
      struct b_cset *cp, *cpx;

      /*
       * Allocate a new cset and then copy each cset word from x
       *  into the new cset words, complementing each bit.
       */
      Protect(cp = alccset(), runerr(0));
      cpx = (struct b_cset *)BlkLoc(x);      /* must come after alccset() */
      for (i = 0; i < CsetSize; i++)
          cp->bits[i] = ~cpx->bits[i];
      return cset(cp);
      }
end


"x -- y - difference of csets x and y or of sets x and y."

operator{1} -- diff(x,y)
   if is:set(x) && is:set(y) then {
      abstract {
         return type(x)
         }
      body {
	 int res;
         register int i;
         register word slotnum;
         tended union block *srcp, *tstp, *dstp;
         tended struct b_slots *seg;
         tended struct b_selem *ep;
         struct b_selem *np;
         union block **hook;

         /*
          * Make a new set based on the size of x.
          */
         dstp = hmake(T_Set, (word)0, BlkLoc(x)->set.size);
         if (dstp == NULL)
            runerr(0);
         /*
          * For each element in set x if it is not in set y
          *  copy it directly into the result set.
	  *
	  * np always has a new element ready for use.  We get one in advance,
	  *  and stay one ahead, because hook can't be tended.
          */
         srcp = BlkLoc(x);
         tstp = BlkLoc(y);
         Protect(np = alcselem(&nulldesc, (uword)0), runerr(0));

         for (i = 0; i < HSegs && (seg = srcp->set.hdir[i]) != NULL; i++)
            for (slotnum = segsize[i] - 1; slotnum >= 0; slotnum--) {
               ep = (struct b_selem *)seg->hslots[slotnum];
               while (ep != NULL) {
                  memb(tstp, &ep->setmem, ep->hashnum, &res);
                  if (res == 0) {
                     hook = memb(dstp, &ep->setmem, ep->hashnum, &res);
		     np->setmem = ep->setmem;
		     np->hashnum = ep->hashnum;
                     addmem(&dstp->set, np, hook);
                     Protect(np = alcselem(&nulldesc, (uword)0), runerr(0));
                     }
                  ep = (struct b_selem *)ep->clink;
                  }
               }
	 deallocate((union block *)np);
         if (TooSparse(dstp))
            hshrink(dstp);
         Desc_EVValD(dstp, E_Screate, D_Set);
         return set(dstp);
         }
      }
   else {
      if !cnv:tmp_cset(x) then
         runerr(120, x)
      if !cnv:tmp_cset(y) then
         runerr(120, y)
      abstract {
         return cset
         }
      /*
       * Allocate a new cset and in each word of it, compute the value
       *  of the bitwise difference of the corresponding words in the
       *  Arg1 and Arg2 csets.
       */
      body {
         struct b_cset *cp, *cpx, *cpy;
         register int i;

         Protect(cp = alccset(), runerr(0));
         cpx = (struct b_cset *)BlkLoc(x);  /* must come after alccset() */
         cpy = (struct b_cset *)BlkLoc(y);  /* must come after alccset() */
         for (i = 0; i < CsetSize; i++)
            cp->bits[i] = cpx->bits[i] & ~cpy->bits[i];
         return cset(cp);
         }
      }
end


"x ** y - intersection of csets x and y or of sets x and y."

operator{1} ** inter(x,y)
   if is:set(x) && is:set(y) then {
      abstract {
         return new set(store[type(x).set_elem] ** store[type(y).set_elem])
         }
      body {
	 int res;
         register int i;
         register word slotnum;
         tended union block *srcp, *tstp, *dstp;
         tended struct b_slots *seg;
         tended struct b_selem *ep;
         struct b_selem *np;
         union block **hook;

         /*
          * Make a new set the size of the smaller argument set.
          */
         dstp = hmake(T_Set, (word)0,
            Min(BlkLoc(x)->set.size, BlkLoc(y)->set.size));
         if (dstp == NULL)
            runerr(0);
         /*
          * Using the smaller of the two sets as the source
          *  copy directly into the result each of its elements
          *  that are also members of the other set.
	  *
	  * np always has a new element ready for use.  We get one in advance,
	  *  and stay one ahead, because hook can't be tended.
          */
         if (BlkLoc(x)->set.size <= BlkLoc(y)->set.size) {
            srcp = BlkLoc(x);
            tstp = BlkLoc(y);
            }
         else {
            srcp = BlkLoc(y);
            tstp = BlkLoc(x);
            }
         Protect(np = alcselem(&nulldesc, (uword)0), runerr(0));
         for (i = 0; i < HSegs && (seg = srcp->set.hdir[i]) != NULL; i++)
            for (slotnum = segsize[i] - 1; slotnum >= 0; slotnum--) {
               ep = (struct b_selem *)seg->hslots[slotnum];
               while (ep != NULL) {
                  memb(tstp, &ep->setmem, ep->hashnum, &res);
                  if (res != 0) {
                     hook = memb(dstp, &ep->setmem, ep->hashnum, &res);
		     np->setmem = ep->setmem;
		     np->hashnum = ep->hashnum;
                     addmem(&dstp->set, np, hook);
                     Protect(np = alcselem(&nulldesc, (uword)0), runerr(0));
                     }
                  ep = (struct b_selem *)ep->clink;
                  }
               }
	 deallocate((union block *)np);
         if (TooSparse(dstp))
            hshrink(dstp);
         Desc_EVValD(dstp, E_Screate, D_Set);
         return set(dstp);
         }
      }
   else {

      if !cnv:tmp_cset(x) then
         runerr(120, x)
      if !cnv:tmp_cset(y) then
         runerr(120, y)
      abstract {
         return cset
         }

      /*
       * Allocate a new cset and in each word of it, compute the value
       *  of the bitwise intersection of the corresponding words in the
       *  x and y csets.
       */
      body {
         struct b_cset *cp, *cpx, *cpy;
         register int i;

         Protect(cp = alccset(), runerr(0));
         cpx = (struct b_cset *)BlkLoc(x);  /* must come after alccset() */
         cpy = (struct b_cset *)BlkLoc(y);  /* must come after alccset() */
         for (i = 0; i < CsetSize; i++) {
            cp->bits[i] = cpx->bits[i] & cpy->bits[i];
            }
         return cset(cp);
         }
      }
end


"x ++ y - union of csets x and y or of sets x and y."

operator{1} ++ union(x,y)
   if is:set(x) && is:set(y) then {
      abstract {
         return new set(store[type(x).set_elem] ++ store[type(y).set_elem])
         }
      body {
	 int res;
	 register int i;
	 register word slotnum;
         struct descrip d;
         tended union block *dstp;
         tended struct b_slots *seg;
         tended struct b_selem *ep;
         struct b_selem *np;
         union block **hook;

         /*
          * Ensure that x is the larger set; if not, swap.
          */
         if (BlkLoc(y)->set.size > BlkLoc(x)->set.size) {
	    d = x;
	    x = y;
	    y = d;
	    }
         /*
          * Copy x and ensure there's room for *x + *y elements.
          */
         if (cpset(&x, &result, BlkLoc(x)->set.size + BlkLoc(y)->set.size)
            == Error)
            runerr(0);
         /*
          * Copy each element from y into the result, if not already there.
	  *
	  * np always has a new element ready for use.  We get one in advance,
	  *  and stay one ahead, because hook can't be tended.
          */
         dstp = BlkLoc(result);
         Protect(np = alcselem(&nulldesc, (uword)0), runerr(0));
         for (i = 0; i < HSegs && (seg = BlkLoc(y)->set.hdir[i]) != NULL; i++)
            for (slotnum = segsize[i] - 1; slotnum >= 0; slotnum--) {
               ep = (struct b_selem *)seg->hslots[slotnum];
               while (ep != NULL) {
                  hook = memb(dstp, &ep->setmem, ep->hashnum, &res);
                  if (res == 0) {
		     np->setmem = ep->setmem;
		     np->hashnum = ep->hashnum;
                     addmem(&dstp->set, np, hook);
                     Protect(np = alcselem(&nulldesc, (uword)0), runerr(0));
                     }
                  ep = (struct b_selem *)ep->clink;
                  }
               }
	 deallocate((union block *)np);
         if (TooCrowded(dstp))		/* if the union got too big, enlarge */
            hgrow(dstp);
         return result;
	 }
      }
   else {
      if !cnv:tmp_cset(x) then
         runerr(120, x)
      if !cnv:tmp_cset(y) then
         runerr(120, y)
      abstract {
         return cset
         }

      /*
       * Allocate a new cset and in each word of it, compute the value
       *  of the bitwise union of the corresponding words in the
       *  x and y csets.
       */
      body {
         struct b_cset *cp, *cpx, *cpy;
         register int i;

         Protect(cp = alccset(), runerr(0));
         cpx = (struct b_cset *)BlkLoc(x);  /* must come after alccset() */
         cpy = (struct b_cset *)BlkLoc(y);  /* must come after alccset() */
         for (i = 0; i < CsetSize; i++)
            cp->bits[i] = cpx->bits[i] | cpy->bits[i];
         return cset(cp);
         }
      }
end
