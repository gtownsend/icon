/*
 * file: lmisc.r
 *   Contents: [O]create, activate
 */

/*
 * create - return an entry block for a co-expression.
 */
#if COMPILER
struct b_coexpr *create(fnc, cproc, ntemps, wrk_size)
continuation fnc;
struct b_proc *cproc;
int ntemps;
int wrk_size;
#else					/* COMPILER */

int Ocreate(entryp, cargp)
word *entryp;
register dptr cargp;
#endif					/* COMPILER */
   {

#ifdef Coexpr
   tended struct b_coexpr *sblkp;
   register struct b_refresh *rblkp;
   register dptr dp, ndp;
   int na, nl, i;

#if !COMPILER
   struct b_proc *cproc;

   /* cproc is the Icon procedure that create occurs in */
   cproc = (struct b_proc *)BlkLoc(glbl_argp[0]);
#endif					/* COMPILER */

   /*
    * Calculate number of arguments and number of local variables.
    */
#if COMPILER
   na = abs((int)cproc->nparam);
#else					/* COMPILER */
   na = pfp->pf_nargs + 1;  /* includes Arg0 */
#endif					/* COMPILER */
   nl = (int)cproc->ndynam;

   /*
    * Get a new co-expression stack and initialize.
    */

#ifdef MultiThread
   Protect(sblkp = alccoexp(0, 0), err_msg(0, NULL));
#else					/* MultiThread */
   Protect(sblkp = alccoexp(), err_msg(0, NULL));
#endif					/* MultiThread */


   if (!sblkp)
#if COMPILER
      return NULL;
#else					/* COMPILER */
      Fail;
#endif					/* COMPILER */

   /*
    * Get a refresh block for the new co-expression.
    */
#if COMPILER
   Protect(rblkp = alcrefresh(na, nl, ntemps, wrk_size), err_msg(0,NULL));
#else					/* COMPILER */
   Protect(rblkp = alcrefresh(entryp, na, nl),err_msg(0,NULL));
#endif					/* COMPILER */
   if (!rblkp)
#if COMPILER
      return NULL;
#else					/* COMPILER */
      Fail;
#endif					/* COMPILER */

   sblkp->freshblk.dword = D_Refresh;
   BlkLoc(sblkp->freshblk) = (union block *) rblkp;

#if !COMPILER
   /*
    * Copy current procedure frame marker into refresh block.
    */
   rblkp->pfmkr = *pfp;
   rblkp->pfmkr.pf_pfp = 0;
#endif					/* COMPILER */

   /*
    * Copy arguments into refresh block.
    */
   ndp = rblkp->elems;
   dp = glbl_argp;
   for (i = 1; i <= na; i++)
      *ndp++ = *dp++;

   /*
    * Copy locals into the refresh block.
    */
#if COMPILER
   dp = pfp->tend.d;
#else					/* COMPILER */
   dp = &(pfp->pf_locals)[0];
#endif					/* COMPILER */
   for (i = 1; i <= nl; i++)
      *ndp++ = *dp++;

   /*
    * Use the refresh block to finish initializing the co-expression stack.
    */
   co_init(sblkp);

#if COMPILER
   sblkp->fnc = fnc;
   if (line_info) {
      if (debug_info)
         PFDebug(sblkp->pf)->proc = cproc;
      PFDebug(sblkp->pf)->old_fname = "";
      PFDebug(sblkp->pf)->old_line = 0;
      }

   return sblkp;
#else					/* COMPILER */
   /*
    * Return the new co-expression.
    */
   Arg0.dword = D_Coexpr;
   BlkLoc(Arg0) = (union block *) sblkp;
   Return;
#endif					/* COMPILER */
#else					/* Coexpr */
   err_msg(401, NULL);
#if COMPILER
   return NULL;
#else					/* COMPILER */
   Fail;
#endif					/* COMPILER */
#endif					/* Coexpr */

   }

/*
 * activate - activate a co-expression.
 */
int activate(val, ncp, result)
dptr val;
struct b_coexpr *ncp;
dptr result;
   {
#ifdef Coexpr

   int first;

   /*
    * Set activator in new co-expression.
    */
   if (ncp->es_actstk == NULL) {
      Protect(ncp->es_actstk = alcactiv(),RunErr(0,NULL));
      first = 0;
      }
   else
      first = 1;

   if (pushact(ncp, (struct b_coexpr *)BlkLoc(k_current)) == Error)
      RunErr(0,NULL);

   if (co_chng(ncp, val, result, A_Coact, first) == A_Cofail)
      return A_Resume;
   else
      return A_Continue;

#else					/* Coexpr */
   RunErr(401,NULL);
#endif					/* Coexpr */
   }
