/*
 * File: omisc.r
 *  Contents: refresh, size, tabmat, toby, to, llist
 */

"^x - create a refreshed copy of a co-expression."
#ifdef Coexpr
/*
 * ^x - return an entry block for co-expression x from the refresh block.
 */
operator{1} ^ refresh(x)
   if !is:coexpr(x) then
       runerr(118, x)
   abstract {
      return coexpr
      }

   body {
      register struct b_coexpr *sblkp;

      /*
       * Get a new co-expression stack and initialize.
       */
#ifdef MultiThread
      Protect(sblkp = alccoexp(0, 0), runerr(0));
#else					/* MultiThread */
      Protect(sblkp = alccoexp(), runerr(0));
#endif					/* MultiThread */

      sblkp->freshblk = BlkLoc(x)->coexpr.freshblk;
      if (ChkNull(sblkp->freshblk))	/* &main cannot be refreshed */
         runerr(215, x);

      /*
       * Use refresh block to finish initializing the new co-expression.
       */
      co_init(sblkp);

#if COMPILER
      sblkp->fnc = BlkLoc(x)->coexpr.fnc;
      if (line_info) {
         if (debug_info)
            PFDebug(sblkp->pf)->proc = PFDebug(BlkLoc(x)->coexpr.pf)->proc;
         PFDebug(sblkp->pf)->old_fname =
            PFDebug(BlkLoc(x)->coexpr.pf)->old_fname;
         PFDebug(sblkp->pf)->old_line =
            PFDebug(BlkLoc(x)->coexpr.pf)->old_line;
         }
#endif					/* COMPILER */

      return coexpr(sblkp);
      }
#else					/* Coexpr */
operator{} ^ refresh(x)
      runerr(401)
#endif					/* Coexpr */

end


"*x - return size of string or object x."

operator{1} * size(x)
   abstract {
      return integer
      }
   type_case x of {
      string: inline {
         return C_integer StrLen(x);
         }
      list: inline {
         return C_integer BlkLoc(x)->list.size;
         }
      table: inline {
         return C_integer BlkLoc(x)->table.size;
         }
      set: inline {
         return C_integer BlkLoc(x)->set.size;
         }
      cset: inline {
         register word i;

         i = BlkLoc(x)->cset.size;
	 if (i < 0)
	    i = cssize(&x);
         return C_integer i;
         }
      record: inline {
         return C_integer BlkLoc(x)->record.recdesc->proc.nfields;
         }
      coexpr: inline {
         return C_integer BlkLoc(x)->coexpr.size;
         }
      default: {
         /*
          * Try to convert it to a string.
          */
         if !cnv:tmp_string(x) then
            runerr(112, x);	/* no notion of size */
         inline {
	    return C_integer StrLen(x);
            }
         }
      }
end


"=x - tab(match(x)).  Reverses effects if resumed."

operator{*} = tabmat(x)
   /*
    * x must be a string.
    */
   if !cnv:string(x) then
      runerr(103, x)
   abstract {
      return string
      }

   body {
      register word l;
      register char *s1, *s2;
      C_integer i, j;
      /*
       * Make a copy of &pos.
       */
      i = k_pos;

      /*
       * Fail if &subject[&pos:0] is not of sufficient length to contain x.
       */
      j = StrLen(k_subject) - i + 1;
      if (j < StrLen(x))
         fail;

      /*
       * Get pointers to x (s1) and &subject (s2).  Compare them on a byte-wise
       *  basis and fail if s1 doesn't match s2 for *s1 characters.
       */
      s1 = StrLoc(x);
      s2 = StrLoc(k_subject) + i - 1;
      l = StrLen(x);
      while (l-- > 0) {
         if (*s1++ != *s2++)
            fail;
         }

      /*
       * Increment &pos to tab over the matched string and suspend the
       *  matched string.
       */
      l = StrLen(x);
      k_pos += l;

      EVVal(k_pos, E_Spos);

      suspend x;

      /*
       * tabmat has been resumed, restore &pos and fail.
       */
      if (i > StrLen(k_subject) + 1)
         runerr(205, kywd_pos);
      else {
         k_pos = i;
         EVVal(k_pos, E_Spos);
         }
      fail;
      }
end


"i to j by k - generate successive values."

operator{*} ... toby(from, to, by)
   /*
    * arguments must be integers.
    */
   if !cnv:C_integer(from) then
      runerr(101, from)
   if !cnv:C_integer(to) then
      runerr(101, to)
   if !cnv:C_integer(by) then
      runerr(101, by)

   abstract {
      return integer
      }

   inline {
      /*
       * by must not be zero.
       */
      if (by == 0) {
         irunerr(211, by);
         errorfail;
         }

      /*
       * Count up or down (depending on relationship of from and to) and
       *  suspend each value in sequence, failing when the limit has been
       *  exceeded.
       */
      if (by > 0)
         for ( ; from <= to; from += by) {
            suspend C_integer from;
            }
      else
         for ( ; from >= to; from += by) {
            suspend C_integer from;
            }
      fail;
      }
end


"i to j - generate successive values."

operator{*} ... to(from, to)
   /*
    * arguments must be integers.
    */
   if !cnv:C_integer(from) then
      runerr(101, from)
   if !cnv:C_integer(to) then
      runerr(101, to)

   abstract {
      return integer
      }

   inline {
      for ( ; from <= to; ++from) {
         suspend C_integer from;
         }
      fail;
      }
end


" [x1, x2, ... ] - create an explicitly specified list."

operator{1} [...] llist(elems[n])
   abstract {
      return new list(type(elems))
      }
   body {
      tended struct b_list *hp;
      register word i;
      register struct b_lelem *bp;  /* need not be tended */
      word nslots;

      nslots = n;
      if (nslots == 0)
         nslots = MinListSlots;

      /*
       * Allocate the list and a list block.
       */
      Protect(hp = alclist(n), runerr(0));
      Protect(bp = alclstb(nslots, (word)0, n), runerr(0));

      /*
       * Make the list block just allocated into the first and last blocks
       *  for the list.
       */
      hp->listhead = hp->listtail = (union block *)bp;
#ifdef ListFix
      bp->listprev = bp->listnext = (union block *)hp;
#endif					/* ListFix */

      /*
       * Assign each argument to a list element.
       */
      for (i = 0; i < n; i++)
         bp->lslots[i] = elems[i];

/*  Not quite right -- should be after list() returns in case it fails */
      Desc_EVValD(hp, E_Lcreate, D_List);

      return list(hp);
      }
end

