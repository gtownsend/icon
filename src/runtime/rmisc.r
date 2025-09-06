/*
 * File: rmisc.r
 *  Contents: deref, eq, getvar, hash, outimage,
 *  qtos, pushact, popact, topact, [dumpact],
 *  findline, findipc, findfile, doimage, getimage
 *  printable, sig_rsm, cmd_line, varargs.
 *
 *  Integer overflow checking.
 */

/*
 * Prototypes.
 */
static void listimage(FILE *f, struct b_list *lp, int noimage);
static void printimage(FILE *f, int c, int q);
static char *csname(dptr dp);

/*
 * eq - compare two Icon strings for equality
 */
int eq(dptr d1, dptr d2)
{
	char *s1, *s2;
	int i;

	if (StrLen(*d1) != StrLen(*d2))
	   return 0;
	s1 = StrLoc(*d1);
	s2 = StrLoc(*d2);
	for (i = 0; i < StrLen(*d1); i++)
	   if (*s1++ != *s2++)
	      return 0;
	return 1;
}

/*
 * Get variable descriptor from name.  Returns the (integer-encoded) scope
 *  of the variable (Succeeded for keywords), or Failed if the variable
 *  does not exist.
 */
int getvar(char *s, dptr vp)
   {
   register dptr dp;
   register dptr np;
   register int i;
   struct b_proc *bp;
   struct pf_marker *fp = pfp;

   /*
    * Is it a keyword that's a variable?
    */
   if (*s == '&') {

      if (strcmp(s,"&error") == 0) {	/* must put basic one first */
         vp->dword = D_Kywdint;
         VarLoc(*vp) = &kywd_err;
         return Succeeded;
         }
      else if (strcmp(s,"&pos") == 0) {
         vp->dword = D_Kywdpos;
         VarLoc(*vp) = &kywd_pos;
         return Succeeded;
         }
      else if (strcmp(s,"&progname") == 0) {
         vp->dword = D_Kywdstr;
         VarLoc(*vp) = &kywd_prog;
         return Succeeded;
         }
      else if (strcmp(s,"&random") == 0) {
         vp->dword = D_Kywdint;
         VarLoc(*vp) = &kywd_ran;
         return Succeeded;
         }
      else if (strcmp(s,"&subject") == 0) {
         vp->dword = D_Kywdsubj;
         VarLoc(*vp) = &k_subject;
         return Succeeded;
         }
      else if (strcmp(s,"&trace") == 0) {
         vp->dword = D_Kywdint;
         VarLoc(*vp) = &kywd_trc;
         return Succeeded;
         }
      else if (strcmp(s,"&dump") == 0) {
         vp->dword = D_Kywdint;
         VarLoc(*vp) = &kywd_dmp;
         return Succeeded;
         }
#ifdef Graphics
      else if (strcmp(s,"&window") == 0) {
         vp->dword = D_Kywdwin;
         VarLoc(*vp) = &(kywd_xwin[XKey_Window]);
         return Succeeded;
         }
#endif					/* Graphics */

      else return Failed;
      }

   /*
    * Look for the variable the name with the local identifiers,
    *  parameters, and static names in each Icon procedure frame on the
    *  stack. If not found among the locals, check the global variables.
    *  If a variable with name is found, variable() returns a variable
    *  descriptor that points to the corresponding value descriptor.
    *  If no such variable exits, it fails.
    */

   /*
    *  If no procedure has been called (as can happen with icon_call(),
    *  dont' try to find local identifier.
    */
   if (pfp == NULL)
      goto glbvars;

   dp = glbl_argp;
   bp = (struct b_proc *)BlkLoc(*dp);	/* get address of procedure block */

   np = bp->lnames;		/* Check the formal parameter names. */

   for (i = abs((int)bp->nparam); i > 0; i--) {
      dp++;
      if (strcmp(s,StrLoc(*np)) == 0) {
         vp->dword = D_Var;
         VarLoc(*vp) = (dptr)dp;
         return ParamName;
         }
      np++;
      }
   dp = &fp->pf_locals[0];

   for (i = (int)bp->ndynam; i > 0; i--) { /* Check the local dynamic names. */
	 if (strcmp(s,StrLoc(*np)) == 0) {
            vp->dword = D_Var;
            VarLoc(*vp) = (dptr)dp;
            return LocalName;
	    }
         np++;
         dp++;
         }

   dp = &statics[bp->fstatic]; /* Check the local static names. */
   for (i = (int)bp->nstatic; i > 0; i--) {
         if (strcmp(s,StrLoc(*np)) == 0) {
            vp->dword = D_Var;
            VarLoc(*vp) = (dptr)dp;
            return StaticName;
	    }
         np++;
         dp++;
         }

glbvars:
   dp = globals;	/* Check the global variable names. */
   np = gnames;
   while (dp < eglobals) {
      if (strcmp(s,StrLoc(*np)) == 0) {
         vp->dword    =  D_Var;
         VarLoc(*vp) =  (dptr)(dp);
         return GlobalName;
         }
      np++;
      dp++;
      }
   return Failed;
   }

/*
 * hash - compute hash value of arbitrary object for table and set accessing.
 */

uword hash(dptr dp)
   {
   register char *s;
   register uword i;
   register word j, n;
   register unsigned int *bitarr;
   double r;

   if (Qual(*dp)) {
   hashstring:
      /*
       * Compute the hash value for the string based on a scaled sum
       *  of its first ten characters, plus its length.
       */
      i = 0;
      s = StrLoc(*dp);
      j = n = StrLen(*dp);
      if (j > 10)		/* limit scan to first ten characters */
         j = 10;
      while (j-- > 0) {
         i += *s++ & 0xFF;	/* add unsigned version of next char */
         i *= 37;		/* scale total by a nice prime number */
         }
      i += n;			/* add the (untruncated) string length */
      }

   else {

      switch (Type(*dp)) {
         /*
          * The hash value of an integer is itself times eight times the golden
	  *  ratio.  We do this calculation in fixed point.  We don't just use
	  *  the integer itself, for that would give bad results with sets
	  *  having entries that are multiples of a power of two.
          */
         case T_Integer:
            i = (13255 * (uword)IntVal(*dp)) >> 10;
            break;

         /*
          * The hash value of a bignum is based on its length and its
          *  most and least significant digits.
          */
	 case T_Lrgint:
	    {
	    struct b_bignum *b = &BlkLoc(*dp)->bignumblk;

	    i = ((b->lsd - b->msd) << 16) ^
		(b->digits[b->msd] << 8) ^ b->digits[b->lsd];
	    }
	    break;

         /*
          * The hash value of a real number is itself times a constant,
          *  converted to an unsigned integer.  The intent is to scramble
	  *  the bits well, in the case of integral values, and to scale up
	  *  fractional values so they don't all land in the same bin.
	  *  The constant below is 32749 / 29, the quotient of two primes,
	  *  and was observed to work well in empirical testing.
          */
         case T_Real:
            GetReal(dp,r);
            i = r * 1129.27586206896558;
            break;

         /*
          * The hash value of a cset is based on a convoluted combination
          *  of all its bits.
          */
         case T_Cset:
            i = 0;
            bitarr = BlkLoc(*dp)->cset.bits + CsetSize - 1;
            for (j = 0; j < CsetSize; j++) {
               i += *bitarr--;
               i *= 37;			/* better distribution */
               }
            i %= 1048583;		/* scramble the bits */
            break;

         /*
          * The hash value of a list, set, table, or record is its id,
          *   hashed like an integer.
          */
         case T_List:
            i = (13255 * BlkLoc(*dp)->list.id) >> 10;
            break;

         case T_Set:
            i = (13255 * BlkLoc(*dp)->set.id) >> 10;
            break;

         case T_Table:
            i = (13255 * BlkLoc(*dp)->table.id) >> 10;
            break;

         case T_Record:
            i = (13255 * BlkLoc(*dp)->record.id) >> 10;
            break;

	 case T_Proc:
	    dp = &(BlkLoc(*dp)->proc.pname);
	    goto hashstring;

         default:
            /*
             * For other types, use the type code as the hash
             *  value.
             */
            i = Type(*dp);
            break;
         }
      }

   return i;
   }


#define StringLimit	16		/* limit on length of imaged string */
#define ListLimit	 6		/* limit on list items in image */

/*
 * outimage - print image of *dp on file f.  If noimage is nonzero,
 *  fields of records will not be imaged.
 */

void outimage(FILE *f, dptr dp, int noimage)
   {
   register word i, j;
   register char *s;
   register union block *bp;
   char *type, *csn;
   FILE *fd;
   struct descrip q;
   double rresult;
   tended struct descrip tdp;

   type_case *dp of {
      string: {
         /*
          * *dp is a string qualifier.  Print StringLimit characters of it
          *  using printimage and denote the presence of additional characters
          *  by terminating the string with "...".
          */
         i = StrLen(*dp);
         s = StrLoc(*dp);
         j = Min(i, StringLimit);
         putc('"', f);
         while (j-- > 0)
            printimage(f, *s++, '"');
         if (i > StringLimit)
            fprintf(f, "...");
         putc('"', f);
         }

      null:
         fprintf(f, "&null");

      integer:
         if (Type(*dp) == T_Lrgint)
            bigprint(f, dp);
         else
            fprintf(f, "%ld", (long)IntVal(*dp));

      real: {
         char s[30];
         struct descrip rd;

         GetReal(dp,rresult);
         rtos(rresult, &rd, s);
         fprintf(f, "%s", StrLoc(rd));
         }

      cset: {
         /*
	  * Check for a predefined cset; use keyword name if found.
	  */
	 if ((csn = csname(dp)) != NULL) {
	    fprintf(f, "%s", csn);
	    return;
	    }
         /*
          * Use printimage to print each character in the cset.  Follow
          *  with "..." if the cset contains more than StringLimit
          *  characters.
          */
         putc('\'', f);
         j = StringLimit;
         for (i = 0; i < 256; i++) {
            if (Testb(i, *dp)) {
               if (j-- <= 0) {
                  fprintf(f, "...");
                  break;
                  }
               printimage(f, (int)i, '\'');
               }
            }
         putc('\'', f);
         }

      file: {
         /*
          * Check for distinguished files by looking at the address of
          *  of the object to image.  If one is found, print its name.
          */
         if ((fd = BlkLoc(*dp)->file.fd) == stdin)
            fprintf(f, "&input");
         else if (fd == stdout)
            fprintf(f, "&output");
         else if (fd == stderr)
            fprintf(f, "&errout");
         else {
            /*
             * The file isn't a special one, just print "file(name)".
             */
	    i = StrLen(BlkLoc(*dp)->file.fname);
	    s = StrLoc(BlkLoc(*dp)->file.fname);
#ifdef Graphics
	    if (BlkLoc(*dp)->file.status & Fs_Window) {
	       s = ((wbp)(BlkLoc(*dp)->file.fd))->window->windowlabel;
	       i = strlen(s);
	       fprintf(f, "window_%d:%d(",
		       ((wbp)BlkLoc(*dp)->file.fd)->window->serial,
		       ((wbp)BlkLoc(*dp)->file.fd)->context->serial
		       );
	       }
	    else
#endif					/* Graphics */
	       fprintf(f, "file(");
	    while (i-- > 0)
	       printimage(f, *s++, '\0');
	    putc(')', f);
            }
         }

      proc: {
         /*
          * Produce one of:
          *  "procedure name"
          *  "function name"
          *  "record constructor name"
          *
          * Note that the number of dynamic locals is used to determine
          *  what type of "procedure" is at hand.
          */
         i = StrLen(BlkLoc(*dp)->proc.pname);
         s = StrLoc(BlkLoc(*dp)->proc.pname);
         switch ((int)BlkLoc(*dp)->proc.ndynam) {
            default:  type = "procedure"; break;
            case -1:  type = "function"; break;
            case -2:  type = "record constructor"; break;
            }
         fprintf(f, "%s ", type);
         while (i-- > 0)
            printimage(f, *s++, '\0');
         }

      list: {
         /*
          * listimage does the work for lists.
          */
         listimage(f, (struct b_list *)BlkLoc(*dp), noimage);
         }

      table: {
         /*
          * Print "table_m(n)" where n is the size of the table.
          */
         fprintf(f, "table_%ld(%ld)", (long)BlkLoc(*dp)->table.id,
            (long)BlkLoc(*dp)->table.size);
         }

      set: {
	/*
         * print "set_m(n)" where n is the cardinality of the set
         */
	fprintf(f,"set_%ld(%ld)",(long)BlkLoc(*dp)->set.id,
           (long)BlkLoc(*dp)->set.size);
        }

      record: {
         /*
          * If noimage is nonzero, print "record(n)" where n is the
          *  number of fields in the record.  If noimage is zero, print
          *  the image of each field instead of the number of fields.
          */
         bp = BlkLoc(*dp);
         i = StrLen(bp->record.recdesc->proc.recname);
         s = StrLoc(bp->record.recdesc->proc.recname);
         fprintf(f, "record ");
         while (i-- > 0)
            printimage(f, *s++, '\0');
         fprintf(f, "_%ld", (long)bp->record.id);
         j = bp->record.recdesc->proc.nfields;
         if (j <= 0)
            fprintf(f, "()");
         else if (noimage > 0)
            fprintf(f, "(%ld)", (long)j);
         else {
            putc('(', f);
            i = 0;
            for (;;) {
               outimage(f, &bp->record.fields[i], noimage+1);
               if (++i >= j)
                  break;
               putc(',', f);
               }
            putc(')', f);
            }
         }

      coexpr: {
         fprintf(f, "co-expression_%ld(%ld)",
            (long)((struct b_coexpr *)BlkLoc(*dp))->id,
            (long)((struct b_coexpr *)BlkLoc(*dp))->size);
         }

      tvsubs: {
         /*
          * Produce "v[i+:j] = value" where v is the image of the variable
          *  containing the substring, i is starting position of the substring
          *  j is the length, and value is the string v[i+:j].	If the length
          *  (j) is one, just produce "v[i] = value".
          */
         bp = BlkLoc(*dp);
	 dp = VarLoc(bp->tvsubs.ssvar);
         if (is:kywdsubj(bp->tvsubs.ssvar)) {
            fprintf(f, "&subject");
            fflush(f);
            }
         else {
            dp = (dptr)((word *)dp + Offset(bp->tvsubs.ssvar));
            outimage(f, dp, noimage);
            }

         if (bp->tvsubs.sslen == 1)
            fprintf(f, "[%ld]", (long)bp->tvsubs.sspos);
         else
            fprintf(f, "[%ld+:%ld]", (long)bp->tvsubs.sspos,
               (long)bp->tvsubs.sslen);

         if (Qual(*dp)) {
            if (bp->tvsubs.sspos + bp->tvsubs.sslen - 1 > StrLen(*dp))
               return;
            StrLen(q) = bp->tvsubs.sslen;
            StrLoc(q) = StrLoc(*dp) + bp->tvsubs.sspos - 1;
            fprintf(f, " = ");
            outimage(f, &q, noimage);
            }
        }

      tvtbl: {
         /*
          * produce "t[s]" where t is the image of the table containing
          *  the element and s is the image of the subscript.
          */
         bp = BlkLoc(*dp);
	 tdp.dword = D_Table;
	 BlkLoc(tdp) = bp->tvtbl.clink;
         outimage(f, &tdp, noimage);
         putc('[', f);
         outimage(f, &bp->tvtbl.tref, noimage);
         putc(']', f);
         }

      kywdint: {
         if (VarLoc(*dp) == &kywd_ran)
            fprintf(f, "&random = ");
         else if (VarLoc(*dp) == &kywd_trc)
            fprintf(f, "&trace = ");
         else if (VarLoc(*dp) == &kywd_dmp)
            fprintf(f, "&dump = ");
         else if (VarLoc(*dp) == &kywd_err)
            fprintf(f, "&error = ");
         outimage(f, VarLoc(*dp), noimage);
         }

      kywdevent: {
         outimage(f, VarLoc(*dp), noimage);
         }

      kywdstr: {
         outimage(f, VarLoc(*dp), noimage);
         }

      kywdpos: {
         fprintf(f, "&pos = ");
         outimage(f, VarLoc(*dp), noimage);
         }

      kywdsubj: {
         fprintf(f, "&subject = ");
         outimage(f, VarLoc(*dp), noimage);
         }
      kywdwin: {
         fprintf(f, "&window = ");
         outimage(f, VarLoc(*dp), noimage);
         }

      default: {
         if (is:variable(*dp)) {
            /*
             * *d is a variable.  Print "variable =", dereference it, and
             *  call outimage to handle the value.
             */
            fprintf(f, "(variable = ");
            dp = (dptr)((word *)VarLoc(*dp) + Offset(*dp));
            outimage(f, dp, noimage);
            putc(')', f);
            }
         else if (Type(*dp) == T_External) {
            q = callextfunc(&extlimage, dp, NULL); /* get image as a string */
            i = StrLen(q);
            s = StrLoc(q);
            while (i-- > 0)
               putc(*s++, f);
            }
         else if (Type(*dp) <= MaxType)
            fprintf(f, "%s", blkname[Type(*dp)]);
         else
            syserr("outimage: unknown type");
         }
      }
   }

/*
 * printimage - print character c on file f using escape conventions
 *  if c is unprintable, '\', or equal to q.
 */

static void printimage(FILE *f, int c, int q)
   {
   if (printable(c)) {
      /*
       * c is printable, but special case ", ', and \.
       */
      switch (c) {
         case '"':
            if (c != q) goto deflt;
            fprintf(f, "\\\"");
            return;
         case '\'':
            if (c != q) goto deflt;
            fprintf(f, "\\'");
            return;
         case '\\':
            fprintf(f, "\\\\");
            return;
         default:
         deflt:
            putc(c, f);
            return;
         }
      }

   /*
    * c is some sort of unprintable character.	If it one of the common
    *  ones, produce a special representation for it, otherwise, produce
    *  its hex value.
    */
   switch (c) {
      case '\b':			/* backspace */
         fprintf(f, "\\b");
         return;
      case '\177':			/* delete */
         fprintf(f, "\\d");
         return;
      case '\33':			/* escape */
         fprintf(f, "\\e");
         return;
      case '\f':			/* form feed */
         fprintf(f, "\\f");
         return;
      case '\n':			/* newline (line feed) */
         fprintf(f, "\\n");
         return;
      case '\r':			/* carriage return */
         fprintf(f, "\\r");
         return;
      case '\t':			/* horizontal tab */
         fprintf(f, "\\t");
         return;
      case '\13':			/* vertical tab */
         fprintf(f, "\\v");
         return;
      default:				/* hex escape sequence */
         fprintf(f, "\\x%02x", c & 0xff);
         return;
      }
   }

/*
 * listimage - print an image of a list.
 */

static void listimage(FILE *f, struct b_list *lp, int noimage)
   {
   register word i, j;
   register struct b_lelem *bp;
   word size, count;

   bp = (struct b_lelem *) lp->listhead;
   size = lp->size;

   if (noimage > 0 && size > 0) {
      /*
       * Just give indication of size if the list isn't empty.
       */
      fprintf(f, "list_%ld(%ld)", (long)lp->id, (long)size);
      return;
      }

   /*
    * Print [e1,...,en] on f.  If more than ListLimit elements are in the
    *  list, produce the first ListLimit/2 elements, an ellipsis, and the
    *  last ListLimit elements.
    */
   fprintf(f, "list_%ld = [", (long)lp->id);
   count = 1;
   i = 0;
   if (size > 0) {
      for (;;) {
         if (++i > bp->nused) {
            i = 1;
            bp = (struct b_lelem *) bp->listnext;
            }
         if (count <= ListLimit/2 || count > size - ListLimit/2) {
            j = bp->first + i - 1;
            if (j >= bp->nslots)
               j -= bp->nslots;
            outimage(f, &bp->lslots[j], noimage+1);
            if (count >= size)
               break;
            putc(',', f);
            }
         else if (count == ListLimit/2 + 1)
            fprintf(f, "...,");
         count++;
         }
      }
   putc(']', f);
   }

/*
 * qtos - convert a qualified string named by *dp to a C-style string.
 *  Put the C-style string in sbuf if it will fit, otherwise put it
 *  in the string region.
 */
int qtos(dptr dp, char *sbuf)
   {
   register word slen;
   register char *c, *s;

   c = StrLoc(*dp);
   slen = StrLen(*dp)++;
   if (slen >= MaxCvtLen) {
      Protect(reserve(Strings, slen+1), return Error);
      c = StrLoc(*dp);
      if (c + slen != strfree) {
         Protect(s = alcstr(c, slen), return Error);
         }
      else
         s = c;
      StrLoc(*dp) = s;
      Protect(alcstr("",(word)1), return Error);
      }
   else {
      StrLoc(*dp) = sbuf;
      for ( ; slen > 0; slen--)
         *sbuf++ = *c++;
      *sbuf = '\0';
      }
   return Succeeded;
   }

/*
 * pushact - push actvtr on the activator stack of ce
 */
int pushact(struct b_coexpr *ce, struct b_coexpr *actvtr)
{
   struct astkblk *abp = ce->es_actstk, *nabp;
   struct actrec *arp;

   /*
    * If the last activator is the same as this one, just increment
    *  its count.
    */
   if (abp->nactivators > 0) {
      arp = &abp->arec[abp->nactivators - 1];
      if (arp->activator == actvtr) {
         arp->acount++;
         return Succeeded;
         }
      }
   /*
    * This activator is different from the last one.  Push this activator
    *  on the stack, possibly adding another block.
    */
   if (abp->nactivators + 1 > ActStkBlkEnts) {
      Protect(nabp = alcactiv(), fatalerr(0,NULL));
      nabp->astk_nxt = abp;
      abp = nabp;
      }
   abp->nactivators++;
   arp = &abp->arec[abp->nactivators - 1];
   arp->acount = 1;
   arp->activator = actvtr;
   ce->es_actstk = abp;
   return Succeeded;
}

/*
 * popact - pop the most recent activator from the activator stack of ce
 *  and return it.
 */
struct b_coexpr *popact(struct b_coexpr *ce)
{
   struct astkblk *abp = ce->es_actstk, *oabp;
   struct actrec *arp;
   struct b_coexpr *actvtr;

   /*
    * If the current stack block is empty, pop it.
    */
   if (abp->nactivators == 0) {
      oabp = abp;
      abp = abp->astk_nxt;
      free((pointer)oabp);
      }

   if (abp == NULL || abp->nactivators == 0)
      syserr("empty activator stack\n");

   /*
    * Find the activation record for the most recent co-expression.
    *  Decrement the activation count and if it is zero, pop that
    *  activation record and decrement the count of activators.
    */
   arp = &abp->arec[abp->nactivators - 1];
   actvtr = arp->activator;
   if (--arp->acount == 0)
      abp->nactivators--;

   ce->es_actstk = abp;
   return actvtr;
}

/*
 * topact - return the most recent activator of ce.
 */
struct b_coexpr *topact(struct b_coexpr *ce)
{
   struct astkblk *abp = ce->es_actstk;

   if (abp->nactivators == 0)
      abp = abp->astk_nxt;
   return abp->arec[abp->nactivators-1].activator;
}

/*
 * findline - find the source line number associated with the ipc
 */
int findline(word *ipc)
{
  return findloc(ipc) & 65535;
}
int findcol(word *ipc)
{
  return findloc(ipc) >> 16;
}

int findloc(word *ipc)
{
   uword ipc_offset;
   uword size;
   struct ipc_line *base;
   extern struct ipc_line *ilines, *elines;
   static int two = 2;	/* some compilers generate bad code for division
			   by a constant that is a power of two ... */

   if (!InRange(code,ipc,ecode))
      return 0;
   ipc_offset = DiffPtrs((char *)ipc,(char *)code);
   base = ilines;
   size = DiffPtrs((char *)elines,(char *)ilines) / sizeof(struct ipc_line *);
   while (size > 1) {
      if (ipc_offset >= base[size / two].ipc) {
         base = &base[size / two];
         size -= size / two;
         }
      else
         size = size / two;
      }
   /*
    * return the line component of the location (column is top 16 bits)
    */
   return (int)(base->line);
}
/*
 * findipc - find the first ipc associated with a source-code line number.
 */
int findipc(int line)
{
   uword size;
   struct ipc_line *base;
   extern struct ipc_line *ilines, *elines;
   static int two = 2;	/* some compilers generate bad code for division
			   by a constant that is a power of two ... */

   base = ilines;
   size = DiffPtrs((char *)elines,(char *)ilines) / sizeof(struct ipc_line *);
   while (size > 1) {
      if (line >= base[size / two].line) {
         base = &base[size / two];
         size -= size / two;
         }
      else
         size = size / two;
      }
   return base->ipc;
}

/*
 * findfile - find source file name associated with the ipc
 */
char *findfile(word *ipc)
{
   uword ipc_offset;
   struct ipc_fname *p;
   extern struct ipc_fname *filenms, *efilenms;

   if (!InRange(code,ipc,ecode))
      return "?";
   ipc_offset = DiffPtrs((char *)ipc,(char *)code);
   for (p = efilenms - 1; p >= filenms; p--)
      if (ipc_offset >= p->ipc)
         return strcons + p->fname;
   fprintf(stderr,"bad ipc/file name table\n");
   fflush(stderr);
   c_exit(EXIT_FAILURE);
   /*NOTREACHED*/
   return 0;  /* avoid gcc warning */
}

/*
 * doimage(c,q) - allocate character c in string space, with escape
 *  conventions if c is unprintable, '\', or equal to q.
 *  Returns number of characters allocated.
 */

int doimage(int c, int q)
   {
   static char cbuf[5];

   if (printable(c)) {

      /*
       * c is printable, but special case ", ', and \.
       */
      switch (c) {
         case '"':
            if (c != q) goto deflt;
            Protect(alcstr("\\\"", (word)(2)), return Error);
            return 2;
         case '\'':
            if (c != q) goto deflt;
            Protect(alcstr("\\'", (word)(2)), return Error);
            return 2;
         case '\\':
            Protect(alcstr("\\\\", (word)(2)), return Error);
            return 2;
         default:
         deflt:
            cbuf[0] = c;
            Protect(alcstr(cbuf, (word)(1)), return Error);
            return 1;
         }
      }

   /*
    * c is some sort of unprintable character.	If it is one of the common
    *  ones, produce a special representation for it, otherwise, produce
    *  its hex value.
    */
   switch (c) {
      case '\b':			/*	   backspace	*/
         Protect(alcstr("\\b", (word)(2)), return Error);
         return 2;
      case '\177':			/*      delete	  */
         Protect(alcstr("\\d", (word)(2)), return Error);
         return 2;
      case '\33':			/*	    escape	 */
         Protect(alcstr("\\e", (word)(2)), return Error);
         return 2;
      case '\f':			/*	   form feed	*/
         Protect(alcstr("\\f", (word)(2)), return Error);
         return 2;
      case '\n':			/*	   new line	*/
         Protect(alcstr("\\n", (word)(2)), return Error);
         return 2;
      case '\r':			/*	   return	*/
         Protect(alcstr("\\r", (word)(2)), return Error);
         return 2;
      case '\t':			/*	   horizontal tab     */
         Protect(alcstr("\\t", (word)(2)), return Error);
         return 2;
      case '\13':			/*	    vertical tab     */
         Protect(alcstr("\\v", (word)(2)), return Error);
         return 2;
      default:				/*	  hex escape sequence  */
         sprintf(cbuf, "\\x%02x", c & 0xff);
         Protect(alcstr(cbuf, (word)(4)), return Error);
         return 4;
      }
   }

/*
 * getimage(dp1,dp2) - return string image of object dp1 in dp2.
 */

int getimage(dptr dp1, dptr dp2)
   {
   register word len, outlen, rnlen;
   int i;
   tended char *s;
   tended struct descrip source = *dp1;    /* the source may move during gc */
   register union block *bp;
   char *type, *t, *csn;
   char sbuf[MaxCvtLen];
   FILE *fd;

   type_case source of {
      string: {
         /*
          * Form the image by putting a quote in the string space, calling
          *  doimage with each character in the string, and then putting
          *  a quote at then end. Note that doimage directly writes into the
          *  string space.  (Hence the indentation.)  This technique is used
          *  several times in this routine.
          */
         s = StrLoc(source);
         len = StrLen(source);
	 Protect (reserve(Strings, (len << 2) + 2), return Error);
         Protect(t = alcstr("\"", (word)(1)), return Error);
         StrLoc(*dp2) = t;
         StrLen(*dp2) = 1;

         while (len-- > 0)
            StrLen(*dp2) += doimage(*s++, '"');
         Protect(alcstr("\"", (word)(1)), return Error);
         ++StrLen(*dp2);
         }

      null: {
         StrLoc(*dp2) = "&null";
         StrLen(*dp2) = 5;
         }

      integer: {
         if (Type(source) == T_Lrgint) {
            word slen;
            word dlen;
            struct b_bignum *blk = &BlkLoc(source)->bignumblk;

            slen = blk->lsd - blk->msd;
            dlen = slen * NB * 0.3010299956639812	/* 1 / log2(10) */
               + log((double)blk->digits[blk->msd]) * 0.4342944819032518 + 0.5;
							/* 1 / ln(10) */
            if (dlen >= MaxDigits) {
               sprintf(sbuf, "integer(~10^%ld)", (long)dlen);
	       len = strlen(sbuf);
               Protect(StrLoc(*dp2) = alcstr(sbuf,len), return Error);


               StrLen(*dp2) = len;
               }
	    else bigtos(&source,dp2);
	    }
         else
            cnv: string(source, *dp2);
	 }

      real: {
         cnv:string(source, *dp2);
         }

      cset: {
         /*
	  * Check for the value of a predefined cset; use keyword name if found.
	  */
	 if ((csn = csname(dp1)) != NULL) {
	    StrLoc(*dp2) = csn;
	    StrLen(*dp2) = strlen(csn);
	    return Succeeded;
	    }
	 /*
	  * Otherwise, describe it in terms of the character membership.
	  */

	 i = BlkLoc(source)->cset.size;
	 if (i < 0)
	    i = cssize(&source);
	 i = (i << 2) + 2;
	 if (i > 730) i = 730;
	 Protect (reserve(Strings, i), return Error);

         Protect(t = alcstr("'", (word)(1)), return Error);
         StrLoc(*dp2) = t;
         StrLen(*dp2) = 1;
         for (i = 0; i < 256; ++i)
            if (Testb(i, source))
               StrLen(*dp2) += doimage((char)i, '\'');
         Protect(alcstr("'", (word)(1)), return Error);
         ++StrLen(*dp2);
         }

      file: {
         /*
          * Check for distinguished files by looking at the address of
          *  of the object to image.  If one is found, make a string
          *  naming it and return.
          */
         if ((fd = BlkLoc(source)->file.fd) == stdin) {
            StrLen(*dp2) = 6;
            StrLoc(*dp2) = "&input";
            }
         else if (fd == stdout) {
            StrLen(*dp2) = 7;
            StrLoc(*dp2) = "&output";
            }
         else if (fd == stderr) {
            StrLen(*dp2) = 7;
            StrLoc(*dp2) = "&errout";
            }
         else {
            /*
             * The file is not a standard one; form a string of the form
             *	file(nm) where nm is the argument originally given to
             *	open.
             */
#ifdef Graphics
	    if (BlkLoc(source)->file.status & Fs_Window) {
	       s = ((wbp)(BlkLoc(source)->file.fd))->window->windowlabel;
	       len = strlen(s);
               Protect (reserve(Strings, (len << 2) + 16), return Error);
	       sprintf(sbuf, "window_%d:%d(",
		       ((wbp)BlkLoc(source)->file.fd)->window->serial,
		       ((wbp)BlkLoc(source)->file.fd)->context->serial
		       );
	       Protect(t = alcstr(sbuf, (word)(strlen(sbuf))), return Error);
	       StrLoc(*dp2) = t;
	       StrLen(*dp2) = strlen(sbuf);
	       }
	    else {
#endif					/* Graphics */
               s = StrLoc(BlkLoc(source)->file.fname);
               len = StrLen(BlkLoc(source)->file.fname);
               Protect (reserve(Strings, (len << 2) + 12), return Error);
	       Protect(t = alcstr("file(", (word)(5)), return Error);
	       StrLoc(*dp2) = t;
	       StrLen(*dp2) = 5;
#ifdef Graphics
	     }
#endif					/* Graphics */
            while (len-- > 0)
               StrLen(*dp2) += doimage(*s++, '\0');
            Protect(alcstr(")", (word)(1)), return Error);
            ++StrLen(*dp2);
            }
         }

      proc: {
         /*
          * Produce one of:
          *  "procedure name"
          *  "function name"
          *  "record constructor name"
          *
          * Note that the number of dynamic locals is used to determine
          *  what type of "procedure" is at hand.
          */
         len = StrLen(BlkLoc(source)->proc.pname);
         s = StrLoc(BlkLoc(source)->proc.pname);
	 Protect (reserve(Strings, len + 22), return Error);
         switch ((int)BlkLoc(source)->proc.ndynam) {
            default:  type = "procedure "; outlen = 10; break;
            case -1:  type = "function "; outlen = 9; break;
            case -2:  type = "record constructor "; outlen = 19; break;
            }
         Protect(t = alcstr(type, outlen), return Error);
         StrLoc(*dp2) = t;
         Protect(alcstr(s, len),  return Error);
         StrLen(*dp2) = len + outlen;
         }

      list: {
         /*
          * Produce:
          *  "list_m(n)"
          * where n is the current size of the list.
          */
         bp = BlkLoc(*dp1);
         sprintf(sbuf, "list_%ld(%ld)", (long)bp->list.id, (long)bp->list.size);
         len = strlen(sbuf);
         Protect(t = alcstr(sbuf, len), return Error);
         StrLoc(*dp2) = t;
         StrLen(*dp2) = len;
         }

      table: {
         /*
          * Produce:
          *  "table_m(n)"
          * where n is the size of the table.
          */
         bp = BlkLoc(*dp1);
         sprintf(sbuf, "table_%ld(%ld)", (long)bp->table.id,
            (long)bp->table.size);
         len = strlen(sbuf);
         Protect(t = alcstr(sbuf, len), return Error);
         StrLoc(*dp2) = t;
         StrLen(*dp2) = len;
         }

      set: {
         /*
          * Produce "set_m(n)" where n is size of the set.
          */
         bp = BlkLoc(*dp1);
         sprintf(sbuf, "set_%ld(%ld)", (long)bp->set.id, (long)bp->set.size);
         len = strlen(sbuf);
         Protect(t = alcstr(sbuf,len), return Error);
         StrLoc(*dp2) = t;
         StrLen(*dp2) = len;
         }

      record: {
         /*
          * Produce:
          *  "record name_m(n)"	-- under construction
          * where n is the number of fields.
          */
         bp = BlkLoc(*dp1);
         rnlen = StrLen(bp->record.recdesc->proc.recname);
         sprintf(sbuf, "_%ld(%ld)", (long)bp->record.id,
            (long)bp->record.recdesc->proc.nfields);
         len = strlen(sbuf);
	 Protect (reserve(Strings, 7 + len + rnlen), return Error);
         Protect(t = alcstr("record ", (word)(7)), return Error);
         bp = BlkLoc(*dp1);		/* refresh pointer */
         StrLoc(*dp2) = t;
	 StrLen(*dp2) = 7;
         Protect(alcstr(StrLoc(bp->record.recdesc->proc.recname),rnlen),
	            return Error);
         StrLen(*dp2) += rnlen;
         Protect(alcstr(sbuf, len),  return Error);
         StrLen(*dp2) += len;
         }

      coexpr: {
         /*
          * Produce:
          *  "co-expression_m (n)"
          *  where m is the number of the co-expressions and n is the
          *  number of results that have been produced.
          */

         sprintf(sbuf, "_%ld(%ld)", (long)BlkLoc(source)->coexpr.id,
            (long)BlkLoc(source)->coexpr.size);
         len = strlen(sbuf);
	 Protect (reserve(Strings, len + 13), return Error);
         Protect(t = alcstr("co-expression", (word)(13)), return Error);
         StrLoc(*dp2) = t;
         Protect(alcstr(sbuf, len), return Error);
         StrLen(*dp2) = 13 + len;
         }

      default:
         if (Type(*dp1) == T_External) {
            *dp2 = callextfunc(&extlimage, dp1, NULL);
            }
         else {
	    ReturnErrVal(123, source, Error);
            }
      }
   return Succeeded;
   }

/*
 * csname(dp) -- return the name of a predefined cset matching dp.
 */
static char *csname(dptr dp)
   {
   register int n;

   n = BlkLoc(*dp)->cset.size;
   if (n < 0)
      n = cssize(dp);

   /*
    * Check for a cset we recognize using a hardwired decision tree.
    *  In ASCII, each of &lcase/&ucase/&digits are complete within 32 bits.
    */
   if (n == 52) {
      if ((Cset32('a',*dp) & Cset32('A',*dp)) == (0377777777l << CsetOff('a')))
	 return ("&letters");
      }
   else if (n < 52) {
      if (n == 26) {
	 if (Cset32('a',*dp) == (0377777777l << CsetOff('a')))
	    return ("&lcase");
	 else if (Cset32('A',*dp) == (0377777777l << CsetOff('A')))
	    return ("&ucase");
	 }
      else if (n == 10 && *CsetPtr('0',*dp) == (01777 << CsetOff('0')))
	 return ("&digits");
      }
   else /* n > 52 */ {
      if (n == 256)
	 return "&cset";
      else if (n == 128 && ~0 ==
	 (Cset32(0,*dp) & Cset32(32,*dp) & Cset32(64,*dp) & Cset32(96,*dp)))
	    return "&ascii";
      }
   return NULL;
   }

/*
 * cssize(dp) - calculate cset size, store it, and return it
 */
int cssize(dptr dp)
{
   register int i, n;
   register unsigned int w, *wp;
   register struct b_cset *cs;

   cs = &BlkLoc(*dp)->cset;
   wp = (unsigned int *)cs->bits;
   n = 0;
   for (i = CsetSize; --i >= 0; )
      for (w = *wp++; w != 0; w >>= 1)
	 n += (w & 1);
   cs->size = n;
   return n;
}

/*
 * printable(c) -- is c a "printable" character?
 */
int printable(int c)
   {
   return (isascii(c) && isprint(c));
   }

/*
 * add, sub, mul, neg with overflow check
 * all return 1 if ok, 0 if would overflow
 */

extern int over_flow;

/*
 * add - integer addition with overflow checking
 */
word add(word a, word b)
{
   if ((a ^ b) >= 0 && (a >= 0 ? b > MaxLong - a : b < MinLong - a)) {
      over_flow = 1;
      return 0;
      }
   else {
     over_flow = 0;
     return a + b;
     }
}

/* 
 * sub - integer subtraction with overflow checking
 */
word sub(word a, word b)
{
   if ((a ^ b) < 0 && (a >= 0 ? b < a - MaxLong : b > a - MinLong)) {
      over_flow = 1;
      return 0;
      }
   else {
      over_flow = 0;
      return a - b;
      }
}

/*
 * mul - integer multiplication with overflow checking
 */
word mul(word a, word b)
{
   if (b != 0) {
      if ((a ^ b) >= 0) {
	 if (a >= 0 ? a > MaxLong / b : a < MaxLong / b) {
            over_flow = 1;
	    return 0;
            }
	 }
      else if (b != -1 && (a >= 0 ? a > MinLong / b : a < MinLong / b)) {
         over_flow = 1;
	 return 0;
         }
      }

   over_flow = 0;
   return a * b;
}

/*
 * mod3 - integer modulo with overflow checking (always rounds to 0)
 */
word mod3(word a, word b)
{
   word retval;

   switch ( b )
   {
      case 0:
	 over_flow = 1; /* Not really an overflow, but definitely an error */
	 return 0;

      case MinLong:
	 /* Handle this separately, since -MinLong can overflow */
	 retval = ( a > MinLong ) ? a : 0;
	 break;

      default:
	 /* First, we make b positive */
	 if ( b < 0 ) b = -b;

	 /* Make sure retval has the same sign as 'a' */
	 retval = a % b;
	 if ( ( a < 0 ) && ( retval > 0 ) )
	    retval -= b;
	 break;
      }

   over_flow = 0;
   return retval;
}

/*
 * div3 - integer divide with overflow checking (always rounds to 0)
 */
word div3(word a, word b)
{
   if ( ( b == 0 ) ||	/* Not really an overflow, but definitely an error */
        ( b == -1 && a == MinLong ) ) {
      over_flow = 1;
      return 0;
      }

   over_flow = 0;
   return ( a - mod3 ( a, b ) ) / b;
}

/*
 * neg - integer negation with overflow checking
 */
word neg(word a)
{
   if (a == MinLong) {
      over_flow = 1;
      return 0;
      }
   over_flow = 0;
   return -a;
}

/*
 * retderef - Dereference local variables and substrings of local
 *  string-valued variables. This is used for return, suspend, and
 *  transmitting values across co-expression context switches.
 */
void retderef(dptr valp, word *low, word *high)
   {
   struct b_tvsubs *tvb;
   word *loc;

   if (Type(*valp) == T_Tvsubs) {
      tvb = (struct b_tvsubs *)BlkLoc(*valp);
      loc = (word *)VarLoc(tvb->ssvar);
      }
   else
      loc = (word *)VarLoc(*valp) + Offset(*valp);
   if (InRange(low, loc, high))
      deref(valp, valp);
   }
