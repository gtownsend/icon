/*
 * File: oref.r
 *  Contents: bang, random, sect, subsc
 */

"!x - generate successive values from object x."

operator{*} ! bang(underef x -> dx)
   declare {
      register C_integer i, j;
      tended union block *ep;
      struct hgstate state;
      char ch;
      }

   if is:variable(x) && is:string(dx) then {
      abstract {
         return new tvsubs(type(x))
         }
      inline {
         /*
          * A nonconverted string from a variable is being banged.
          *  Loop through the string suspending one-character substring
          *  trapped variables.
          */
         for (i = 1; i <= StrLen(dx); i++) {
            suspend tvsubs(&x, i, (word)1);
            deref(&x, &dx);
            if (!is:string(dx))
               runerr(103, dx);
            }
         }
      }
   else type_case dx of {

      list: {
         abstract {
            return type(dx).lst_elem
	    }
         inline {

#ifdef EventMon
            word xi = 0;

            EVValD(&dx, E_Lbang);
#endif					/* EventMon */

            /*
             * x is a list.  Chain through each list element block and for
             * each one, suspend with a variable pointing to each
             * element contained in the block.
             */
            for (ep = BlkLoc(dx)->list.listhead;
#ifdef ListFix
		 BlkType(ep) == T_Lelem;
#else					/* ListFix */
		 ep != NULL;
#endif					/* ListFix */
                 ep = ep->lelem.listnext){
               for (i = 0; i < ep->lelem.nused; i++) {
                  j = ep->lelem.first + i;
                  if (j >= ep->lelem.nslots)
                     j -= ep->lelem.nslots;

#ifdef EventMon
                  MakeInt(++xi, &eventdesc);
                  EVValD(&eventdesc, E_Lsub);
#endif					/* EventMon */

                  suspend struct_var(&ep->lelem.lslots[j], ep);
                  }
               }
            }
         }

      file: {
         abstract {
            return string
	       }
         body {
            FILE *fd;
            char sbuf[MaxCvtLen];
            register char *sp;
            register C_integer slen, rlen;
            word status;

            /*
             * x is a file.  Read the next line into the string space
             *	and suspend the newly allocated string.
             */
            fd = BlkLoc(dx)->file.fd;

            status = BlkLoc(dx)->file.status;
            if ((status & Fs_Read) == 0)
               runerr(212, dx);

#ifdef ReadDirectory
            if ((status & Fs_Directory) != 0) {
               for (;;) {
                  struct dirent *de = readdir((DIR*) fd);
                  if (de == NULL)
                     fail;
                  slen = strlen(de->d_name);
                  Protect(sp = alcstr(de->d_name, slen), runerr(0));
                  suspend string(slen, sp);
                  }
               }
#endif					/* ReadDirectory */

            if (status & Fs_Writing) {
               fseek(fd, 0L, SEEK_CUR);
               BlkLoc(dx)->file.status &= ~Fs_Writing;
               }
            BlkLoc(dx)->file.status |= Fs_Reading;
            status = BlkLoc(dx)->file.status;

            for (;;) {
               StrLen(result) = 0;
               do {

#ifdef Graphics
                  pollctr >>= 1; pollctr++;
                  if (status & Fs_Window) {
                     slen = wgetstrg(sbuf,MaxCvtLen,fd);
		     if (slen == -1)
			runerr(141);
		     else if (slen < -1)
			runerr(143);
                     }
                  else
#endif					/* Graphics */

                  if ((slen = getstrg(sbuf,MaxCvtLen,&BlkLoc(dx)->file)) == -1)
                     fail;
                  rlen = slen < 0 ? (word)MaxCvtLen : slen;

		  Protect(reserve(Strings, rlen), runerr(0));
		  if (!InRange(strbase,StrLoc(result),strfree)) {
		     Protect(reserve(Strings, StrLen(result)+rlen), runerr(0));
		     Protect((StrLoc(result) = alcstr(StrLoc(result),
                        StrLen(result))), runerr(0));
		     }

                  Protect(sp = alcstr(sbuf,rlen), runerr(0));
                  if (StrLen(result) == 0)
                     StrLoc(result) = sp;
                  StrLen(result) += rlen;
                  } while (slen < 0);
               suspend result;
               }
            }
         }

      table: {
         abstract {
            return type(dx).tbl_val
	       }
         inline {
            struct b_tvtbl *tp;

            EVValD(&dx, E_Tbang);

            /*
             * x is a table.  Chain down the element list in each bucket
             * and suspend a variable pointing to each element in turn.
             */
	    for (ep = hgfirst(BlkLoc(dx), &state); ep != 0;
	       ep = hgnext(BlkLoc(dx), &state, ep)) {

                  EVValD(&ep->telem.tval, E_Tval);

		  Protect(tp = alctvtbl(&dx, &ep->telem.tref, ep->telem.hashnum), runerr(0));
		  suspend tvtbl(tp);
                  }
            }
         }

      set: {
         abstract {
            return store[type(dx).set_elem]
            }
         inline {
            EVValD(&dx, E_Sbang);
            /*
             *  This is similar to the method for tables except that a
             *  value is returned instead of a variable.
             */
	    for (ep = hgfirst(BlkLoc(dx), &state); ep != 0;
	       ep = hgnext(BlkLoc(dx), &state, ep)) {
                  EVValD(&ep->selem.setmem, E_Sval);
                  suspend ep->selem.setmem;
                  }
	    }
         }

      record: {
         abstract {
            return type(dx).all_fields
	       }
         inline {
            /*
             * x is a record.  Loop through the fields and suspend
             * a variable pointing to each one.
             */

#ifdef EventMon
            word xi = 0;

            EVValD(&dx, E_Rbang);
#endif					/* EventMon */

            j = BlkLoc(dx)->record.recdesc->proc.nfields;
            for (i = 0; i < j; i++) {

#ifdef EventMon
                  MakeInt(++xi, &eventdesc);
                  EVValD(&eventdesc, E_Rsub);
#endif					/* EventMon */

               suspend struct_var(&BlkLoc(dx)->record.fields[i],
                  (struct b_record *)BlkLoc(dx));
               }
            }
         }

      default:
         if cnv:tmp_string(dx) then {
            abstract {
               return string
               }
            inline {
               /*
                * A (converted or non-variable) string is being banged.
                * Loop through the string suspending simple one character
                *  substrings.
                */
               for (i = 1; i <= StrLen(dx); i++) {
                  ch = *(StrLoc(dx) + i - 1);
                  suspend string(1, (char *)&allchars[ch & 0xFF]);
                  }
               }
            }
         else
            runerr(116, dx);
      }

   inline {
      fail;
      }
end


#define RandVal (RanScale*(k_random=(RandA*k_random+RandC)&0x7FFFFFFFL))

"?x - produce a randomly selected element of x."

operator{0,1} ? random(underef x -> dx)

#ifndef LargeInts
   declare {
      C_integer v = 0;
      }
#endif					/* LargeInts */

   if is:variable(x) && is:string(dx) then {
      abstract {
         return new tvsubs(type(x))
         }
      body {
         C_integer val;
         double rval;

         /*
          * A string from a variable is being banged. Produce a one
          *  character substring trapped variable.
          */
         if ((val = StrLen(dx)) <= 0)
            fail;
         rval = RandVal;	/* This form is used to get around */
         rval *= val;		/* a bug in a certain C compiler */
         return tvsubs(&x, (word)rval + 1, (word)1);
         }
      }
   else type_case dx of {
      string: {
         /*
          * x is a string, but it is not a variable. Produce a
          *   random character in it as the result; a substring
          *   trapped variable is not needed.
          */
         abstract {
            return string
            }
         body {
            C_integer val;
            double rval;

            if ((val = StrLen(dx)) <= 0)
               fail;
            rval = RandVal;
            rval *= val;
            return string(1, StrLoc(dx)+(word)rval);
            }
         }

      cset: {
         /*
          * x is a cset.  Convert it to a string, select a random character
          *  of that string and return it. A substring trapped variable is
          *  not needed.
          */
         if !cnv:tmp_string(dx) then
            { /* cannot fail */ }
         abstract {
            return string
            }
         body {
            C_integer val;
            double rval;
	    char ch;

            if ((val = StrLen(dx)) <= 0)
               fail;
            rval = RandVal;
            rval *= val;
            ch = *(StrLoc(dx) + (word)rval);
            return string(1, (char *)&allchars[ch & 0xFF]);
            }
         }

      list: {
         abstract {
            return type(dx).lst_elem
            }
         /*
          * x is a list.  Set i to a random number in the range [1,*x],
          *  failing if the list is empty.
          */
         body {
            C_integer val;
            double rval;
            register C_integer i, j;
            union block *bp;     /* doesn't need to be tended */
            val = BlkLoc(dx)->list.size;
            if (val <= 0)
               fail;
            rval = RandVal;
            rval *= val;
            i = (word)rval + 1;

#ifdef EventMon
            EVValD(&dx, E_Lrand);
            MakeInt(i, &eventdesc);
            EVValD(&eventdesc, E_Lsub);
#endif					/* EventMon */

            j = 1;
            /*
             * Work down chain list of list blocks and find the block that
             *  contains the selected element.
             */
            bp = BlkLoc(dx)->list.listhead;
            while (i >= j + bp->lelem.nused) {
               j += bp->lelem.nused;
               bp = bp->lelem.listnext;
#ifdef ListFix
               if (BlkType(bp) == T_List)
#else					/* ListFix */
	       if (bp == NULL)
#endif					/* ListFix */
                  syserr("list reference out of bounds in random");
               }
            /*
             * Locate the appropriate element and return a variable
             * that points to it.
             */
            i += bp->lelem.first - j;
            if (i >= bp->lelem.nslots)
               i -= bp->lelem.nslots;
            return struct_var(&bp->lelem.lslots[i], bp);
            }
         }

      table: {
         abstract {
            return type(dx).tbl_val
            }
          /*
           * x is a table.  Set n to a random number in the range [1,*x],
           *  failing if the table is empty.
           */
         body {
            C_integer val;
            double rval;
            register C_integer i, j, n;
            union block *ep, *bp;   /* doesn't need to be tended */
	    struct b_slots *seg;
	    struct b_tvtbl *tp;

            bp = BlkLoc(dx);
            val = bp->table.size;
            if (val <= 0)
               fail;
            rval = RandVal;
            rval *= val;
            n = (word)rval + 1;

#ifdef EventMon
            EVValD(&dx, E_Trand);
            MakeInt(n, &eventdesc);
            EVValD(&eventdesc, E_Tsub);
#endif					/* EventMon */


            /*
             * Walk down the hash chains to find and return the nth element
	     *  as a variable.
             */
            for (i = 0; i < HSegs && (seg = bp->table.hdir[i]) != NULL; i++)
               for (j = segsize[i] - 1; j >= 0; j--)
                  for (ep = seg->hslots[j];
#ifdef TableFix
		       BlkType(ep) == T_Telem;
#else					/* TableFix */
		       ep != NULL;
#endif					/* TableFix */
		       ep = ep->telem.clink)
                     if (--n <= 0) {
			Protect(tp = alctvtbl(&dx, &ep->telem.tref, ep->telem.hashnum), runerr(0));
			return tvtbl(tp);
			}
            syserr("table reference out of bounds in random");
            }
         }

      set: {
         abstract {
            return store[type(dx).set_elem]
            }
         /*
          * x is a set.  Set n to a random number in the range [1,*x],
          *  failing if the set is empty.
          */
         body {
            C_integer val;
            double rval;
            register C_integer i, j, n;
            union block *bp, *ep;  /* doesn't need to be tended */
	    struct b_slots *seg;

            bp = BlkLoc(dx);
            val = bp->set.size;
            if (val <= 0)
               fail;
            rval = RandVal;
            rval *= val;
            n = (word)rval + 1;

#ifdef EventMon
            EVValD(&dx, E_Srand);
            MakeInt(n, &eventdesc);
#endif					/* EventMon */

            /*
             * Walk down the hash chains to find and return the nth element.
             */
            for (i = 0; i < HSegs && (seg = bp->table.hdir[i]) != NULL; i++)
               for (j = segsize[i] - 1; j >= 0; j--)
                  for (ep = seg->hslots[j]; ep != NULL; ep = ep->telem.clink)
                     if (--n <= 0)
                        return ep->selem.setmem;
            syserr("set reference out of bounds in random");
            }
         }

      record: {
         abstract {
            return type(dx).all_fields
            }
         /*
          * x is a record.  Set val to a random number in the range
          *  [1,*x] (*x is the number of fields), failing if the
          *  record has no fields.
          */
         body {
            C_integer val;
            double rval;
            struct b_record *rec;  /* doesn't need to be tended */

            rec = (struct b_record *)BlkLoc(dx);
            val = rec->recdesc->proc.nfields;
            if (val <= 0)
               fail;
            /*
             * Locate the selected element and return a variable
             * that points to it
             */
            rval = RandVal;
            rval *= val;

#ifdef EventMon
            EVValD(&dx, E_Rrand);
            MakeInt(rval + 1, &eventdesc);
            EVValD(&eventdesc, E_Rsub);
#endif					/* EventMon */

            return struct_var(&rec->fields[(word)rval], rec);
            }
         }

      default: {

#ifdef LargeInts
         if !cnv:integer(dx) then
            runerr(113, dx)
#else					/* LargeInts */
         if !cnv:C_integer(dx,v) then
            runerr(113, dx)
#endif					/* LargeInts */

         abstract {
            return integer ++ real
            }
         body {
            double rval;

#ifdef LargeInts
            C_integer v;
            if (Type(dx) == T_Lrgint) {
	       if (bigrand(&dx, &result) == Error)  /* alcbignum failed */
	          runerr(0);
	       return result;
	       }

            v = IntVal(dx);
#endif					/* LargeInts */
            /*
             * x is an integer, be sure that it's non-negative.
             */
            if (v < 0)
               runerr(205, dx);

            /*
             * val contains the integer value of x. If val is 0, return
             *	a real in the range [0,1), else return an integer in the
             *	range [1,val].
             */
            if (v == 0) {
               rval = RandVal;
               return C_double rval;
               }
            else {
               rval = RandVal;
               rval *= v;
               return C_integer (long)rval + 1;
               }
            }
         }
      }
end

"x[i:j] - form a substring or list section of x."

operator{0,1} [:] sect(underef x -> dx, i, j)
   declare {
      int use_trap = 0;
      }

   if is:list(dx) then {
      abstract {
         return type(dx)
         }
      /*
       * If it isn't a C integer, but is a large integer, fail on
       * the out-of-range index.
       */
      if !cnv:C_integer(i) then {
	 if cnv : integer(i) then inline { fail; }
	 runerr(101, i)
	 }
      if !cnv:C_integer(j) then {
         if cnv : integer(j) then inline { fail; }
	 runerr(101, j)
         }

      body {
         C_integer t;

         i = cvpos((long)i, (long)BlkLoc(dx)->list.size);
         if (i == CvtFail)
            fail;
         j = cvpos((long)j, (long)BlkLoc(dx)->list.size);
         if (j == CvtFail)
            fail;
         if (i > j) {
            t = i;
            i = j;
            j = t;
            }
         if (cplist(&dx, &result, i, j) == Error)
	    runerr(0);
         return result;
         }
      }
   else {

      /*
       * x should be a string. If x is a variable, we must create a
       *  substring trapped variable.
       */
      if is:variable(x) && is:string(dx) then {
         abstract {
            return new tvsubs(type(x))
            }
         inline {
            use_trap = 1;
            }
         }
      else if cnv:string(dx) then
         abstract {
            return string
            }
      else
         runerr(110, dx)

      /*
       * If it isn't a C integer, but is a large integer, fail on
       * the out-of-range index.
       */
      if !cnv:C_integer(i) then {
	 if cnv : integer(i) then inline { fail; }
	 runerr(101, i)
	 }
      if !cnv:C_integer(j) then {
         if cnv : integer(j) then inline { fail; }
	 runerr(101, j)
         }

      body {
         C_integer t;

         i = cvpos((long)i, (long)StrLen(dx));
         if (i == CvtFail)
            fail;
         j = cvpos((long)j, (long)StrLen(dx));
         if (j == CvtFail)
            fail;
         if (i > j) {			/* convert section to substring */
            t = i;
            i = j;
            j = t - j;
            }
         else
            j = j - i;

         if (use_trap) {
            return tvsubs(&x, i, j);
            }
         else
            return string(j, StrLoc(dx)+i-1);
         }
      }
end

"x[y] - access yth character or element of x."

operator{0,1} [] subsc(underef x -> dx,y)
   declare {
      int use_trap = 0;
      }

   type_case dx of {
      list: {
         abstract {
            return type(dx).lst_elem
            }
         /*
          * Make sure that y is a C integer.
          */
         if !cnv:C_integer(y) then {
	    /*
	     * If it isn't a C integer, but is a large integer, fail on
	     * the out-of-range index.
	     */
	    if cnv : integer(y) then inline { fail; }
	    runerr(101, y)
	    }
         body {
            word i, j;
            register union block *bp; /* doesn't need to be tended */
            struct b_list *lp;        /* doesn't need to be tended */

#ifdef EventMon
            EVValD(&dx, E_Lref);
            MakeInt(y, &eventdesc);
            EVValD(&eventdesc, E_Lsub);
#endif					/* EventMon */

	    /*
	     * Make sure that subscript y is in range.
	     */
            lp = (struct b_list *)BlkLoc(dx);
            i = cvpos((long)y, (long)lp->size);
            if (i == CvtFail || i > lp->size)
               fail;
            /*
             * Locate the list-element block containing the desired
             *  element.
             */
            bp = lp->listhead;
            j = 1;
	    /*
	     * y is in range, so bp can never be null here. if it was, a memory
	     * violation would occur in the code that follows, anyhow, so
	     * exiting the loop on a NULL bp makes no sense.
	     */
            while (i >= j + bp->lelem.nused) {
               j += bp->lelem.nused;
               bp = bp->lelem.listnext;
               }

            /*
             * Locate the desired element and return a pointer to it.
             */
            i += bp->lelem.first - j;
            if (i >= bp->lelem.nslots)
               i -= bp->lelem.nslots;
            return struct_var(&bp->lelem.lslots[i], bp);
            }
         }

      table: {
         abstract {
            store[type(dx).tbl_key] = type(y) /* the key might be added */
            return type(dx).tbl_val ++ new tvtbl(type(dx))
            }
         /*
          * x is a table.  Return a table element trapped variable
	  *  representing the result; defer actual lookup until later.
          */
         body {
            uword hn;
	    struct b_tvtbl *tp;

            EVValD(&dx, E_Tref);
            EVValD(&y, E_Tsub);

	    hn = hash(&y);
            Protect(tp = alctvtbl(&dx, &y, hn), runerr(0));
            return tvtbl(tp);
            }
         }

      record: {
         abstract {
            return type(dx).all_fields
            }
         /*
          * x is a record.  Convert y to an integer and be sure that it
          *  it is in range as a field number.
          */
	 if !cnv:C_integer(y) then body {
	    if (!cnv:tmp_string(y,y))
	       runerr(101,y);
	    else {
	       register union block *bp;  /* doesn't need to be tended */
	       register union block *bp2; /* doesn't need to be tended */
	       register word i;
	       register int len;
	       char *loc;
	       int nf;
	       bp = BlkLoc(dx);
	       bp2 = BlkLoc(dx)->record.recdesc;
	       nf = bp2->proc.nfields;
	       loc = StrLoc(y);
	       len = StrLen(y);
	       for(i=0; i<nf; i++) {
		  if (len == StrLen(bp2->proc.lnames[i]) &&
		      !strncmp(loc, StrLoc(bp2->proc.lnames[i]), len)) {

#ifdef EventMon
		     EVValD(&dx, E_Rref);
		     MakeInt(i+1, &eventdesc);
		     EVValD(&eventdesc, E_Rsub);
#endif					/* EventMon */

		     /*
		      * Found the field, return a pointer to it.
		      */
		     return struct_var(&bp->record.fields[i], bp);
		     }
		  }
	       fail;
               }
	    }
	 else
         body {
            word i;
            register union block *bp; /* doesn't need to be tended */

            bp = BlkLoc(dx);
            i = cvpos(y, (word)(bp->record.recdesc->proc.nfields));
            if (i == CvtFail || i > bp->record.recdesc->proc.nfields)
               fail;

#ifdef EventMon
            EVValD(&dx, E_Rref);
            MakeInt(i, &eventdesc);
            EVValD(&eventdesc, E_Rsub);
#endif					/* EventMon */

            /*
             * Locate the appropriate field and return a pointer to it.
             */
            return struct_var(&bp->record.fields[i-1], bp);
            }
         }

      default: {
         /*
          * dx must either be a string or be convertible to one. Decide
          *  whether a substring trapped variable can be created.
          */
         if is:variable(x) && is:string(dx) then {
            abstract {
               return new tvsubs(type(x))
               }
            inline {
               use_trap = 1;
               }
            }
         else if cnv:tmp_string(dx) then
            abstract {
               return string
               }
         else
            runerr(114, dx)

         /*
          * Make sure that y is a C integer.
          */
         if !cnv:C_integer(y) then {
	    /*
	     * If it isn't a C integer, but is a large integer, fail on
	     * the out-of-range index.
	     */
	    if cnv : integer(y) then inline { fail; }
	    runerr(101, y)
	    }

         body {
            char ch;
            word i;

            /*
             * Convert y to a position in x and fail if the position
             *  is out of bounds.
             */
            i = cvpos(y, StrLen(dx));
            if (i == CvtFail || i > StrLen(dx))
               fail;
            if (use_trap) {
               /*
                * x is a string, make a substring trapped variable for the
                * one character substring selected and return it.
                */
               return tvsubs(&x, i, (word)1);
               }
            else {
               /*
                * x was converted to a string, so it cannot be assigned
                * back into. Just return a string containing the selected
                * character.
                */
               ch = *(StrLoc(dx)+i-1);
               return string(1, (char *)&allchars[ch & 0xFF]);
               }
            }
         }
      }
end
