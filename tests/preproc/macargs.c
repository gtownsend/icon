/*
 * test null arugments (this is undefined in the standard), white
 * space around arguments, etc.
 */
#define f0() F_ZERO
#define f1(p1) 	/* sone white space */   F_ONE(p1)  /* some more */
#define f3(p1, p2, p3) F_THREE(p1,p2,p3)
>f0(		)<
>f1()<
f1(   	   /* white space only */ )
f1(   3   )
f3(,,)
f3(     ,       ,       )
f3(   one one    ,     two  two  two    ,   three    three   three   )
f3(	1,,3	)
