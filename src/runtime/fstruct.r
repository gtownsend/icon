/*
 * File: fstruct.r
 *  Contents: delete, get, key, insert, list, member, pop, pull, push, put,
 *  set, table
 */

"delete(x1,x2) - delete element x2 from set or table x1 if it is there"
" (always succeeds and returns x1)."

function{1} delete(s,x)
   abstract {
      return type(s) ** (set ++ table)
      }

   /*
    * The technique and philosophy here are the same
    *  as used in insert - see comment there.
    */
   type_case s of {
      set:
         body {
            register uword hn;
            register union block **pd;
            int res;

            hn = hash(&x);

            pd = memb(BlkLoc(s), &x, hn, &res);
            if (res == 1) {
               /*
               * The element is there so delete it.
               */
               *pd = (*pd)->selem.clink;
               (BlkLoc(s)->set.size)--;
               }

            EVValD(&s, E_Sdelete);
            EVValD(&x, E_Sval);
            return s;
	    }
      table:
         body {
            register union block **pd;
            register uword hn;
            int res;

            hn = hash(&x);
            pd = memb(BlkLoc(s), &x, hn, &res);
            if (res == 1) {
               /*
                * The element is there so delete it.
                */
               *pd = (*pd)->telem.clink;
               (BlkLoc(s)->table.size)--;
               }

            EVValD(&s, E_Tdelete);
            EVValD(&x, E_Tsub);
            return s;
            }
      default:
         runerr(122, s)
      }
end


/*
 * c_get - convenient C-level access to the get function
 *  returns 0 on failure, otherwise fills in res
 */
int c_get(hp, res)
struct b_list *hp;
struct descrip *res;
{
   register word i;
   register struct b_lelem *bp;

   /*
    * Fail if the list is empty.
    */
   if (hp->size <= 0)
      return 0;

   /*
    * Point bp at the first list block.  If the first block has no
    *  elements in use, point bp at the next list block.
    */
   bp = (struct b_lelem *) hp->listhead;
   if (bp->nused <= 0) {
      bp = (struct b_lelem *) bp->listnext;
      hp->listhead = (union block *) bp;
#ifdef ListFix
      bp->listprev = (union block *) hp;
#else					/* ListFix */
      bp->listprev = NULL;
#endif					/* ListFix */
      }

   /*
    * Locate first element and assign it to result for return.
    */
   i = bp->first;
   *res = bp->lslots[i];

   /*
    * Set bp->first to new first element, or 0 if the block is now
    *  empty.  Decrement the usage count for the block and the size
    *  of the list.
    */
   if (++i >= bp->nslots)
      i = 0;
   bp->first = i;
   bp->nused--;
   hp->size--;

   return 1;
}

#begdef GetOrPop(get_or_pop)
#get_or_pop "(x) - " #get_or_pop " an element from the left end of list x."
/*
 * get(L) - get an element from end of list L.
 *  Identical to pop(L).
 */
function{0,1} get_or_pop(x)
   if !is:list(x) then
      runerr(108, x)

   abstract {
      return store[type(x).lst_elem]
      }

   body {
      EVValD(&x, E_Lget);
      if (!c_get((struct b_list *)BlkLoc(x), &result)) fail;
      return result;
      }
end
#enddef

GetOrPop(get) /* get(x) - get an element from the left end of list x. */
GetOrPop(pop) /* pop(x) - pop an element from the left end of list x. */


"key(T) - generate successive keys (entry values) from table T."

function{*} key(t)
   if !is:table(t) then
         runerr(124, t)

   abstract {
      return store[type(t).tbl_key]
      }

   inline {
      tended union block *ep;
      struct hgstate state;

      EVValD(&t, E_Tkey);
      for (ep = hgfirst(BlkLoc(t), &state); ep != 0;
	 ep = hgnext(BlkLoc(t), &state, ep)) {
            EVValD(&ep->telem.tref, E_Tsub);
            suspend ep->telem.tref;
            }
      fail;
      }
end


"insert(x1, x2, x3) - insert element x2 into set or table x1 if not already there"
" if x1 is a table, the assigned value for element x2 is x3."
" (always succeeds and returns x1)."

function{1} insert(s, x, y)
   type_case s of {

      set: {
         abstract {
            store[type(s).set_elem] = type(x)
            return type(s)
            }

         body {
            tended union block *bp, *bp2;
            register uword hn;
            int res;
            struct b_selem *se;
            register union block **pd;

            bp = BlkLoc(s);
            hn = hash(&x);
            /*
             * If x is a member of set s then res will have the value 1,
             *  and pd will have a pointer to the pointer
             *  that points to that member.
             *  If x is not a member of the set then res will have
             *  the value 0 and pd will point to the pointer
             *  which should point to the member - thus we know where
             *  to link in the new element without having to do any
             *  repetitive looking.
             */

	    /* get this now because can't tend pd */
            Protect(se = alcselem(&x, hn), runerr(0));

            pd = memb(bp, &x, hn, &res);
            if (res == 0) {
               /*
               * The element is not in the set - insert it.
               */
               addmem((struct b_set *)bp, se, pd);
               if (TooCrowded(bp))
                  hgrow(bp);
               }
	    else
	       deallocate((union block *)se);

            EVValD(&s, E_Sinsert);
            EVValD(&x, E_Sval);
            return s;
            }
         }

      table: {
         abstract {
            store[type(s).tbl_key] = type(x)
            store[type(s).tbl_val] = type(y)
            return type(s)
            }

         body {
            tended union block *bp, *bp2;
            union block **pd;
            struct b_telem *te;
            register uword hn;
            int res;

            bp = BlkLoc(s);
            hn = hash(&x);

	    /* get this now because can't tend pd */
            Protect(te = alctelem(), runerr(0));

            pd = memb(bp, &x, hn, &res);	/* search table for key */
            if (res == 0) {
               /*
               * The element is not in the table - insert it.
               */
               bp->table.size++;
               te->clink = *pd;
               *pd = (union block *)te;
               te->hashnum = hn;
               te->tref = x;
               te->tval = y;
               if (TooCrowded(bp))
                  hgrow(bp);
               }
            else {
	       /*
		* We found an existing entry; just change its value.
		*/
	       deallocate((union block *)te);
               te = (struct b_telem *) *pd;
               te->tval = y;
               }

            EVValD(&s, E_Tinsert);
            EVValD(&x, E_Tsub);
            return s;
            }
         }

      default:
         runerr(122, s);
      }
end


"list(i, x) - create a list of size i, with initial value x."

function{1} list(n, x)
   if !def:C_integer(n, 0L) then
      runerr(101, n)

   abstract {
      return new list(type(x))
      }

   body {
      tended struct b_list *hp;
      register word i, size;
      word nslots;
      register struct b_lelem *bp; /* does not need to be tended */

      nslots = size = n;

      /*
       * Ensure that the size is positive and that the list-element block
       *  has at least MinListSlots slots.
       */
      if (size < 0) {
         irunerr(205, n);
         errorfail;
         }
      if (nslots == 0)
         nslots = MinListSlots;

      /*
       * Allocate the list-header block and a list-element block.
       *  Note that nslots is the number of slots in the list-element
       *  block while size is the number of elements in the list.
       */
      Protect(hp = alclist(size), runerr(0));
      Protect(bp = alclstb(nslots, (word)0, size), runerr(0));
      hp->listhead = hp->listtail = (union block *) bp;
#ifdef ListFix
      bp->listprev = bp->listnext = (union block *) hp;
#endif					/* ListFix */

      /*
       * Initialize each slot.
       */
      for (i = 0; i < size; i++)
         bp->lslots[i] = x;

      Desc_EVValD(hp, E_Lcreate, D_List);

      /*
       * Return the new list.
       */
      return list(hp);
      }
end


"member(x1, x2) - returns x1 if x2 is a member of set or table x2 but fails"
" otherwise."

function{0,1} member(s, x)
   type_case s of {

      set: {
         abstract {
            return type(x) ** store[type(s).set_elem]
            }
         inline {
            int res;
            register uword hn;

            EVValD(&s, E_Smember);
            EVValD(&x, E_Sval);

            hn = hash(&x);
            memb(BlkLoc(s), &x, hn, &res);
            if (res==1)
               return x;
            else
               fail;
            }
         }
      table: {
         abstract {
            return type(x) ** store[type(s).tbl_key]
            }
         inline {
            int res;
            register uword hn;

            EVValD(&s, E_Tmember);
            EVValD(&x, E_Tsub);

            hn = hash(&x);
            memb(BlkLoc(s), &x, hn, &res);
            if (res == 1)
               return x;
            else
               fail;
            }
         }
      default:
         runerr(122, s)
      }
end


"pull(L) - pull an element from end of list L."

function{0,1} pull(x)
   /*
    * x must be a list.
    */
   if !is:list(x) then
      runerr(108, x)
   abstract {
      return store[type(x).lst_elem]
      }

   body {
      register word i;
      register struct b_list *hp;
      register struct b_lelem *bp;

      EVValD(&x, E_Lpull);

      /*
       * Point at list header block and fail if the list is empty.
       */
      hp = (struct b_list *) BlkLoc(x);
      if (hp->size <= 0)
         fail;

      /*
       * Point bp at the last list element block.  If the last block has no
       *  elements in use, point bp at the previous list element block.
       */
      bp = (struct b_lelem *) hp->listtail;
      if (bp->nused <= 0) {
         bp = (struct b_lelem *) bp->listprev;
         hp->listtail = (union block *) bp;
#ifdef ListFix
         bp->listnext = (union block *) hp;
#else					/* ListFix */
         bp->listnext = NULL;
#endif					/* ListFix */
         }

      /*
       * Set i to position of last element and assign the element to
       *  result for return.  Decrement the usage count for the block
       *  and the size of the list.
       */
      i = bp->first + bp->nused - 1;
      if (i >= bp->nslots)
         i -= bp->nslots;
      result = bp->lslots[i];
      bp->nused--;
      hp->size--;
      return result;
      }
end

#ifdef Graphics
/*
 * c_push - C-level, nontending push operation
 */
void c_push(l, val)
dptr l;
dptr val;
{
   register word i;
   register struct b_lelem *bp; /* does not need to be tended */
   static int two = 2;		/* some compilers generate bad code for
				   division by a constant that's a power of 2*/
   /*
    * Point bp at the first list-element block.
    */
   bp = (struct b_lelem *) BlkLoc(*l)->list.listhead;

#ifdef EventMon		/* initialize i so it's 0 if first list-element */
   i = 0;			/* block isn't full */
#endif					/* EventMon */

   /*
    * If the first list-element block is full, allocate a new
    *  list-element block, make it the first list-element block,
    *  and make it the previous block of the former first list-element
    *  block.
    */
   if (bp->nused >= bp->nslots) {
      /*
       * Set i to the size of block to allocate.
       */
      i = BlkLoc(*l)->list.size / two;
      if (i < MinListSlots)
         i = MinListSlots;
#ifdef MaxListSlots
      if (i > MaxListSlots)
         i = MaxListSlots;
#endif					/* MaxListSlots */

      /*
       * Allocate a new list element block.  If the block can't
       *  be allocated, try smaller blocks.
       */
      while ((bp = alclstb(i, (word)0, (word)0)) == NULL) {
         i /= 4;
         if (i < MinListSlots)
            fatalerr(0, NULL);
         }

      BlkLoc(*l)->list.listhead->lelem.listprev = (union block *) bp;
#ifdef ListFix
      bp->listprev = BlkLoc(*l);
#endif					/* ListFix */
      bp->listnext = BlkLoc(*l)->list.listhead;
      BlkLoc(*l)->list.listhead = (union block *) bp;
      }

   /*
    * Set i to position of new first element and assign val to
    *  that element.
    */
   i = bp->first - 1;
   if (i < 0)
      i = bp->nslots - 1;
   bp->lslots[i] = *val;
   /*
    * Adjust value of location of first element, block usage count,
    *  and current list size.
    */
   bp->first = i;
   bp->nused++;
   BlkLoc(*l)->list.size++;
   }
#endif					/* Graphics */


"push(L, x1, ..., xN) - push x onto beginning of list L."

function{1} push(x, vals[n])
   /*
    * x must be a list.
    */
   if !is:list(x) then
      runerr(108, x)
   abstract {
      store[type(x).lst_elem] = type(vals)
      return type(x)
      }

   body {
      tended struct b_list *hp;
      dptr dp;
      register word i, val, num;
      register struct b_lelem *bp; /* does not need to be tended */
      static int two = 2;	/* some compilers generate bad code for
				   division by a constant that's a power of 2*/

      if (n == 0) {
	 dp = &nulldesc;
	 num = 1;
	 }
      else {
	 dp = vals;
	 num = n;
	 }

      for (val = 0; val < num; val++) {
	 /*
	  * Point hp at the list-header block and bp at the first
	  *  list-element block.
	  */
	 hp = (struct b_list *) BlkLoc(x);
	 bp = (struct b_lelem *) hp->listhead;

#ifdef EventMon		/* initialize i so it's 0 if first list-element */
	 i = 0;			/* block isn't full */
#endif					/* EventMon */

	 /*
	  * If the first list-element block is full, allocate a new
	  *  list-element block, make it the first list-element block,
	  *  and make it the previous block of the former first list-element
	  *  block.
	  */
	 if (bp->nused >= bp->nslots) {
	    /*
	     * Set i to the size of block to allocate.
	     */
	    i = hp->size / two;
	    if (i < MinListSlots)
	       i = MinListSlots;
#ifdef MaxListSlots
	    if (i > MaxListSlots)
	       i = MaxListSlots;
#endif					/* MaxListSlots */

	    /*
	     * Allocate a new list element block.  If the block can't
	     *  be allocated, try smaller blocks.
	     */
	    while ((bp = alclstb(i, (word)0, (word)0)) == NULL) {
	       i /= 4;
	       if (i < MinListSlots)
		  runerr(0);
	       }

	    hp->listhead->lelem.listprev = (union block *) bp;
#ifdef ListFix
	    bp->listprev = (union block *) hp;
#endif					/* ListFix */
	    bp->listnext = hp->listhead;
	    hp->listhead = (union block *) bp;
	    }

	 /*
	  * Set i to position of new first element and assign val to
	  *  that element.
	  */
	 i = bp->first - 1;
	 if (i < 0)
	    i = bp->nslots - 1;
	 bp->lslots[i] = dp[val];
	 /*
	  * Adjust value of location of first element, block usage count,
	  *  and current list size.
	  */
	 bp->first = i;
	 bp->nused++;
	 hp->size++;
	 }

      EVValD(&x, E_Lpush);

      /*
       * Return the list.
       */
      return x;
      }
end

/*
 * c_put - C-level, nontending list put function
 */
void c_put(l, val)
struct descrip *l;
struct descrip *val;
{
   register word i;
   register struct b_lelem *bp;  /* does not need to be tended */
   static int two = 2;		/* some compilers generate bad code for
				   division by a constant that's a power of 2*/

   /*
    * Point hp at the list-header block and bp at the last
    *  list-element block.
    */
   bp = (struct b_lelem *) BlkLoc(*l)->list.listtail;

#ifdef EventMon		/* initialize i so it's 0 if last list-element */
   i = 0;			/* block isn't full */
#endif					/* EventMon */

   /*
    * If the last list-element block is full, allocate a new
    *  list-element block, make it the last list-element block,
    *  and make it the next block of the former last list-element
    *  block.
    */
   if (bp->nused >= bp->nslots) {
      /*
       * Set i to the size of block to allocate.
       */
      i = ((struct b_list *)BlkLoc(*l))->size / two;
      if (i < MinListSlots)
         i = MinListSlots;
#ifdef MaxListSlots
      if (i > MaxListSlots)
         i = MaxListSlots;
#endif					/* MaxListSlots */

      /*
       * Allocate a new list element block.  If the block can't
       *  be allocated, try smaller blocks.
       */
      while ((bp = alclstb(i, (word)0, (word)0)) == NULL) {
         i /= 4;
         if (i < MinListSlots)
            fatalerr(0, NULL);
         }

      ((struct b_list *)BlkLoc(*l))->listtail->lelem.listnext =
	(union block *) bp;
      bp->listprev = ((struct b_list *)BlkLoc(*l))->listtail;
#ifdef ListFix
      bp->listnext = BlkLoc(*l);
#endif					/* ListFix */
      ((struct b_list *)BlkLoc(*l))->listtail = (union block *) bp;
      }

   /*
    * Set i to position of new last element and assign val to
    *  that element.
    */
   i = bp->first + bp->nused;
   if (i >= bp->nslots)
      i -= bp->nslots;
   bp->lslots[i] = *val;

   /*
    * Adjust block usage count and current list size.
    */
   bp->nused++;
   ((struct b_list *)BlkLoc(*l))->size++;
}


"put(L, x1, ..., xN) - put elements onto end of list L."

function{1} put(x, vals[n])
   /*
    * x must be a list.
    */
   if !is:list(x) then
      runerr(108, x)
   abstract {
      store[type(x).lst_elem] = type(vals)
      return type(x)
      }

   body {
      tended struct b_list *hp;
      dptr dp;
      register word i, val, num;
      register struct b_lelem *bp;  /* does not need to be tended */
      static int two = 2;	/* some compilers generate bad code for
				   division by a constant that's a power of 2*/
      if (n == 0) {
	 dp = &nulldesc;
	 num = 1;
	 }
      else {
	 dp = vals;
	 num = n;
	 }

      /*
       * Point hp at the list-header block and bp at the last
       *  list-element block.
       */
      for(val = 0; val < num; val++) {

	 hp = (struct b_list *)BlkLoc(x);
	 bp = (struct b_lelem *) hp->listtail;

#ifdef EventMon		/* initialize i so it's 0 if last list-element */
	 i = 0;			/* block isn't full */
#endif					/* EventMon */

	 /*
	  * If the last list-element block is full, allocate a new
	  *  list-element block, make it the last list-element block,
	  *  and make it the next block of the former last list-element
	  *  block.
	  */
	 if (bp->nused >= bp->nslots) {
	    /*
	     * Set i to the size of block to allocate.
	     */
	    i = hp->size / two;
	    if (i < MinListSlots)
	       i = MinListSlots;
#ifdef MaxListSlots
	    if (i > MaxListSlots)
	       i = MaxListSlots;
#endif					/* MaxListSlots */
	    /*
	     * Allocate a new list element block.  If the block can't
	     *  be allocated, try smaller blocks.
	     */
	    while ((bp = alclstb(i, (word)0, (word)0)) == NULL) {
	       i /= 4;
	       if (i < MinListSlots)
		  runerr(0);
	       }

	    hp->listtail->lelem.listnext = (union block *) bp;
	    bp->listprev = hp->listtail;
#ifdef ListFix
	    bp->listnext = (union block *)hp;
#endif					/* ListFix */
	    hp->listtail = (union block *) bp;
	    }

	 /*
	  * Set i to position of new last element and assign val to
	  *  that element.
	  */
	 i = bp->first + bp->nused;
	 if (i >= bp->nslots)
	    i -= bp->nslots;
	 bp->lslots[i] = dp[val];

	 /*
	  * Adjust block usage count and current list size.
	  */
	 bp->nused++;
	 hp->size++;

	 }

      EVValD(&x, E_Lput);

      /*
       * Return the list.
       */
      return x;
      }
end


"set(L) - create a set with members in list L."
"  The members are linked into hash chains which are"
" arranged in increasing order by hash number."

function{1} set(l)

   type_case l of {
      null: {
         abstract {
            return new set(empty_type)
            }
         inline {
            register union block * ps;
            ps = hmake(T_Set, (word)0, (word)0);
            if (ps == NULL)
               runerr(0);
            Desc_EVValD(ps, E_Screate, D_Set);
            return set(ps);
            }
         }

      list: {
         abstract {
            return new set(store[type(l).lst_elem])
            }

         body {
            tended union block *pb;
            register uword hn;
            dptr pd;
            struct b_selem *ne;      /* does not need to be tended */
            int res;
            word i, j;
            tended union block *ps;
            union block **pe;

            /*
             * Make a set of the appropriate size.
             */
            pb = BlkLoc(l);
            ps = hmake(T_Set, (word)0, pb->list.size);
            if (ps == NULL)
               runerr(0);

            /*
             * Chain through each list block and for
             *  each element contained in the block
             *  insert the element into the set if not there.
	     *
	     * ne always has a new element ready for use.  We must get one
	     *  in advance, and stay one ahead, because pe can't be tended.
	     */
	    Protect(ne = alcselem(&nulldesc, (uword)0), runerr(0));

            for (pb = pb->list.listhead;
#ifdef ListFix
		 BlkType(pb) == T_Lelem;
#else					/* ListFix */
		 pb != NULL;
#endif					/* ListFix */
		 pb = pb->lelem.listnext) {
               for (i = 0; i < pb->lelem.nused; i++) {
                  j = pb->lelem.first + i;
                  if (j >= pb->lelem.nslots)
                     j -= pb->lelem.nslots;
                  pd = &pb->lelem.lslots[j];
                  pe = memb(ps, pd, hn = hash(pd), &res);
                  if (res == 0) {
		     ne->setmem = *pd;			/* add new element */
		     ne->hashnum = hn;
                     addmem((struct b_set *)ps, ne, pe);
							/* get another blk */
	             Protect(ne = alcselem(&nulldesc, (uword)0), runerr(0));
                     }
                  }
               }
	    deallocate((union block *)ne);
            Desc_EVValD(ps, E_Screate, D_Set);
            return set(ps);
            }
         }

      default :
         runerr(108, l)
      }
end


"table(x) - create a table with default value x."

function{1} table(x)
   abstract {
      return new table(empty_type, empty_type, type(x))
      }
   inline {
      union block *bp;

      bp = hmake(T_Table, (word)0, (word)0);
      if (bp == NULL)
         runerr(0);
      bp->table.defvalue = x;
      Desc_EVValD(bp, E_Tcreate, D_Table);
      return table(bp);
      }
end
