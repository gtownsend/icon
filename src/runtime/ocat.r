/*
 * File: ocat.r -- caterr, lconcat
 */
"x || y - concatenate strings x and y."

operator{1} || cater(x, y)

   if !cnv:string(x) then
      runerr(103, x)
   if !cnv:string(y) then
      runerr(103, y)

   abstract {
      return string
      }

   body {
      char *s, *s2;
      word len, i;

      /*
       *  Optimization 1:  The strings to be concatenated are already
       *   adjacent in memory; no allocation is required.
       */
      if (StrLoc(x) + StrLen(x) == StrLoc(y)) {
         StrLoc(result) = StrLoc(x);
         StrLen(result) = StrLen(x) + StrLen(y);
         return result;
         }
      else if ((StrLoc(x) + StrLen(x) == strfree)
      && (DiffPtrs(strend,strfree) > StrLen(y))) {
         /*
          * Optimization 2: The end of x is at the end of the string space.
          *  Hence, x was the last string allocated and need not be
          *  re-allocated. y is appended to the string space and the
          *  result is pointed to the start of x.
          */
	 result = x;
	 /*
	  * Append y to the end of the string space.
	  */
	 Protect(alcstr(StrLoc(y),StrLen(y)), runerr(0));
	 /*
	  *  Set the length of the result and return.
	  */
	 StrLen(result) = StrLen(x) + StrLen(y);
	 return result;
         }

      /*
       * Otherwise, allocate space for x and y, and copy them
       *  to the end of the string space.
       */
      Protect(StrLoc(result) = alcstr(NULL, StrLen(x) + StrLen(y)), runerr(0));
      s = StrLoc(result);
      s2 = StrLoc(x);
      len = StrLen(x);
      for(i = 0; i < len; i++)
	 *s++ = *s2++;
      s2 = StrLoc(y);
      len = StrLen(y);
      for(i = 0; i < len; i++)
	 *s++ = *s2++;

      /*
       *  Set the length of the result and return.
       */
      StrLen(result) = StrLen(x) + StrLen(y);
      return result;
      }
end


"x ||| y - concatenate lists x and y."

operator{1} ||| lconcat(x, y)
   /*
    * x and y must be lists.
    */
   if !is:list(x) then
      runerr(108, x)
   if !is:list(y) then
      runerr(108, y)

   abstract {
      return new list(store[(type(x) ++ type(y)).lst_elem])
      }

   body {
      register struct b_list *bp1;
      register struct b_lelem *lp1;
      word size1, size2, size3;

      /*
       * Get the size of both lists.
       */
      size1 = BlkLoc(x)->list.size;
      size2 = BlkLoc(y)->list.size;
      size3 = size1 + size2;

      Protect(bp1 = (struct b_list *)alclist(size3), runerr(0));
      Protect(lp1 = (struct b_lelem *)alclstb(size3,(word)0,size3), runerr(0));
      bp1->listhead = bp1->listtail = (union block *)lp1;
#ifdef ListFix
      lp1->listprev = lp1->listnext = (union block *)bp1;
#endif					/* ListFix */

      /*
       * Make a copy of both lists in adjacent slots.
       */
      cpslots(&x, lp1->lslots, (word)1, size1 + 1);
      cpslots(&y, lp1->lslots + size1, (word)1, size2 + 1);

      BlkLoc(x) = (union block *)bp1;

      EVValD(&x, E_Lcreate);

      return x;
      }
end
