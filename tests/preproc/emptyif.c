/* test empty branches */

#if 1
#endif

a

#if 0
#else
#endif

b

#ifdef x
#else
#endif

#ifndef x
#else
#endif

c

#define x 1
#ifdef x
#else
#endif

#ifndef x
#else
#endif

d

#if 0
#elif 0
#elif 1
#elif 0
#endif

e
