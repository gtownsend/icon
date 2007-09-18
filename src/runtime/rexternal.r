/*
 * File: rexternal.r
 *  Functions dealing with external values and their custom functions.
 */


/*
 * extlname(dp1,dp2) - create as a string in *dp2 the type name of extl *dp1.
 */
void extlname(dptr dp1, dptr dp2)
   {
   struct b_external *block = (struct b_external *)BlkLoc(*dp1);
   struct b_extlfuns *funcs = block->funcs;

   if (funcs->extlname != NULL) {
      *dp2 = funcs->extlname(block);	/* call custom name function */
      if (! is:string(*dp2))
         syserr("extlname: not a string");
      }
   else {
      dp2->dword = 8;			/* strlen("external") */
      dp2->vword.sptr = "external";
      }
   }

/*
 * extlimage(dp1,dp2) - create as a string in *dp2 the image of external *dp1.
 *
 * Always sets *dp2 to a valid string, but returns Error
 * if storage is not available for formatting the details.
 */
int extlimage(dptr dp1, dptr dp2)
   {
   struct b_external *block = (struct b_external *)BlkLoc(*dp1);
   struct b_extlfuns *funcs = block->funcs;
   word len;

   if (funcs->extlimage != NULL) {
      *dp2 = funcs->extlimage(block);	/* call custom image function */
      if (! is:string(*dp2))
         syserr("extlimage: not a string");
      return;
      }

   extlname(dp1, dp2);			/* get type name */
   len = StrLen(*dp2);
   Protect(reserve(Strings, len + 30), return Error);
   Protect(StrLoc(*dp2) = alcstr(StrLoc(*dp2), len), return Error);
   /*
    * to type name append _<id>(<hexvalue>)
    */
   len += sprintf(StrLoc(*dp2) + len, "_%ld(%lX)",
      (long)block->id, (long)block->data[0]);
   StrLen(*dp2) = len;
   return Succeeded;
   }

/*
 * extlcmp...
 */

/*
 * extlcopy...
 */
