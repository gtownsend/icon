/* variants of conditional inclusion */

#if 1
1
#endif

#if 0
2
#endif

#ifdef x
4
#endif

#ifdef x
5
#else
6
#endif

#ifndef x
7
#endif

#ifndef x
8
#else
9
#endif

#define x 0

#ifdef x
10
#endif

#ifdef x
11
#else
12
#endif

#ifndef x
13
#endif

#ifndef x
14
#else
15
#endif
