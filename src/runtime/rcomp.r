/*
 * File: rcomp.r
 *  Contents: anycmp, equiv, lexcmp
 */

/*
 * anycmp - compare any two objects.
 */

int anycmp(dp1,dp2)
dptr dp1, dp2;
   {
   register int o1, o2;
   register long v1, v2, lresult;
   int iresult;
   double rres1, rres2, rresult;

   /*
    * Get a collating number for dp1 and dp2.
    */
   o1 = order(dp1);
   o2 = order(dp2);

   /*
    * If dp1 and dp2 aren't of the same type, compare their collating numbers.
    */
   if (o1 != o2)
      return (o1 > o2 ? Greater : Less);

   if (o1 == 3)
      /*
       * dp1 and dp2 are strings, use lexcmp to compare them.
       */
      return lexcmp(dp1,dp2);

   switch (Type(*dp1)) {

#ifdef LargeInts

      case T_Integer:
	 if (Type(*dp2) != T_Lrgint) {
            v1 = IntVal(*dp1);
            v2 = IntVal(*dp2);
            if (v1 < v2)
               return Less;
            else if (v1 == v2)
               return Equal;
            else
               return Greater;
            }
	 /* if dp2 is a Lrgint, flow into next case */

      case T_Lrgint:
	 lresult = bigcmp(dp1, dp2);
	 if (lresult == 0)
	    return Equal;
	 return ((lresult > 0) ? Greater : Less);

#else					/* LargeInts */

      case T_Integer:
         v1 = IntVal(*dp1);
         v2 = IntVal(*dp2);
         if (v1 < v2)
            return Less;
         else if (v1 == v2)
            return Equal;
         else
            return Greater;

#endif					/* LargeInts */

      case T_Coexpr:
         /*
          * Collate on co-expression id.
          */
         lresult = (BlkLoc(*dp1)->coexpr.id - BlkLoc(*dp2)->coexpr.id);
         if (lresult == 0)
            return Equal;
         return ((lresult > 0) ? Greater : Less);

      case T_Cset:
         return csetcmp((unsigned int *)((struct b_cset *)BlkLoc(*dp1))->bits,
            (unsigned int *)((struct b_cset *)BlkLoc(*dp2))->bits);

      case T_File:
         /*
          * Collate on file name or window label.
          */
	 {
	 struct descrip s1, s2; /* live only long enough to lexcmp them */
	 dptr ps1 = &(BlkLoc(*dp1)->file.fname);
	 dptr ps2 = &(BlkLoc(*dp2)->file.fname);
#ifdef Graphics
	 if (BlkLoc(*dp1)->file.status & Fs_Window) {
	    wbp w = (wbp) BlkLoc(*dp1)->file.fd;
	    StrLoc(s1) = w->window->windowlabel;
	    StrLen(s1) = strlen(w->window->windowlabel);
	    ps1 = &s1;
	    }
	 if (BlkLoc(*dp2)->file.status & Fs_Window) {
	    wbp w = (wbp) BlkLoc(*dp2)->file.fd;
	    StrLoc(s2) = w->window->windowlabel;
	    StrLen(s2) = strlen(w->window->windowlabel);
	    ps2 = &s2;
	    }
#endif					/* Graphics */
         return lexcmp(ps1, ps2);
         }

      case T_List:
         /*
          * Collate on list id.
          */
         lresult = (BlkLoc(*dp1)->list.id - BlkLoc(*dp2)->list.id);
         if (lresult == 0)
            return Equal;
         return ((lresult > 0) ? Greater : Less);

      case T_Null:
         return Equal;

      case T_Proc:
         /*
          * Collate on procedure name.
          */
         return lexcmp(&(BlkLoc(*dp1)->proc.pname),
            &(BlkLoc(*dp2)->proc.pname));

      case T_Real:
         GetReal(dp1,rres1);
         GetReal(dp2,rres2);
         rresult = rres1 - rres2;
	 if (rresult == 0.0)
	    return Equal;
	 return ((rresult > 0.0) ? Greater : Less);

      case T_Record:
         /*
          * Collate on record id within record name.
          */
         iresult = lexcmp(&(BlkLoc(*dp1)->record.recdesc->proc.pname),
            &(BlkLoc(*dp2)->record.recdesc->proc.pname));
         if (iresult == Equal) {
            lresult = (BlkLoc(*dp1)->record.id - BlkLoc(*dp2)->record.id);
            if (lresult > 0)	/* coded this way because of code-generation */
               return Greater;  /* bug in MSC++ 7.0A;  do not change. */
            else if (lresult < 0)
               return Less;
            else
               return Equal;
            }
        return iresult;

      case T_Set:
         /*
          * Collate on set id.
          */
         lresult = (BlkLoc(*dp1)->set.id - BlkLoc(*dp2)->set.id);
         if (lresult == 0)
            return Equal;
         return ((lresult > 0) ? Greater : Less);

      case T_Table:
         /*
          * Collate on table id.
          */
         lresult = (BlkLoc(*dp1)->table.id - BlkLoc(*dp2)->table.id);
         if (lresult == 0)
            return Equal;
         return ((lresult > 0) ? Greater : Less);

      case T_External:
	 /*
          * Collate these values according to the relative positions of
          *  their blocks in the heap.
	  */
         lresult = ((long)BlkLoc(*dp1) - (long)BlkLoc(*dp2));
         if (lresult == 0)
            return Equal;
         return ((lresult > 0) ? Greater : Less);

      default:
	 syserr("anycmp: unknown datatype.");
	 /*NOTREACHED*/
	 return 0;  /* avoid gcc warning */
      }
   }

/*
 * order(x) - return collating number for object x.
 */

int order(dp)
dptr dp;
   {
   if (Qual(*dp))
      return 3;			     /* string */
   switch (Type(*dp)) {
      case T_Null:
	 return 0;
      case T_Integer:
	 return 1;

#ifdef LargeInts
      case T_Lrgint:
	 return 1;
#endif					/* LargeInts */

      case T_Real:
	 return 2;

      /* string: return 3 (see above) */

      case T_Cset:
	 return 4;
      case T_File:
	 return 5;
      case T_Coexpr:
	 return 6;
      case T_Proc:
	 return 7;
      case T_List:
	 return 8;
      case T_Set:
	 return 9;
      case T_Table:
	 return 10;
      case T_Record:
	 return 11;
      case T_External:
         return 12;
      default:
	 syserr("order: unknown datatype.");
	 /*NOTREACHED*/
	 return 0;  /* avoid gcc warning */
      }
   }

/*
 * equiv - test equivalence of two objects.
 */

int equiv(dp1, dp2)
dptr dp1, dp2;
   {
   register int result;
   register word i;
   register char *s1, *s2;
   double rres1, rres2;

   result = 0;

      /*
       * If the descriptors are identical, the objects are equivalent.
       */
   if (EqlDesc(*dp1,*dp2))
      result = 1;
   else if (Qual(*dp1) && Qual(*dp2)) {

      /*
       *  If both are strings of equal length, compare their characters.
       */

      if ((i = StrLen(*dp1)) == StrLen(*dp2)) {


	 s1 = StrLoc(*dp1);
	 s2 = StrLoc(*dp2);
	 result = 1;
	 while (i--)
	   if (*s1++ != *s2++) {
	      result = 0;
	      break;
	      }

	 }
      }
   else if (dp1->dword == dp2->dword)
      switch (Type(*dp1)) {
	 /*
	  * For integers and reals, just compare the values.
	  */
	 case T_Integer:
	    result = (IntVal(*dp1) == IntVal(*dp2));
	    break;

#ifdef LargeInts
	 case T_Lrgint:
	    result = (bigcmp(dp1, dp2) == 0);
	    break;
#endif					/* LargeInts */


	 case T_Real:
            GetReal(dp1, rres1);
            GetReal(dp2, rres2);
            result = (rres1 == rres2);
	    break;

	 case T_Cset:
	    /*
	     * Compare the bit arrays of the csets.
	     */
	    result = 1;
	    for (i = 0; i < CsetSize; i++)
	       if (BlkLoc(*dp1)->cset.bits[i] != BlkLoc(*dp2)->cset.bits[i]) {
		  result = 0;
		  break;
		  }
	 }
   else
      /*
       * dp1 and dp2 are of different types, so they can't be
       *  equivalent.
       */
      result = 0;

   return result;
   }

/*
 * lexcmp - lexically compare two strings.
 */

int lexcmp(dp1, dp2)
dptr dp1, dp2;
   {


   register char *s1, *s2;
   register word minlen;
   word l1, l2;

   /*
    * Get length and starting address of both strings.
    */
   l1 = StrLen(*dp1);
   s1 = StrLoc(*dp1);
   l2 = StrLen(*dp2);
   s2 = StrLoc(*dp2);

   /*
    * Set minlen to length of the shorter string.
    */
   minlen = Min(l1, l2);

   /*
    * Compare as many bytes as are in the smaller string.  If an
    *  inequality is found, compare the differing bytes.
    */
   while (minlen--)
      if (*s1++ != *s2++)
         return (*--s1 & 0377) > (*--s2 & 0377) ?  Greater : Less;

   /*
    * The strings compared equal for the length of the shorter.
    */
   if (l1 == l2)
      return Equal;
   else if (l1 > l2)
      return Greater;
   else
      return Less;

   }

/*
 * csetcmp - compare two cset bit arrays.
 *  The order defined by this function is identical to the lexical order of
 *  the two strings that the csets would be converted into.
 */

int csetcmp(cs1, cs2)
unsigned int *cs1, *cs2;
   {
   unsigned int nbit, mask, *cs_end;

   if (cs1 == cs2) return Equal;

   /*
    * The longest common prefix of the two bit arrays converts to some
    *  common prefix string.  The first bit on which the csets disagree is
    *  the first character of the conversion strings that disagree, and so this
    *  is the character on which the order is determined.  The cset that has
    *  this first non-common bit = one, has in that position the lowest
    *  character, so this cset is lexically least iff the other cset has some
    *  following bit set.  If the other cset has no bits set after the first
    *  point of disagreement, then it is a prefix of the other, and is therefor
    *  lexically less.
    *
    * Find the first word where cs1 and cs2 are different.
    */
   for (cs_end = cs1 + CsetSize; cs1 < cs_end; cs1++, cs2++)
      if (*cs1 != *cs2) {
	 /*
	  * Let n be the position at which the bits first differ within
	  *  the word.  Set nbit to some integer for which the nth bit
	  *  is the first bit in the word that is one.  Note here and in the
	  *  following, that bits go from right to left within a word, so
	  *  the _first_ bit is the _rightmost_ bit.
	  */
	 nbit = *cs1 ^ *cs2;

	 /* Set mask to an integer that has all zeros in bit positions
	  *  upto and including position n, and all ones in bit positions
	  *  _after_ bit position n.
	  */
	 for (mask = (unsigned)MaxLong << 1; !(~mask & nbit); mask <<= 1);

	 /*
	  * nbit & ~mask contains zeros everywhere except position n, which
	  *  is a one, so *cs2 & (nbit & ~mask) is non-zero iff the nth bit
	  *  of *cs2 is one.
	  */
	 if (*cs2 & (nbit & ~mask)) {
	    /*
	     * If there are bits set in cs1 after bit position n in the
	     *  current word, then cs1 is lexically greater than cs2.
	     */
	    if (*cs1 & mask) return Greater;
	    while (++cs1 < cs_end)
	       if (*cs1) return Greater;

	    /*
	     * Otherwise cs1 is a proper prefix of cs2 and is therefore
	     *  lexically less.
	     */
	     return Less;
	     }

	 /*
	  * If the nth bit of *cs2 isn't one, then the nth bit of cs1
	  *  must be one.  Just reverse the logic for the previous
	  *  case.
	  */
	 if (*cs2 & mask) return Less;
	 cs_end = cs2 + (cs_end - cs1);
	 while (++cs2 < cs_end)
	    if (*cs2) return Less;
	 return Greater;
	 }
   return Equal;
   }
