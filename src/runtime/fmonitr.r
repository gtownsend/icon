/*
 *  fmonitr.r -- event, EvGet
 *
 *   This file contains event monitoring code, used only if EventMon
 *   (event monitoring) is defined.  Event monitoring is normally is
 *   not enabled.
 */

#ifdef EventMon

/*
 * Prototypes.
 */

void mmrefresh		(void);

#define evforget()


char typech[MaxType+1];	/* output character for each type */

int noMTevents;			/* don't produce events in EVAsgn */

#ifdef MultiThread

static char scopechars[] = "+:^-";

/*
 * Special event function for E_Assign; allocates out of monitor's heap.
 */
void EVAsgn(dx)
dptr dx;
{
   int i;
   dptr procname;
   struct progstate *parent = curpstate->parent;
   struct region *rp = curpstate->stringregion;

#if COMPILER
   procname = &(PFDebug(*pfp)->proc->pname);
#else					/* COMPILER */
   procname = &((&BlkLoc(*glbl_argp)->proc)->pname);
#endif					/* COMPILER */
   /*
    * call get_name, allocating out of the monitor if necessary.
    */
   curpstate->stringregion = parent->stringregion;
   parent->stringregion = rp;
   noMTevents++;
   i = get_name(dx,&(parent->eventval));

   if (i == GlobalName) {
      if (reserve(Strings, StrLen(parent->eventval) + 1) == NULL)
	 syserr("event monitoring out-of-memory error");
      StrLoc(parent->eventval) =
	 alcstr(StrLoc(parent->eventval), StrLen(parent->eventval));
      alcstr("+",1);
      StrLen(parent->eventval)++;
      }
   else if (i == StaticName || i == LocalName || i == ParamName) {
      if (!reserve(Strings, StrLen(parent->eventval) + StrLen(*procname) + 1))
	 syserr("event monitoring out-of-memory error");
      StrLoc(parent->eventval) =
	 alcstr(StrLoc(parent->eventval), StrLen(parent->eventval));
      alcstr(scopechars+i,1);
      alcstr(StrLoc(*procname), StrLen(*procname));
      StrLen(parent->eventval) += StrLen(*procname) + 1;
      }
   else if (i == Error) {
      noMTevents--;
      return; /* should be more violent than this */
      }

   parent->stringregion = curpstate->stringregion;
   curpstate->stringregion = rp;
   noMTevents--;
   actparent(E_Assign);
}


/*
 * event(x, y, C) -- generate an event at the program level.
 */

"event(x, y, C) - create event with event code x and event value y."

function{0,1} event(x,y,ce)
   body {
      struct progstate *dest;

      if (is:null(x)) {
	 x = curpstate->eventcode;
	 if (is:null(y)) y = curpstate->eventval;
	 }
      if (is:null(ce) && is:coexpr(curpstate->parentdesc))
	 ce = curpstate->parentdesc;
      else if (!is:coexpr(ce)) runerr(118,ce);
      dest = BlkLoc(ce)->coexpr.program;
      dest->eventcode = x;
      dest->eventval = y;
      if (mt_activate(&(dest->eventcode),&result,
			 (struct b_coexpr *)BlkLoc(ce)) == A_Cofail) {
         fail;
         }
       return result;
      }
end

/*
 * EvGet(c) - user function for reading event streams.
 */

"EvGet(c,flag) - read through the next event token having a code matched "
" by cset c."

/*
 *  EvGet returns the code of the matched token.  These keywords are also set:
 *    &eventcode     token code
 *    &eventvalue    token value
 */
function{0,1} EvGet(cs,flag)
   if !def:cset(cs,fullcs) then
      runerr(104,cs)

   body {
      register int c;
      tended struct descrip dummy;
      struct progstate *p;

      /*
       * Be sure an eventsource is available
       */
      if (!is:coexpr(curpstate->eventsource))
         runerr(118,curpstate->eventsource);

      /*
       * If our event source is a child of ours, assign its event mask.
       */
      p = BlkLoc(curpstate->eventsource)->coexpr.program;
      if (p->parent == curpstate)
	 p->eventmask = cs;

#ifdef Graphics
      if (Testb((word)E_MXevent, cs) &&
	  is:file(kywd_xwin[XKey_Window])) {
	 wbp _w_ = (wbp)BlkLoc(kywd_xwin[XKey_Window])->file.fd;
	 pollctr = pollevent();
	 if (pollctr == -1)
	    fatalerr(141, NULL);
	 if (BlkLoc(_w_->window->listp)->list.size > 0) {
	    c = wgetevent(_w_, &curpstate->eventval);
	    if (c == 0) {
	       StrLen(curpstate->eventcode) = 1;
	       StrLoc(curpstate->eventcode) =
		  (char *)&allchars[E_MXevent & 0xFF];
	       return curpstate->eventcode;
	       }
	    else if (c == -1)
	       runerr(141);
	    else
	       runerr(143);
	    }
	 }
#endif					/* Graphics */

      /*
       * Loop until we read an event allowed.
       */
      while (1) {
         /*
          * Activate the event source to produce the next event.
          */
	 dummy = cs;
	 if (mt_activate(&dummy, &curpstate->eventcode,
			 (struct b_coexpr *)BlkLoc(curpstate->eventsource)) ==
	     A_Cofail) fail;
	 deref(&curpstate->eventcode, &curpstate->eventcode);
	 if (!is:string(curpstate->eventcode) ||
	     StrLen(curpstate->eventcode) != 1) {
	    /*
	     * this event is out-of-band data; return or reject it
	     * depending on whether flag is null.
	     */
	    if (!is:null(flag))
	       return curpstate->eventcode;
	    else continue;
	    }

	 switch(*StrLoc(curpstate->eventcode)) {
	 case E_Cofail: case E_Coret: {
	    if (BlkLoc(curpstate->eventsource)->coexpr.id == 1) {
	       fail;
	       }
	    }
	    }

	 return curpstate->eventcode;
	 }
      }
end

#endif					/* MultiThread */

/*
 *  EVInit() - initialization.
 */

void EVInit()
   {
   int i;

   /*
    * Initialize the typech array, which is used if either file-based
    * or MT-based event monitoring is enabled.
    */

   for (i = 0; i <= MaxType; i++)
      typech[i] = '?';	/* initialize with error character */

#ifdef LargeInts
   typech[T_Lrgint]  = E_Lrgint;	/* long integer */
#endif					/* LargeInts */

   typech[T_Real]    = E_Real;		/* real number */
   typech[T_Cset]    = E_Cset;		/* cset */
   typech[T_File]    = E_File;		/* file block */
   typech[T_Record]  = E_Record;	/* record block */
   typech[T_Tvsubs]  = E_Tvsubs;	/* substring trapped variable */
   typech[T_External]= E_External;	/* external block */
   typech[T_List]    = E_List;		/* list header block */
   typech[T_Lelem]   = E_Lelem;		/* list element block */
   typech[T_Table]   = E_Table;		/* table header block */
   typech[T_Telem]   = E_Telem;		/* table element block */
   typech[T_Tvtbl]   = E_Tvtbl;		/* table elem trapped variable*/
   typech[T_Set]     = E_Set;		/* set header block */
   typech[T_Selem]   = E_Selem;		/* set element block */
   typech[T_Slots]   = E_Slots;		/* set/table hash slots */
   typech[T_Coexpr]  = E_Coexpr;	/* co-expression block (static) */
   typech[T_Refresh] = E_Refresh;	/* co-expression refresh block */


   /*
    * codes used elsewhere but not shown here:
    *    in the static region: E_Alien = alien (malloc block)
    *    in the static region: E_Free = free
    *    in the string region: E_String = string
    */
   }

/*
 * mmrefresh() - redraw screen, initially or after garbage collection.
 */

void mmrefresh()
   {
   char *p;
   word n;

   /*
    * If the monitor is asking for E_EndCollect events, then it
    * can handle these memory allocation "redraw" events.
    */
  if (!is:null(curpstate->eventmask) &&
       Testb((word)E_EndCollect, curpstate->eventmask)) {
      for (p = blkbase; p < blkfree; p += n) {
	 n = BlkSize(p);
	 EVVal(n, typech[(int)BlkType(p)]);	/* block region */
	 }
      EVVal(DiffPtrs(strfree, strbase), E_String);	/* string region */
      }
   }

#endif					/* EventMon */
