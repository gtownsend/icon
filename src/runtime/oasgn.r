/*
 * File: oasgn.r
 */

/*
 * Asgn - perform an assignment when the destination descriptor might
 *  be within a block.
 */
#define Asgn(dest, src) *(dptr)((word *)VarLoc(dest) + Offset(dest)) = src;

/*
 * GeneralAsgn - perform the assignment x := y, where x is known to be
 *  a variable and y is has been dereferenced.
 */
#begdef GeneralAsgn(x, y)

#ifdef EventMon
   body {
      if (!is:null(curpstate->eventmask) &&
         Testb((word)E_Assign, curpstate->eventmask)) {
            EVAsgn(&x);
	    }
      }
#endif					/* EventMon */

   type_case x of {
      tvsubs: {
        abstract {
           store[store[type(x).str_var]] = string
           }
        inline {
           if (subs_asgn(&x, (const dptr)&y) == Error)
              runerr(0);
           }
        }
      tvtbl: {
        abstract {
           store[store[type(x).trpd_tbl].tbl_val] = type(y)
           }
        inline {
           if (tvtbl_asgn(&x, (const dptr)&y) == Error)
              runerr(0);
           }
         }
      kywdevent:
	 body {
	    *VarLoc(x) = y;
	    }
      kywdwin:
	 body {
#ifdef Graphics
	    if (is:null(y))
	       *VarLoc(x) = y;
	    else {
	       if ((!is:file(y)) || !(BlkLoc(y)->file.status & Fs_Window))
		  runerr(140,y);
	       *VarLoc(x) = y;
	       }
#endif					/* Graphics */
	    }
      kywdint:
	 {
         /*
          * No side effect in the type realm - keyword x is still an int.
          */
         body {
            C_integer i;

            if (!cnv:C_integer(y, i))
               runerr(101, y);
            IntVal(*VarLoc(x)) = i;

#ifdef Graphics
	    if (xyrowcol(&x) == -1)
	       runerr(140,kywd_xwin[XKey_Window]);
#endif					/* Graphics */
	    }
	}
      kywdpos: {
         /*
          * No side effect in the type realm - &pos is still an int.
          */
         body {
            C_integer i;

            if (!cnv:C_integer(y, i))
               runerr(101, y);

#ifdef MultiThread
	    i = cvpos((long)i, StrLen(*(VarLoc(x)+1)));
#else					/* MultiThread */
            i = cvpos((long)i, StrLen(k_subject));
#endif					/* MultiThread */

            if (i == CvtFail)
               fail;
	    IntVal(*VarLoc(x)) = i;

            EVVal(k_pos, E_Spos);
            }
         }
      kywdsubj: {
         /*
          * No side effect in the type realm - &subject is still a string
          *  and &pos is still an int.
          */
         if !cnv:string(y, *VarLoc(x)) then
            runerr(103, y);
         inline {
#ifdef MultiThread
	    IntVal(*(VarLoc(x)-1)) = 1;
#else					/* MultiThread */
            k_pos = 1;
#endif					/* MultiThread */
            EVVal(k_pos, E_Spos);
            }
         }
      kywdstr: {
         /*
          *  No side effect in the type realm.
          */
         if !cnv:string(y, *VarLoc(x)) then
            runerr(103, y);
         }
      default: {
         abstract {
            store[type(x)] = type(y)
            }
         inline {
            Asgn(x, y)
            }
         }
      }

#ifdef EventMon
   body {
      EVValD(&y, E_Value);
      }
#endif					/* EventMon */

#enddef


"x := y - assign y to x."

operator{0,1} := asgn(underef x, y)

   if !is:variable(x) then
      runerr(111, x)

   abstract {
      return type(x)
      }

   GeneralAsgn(x, y)

   inline {
      /*
       * The returned result is the variable to which assignment is being
       *  made.
       */
      return x;
      }
end


"x <- y - assign y to x."
" Reverses assignment if resumed."

operator{0,1+} <- rasgn(underef x -> saved_x, y)

   if !is:variable(x) then
      runerr(111, x)

   abstract {
      return type(x)
      }

   GeneralAsgn(x, y)

   inline {
      suspend x;
      }

   GeneralAsgn(x, saved_x)

   inline {
      fail;
      }
end


"x <-> y - swap values of x and y."
" Reverses swap if resumed."

operator{0,1+} <-> rswap(underef x -> dx, underef y -> dy)

   declare {
      tended union block *bp_x, *bp_y;
      word adj1 = 0;
      word adj2 = 0;
      }

   if !is:variable(x) then
      runerr(111, x)
   if !is:variable(y) then
      runerr(111, y)

   abstract {
      return type(x)
      }

   if is:tvsubs(x) && is:tvsubs(y) then
      body {
         bp_x = BlkLoc(x);
         bp_y = BlkLoc(y);
         if (VarLoc(bp_x->tvsubs.ssvar) == VarLoc(bp_y->tvsubs.ssvar) &&
	  Offset(bp_x->tvsubs.ssvar) == Offset(bp_y->tvsubs.ssvar)) {
            /*
             * x and y are both substrings of the same string, set
             *  adj1 and adj2 for use in locating the substrings after
             *  an assignment has been made.  If x is to the right of y,
             *  set adj1 := *x - *y, otherwise if y is to the right of
             *  x, set adj2 := *y - *x.  Note that the adjustment
             *  values may be negative.
             */
            if (bp_x->tvsubs.sspos > bp_y->tvsubs.sspos)
               adj1 = bp_x->tvsubs.sslen - bp_y->tvsubs.sslen;
            else if (bp_y->tvsubs.sspos > bp_x->tvsubs.sspos)
               adj2 = bp_y->tvsubs.sslen - bp_x->tvsubs.sslen;
	    }
         }

   /*
    * Do x := y
    */
   GeneralAsgn(x, dy)

   if is:tvsubs(x) && is:tvsubs(y) then
      inline {
         if (adj2 != 0)
            /*
             * Arg2 is to the right of Arg1 and the assignment Arg1 := Arg2 has
             *  shifted the position of Arg2.  Add adj2 to the position of Arg2
             *  to account for the replacement of Arg1 by Arg2.
             */
            bp_y->tvsubs.sspos += adj2;
         }

   /*
    * Do y := x
    */
   GeneralAsgn(y, dx)

   if is:tvsubs(x) && is:tvsubs(y) then
      inline {
         if (adj1 != 0)
            /*
             * Arg1 is to the right of Arg2 and the assignment Arg2 := Arg1
             *  has shifted the position of Arg1.  Add adj2 to the position
             *  of Arg1 to account for the replacement of Arg2 by Arg1.
             */
            bp_x->tvsubs.sspos += adj1;
         }

   inline {
      suspend x;
      }
   /*
    * If resumed, the assignments are undone.  Note that the string position
    *  adjustments are opposite those done earlier.
    */
   GeneralAsgn(x, dx)
   if is:tvsubs(x) && is:tvsubs(y) then
      inline {
         if (adj2 != 0)
           bp_y->tvsubs.sspos -= adj2;
         }

   GeneralAsgn(y, dy)
   if is:tvsubs(x) && is:tvsubs(y) then
      inline {
         if (adj1 != 0)
            bp_x->tvsubs.sspos -= adj1;
         }

   inline {
      fail;
      }
end


"x :=: y - swap values of x and y."

operator{0,1} :=: swap(underef x -> dx, underef y -> dy)
   declare {
      tended union block *bp_x, *bp_y;
      word adj1 = 0;
      word adj2 = 0;
      }

   /*
    * x and y must be variables.
    */
   if !is:variable(x) then
      runerr(111, x)
   if !is:variable(y) then
      runerr(111, y)

   abstract {
      return type(x)
      }

   if is:tvsubs(x) && is:tvsubs(y) then
      body {
         bp_x = BlkLoc(x);
         bp_y = BlkLoc(y);
         if (VarLoc(bp_x->tvsubs.ssvar) == VarLoc(bp_y->tvsubs.ssvar) &&
	  Offset(bp_x->tvsubs.ssvar) == Offset(bp_y->tvsubs.ssvar)) {
            /*
             * x and y are both substrings of the same string, set
             *  adj1 and adj2 for use in locating the substrings after
             *  an assignment has been made.  If x is to the right of y,
             *  set adj1 := *x - *y, otherwise if y is to the right of
             *  x, set adj2 := *y - *x.  Note that the adjustment
             *  values may be negative.
             */
            if (bp_x->tvsubs.sspos > bp_y->tvsubs.sspos)
               adj1 = bp_x->tvsubs.sslen - bp_y->tvsubs.sslen;
            else if (bp_y->tvsubs.sspos > bp_x->tvsubs.sspos)
               adj2 = bp_y->tvsubs.sslen - bp_x->tvsubs.sslen;
	    }
         }

   /*
    * Do x := y
    */
   GeneralAsgn(x, dy)

   if is:tvsubs(x) && is:tvsubs(y) then
      inline {
         if (adj2 != 0)
            /*
             * Arg2 is to the right of Arg1 and the assignment Arg1 := Arg2 has
             *  shifted the position of Arg2.  Add adj2 to the position of Arg2
             *  to account for the replacement of Arg1 by Arg2.
             */
            bp_y->tvsubs.sspos += adj2;
         }

   /*
    * Do y := x
    */
   GeneralAsgn(y, dx)

   if is:tvsubs(x) && is:tvsubs(y) then
      inline {
         if (adj1 != 0)
            /*
             * Arg1 is to the right of Arg2 and the assignment Arg2 := Arg1
             *  has shifted the position of Arg1.  Add adj2 to the position
             *  of Arg1 to account for the replacement of Arg2 by Arg1.
             */
            bp_x->tvsubs.sspos += adj1;
         }

   inline {
      return x;
      }
end

/*
 * subs_asgn - perform assignment to a substring. Leave the updated substring
 *  in dest in case it is needed as the result of the assignment.
 */
int subs_asgn(dest, src)
dptr dest;
const dptr src;
   {
   tended struct descrip deststr;
   tended struct descrip srcstr;
   tended struct descrip rsltstr;
   tended struct b_tvsubs *tvsub;

   char *s, *s2;
   word i, len;
   word prelen;   /* length of portion of string before substring */
   word poststrt; /* start of portion of string following substring */
   word postlen;  /* length of portion of string following substring */

   if (!cnv:tmp_string(*src, srcstr))
      ReturnErrVal(103, *src, Error);

   /*
    * Be sure that the variable in the trapped variable points
    *  to a string and that the string is big enough to contain
    *  the substring.
    */
   tvsub = (struct b_tvsubs *)BlkLoc(*dest);
   deref(&tvsub->ssvar, &deststr);
   if (!is:string(deststr))
      ReturnErrVal(103, deststr, Error);
   prelen = tvsub->sspos - 1;
   poststrt = prelen + tvsub->sslen;
   if (poststrt > StrLen(deststr))
      ReturnErrNum(205, Error);

   /*
    * Form the result string.
    *  Start by allocating space for the entire result.
    */
   len = prelen + StrLen(srcstr) + StrLen(deststr) - poststrt;
   Protect(s = alcstr(NULL, len), return Error);
   StrLoc(rsltstr) = s;
   StrLen(rsltstr) = len;
   /*
    * First, copy the portion of the substring string to the left of
    *  the substring into the string space.
    */
   s2 = StrLoc(deststr);
   for (i = 0; i < prelen; i++)
      *s++ = *s2++;
   /*
    * Copy the string to be assigned into the string space,
    *  effectively concatenating it.
    */
   s2 = StrLoc(srcstr);
   for (i = 0; i < StrLen(srcstr); i++)
      *s++ = *s2++;
   /*
    * Copy the portion of the substring to the right of
    *  the substring into the string space, completing the
    *  result.
    */
   s2 = StrLoc(deststr) + poststrt;
   postlen = StrLen(deststr) - poststrt;
   for (i = 0; i < postlen; i++)
      *s++ = *s2++;

   /*
    * Perform the assignment and update the trapped variable.
    */
   type_case tvsub->ssvar of {
      kywdevent: {
         *VarLoc(tvsub->ssvar) = rsltstr;
         }
      kywdstr: {
         *VarLoc(tvsub->ssvar) = rsltstr;
         }
      kywdsubj: {
         *VarLoc(tvsub->ssvar) = rsltstr;
         k_pos = 1;
         }
      tvtbl: {
         if (tvtbl_asgn(&tvsub->ssvar, (const dptr)&rsltstr) == Error)
            return Error;
         }
      default: {
         Asgn(tvsub->ssvar, rsltstr);
         }
      }
   tvsub->sslen = StrLen(srcstr);

   EVVal(tvsub->sslen, E_Ssasgn);
   return Succeeded;
   }

/*
 * tvtbl_asgn - perform an assignment to a table element trapped variable,
 *  inserting the element in the table if needed.
 */
int tvtbl_asgn(dest, src)
dptr dest;
const dptr src;
   {
   tended struct b_tvtbl *bp;
   tended struct descrip tval;
   struct b_telem *te;
   union block **slot;
   struct b_table *tp;
   int res;

   /*
    * Allocate te now (even if we may not need it)
    * because slot cannot be tended.
    */
   bp = (struct b_tvtbl *) BlkLoc(*dest);	/* Save params to tended vars */
   tval = *src;
   Protect(te = alctelem(), return Error);

   /*
    * First see if reference is in the table; if it is, just update
    *  the value.  Otherwise, allocate a new table entry.
    */
   slot = memb(bp->clink, &bp->tref, bp->hashnum, &res);

   if (res == 1) {
      /*
       * Do not need new te, just update existing entry.
       */
      deallocate((union block *) te);
      (*slot)->telem.tval = tval;
      }
   else {
      /*
       * Link te into table, fill in entry.
       */
      tp = (struct b_table *) bp->clink;
      tp->size++;

      te->clink = *slot;
      *slot = (union block *) te;

      te->hashnum = bp->hashnum;
      te->tref = bp->tref;
      te->tval = tval;

      if (TooCrowded(tp))		/* grow hash table if now too full */
         hgrow((union block *)tp);
      }
   return Succeeded;
   }
