/*
 * File: rmemmgt.r
 *  Contents: block description arrays, memory initialization,
 *   garbage collection, dump routines
 */

/*
 * Prototypes
 */
static void postqual		(dptr dp);
static void markblock	(dptr dp);
static void markptr		(union block **ptr);
static void sweep		(struct b_coexpr *ce);
static void sweep_stk	(struct b_coexpr *ce);
static void reclaim		(void);
static void cofree		(void);
static void scollect		(word extra);
static int     qlcmp		(dptr  *q1,dptr  *q2);
static void adjust		(char *source, char *dest);
static void compact		(char *source);
static void mvc		(uword n, char *src, char *dest);

#ifdef MultiThread
static void markprogram	(struct progstate *pstate);
#endif					/*MultiThread*/

/*
 * Variables
 */

#ifndef MultiThread
word coll_stat = 0;             /* collections in static region */
word coll_str = 0;              /* collections in string region */
word coll_blk = 0;              /* collections in block region */
word coll_tot = 0;              /* total collections */
#endif					/* MultiThread */
word alcnum = 0;                /* co-expressions allocated since g.c. */

dptr *quallist;                 /* string qualifier list */
dptr *qualfree;                 /* qualifier list free pointer */
dptr *equallist;                /* end of qualifier list */

int qualfail;                   /* flag: qualifier list overflow */

/*
 * Allocated block size table (sizes given in bytes).  A size of -1 is used
 *  for types that have no blocks; a size of 0 indicates that the
 *  second word of the block contains the size; a value greater than
 *  0 is used for types with constant sized blocks.
 */

int bsizes[] = {
    -1,                       /* T_Null (0), not block */
    -1,                       /* T_Integer (1), not block */
     0,                       /* T_Lrgint (2), large integer */
     sizeof(struct b_real),   /* T_Real (3), real number */
     sizeof(struct b_cset),   /* T_Cset (4), cset */
     sizeof(struct b_file),   /* T_File (5), file block */
     0,                       /* T_Proc (6), procedure block */
     0,                       /* T_Record (7), record block */
     sizeof(struct b_list),   /* T_List (8), list header block */
     0,                       /* T_Lelem (9), list element block */
     sizeof(struct b_set),    /* T_Set (10), set header block */
     sizeof(struct b_selem),  /* T_Selem (11), set element block */
     sizeof(struct b_table),  /* T_Table (12), table header block */
     sizeof(struct b_telem),  /* T_Telem (13), table element block */
     sizeof(struct b_tvtbl),  /* T_Tvtbl (14), table element trapped variable */
     0,                       /* T_Slots (15), set/table hash block */
     sizeof(struct b_tvsubs), /* T_Tvsubs (16), substring trapped variable */
     0,                       /* T_Refresh (17), refresh block */
    -1,                       /* T_Coexpr (18), co-expression block */
     0,                       /* T_External (19) external block */
     -1,                      /* T_Kywdint (20), integer keyword variable */
     -1,                      /* T_Kywdpos (21), keyword &pos */
     -1,                      /* T_Kywdsubj (22), keyword &subject */
     -1,                      /* T_Kywdwin (23), keyword &window */
     -1,                      /* T_Kywdstr (24), string keyword variable */
     -1,                      /* T_Kywdevent (25), event keyword variable */
    };

/*
 * Table of offsets (in bytes) to first descriptor in blocks.  -1 is for
 *  types not allocated, 0 for blocks with no descriptors.
 */
int firstd[] = {
    -1,                       /* T_Null (0), not block */
    -1,                       /* T_Integer (1), not block */
     0,                       /* T_Lrgint (2), large integer */
     0,                       /* T_Real (3), real number */
     0,                       /* T_Cset (4), cset */
     3*WordSize,              /* T_File (5), file block */

#ifdef MultiThread
     8*WordSize,              /* T_Proc (6), procedure block */
#else					/* MultiThread */
     7*WordSize,              /* T_Proc (6), procedure block */
#endif					/* MultiThread */

     4*WordSize,              /* T_Record (7), record block */
     0,                       /* T_List (8), list header block */
     7*WordSize,              /* T_Lelem (9), list element block */
     0,                       /* T_Set (10), set header block */
     3*WordSize,              /* T_Selem (11), set element block */
     (4+HSegs)*WordSize,      /* T_Table (12), table header block */
     3*WordSize,              /* T_Telem (13), table element block */
     3*WordSize,              /* T_Tvtbl (14), table element trapped variable */
     0,                       /* T_Slots (15), set/table hash block */
     3*WordSize,              /* T_Tvsubs (16), substring trapped variable */

#if COMPILER
     2*WordSize,              /* T_Refresh (17), refresh block */
#else					/* COMPILER */
     (4+Wsizeof(struct pf_marker))*WordSize, /* T_Refresh (17), refresh block */
#endif					/* COMPILER */

    -1,                       /* T_Coexpr (18), co-expression block */
     0,                       /* T_External (19), external block */
     -1,                      /* T_Kywdint (20), integer keyword variable */
     -1,                      /* T_Kywdpos (21), keyword &pos */
     -1,                      /* T_Kywdsubj (22), keyword &subject */
     -1,                      /* T_Kywdwin (23), keyword &window */
     -1,                      /* T_Kywdstr (24), string keyword variable */
     -1,                      /* T_Kywdevent (25), event keyword variable */
    };

/*
 * Table of offsets (in bytes) to first pointer in blocks.  -1 is for
 *  types not allocated, 0 for blocks with no pointers.
 */
int firstp[] = {
    -1,                       /* T_Null (0), not block */
    -1,                       /* T_Integer (1), not block */
     0,                       /* T_Lrgint (2), large integer */
     0,                       /* T_Real (3), real number */
     0,                       /* T_Cset (4), cset */
     0,                       /* T_File (5), file block */
     0,                       /* T_Proc (6), procedure block */
     3*WordSize,              /* T_Record (7), record block */
     3*WordSize,              /* T_List (8), list header block */
     2*WordSize,              /* T_Lelem (9), list element block */
     4*WordSize,              /* T_Set (10), set header block */
     1*WordSize,              /* T_Selem (11), set element block */
     4*WordSize,              /* T_Table (12), table header block */
     1*WordSize,              /* T_Telem (13), table element block */
     1*WordSize,              /* T_Tvtbl (14), table element trapped variable */
     2*WordSize,              /* T_Slots (15), set/table hash block */
     0,                       /* T_Tvsubs (16), substring trapped variable */
     0,                       /* T_Refresh (17), refresh block */
    -1,                       /* T_Coexpr (18), co-expression block */
     0,                       /* T_External (19), external block */
     -1,                      /* T_Kywdint (20), integer keyword variable */
     -1,                      /* T_Kywdpos (21), keyword &pos */
     -1,                      /* T_Kywdsubj (22), keyword &subject */
     -1,                      /* T_Kywdwin (23), keyword &window */
     -1,                      /* T_Kywdstr (24), string keyword variable */
     -1,                      /* T_Kywdevent (25), event keyword variable */
    };

/*
 * Table of number of pointers in blocks.  -1 is for types not allocated and
 *  types without pointers, 0 for pointers through the end of the block.
 */
int ptrno[] = {
    -1,                       /* T_Null (0), not block */
    -1,                       /* T_Integer (1), not block */
    -1,                       /* T_Lrgint (2), large integer */
    -1,                       /* T_Real (3), real number */
    -1,                       /* T_Cset (4), cset */
    -1,                       /* T_File (5), file block */
    -1,                       /* T_Proc (6), procedure block */
     1,                       /* T_Record (7), record block */
     2,                       /* T_List (8), list header block */
     2,                       /* T_Lelem (9), list element block */
     HSegs,                   /* T_Set (10), set header block */
     1,                       /* T_Selem (11), set element block */
     HSegs,                   /* T_Table (12), table header block */
     1,                       /* T_Telem (13), table element block */
     1,                       /* T_Tvtbl (14), table element trapped variable */
     0,                       /* T_Slots (15), set/table hash block */
    -1,                       /* T_Tvsubs (16), substring trapped variable */
    -1,                       /* T_Refresh (17), refresh block */
    -1,                       /* T_Coexpr (18), co-expression block */
    -1,                       /* T_External (19), external block */
    -1,                       /* T_Kywdint (20), integer keyword variable */
    -1,                       /* T_Kywdpos (21), keyword &pos */
    -1,                       /* T_Kywdsubj (22), keyword &subject */
    -1,                       /* T_Kywdwin (23), keyword &window */
    -1,                       /* T_Kywdstr (24), string keyword variable */
    -1,                       /* T_Kywdevent (25), event keyword variable */
    };

/*
 * Table of block names used by debugging functions.
 */
char *blkname[] = {
   "illegal object",                    /* T_Null (0), not block */
   "illegal object",                    /* T_Integer (1), not block */
   "large integer",                     /* T_Largint (2) */
   "real number",                       /* T_Real (3) */
   "cset",                              /* T_Cset (4) */
   "file",                              /* T_File (5) */
   "procedure",                         /* T_Proc (6) */
   "record",                            /* T_Record (7) */
   "list",                              /* T_List (8) */
   "list element",                      /* T_Lelem (9) */
   "set",                               /* T_Set (10) */
   "set element",                       /* T_Selem (11) */
   "table",                             /* T_Table (12) */
   "table element",                     /* T_Telem (13) */
   "table element trapped variable",    /* T_Tvtbl (14) */
   "hash block",                        /* T_Slots (15) */
   "substring trapped variable",        /* T_Tvsubs (16) */
   "refresh block",                     /* T_Refresh (17) */
   "co-expression",                     /* T_Coexpr (18) */
   "external block",                    /* T_External (19) */
   "integer keyword variable",          /* T_Kywdint (20) */
   "&pos",                              /* T_Kywdpos (21) */
   "&subject",                          /* T_Kywdsubj (22) */
   "illegal object",                    /* T_Kywdwin (23) */
   "illegal object",                    /* T_Kywdstr (24) */
   "illegal object",                    /* T_Kywdevent (25) */
   };

/*
 * Sizes of hash chain segments.
 *  Table size must equal or exceed HSegs.
 */
uword segsize[] = {
   ((uword)HSlots),			/* segment 0 */
   ((uword)HSlots),			/* segment 1 */
   ((uword)HSlots) << 1,		/* segment 2 */
   ((uword)HSlots) << 2,		/* segment 3 */
   ((uword)HSlots) << 3,		/* segment 4 */
   ((uword)HSlots) << 4,		/* segment 5 */
   ((uword)HSlots) << 5,		/* segment 6 */
   ((uword)HSlots) << 6,		/* segment 7 */
   ((uword)HSlots) << 7,		/* segment 8 */
   ((uword)HSlots) << 8,		/* segment 9 */
   ((uword)HSlots) << 9,		/* segment 10 */
   ((uword)HSlots) << 10,		/* segment 11 */
   ((uword)HSlots) << 11,		/* segment 12 */
   ((uword)HSlots) << 12,		/* segment 13 */
   ((uword)HSlots) << 13,		/* segment 14 */
   ((uword)HSlots) << 14,		/* segment 15 */
   ((uword)HSlots) << 15,		/* segment 16 */
   ((uword)HSlots) << 16,		/* segment 17 */
   ((uword)HSlots) << 17,		/* segment 18 */
   ((uword)HSlots) << 18,		/* segment 19 */
   };

/*
 * initalloc - initialization routine to allocate memory regions
 */

#if COMPILER
void initalloc()
   {

#else					/* COMPILER */
#ifdef MultiThread
void initalloc(codesize,p)
struct progstate *p;
#else					/* MultiThread */
void initalloc(codesize)
#endif					/* MultiThread */
word codesize;
   {
#ifdef MultiThread
   struct region *ps, *pb;
#endif

   if ((uword)codesize > (unsigned)MaxBlock)
      error(NULL, "icode file too large");
   /*
    * Allocate icode region
    */
#ifdef MultiThread
   if (codesize)
#endif					/* MultiThread */
   if ((code = (char *)AllocReg(codesize)) == NULL)
      error(NULL,
	 "insufficient memory, corrupted icode file, or wrong platform");
#endif					/* COMPILER */

   /*
    * Set up allocated memory.	The regions are:
    *	Static memory region (not used)
    *	Allocated string region
    *	Allocate block region
    *	Qualifier list
    */

#ifdef MultiThread
   ps = p->stringregion;
   ps->free = ps->base = (char *)AllocReg(ps->size);
   if (ps->free == NULL)
      error(NULL, "insufficient memory for string region");
   ps->end = ps->base + ps->size;

   pb = p->blockregion;
   pb->free = pb->base = (char *)AllocReg(pb->size);
   if (pb->free == NULL)
      error(NULL, "insufficient memory for block region");
   pb->end = pb->base + pb->size;

   if (p == &rootpstate) {
      if ((quallist = (dptr *)malloc(qualsize)) == NULL)
         error(NULL, "insufficient memory for qualifier list");
      equallist = (dptr *)((char *)quallist + qualsize);
      }
#else					/* MultiThread */
   {
   uword t1, t2;
   t1 = ssize;
   t2 = abrsize;
   curstring = (struct region *)malloc(sizeof(struct region));
   curblock = (struct region *)malloc(sizeof(struct region));
   curstring->size = t1;
   curblock->size = t2;
   }
   curstring->next = curstring->prev = NULL;
   curstring->Gnext = curstring->Gprev = NULL;
   curblock->next = curblock->prev = NULL;
   curblock->Gnext = curblock->Gprev = NULL;
   if ((strfree = strbase = (char *)AllocReg(ssize)) == NULL)
      error(NULL, "insufficient memory for string region");
   strend = strbase + ssize;
   if ((blkfree = blkbase = (char *)AllocReg(abrsize)) == NULL)
      error(NULL, "insufficient memory for block region");
   blkend = blkbase + abrsize;
   if ((quallist = (dptr *)malloc(qualsize)) == NULL)
      error(NULL, "insufficient memory for qualifier list");
   equallist = (dptr *)((char *)quallist + qualsize);
#endif					/* MultiThread */
   }

/*
 * collect - do a garbage collection of currently active regions.
 */

int collect(region)
int region;
   {
   struct b_coexpr *cp;

#ifdef EventMon
   if (!noMTevents)
      EVVal((word)region,E_Collect);
#endif					/* EventMon */

   switch (region) {
      case Static:
         coll_stat++;
         break;
      case Strings:
         coll_str++;
         break;
      case Blocks:
         coll_blk++;
         break;
      }
   coll_tot++;

   alcnum = 0;

   /*
    * Garbage collection cannot be done until initialization is complete.
    */

#if !COMPILER
   if (sp == NULL)
      return 0;
#endif					/* !COMPILER */

   /*
    * Sync the values (used by sweep) in the coexpr block for &current
    *  with the current values.
    */
   cp = (struct b_coexpr *)BlkLoc(k_current);
   cp->es_tend = tend;

#if !COMPILER
   cp->es_pfp = pfp;
   cp->es_gfp = gfp;
   cp->es_efp = efp;
   cp->es_sp = sp;
#endif					/* !COMPILER */

   /*
    * Reset qualifier list.
    */
   qualfree = quallist;
   qualfail = 0;

   /*
    * Mark the stacks for &main and the current co-expression.
    */
#ifdef MultiThread
   markprogram(&rootpstate);
#endif					/* MultiThread */
   markblock(&k_main);
   markblock(&k_current);
   /*
    * Mark &subject and the cached s2 and s3 strings for map.
    */
#ifndef MultiThread
   postqual(&k_subject);
   postqual(&kywd_prog);
#endif					/* MultiThread */
   if (Qual(maps2))                     /*  caution: the cached arguments of */
      postqual(&maps2);                 /*  map may not be strings. */
   else if (Pointer(maps2))
      markblock(&maps2);
   if (Qual(maps3))
      postqual(&maps3);
   else if (Pointer(maps3))
      markblock(&maps3);

#ifdef Graphics
   /*
    * Mark file and list values for windows
    */
   {
     wsp ws;

     for (ws = wstates; ws ; ws = ws->next) {
	    if (is:file(ws->filep))
	      markblock(&(ws->filep));
	    if (is:list(ws->listp))
	      markblock(&(ws->listp));
        }
   }
#endif					/* Graphics */

   /*
    * Mark the globals and the statics.
    */

#ifndef MultiThread
   { register struct descrip *dp;
   for (dp = globals; dp < eglobals; dp++)
      if (Qual(*dp))
	 postqual(dp);
      else if (Pointer(*dp))
	 markblock(dp);

   for (dp = statics; dp < estatics; dp++)
      if (Qual(*dp))
	 postqual(dp);
      else if (Pointer(*dp))
	 markblock(dp);
   }

#ifdef Graphics
   if (is:file(kywd_xwin[XKey_Window]))
      markblock(&(kywd_xwin[XKey_Window]));
   if (is:file(lastEventWin))
      markblock(&(lastEventWin));
#endif					/* Graphics */
#endif					/* MultiThread */

   reclaim();

   /*
    * Turn off all the marks in all the block regions everywhere
    */
   { struct region *br;
   for (br = curblock->Gnext; br; br = br->Gnext) {
      char *source = br->base, *free = br->free;
      uword NoMark = (uword) ~F_Mark;
      while (source < free) {
	 BlkType(source) &= NoMark;
         source += BlkSize(source);
         }
      }
   for (br = curblock->Gprev; br; br = br->Gprev) {
      char *source = br->base, *free = br->free;
      uword NoMark = (uword) ~F_Mark;
      while (source < free) {
	 BlkType(source) &= NoMark;
         source += BlkSize(source);
         }
      }
   }

#ifdef EventMon
   if (!noMTevents) {
      mmrefresh();
      EVValD(&nulldesc, E_EndCollect);
      }
#endif					/* EventMon */

   return 1;
   }

/*
 * markprogram - traverse pointers out of a program state structure
 */

#ifdef MultiThread
#define PostDescrip(d) \
   if (Qual(d)) \
      postqual(&(d)); \
   else if (Pointer(d))\
      markblock(&(d));

static void markprogram(pstate)
struct progstate *pstate;
   {
   struct descrip *dp;

   PostDescrip(pstate->parentdesc);
   PostDescrip(pstate->eventmask);
   PostDescrip(pstate->opcodemask);
   PostDescrip(pstate->eventcode);
   PostDescrip(pstate->eventval);
   PostDescrip(pstate->eventsource);

   /* Kywd_err, &error, always an integer */
   /* Kywd_pos, &pos, always an integer */
   postqual(&(pstate->ksub));
   postqual(&(pstate->Kywd_prog));
   /* Kywd_ran, &random, always an integer */
   /* Kywd_trc, &trace, always an integer */

   /*
    * Mark the globals and the statics.
    */
   for (dp = pstate->Globals; dp < pstate->Eglobals; dp++)
      if (Qual(*dp))
	 postqual(dp);
      else if (Pointer(*dp))
	 markblock(dp);

   for (dp = pstate->Statics; dp < pstate->Estatics; dp++)
      if (Qual(*dp))
	 postqual(dp);
      else if (Pointer(*dp))
	 markblock(dp);

   /*
    * no marking for &x, &y, &row, &col, &interval, all integers
    */
#ifdef Graphics
   PostDescrip(pstate->LastEventWin);	/* last Event() win */
   PostDescrip(pstate->Kywd_xwin[XKey_Window]);	/* &window */
#endif					/* Graphics */

   PostDescrip(pstate->K_errorvalue);
   PostDescrip(pstate->T_errorvalue);
   }
#endif					/* MultiThread */

/*
 * postqual - mark a string qualifier.  Strings outside the string space
 *  are ignored.
 */

static void postqual(dp)
dptr dp;
   {
   char *newqual;

   if (InRange(strbase,StrLoc(*dp),strfree + 1)) {
      /*
       * The string is in the string space.  Add it to the string qualifier
       *  list, but before adding it, expand the string qualifier list if
       *  necessary.
       */
      if (qualfree >= equallist) {

	 /* reallocate a new qualifier list that's twice as large */
	 newqual = realloc(quallist, 2 * qualsize);
	 if (newqual) {
	    quallist = (dptr *)newqual;
	    qualfree = (dptr *)(newqual + qualsize);
	    qualsize *= 2;
	    equallist = (dptr *)(newqual + qualsize);
	    }
	 else {
            qualfail = 1;
            return;
	    }

         }
      *qualfree++ = dp;
      }
   }

/*
 * markblock - mark each accessible block in the block region and build
 *  back-list of descriptors pointing to that block. (Phase I of garbage
 *  collection.)
 */
static void markblock(dp)
dptr dp;
   {
   register dptr dp1;
   register char *block, *endblock;
   word type, fdesc;
   int numptr;
   register union block **ptr, **lastptr;

   if (Var(*dp)) {
       if (dp->dword & F_Typecode) {
          switch (Type(*dp)) {
             case T_Kywdint:
             case T_Kywdpos:
             case T_Kywdsubj:
                /*
                 * The descriptor points to a keyword, not a block.
                 */
                return;
             }
          }
       else if (Offset(*dp) == 0) {
          /*
           * The descriptor is a simple variable not residing in a block.
           */
          return;
          }
      }

   /*
    * Get the block to which dp points.
    */
   block = (char *)BlkLoc(*dp);

   if (InRange(blkbase,block,blkfree)) {
      type = BlkType(block);
      if ((uword)type <= MaxType) {

         /*
          * The type is valid, which indicates that this block has not
          *  been marked.  Point endblock to the byte past the end
          *  of the block.
          */
         endblock = block + BlkSize(block);
         }

      /*
       * Add dp to the back chain for the block and point the
       *  block (via the type field) to dp.vword.
       */
      BlkLoc(*dp) = (union block *)type;
      BlkType(block) = (uword)&BlkLoc(*dp);

      if ((uword)type <= MaxType) {
         /*
          * The block was not marked; process pointers and descriptors
          *  within the block.
          */
         if ((fdesc = firstp[type]) > 0) {
            /*
             * The block contains pointers; mark each pointer.
             */
            ptr = (union block **)(block + fdesc);
	    numptr = ptrno[type];
	    if (numptr > 0)
	       lastptr = ptr + numptr;
	    else
	       lastptr = (union block **)endblock;
	    for (; ptr < lastptr; ptr++)
	       if (*ptr != NULL)
                  markptr(ptr);
	    }
         if ((fdesc = firstd[type]) > 0)
            /*
             * The block contains descriptors; mark each descriptor.
             */
            for (dp1 = (dptr)(block + fdesc);
                 (char *)dp1 < endblock; dp1++) {
               if (Qual(*dp1))
                  postqual(dp1);
               else if (Pointer(*dp1))
                  markblock(dp1);
               }
         }
      }

   else if ((unsigned int)BlkType(block) == T_Coexpr) {
      struct b_coexpr *cp;
      struct astkblk *abp;
      int i;
      struct descrip adesc;

      /*
       * dp points to a co-expression block that has not been
       *  marked.  Point the block to dp.  Sweep the interpreter
       *  stack in the block.  Then mark the block for the
       *  activating co-expression and the refresh block.
       */
      BlkType(block) = (uword)dp;
      sweep((struct b_coexpr *)block);

#ifdef MultiThread
      if (((struct b_coexpr *)block)+1 ==
         (struct b_coexpr *)((struct b_coexpr *)block)->program){
         /*
          * This coexpr is an &main; traverse its roots
          */
         markprogram(((struct b_coexpr *)block)->program);
         }
#endif					/* MultiThread */

#ifdef Coexpr
      /*
       * Mark the activators of this co-expression.   The activators are
       *  stored as a list of addresses, but markblock requires the address
       *  of a descriptor.  To accommodate markblock, the dummy descriptor
       *  adesc is filled in with each activator address in turn and then
       *  marked.  Since co-expressions and the descriptors that reference
       *  them don't participate in the back-chaining scheme, it's ok to
       *  reuse the descriptor in this manner.
       */
      cp = (struct b_coexpr *)block;
      adesc.dword = D_Coexpr;
      for (abp = cp->es_actstk; abp != NULL; abp = abp->astk_nxt) {
         for (i = 1; i <= abp->nactivators; i++) {
            BlkLoc(adesc) = (union block *)abp->arec[i-1].activator;
            markblock(&adesc);
            }
         }
      if(BlkLoc(cp->freshblk) != NULL)
         markblock(&((struct b_coexpr *)block)->freshblk);
#endif					/* Coexpr */
      }

   else {
      struct region *rp;

      /*
       * Look for this block in other allocated block regions.
       */
      for (rp = curblock->Gnext; rp; rp = rp->Gnext)
	 if (InRange(rp->base,block,rp->free)) break;

      if (rp == NULL)
         for (rp = curblock->Gprev; rp; rp = rp->Gprev)
            if (InRange(rp->base,block,rp->free)) break;

      /*
       * If this block is not in a block region, its something else
       *  like a procedure block.
       */
      if (rp == NULL)
         return;

      /*
       * Get this block's type field; return if it is marked
       */
      type = BlkType(block);
      if ((uword)type > MaxType)
         return;

      /*
       * this is an unmarked block outside the (collecting) block region;
       * process pointers and descriptors within the block.
       *
       * The type is valid, which indicates that this block has not
       *  been marked.  Point endblock to the byte past the end
       *  of the block.
       */
      endblock = block + BlkSize(block);

      BlkType(block) |= F_Mark;			/* mark the block */

      if ((fdesc = firstp[type]) > 0) {
         /*
          * The block contains pointers; mark each pointer.
          */
         ptr = (union block **)(block + fdesc);
	 numptr = ptrno[type];
	 if (numptr > 0)
	    lastptr = ptr + numptr;
	 else
	    lastptr = (union block **)endblock;
	 for (; ptr < lastptr; ptr++)
	    if (*ptr != NULL)
               markptr(ptr);
	 }
      if ((fdesc = firstd[type]) > 0)
         /*
          * The block contains descriptors; mark each descriptor.
          */
         for (dp1 = (dptr)(block + fdesc);
              (char *)dp1 < endblock; dp1++) {
            if (Qual(*dp1))
               postqual(dp1);
            else if (Pointer(*dp1))
               markblock(dp1);
            }
      }
   }

/*
 * markptr - just like mark block except the object pointing at the block
 *  is just a block pointer, not a descriptor.
 */

static void markptr(ptr)
union block **ptr;
   {
   register dptr dp;
   register char *block, *endblock;
   word type, fdesc;
   int numptr;
   register union block **ptr1, **lastptr;

   /*
    * Get the block to which ptr points.
    */
   block = (char *)*ptr;
   if (InRange(blkbase,block,blkfree)) {
      type = BlkType(block);
      if ((uword)type <= MaxType) {
         /*
          * The type is valid, which indicates that this block has not
          *  been marked.  Point endblock to the byte past the end
          *  of the block.
          */
         endblock = block + BlkSize(block);
         }

      /*
       * Add ptr to the back chain for the block and point the
       *  block (via the type field) to ptr.
       */
      *ptr = (union block *)type;
      BlkType(block) = (uword)ptr;

      if ((uword)type <= MaxType) {
         /*
          * The block was not marked; process pointers and descriptors
          *  within the block.
          */
         if ((fdesc = firstp[type]) > 0) {
            /*
             * The block contains pointers; mark each pointer.
             */
            ptr1 = (union block **)(block + fdesc);
            numptr = ptrno[type];
            if (numptr > 0)
               lastptr = ptr1 + numptr;
            else
               lastptr = (union block **)endblock;
            for (; ptr1 < lastptr; ptr1++)
               if (*ptr1 != NULL)
                  markptr(ptr1);
            }
         if ((fdesc = firstd[type]) > 0)
            /*
             * The block contains descriptors; mark each descriptor.
             */
            for (dp = (dptr)(block + fdesc);
                 (char *)dp < endblock; dp++) {
               if (Qual(*dp))
                  postqual(dp);
               else if (Pointer(*dp))
                  markblock(dp);
               }
         }
      }

   else {
      struct region *rp;

      /*
       * Look for this block in other allocated block regions.
       */
      for (rp = curblock->Gnext;rp;rp = rp->Gnext)
	 if (InRange(rp->base,block,rp->free)) break;

      if (rp == NULL)
         for (rp = curblock->Gprev;rp;rp = rp->Gprev)
            if (InRange(rp->base,block,rp->free)) break;

      /*
       * If this block is not in a block region, its something else
       *  like a procedure block.
       */
      if (rp == NULL)
         return;

      /*
       * Get this block's type field; return if it is marked
       */
      type = BlkType(block);
      if ((uword)type > MaxType)
         return;

      /*
       * this is an unmarked block outside the (collecting) block region;
       * process pointers and descriptors within the block.
       *
       * The type is valid, which indicates that this block has not
       *  been marked.  Point endblock to the byte past the end
       *  of the block.
       */
      endblock = block + BlkSize(block);

      BlkType(block) |= F_Mark;			/* mark the block */

      if ((fdesc = firstp[type]) > 0) {
         /*
          * The block contains pointers; mark each pointer.
          */
         ptr1 = (union block **)(block + fdesc);
         numptr = ptrno[type];
         if (numptr > 0)
	    lastptr = ptr1 + numptr;
	 else
	    lastptr = (union block **)endblock;
	 for (; ptr1 < lastptr; ptr1++)
	    if (*ptr1 != NULL)
               markptr(ptr1);
	 }
      if ((fdesc = firstd[type]) > 0)
         /*
          * The block contains descriptors; mark each descriptor.
          */
         for (dp = (dptr)(block + fdesc);
              (char *)dp < endblock; dp++) {
            if (Qual(*dp))
               postqual(dp);
            else if (Pointer(*dp))
               markblock(dp);
            }
         }
   }

/*
 * sweep - sweep the chain of tended descriptors for a co-expression
 *  marking the descriptors.
 */

static void sweep(ce)
struct b_coexpr *ce;
   {
   register struct tend_desc *tp;
   register int i;

   for (tp = ce->es_tend; tp != NULL; tp = tp->previous) {
      for (i = 0; i < tp->num; ++i) {
         if (Qual(tp->d[i]))
            postqual(&tp->d[i]);
         else if (Pointer(tp->d[i])) {
            if(BlkLoc(tp->d[i]) != NULL)
               markblock(&tp->d[i]);
	    }
         }
      }
#if !COMPILER
   sweep_stk(ce);
#endif					/* !COMPILER */
   }

#if !COMPILER
/*
 * sweep_stk - sweep the stack, marking all descriptors there.  Method
 *  is to start at a known point, specifically, the frame that the
 *  fp points to, and then trace back along the stack looking for
 *  descriptors and local variables, marking them when they are found.
 *  The sp starts at the first frame, and then is moved down through
 *  the stack.  Procedure, generator, and expression frames are
 *  recognized when the sp is a certain distance from the fp, gfp,
 *  and efp respectively.
 *
 * Sweeping problems can be manifested in a variety of ways due to
 *  the "if it can't be identified it's a descriptor" methodology.
 */

static void sweep_stk(ce)
struct b_coexpr *ce;
   {
   register word *s_sp;
   register struct pf_marker *fp;
   register struct gf_marker *s_gfp;
   register struct ef_marker *s_efp;
   word nargs, type = 0, gsize = 0;

   fp = ce->es_pfp;
   s_gfp = ce->es_gfp;
   if (s_gfp != 0) {
      type = s_gfp->gf_gentype;
      if (type == G_Psusp)
         gsize = Wsizeof(*s_gfp);
      else
         gsize = Wsizeof(struct gf_smallmarker);
      }
   s_efp = ce->es_efp;
   s_sp =  ce->es_sp;
   nargs = 0;                           /* Nargs counter is 0 initially. */

#ifdef MultiThread
   if (fp == 0) {
      if (is:list(* (dptr) (s_sp - 1))) {
	 /*
	  * this is the argument list of an un-started task
	  */
         if (Pointer(*((dptr)(&s_sp[-1])))) {
            markblock((dptr)&s_sp[-1]);
	    }
	 }
      }
#endif					/* MultiThread */

   while ((fp != 0 || nargs)) {         /* Keep going until current fp is
                                            0 and no arguments are left. */
      if (s_sp == (word *)fp + Vwsizeof(*pfp) - 1) {
                                        /* sp has reached the upper
                                            boundary of a procedure frame,
                                            process the frame. */
         s_efp = fp->pf_efp;            /* Get saved efp out of frame */
         s_gfp = fp->pf_gfp;            /* Get save gfp */
         if (s_gfp != 0) {
            type = s_gfp->gf_gentype;
            if (type == G_Psusp)
               gsize = Wsizeof(*s_gfp);
            else
               gsize = Wsizeof(struct gf_smallmarker);
            }
         s_sp = (word *)fp - 1;         /* First argument descriptor is
                                            first word above proc frame */
         nargs = fp->pf_nargs;
         fp = fp->pf_pfp;
         }
      else if (s_gfp != NULL && s_sp == (word *)s_gfp + gsize - 1) {
                                        /* The sp has reached the lower end
                                            of a generator frame, process
                                            the frame.*/
         if (type == G_Psusp)
            fp = s_gfp->gf_pfp;
         s_sp = (word *)s_gfp - 1;
         s_efp = s_gfp->gf_efp;
         s_gfp = s_gfp->gf_gfp;
         if (s_gfp != 0) {
            type = s_gfp->gf_gentype;
            if (type == G_Psusp)
               gsize = Wsizeof(*s_gfp);
            else
               gsize = Wsizeof(struct gf_smallmarker);
            }
         nargs = 1;
         }
      else if (s_sp == (word *)s_efp + Wsizeof(*s_efp) - 1) {
                                            /* The sp has reached the upper
                                                end of an expression frame,
                                                process the frame. */
         s_gfp = s_efp->ef_gfp;         /* Restore gfp, */
         if (s_gfp != 0) {
            type = s_gfp->gf_gentype;
            if (type == G_Psusp)
               gsize = Wsizeof(*s_gfp);
            else
               gsize = Wsizeof(struct gf_smallmarker);
            }
         s_efp = s_efp->ef_efp;         /*  and efp from frame. */
         s_sp -= Wsizeof(*s_efp);       /* Move past expression frame marker. */
         }
      else {                            /* Assume the sp is pointing at a
                                            descriptor. */
         if (Qual(*((dptr)(&s_sp[-1]))))
            postqual((dptr)&s_sp[-1]);
         else if (Pointer(*((dptr)(&s_sp[-1])))) {
            markblock((dptr)&s_sp[-1]);
	    }
         s_sp -= 2;                     /* Move past descriptor. */
         if (nargs)                     /* Decrement argument count if in an*/
            nargs--;                    /*  argument list. */
         }
      }
   }
#endif					/* !COMPILER */

/*
 * reclaim - reclaim space in the allocated memory regions. The marking
 *   phase has already been completed.
 */

static void reclaim()
   {
   /*
    * Collect available co-expression blocks.
    */
   cofree();

   /*
    * Collect the string space leaving it where it is.
    */
   if (!qualfail)
      scollect((word)0);

   /*
    * Adjust the blocks in the block region in place.
    */
   adjust(blkbase,blkbase);

   /*
    * Compact the block region.
    */
   compact(blkbase);
   }

/*
 * cofree - collect co-expression blocks.  This is done after
 *  the marking phase of garbage collection and the stacks that are
 *  reachable have pointers to data blocks, rather than T_Coexpr,
 *  in their type field.
 */

static void cofree()
   {
   register struct b_coexpr **ep, *xep;
   register struct astkblk *abp, *xabp;

   /*
    * Reset the type for &main.
    */

#ifdef MultiThread
   rootpstate.Mainhead->title = T_Coexpr;
#else					/* MultiThread */
   BlkLoc(k_main)->coexpr.title = T_Coexpr;
#endif					/* MultiThread */

   /*
    * The co-expression blocks are linked together through their
    *  nextstk fields, with stklist pointing to the head of the list.
    *  The list is traversed and each stack that was not marked
    *  is freed.
    */
   ep = &stklist;
   while (*ep != NULL) {
      if (BlkType(*ep) == T_Coexpr) {
         xep = *ep;
         *ep = (*ep)->nextstk;
         /*
          * Free the astkblks.  There should always be one and it seems that
          *  it's not possible to have more than one, but nonetheless, the
          *  code provides for more than one.
          */
         for (abp = xep->es_actstk; abp; ) {
            xabp = abp;
            abp = abp->astk_nxt;
            free((pointer)xabp);
            }
         #ifdef CoClean
	    coclean(xep->cstate);
         #endif				/* CoClean */
         free((pointer)xep);
         }
      else {
         BlkType(*ep) = T_Coexpr;
         ep = &(*ep)->nextstk;
         }
      }
   }

/*
 * scollect - collect the string space.  quallist is a list of pointers to
 *  descriptors for all the reachable strings in the string space.  For
 *  ease of description, it is referred to as if it were composed of
 *  descriptors rather than pointers to them.
 */

static void scollect(extra)
word extra;
   {
   register char *source, *dest;
   register dptr *qptr;
   char *cend;

   if (qualfree <= quallist) {
      /*
       * There are no accessible strings.  Thus, there are none to
       *  collect and the whole string space is free.
       */
      strfree = strbase;
      return;
      }
   /*
    * Sort the pointers on quallist in ascending order of string
    *  locations.
    */
   qsort((char *)quallist, (int)(DiffPtrs((char *)qualfree,(char *)quallist)) /
     sizeof(dptr *), sizeof(dptr), (int (*)())qlcmp);
   /*
    * The string qualifiers are now ordered by starting location.
    */
   dest = strbase;
   source = cend = StrLoc(**quallist);

   /*
    * Loop through qualifiers for accessible strings.
    */
   for (qptr = quallist; qptr < qualfree; qptr++) {
      if (StrLoc(**qptr) > cend) {

         /*
          * qptr points to a qualifier for a string in the next clump.
          *  The last clump is moved, and source and cend are set for
          *  the next clump.
          */
         while (source < cend)
            *dest++ = *source++;
         source = cend = StrLoc(**qptr);
         }
      if ((StrLoc(**qptr) + StrLen(**qptr)) > cend)
         /*
          * qptr is a qualifier for a string in this clump; extend
          *  the clump.
          */
         cend = StrLoc(**qptr) + StrLen(**qptr);
      /*
       * Relocate the string qualifier.
       */
      StrLoc(**qptr) = StrLoc(**qptr) + DiffPtrs(dest,source) + (uword)extra;
      }

   /*
    * Move the last clump.
    */
   while (source < cend)
      *dest++ = *source++;
   strfree = dest;
   }

/*
 * qlcmp - compare the location fields of two string qualifiers for qsort.
 */

static int qlcmp(q1,q2)
dptr *q1, *q2;
   {
   return (int)(DiffPtrs(StrLoc(**q1),StrLoc(**q2)));
   }

/*
 * adjust - adjust pointers into the block region, beginning with block oblk
 *  and basing the "new" block region at nblk.  (Phase II of garbage
 *  collection.)
 */

static void adjust(source,dest)
char *source, *dest;
   {
   register union block **nxtptr, **tptr;

   /*
    * Loop through to the end of allocated block region, moving source
    *  to each block in turn and using the size of a block to find the
    *  next block.
    */
   while (source < blkfree) {
      if ((uword)(nxtptr = (union block **)BlkType(source)) > MaxType) {

         /*
          * The type field of source is a back pointer.  Traverse the
          *  chain of back pointers, changing each block location from
          *  source to dest.
          */
         while ((uword)nxtptr > MaxType) {
            tptr = nxtptr;
            nxtptr = (union block **) *nxtptr;
            *tptr = (union block *)dest;
            }
         BlkType(source) = (uword)nxtptr | F_Mark;
         dest += BlkSize(source);
         }
      source += BlkSize(source);
      }
   }

/*
 * compact - compact good blocks in the block region. (Phase III of garbage
 *  collection.)
 */

static void compact(source)
char *source;
   {
   register char *dest;
   register word size;

   /*
    * Start dest at source.
    */
   dest = source;

   /*
    * Loop through to end of allocated block space, moving source
    *  to each block in turn, using the size of a block to find the next
    *  block.  If a block has been marked, it is copied to the
    *  location pointed to by dest and dest is pointed past the end
    *  of the block, which is the location to place the next saved
    *  block.  Marks are removed from the saved blocks.
    */
   while (source < blkfree) {
      size = BlkSize(source);
      if (BlkType(source) & F_Mark) {
         BlkType(source) &= ~F_Mark;
         if (source != dest)
            mvc((uword)size,source,dest);
         dest += size;
         }
      source += size;
      }

   /*
    * dest is the location of the next free block.  Now that compaction
    *  is complete, point blkfree to that location.
    */
   blkfree = dest;
   }

/*
 * mvc - move n bytes from src to dest
 *
 *      The algorithm is to copy the data (using memcpy) in the largest
 * chunks possible, which is the size of area of the source data not in
 * the destination area (ie non-overlapped area).  (Chunks are expected to
 * be fairly large.)
 */

static void mvc(n, src, dest)
uword n;
register char *src, *dest;
   {
   register char *srcend, *destend;        /* end of data areas */
   word copy_size;                  /* of size copy_size */
   word left_over;         /* size of last chunk < copy_size */

   if (n == 0)
      return;

   srcend  = src + n;    /* point at byte after src data */
   destend = dest + n;   /* point at byte after dest area */

   if ((destend <= src) || (srcend <= dest))  /* not overlapping */
      memcpy(dest,src,n);

   else {                     /* overlapping data areas */
      if (dest < src) {
         /*
          * The move is from higher memory to lower memory.
          */
         copy_size = DiffPtrs(src,dest);

         /* now loop round copying copy_size chunks of data */

         do {
            memcpy(dest,src,copy_size);
            dest = src;
            src = src + copy_size;
            }
         while (DiffPtrs(srcend,src) > copy_size);

         left_over = DiffPtrs(srcend,src);

         /* copy final fragment of data - if there is one */

         if (left_over > 0)
            memcpy(dest,src,left_over);
         }

      else if (dest > src) {
         /*
          * The move is from lower memory to higher memory.
          */
         copy_size = DiffPtrs(destend,srcend);

         /* now loop round copying copy_size chunks of data */

         do {
            destend = srcend;
            srcend  = srcend - copy_size;
            memcpy(destend,srcend,copy_size);
            }
         while (DiffPtrs(srcend,src) > copy_size);

         left_over = DiffPtrs(srcend,src);

         /* copy intial fragment of data - if there is one */

         if (left_over > 0) memcpy(dest,src,left_over);
         }

      } /* end of overlapping data area code */

   /*
    *  Note that src == dest implies no action
    */
   }

#ifdef DeBugIconx
/*
 * descr - dump a descriptor.  Used only for debugging.
 */

void descr(dp)
dptr dp;
   {
   int i;

   fprintf(stderr,"%08lx: ",(long)dp);
   if (Qual(*dp))
      fprintf(stderr,"%15s","qualifier");

   else if (Var(*dp))
      fprintf(stderr,"%15s","variable");
   else {
      i =  Type(*dp);
      switch (i) {
         case T_Null:
            fprintf(stderr,"%15s","null");
            break;
         case T_Integer:
            fprintf(stderr,"%15s","integer");
            break;
         default:
            fprintf(stderr,"%15s",blkname[i]);
         }
      }
   fprintf(stderr," %08lx %08lx\n",(long)dp->dword,(long)IntVal(*dp));
   }

/*
 * blkdump - dump the allocated block region.  Used only for debugging.
 *   NOTE:  Not adapted for multiple regions.
 */

void blkdump()
   {
   register char *blk;
   register word type, size, fdesc;
   register dptr ndesc;

   fprintf(stderr,
      "\nDump of allocated block region.  base:%08lx free:%08lx max:%08lx\n",
         (long)blkbase,(long)blkfree,(long)blkend);
   fprintf(stderr,"  loc     type              size  contents\n");

   for (blk = blkbase; blk < blkfree; blk += BlkSize(blk)) {
      type = BlkType(blk);
      size = BlkSize(blk);
      fprintf(stderr," %08lx   %15s   %4ld\n",(long)blk,blkname[type],
         (long)size);
      if ((fdesc = firstd[type]) > 0)
         for (ndesc = (dptr)(blk + fdesc);
               ndesc < (dptr)(blk + size); ndesc++) {
            fprintf(stderr,"                                 ");
            descr(ndesc);
            }
      fprintf(stderr,"\n");
      }
   fprintf(stderr,"end of block region.\n");
   }
#endif					/* DeBugIconx */
