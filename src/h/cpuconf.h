/*
 *  Configuration parameters that depend on computer architecture.
 *  Some depend on values defined in config.h, which is always
 *  included before this file.
 */

#ifndef CStateSize
   #define CStateSize 15		/* size of C state for co-expressions */
#endif					/* CStateSize */

/*
 * The following definitions depend on the sizes of ints and pointers.
 */

/*
 * Most of the present implementations use 32-bit "words".  Note that
 *  WordBits is the number of bits in an Icon integer, not necessarily
 *  the number of bits in an int (given by IntBits).  In some systems
 *  an Icon integer is a long, not an int.
 *
 *  MaxStrLen must not be so large as to overlap flags.
 */

/*
 * 64-bit words.
 */

#if WordBits == 64

   #ifndef MinLong
      #define MinLong  ((long int)0x8000000000000000) /* smallest long int */
   #endif

   #ifndef MaxLong
      #define MaxLong  ((long int)0x7fffffffffffffff) /* largest long integer */
   #endif

   #define MaxStrLen 017777777777L		/* maximum string length */

   #ifndef MaxNegInt
      #define MaxNegInt "-9223372036854775808"
   #endif

   #ifndef F_Nqual
      #define F_Nqual 0x8000000000000000	/* set if NOT string qualifier*/
   #endif				/* F_Nqual */

   #ifndef F_Var
      #define F_Var	0x4000000000000000	/* set if variable */
   #endif				/* F_Var */

   #ifndef F_Ptr
      #define F_Ptr	0x1000000000000000	/* set if value field is ptr */
   #endif				/* F_Ptr */

   #ifndef F_Typecode
      #define F_Typecode  0x2000000000000000	/* set if dword incls typecode*/
   #endif				/* F_Typecode */

#endif					/* WordBits == 64 */

/*
 * 32-bit words.
 */

#if WordBits == 32

   #define MaxLong  ((long int)017777777777L)   /* largest long integer */
   #define MinLong  ((long int)020000000000L)   /* smallest long integer */

   #define MaxNegInt "-2147483648"

   #define MaxStrLen 0777777777		/* maximum string length */

   #define F_Nqual	0x80000000	/* set if NOT string qualifier */
   #define F_Var	0x40000000	/* set if variable */
   #define F_Ptr	0x10000000	/* set if value field is pointer */
   #define F_Typecode	0x20000000	/* set if dword includes type code */

#endif					/* WordBits == 32 */

/*
 * Values that depend on the number of bits in an int (not necessarily
 * the same as the number of bits in a word).
 */

#if IntBits == 64
   #define LogIntBits	6			/* log of IntBits */
   #define MaxUnsigned 01777777777777777777777L /* largest unsigned integer */
   #define MaxInt	0777777777777777777777L /* largest int */
   /*
    * Cset initialization and access macros.
    */
   #define fwd(w0, w1, w2, w3) \
      (((w0) & 0xffff) | (((unsigned)(w1) & 0xffff) << 16) | \
      (((unsigned)(w2) & 0xffff) << 32) | (((unsigned)(w3) & 0xffff) << 48))
   #define cset_display(w0,w1,w2,w3,w4,w5,w6,w7,w8,w9,wa,wb,wc,wd,we,wf) \
      {fwd(w0,w1,w2,w3),fwd(w4,w5,w6,w7),fwd(w8,w9,wa,wb),fwd(wc,wd,we,wf)}
   #define Cset32(b,c) (*CsetPtr(b,c)>>(32*CsetOff((b)>>5)))  /* 32b of cset */
#endif					/* IntBits == 64 */

#if IntBits == 32
   #define LogIntBits	5		/* log of IntBits */
   #define MaxUnsigned	037777777777	/* largest unsigned integer */
   #define MaxInt	017777777777	/* largest int */
   /*
    * Cset initialization and access macros.
    */
   #define twd(w0,w1)	(((w0)&0xffff) | (((unsigned)w1)<<16))
   #define cset_display(w0,w1,w2,w3,w4,w5,w6,w7,w8,w9,wa,wb,wc,wd,we,wf) \
      {twd(w0,w1),twd(w2,w3),twd(w4,w5),twd(w6,w7), \
       twd(w8,w9),twd(wa,wb),twd(wc,wd),twd(we,wf)}
   #define Cset32(b,c) (*CsetPtr(b,c))	/* 32 bits of cset */
#endif					/* IntBits == 32 */

#ifndef LogHuge
   #define LogHuge 309			/* maximum base-10 exp+1 of real */
#endif					/* LogHuge */

#ifndef Big
   #define Big 9007199254740992.	/* larger than 2^53 lose precision */
#endif					/* Big */

#ifndef Precision
   #define Precision 10			/* digits in string from real */
#endif					/* Precision */

/*
 * Parameters that configure tables and sets:
 *
 *  HSlots	Initial number of hash buckets; must be a power of 2.
 *  LogHSlots	Log to the base 2 of HSlots.
 *
 *  HSegs	Maximum number of hash bin segments; the maximum number of
 *		hash bins is HSlots * 2 ^ (HSegs - 1).
 *
 *		If Hsegs is increased above 20, the arrays log2h[] and segsize[]
 *		in the runtime system will need modification.
 *
 *  MaxHLoad	Maximum loading factor; more hash bins are allocated when
 *		the average bin exceeds this many entries.
 *
 *  MinHLoad	Minimum loading factor; if a newly created table (e.g. via
 *		copy()) is more lightly loaded than this, bins are combined.
 *
 *  Because splitting doubles the number of hash bins, and combining halves it,
 *  MaxHLoad should be at least twice MinHLoad.
 */

#ifndef HSlots
   #define HSlots     16
   #define LogHSlots  4
#endif					/* HSlots */

#if ((1 << LogHSlots) != HSlots)
   #error HSlots and LogHSlots are inconsistent
#endif					/* HSlots / LogHSlots consistency */

#ifndef HSegs
   #define HSegs	 20
#endif					/* HSegs */

#ifndef MinHLoad
   #define MinHLoad  1
#endif					/* MinHLoad */

#ifndef MaxHLoad
   #define MaxHLoad  5
#endif					/* MaxHLoad */

/*
 * The number of bits in each base-B digit; the type DIGIT (unsigned int)
 *  in rt.h must be large enough to hold this many bits.
 *  It must be at least 2 and at most WordBits / 2.
 */
#define NB           (WordBits / 2)

/*
 * The number of decimal digits at which the image lf a large integer
 * goes from exact to approximate (to avoid possible long delays in
 * conversion from large integer to string because of its quadratic
 * complexity).
 */
#define MaxDigits	30

/*
 * Memory sizing.
 */
#ifndef AlcMax
   #define AlcMax 25
#endif					/* AlcMax */

/*
 * Maximum sized block that can be allocated (via malloc() or such).
 */
#ifndef MaxBlock
   #define MaxBlock MaxUnsigned
#endif					/* MaxBlock */

/*
 * What follows is default memory sizing. Implementations with special
 *  requirements may specify these values in define.h.
 */

#ifndef MaxStrSpace
   #define MaxStrSpace 500000		/* size of the string space in bytes */
#endif					/* MaxStrSpace */

#ifndef MaxAbrSize
   #define MaxAbrSize 500000		/* size of the block region in bytes */
#endif					/* MaxAbrSize */

#ifndef MinAbrSize
   #define MinAbrSize	 5000		/* minimum block region size */
#endif					/* MinAbrSize */

#ifndef MStackSize
   #ifdef MultiThread
      #define MStackSize 20000		/* size of the main stack in words */
   #else				/* MultiThread */
      #define MStackSize 10000		/* size of the main stack in words */
   #endif				/* MultiThread */
#endif					/* MStackSize */

#ifndef StackSize
   #define StackSize	2000		/* words in co-expression stack */
#endif					/* StackSize */

#ifndef QualLstSize
   #define QualLstSize	5000		/* size of qualifier pointer region */
#endif					/* QualLstSize */

#ifndef ActStkBlkEnts
   #ifdef Coexpr
      #define ActStkBlkEnts   25	/* number of entries in an astkblk */
   #else				/* Coexpr */
      #define ActStkBlkEnts    1	/* number of entries in an astkblk */
   #endif				/* Coexpr */
#endif					/* ActStkBlkEnts */

#ifndef RegionCushion
   #define RegionCushion 10		/* % memory cushion to avoid thrashing*/
#endif					/* RegionCushion */

#ifndef RegionGrowth
   #define RegionGrowth 200		/* % region growth when full */
#endif					/* RegionGrowth */
