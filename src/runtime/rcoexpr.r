/*
 * File: rcoexpr.r -- co_init, co_chng
 */

/*
 * co_init - use the contents of the refresh block to initialize the
 *  co-expression.
 */
void co_init(sblkp)
struct b_coexpr *sblkp;
{
   register word *newsp;
   register struct b_refresh *rblkp;
   register dptr dp, dsp;
   int frame_size;
   word stack_strt;
   int na, nl, nt, i;

   /*
    * Get pointer to refresh block.
    */
   rblkp = (struct b_refresh *)BlkLoc(sblkp->freshblk);
   na = (rblkp->pfmkr).pf_nargs + 1; /* number of arguments */
   nl = (int)rblkp->numlocals;       /* number of locals */

   /*
    * The interpreter stack starts at word after co-expression stack block.
    * There is no longer C state in this region; pthreads makes another stack.
    */

   newsp = (word *)((char *)sblkp + sizeof(struct b_coexpr));
   sblkp->es_argp = (dptr)newsp;	/* args are first thing on stack */

   /*
    * Copy arguments onto new stack.
    */
   dsp = sblkp->es_argp;
   dp = rblkp->elems;
   for (i = 1; i <=  na; i++)
      *dsp++ = *dp++;

   /*
    * Set up state variables and initialize procedure frame.
    */
   *((struct pf_marker *)dsp) = rblkp->pfmkr;
   sblkp->es_pfp = (struct pf_marker *)dsp;
   sblkp->es_tend = NULL;
   dsp = (dptr)((word *)dsp + Vwsizeof(*pfp));
   sblkp->es_ipc.opnd = rblkp->ep;
   sblkp->es_gfp = 0;
   sblkp->es_efp = 0;
   sblkp->es_ilevel = 0;
   sblkp->tvalloc = NULL;

   /*
    * Copy locals into the co-expression.
    */
   for (i = 1; i <= nl; i++)
      *dsp++ = *dp++;

   /*
    * Push two null descriptors on the stack.
    */
   *dsp++ = nulldesc;
   *dsp++ = nulldesc;

   sblkp->es_sp = (word *)dsp - 1;
   }

/*
 * co_chng - high-level co-expression context switch.
 */
int co_chng(ncp, valloc, rsltloc, swtch_typ, first)
struct b_coexpr *ncp;
struct descrip *valloc; /* location of value being transmitted */
struct descrip *rsltloc;/* location to put result */
int swtch_typ;          /* A_Coact, A_Coret, A_Cofail, or A_MTEvent */
int first;
{
   register struct b_coexpr *ccp;
   static int coexp_act;     /* used to pass signal across activations */
                             /* back to whomever activates, if they care */

   ccp = (struct b_coexpr *)BlkLoc(k_current);

   /*
    * Determine if we need to transmit a value.
    */
   if (valloc != NULL) {

      /*
       * Determine if we need to dereference the transmitted value.
       */
      if (Var(*valloc))
         retderef(valloc, (word *)glbl_argp, sp);

      if (ncp->tvalloc != NULL)
         *ncp->tvalloc = *valloc;
      }
   ncp->tvalloc = NULL;
   ccp->tvalloc = rsltloc;

   /*
    * Save state of current co-expression.
    */
   ccp->es_pfp = pfp;
   ccp->es_argp = glbl_argp;
   ccp->es_tend = tend;

   ccp->es_efp = efp;
   ccp->es_gfp = gfp;
   ccp->es_ipc = ipc;
   ccp->es_sp = sp;
   ccp->es_ilevel = ilevel;

   if (k_trace)
      cotrace(ccp, ncp, swtch_typ, valloc);

   /*
    * Establish state for new co-expression.
    */
   pfp = ncp->es_pfp;
   tend = ncp->es_tend;

   efp = ncp->es_efp;
   gfp = ncp->es_gfp;
   ipc = ncp->es_ipc;
   sp = ncp->es_sp;
   ilevel = (int)ncp->es_ilevel;

   glbl_argp = ncp->es_argp;
   BlkLoc(k_current) = (union block *)ncp;

   coexp_act = swtch_typ;
   coswitch(ccp->cstate, ncp->cstate,first);
   return coexp_act;
   }

/*
 * new_context - determine what function to call to execute the new
 *  co-expression; this completes the context switch.
 */
void new_context(fsig,cargp)
int fsig;
dptr cargp;
   {
   interp(fsig, cargp);
   }
