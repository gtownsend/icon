/* test #elif */

#if 1
1
#elif 1
2
#endif

#if 0
3
#elif 1
4
#endif

#if 0
5
#elif 1
6
#else
7
#endif

#if 0
8
#elif 0
9
#else
10
#endif

#if 1
11
#elif 1
12
#elif 1
13
#else
14
#endif

#if 0
15
#elif 1
16
#elif 1
17
#else
18
#endif

#ifdef x
19
#elif 0
20
#elif 1
21
#endif

#define y(a) a

#ifndef y
22
#elif 12 == 13
23
#elif y(1) - 1
24
#else
25
#endif
