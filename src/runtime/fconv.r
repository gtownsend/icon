/*
 * fconv.r -- abs, cset, integer, numeric, proc, real, string.
 */

"abs(N) - produces the absolute value of N."

function{1} abs(n)
   /*
    * If n is convertible to a (large or small) integer or real,
    * this code returns -n if n is negative
    */
   if cnv:(exact)C_integer(n) then {
      abstract {
         return integer
         }
      inline {
	 C_integer i;
	 extern int over_flow;

	 if (n >= 0)
	    i = n;
	 else {
	    i = neg(n);
	    if (over_flow) {
#ifdef LargeInts
	       struct descrip tmp;
	       MakeInt(n,&tmp);
	       if (bigneg(&tmp, &result) == Error)  /* alcbignum failed */
	          runerr(0);
               return result;
#else					/* LargeInts */
	       irunerr(203,n);
               errorfail;
#endif					/* LargeInts */
	       }
	    }
         return C_integer i;
         }
      }


#ifdef LargeInts
   else if cnv:(exact)integer(n) then {
      abstract {
         return integer
         }
      inline {
	 if (BlkLoc(n)->bignumblk.sign == 0)
	    result = n;
	 else {
	    if (bigneg(&n, &result) == Error)  /* alcbignum failed */
	       runerr(0);
	    }
         return result;
         }
      }
#endif					/* LargeInts */

   else if cnv:C_double(n) then {
      abstract {
         return real
         }
      inline {
         return C_double Abs(n);
         }
      }
   else
      runerr(102,n)
end


/*
 * The convertible types cset, integer, real, and string are identical
 *  enough to be expansions of a single macro, parameterized by type.
 */
#begdef ReturnYourselfAs(t)
#t "(x) - produces a value of type " #t " resulting from the conversion of x, "
   "but fails if the conversion is not possible."
function{0,1} t(x)

   if cnv:t(x) then {
      abstract {
         return t
         }
      inline {
         return x;
         }
      }
   else {
      abstract {
         return empty_type
         }
      inline {
         fail;
         }
      }
end

#enddef

ReturnYourselfAs(cset)     /* cset(x) - convert to cset or fail */
ReturnYourselfAs(integer)  /* integer(x) - convert to integer or fail */
ReturnYourselfAs(real)     /* real(x) - convert to real or fail */
ReturnYourselfAs(string)   /* string(x) - convert to string or fail */



"numeric(x) - produces an integer or real number resulting from the "
"type conversion of x, but fails if the conversion is not possible."

function{0,1} numeric(n)

   if cnv:(exact)integer(n) then {
      abstract {
         return integer
         }
      inline {
         return n;
         }
      }
   else if cnv:real(n) then {
      abstract {
         return real
         }
      inline {
         return n;
         }
      }
   else {
      abstract {
         return empty_type
         }
      inline {
         fail;
         }
      }
end


"proc(x,i) - convert x to a procedure if possible; use i to resolve "
"ambiguous string names."

#ifdef MultiThread
function{0,1} proc(x,i,c)
#else					/* MultiThread */
function{0,1} proc(x,i)
#endif					/* MultiThread */

#ifdef MultiThread
   if is:coexpr(x) then {
      abstract {
         return proc
         }
      inline {
	 struct b_coexpr *ce = NULL;
	 struct b_proc *bp = NULL;
	 struct pf_marker *fp;
	 dptr dp=NULL;
	 if (BlkLoc(x) != BlkLoc(k_current)) {
	    ce = (struct b_coexpr *)BlkLoc(x);
	    dp = ce->es_argp;
	    if (dp == NULL) fail;
	    bp = (struct b_proc *)BlkLoc(*(dp));
	    }
	 else
	    bp = (struct b_proc *)BlkLoc(*(glbl_argp));
	 return proc(bp);
	 }
      }
#endif					/* MultiThread */

   if is:proc(x) then {
      abstract {
         return proc
         }
      inline {

#ifdef MultiThread
	 if (!is:null(c)) {
	    struct progstate *p;
	    if (!is:coexpr(c)) runerr(118,c);
	    /*
	     * Test to see whether a given procedure belongs to a given
	     * program.  Currently this is a sleazy pointer arithmetic check.
	     */
	    p = BlkLoc(c)->coexpr.program;
	    if (! InRange(p, BlkLoc(x)->proc.entryp.icode,
			  (char *)p + p->hsize))
	       fail;
	    }
#endif					/* MultiThread */
         return x;
         }
      }

   else if cnv:tmp_string(x) then {
      /*
       * i must be 0, 1, 2, or 3; it defaults to 1.
       */
      if !def:C_integer(i, 1) then
         runerr(101, i)
      inline {
         if (i < 0 || i > 3) {
            irunerr(205, i);
            errorfail;
            }
         }

      abstract {
         return proc
         }
      inline {
         struct b_proc *prc;

#ifdef MultiThread
	 struct progstate *prog, *savedprog;

	 savedprog = curpstate;
	 if (is:null(c)) {
	    prog = curpstate;
	    }
	 else if (is:coexpr(c)) {
	    prog = BlkLoc(c)->coexpr.program;
	    }
	 else {
	    runerr(118,c);
	    }

	 ENTERPSTATE(prog);
#endif					/* MultiThread */

         /*
          * Attempt to convert Arg0 to a procedure descriptor using i to
          *  discriminate between procedures with the same names.  If i
          *  is zero, only check builtins and ignore user procedures.
          *  Fail if the conversion isn't successful.
          */
	 if (i == 0)
            prc = bi_strprc(&x, 0);
	 else
         prc = strprc(&x, i);

#ifdef MultiThread
	 ENTERPSTATE(savedprog);
#endif					/* MultiThread */
         if (prc == NULL)
            fail;
         else
            return proc(prc);
         }
      }
   else {
      abstract {
         return empty_type
         }
      inline {
         fail;
         }
      }
end
