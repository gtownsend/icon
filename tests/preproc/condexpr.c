/* test the various expressions allowed in conditions */
#if 1
yes
#else
no
#endif

#if 0
yes
#else
no
#endif

#if '0'
yes
#else
no
#endif

#if '\0'
yes
#else
no
#endif

#if x
yes
#else
no
#endif

#define x 1

#if x
yes
#else
no
#endif

#if '\xf' == 15
yes
#else
no
#endif

#if '\xff' == 255
yes
#else
no
#endif

#if '\xa' != '\12'
yes
#else
no
#endif

#if 1 != y
yes
#else
no
#endif

#if 3 < 10
yes
#else
no
#endif

#if 123456789 < 123456788
yes
#else
no
#endif

#if '\x7e' <= '\176'
yes
#else
no
#endif

#if 4 <= 3
yes
#else
no
#endif

#if     10    >    9     
yes
#else
no
#endif

#if		8		>		9		
yes
#else
no
#endif

#if 10 >= 9
yes
#else
no
#endif

#if 8 >= 9
yes
#else
no
#endif

#if defined y == 0
yes
#else
no
#endif

#define y(a) 0

#if defined y == 0
yes
#else
no
#endif

#if defined(y) == 1
yes
#else
no
#endif

#if 0x4e85  == 047205
yes
#else
no
#endif

#if 0xabcde == 0XABCDE
yes
#else
no
#endif

#if 1 > 2 ? 0 : 1
yes
#else
no
#endif

#if 1 < 2 ? 0 : 1
yes
#else
no
#endif

#if 0 || 1
yes
#else
no
#endif

#if 0 || 0
yes
#else
no
#endif

#if 1 && 1
yes
#else
no
#endif

#if 1 && 0
yes
#else
no
#endif

#if (9 | 3) == 11
yes
#else
no
#endif

#if (0720 & 0230) == 0220
yes
#else
no
#endif

#if (0x99 ^ 0x66) == 0xff
yes
#else
no
#endif

#if (3 << 3) == 24
yes
#else
no
#endif

#if (25 >> 3) == 3
yes
#else
no
#endif

#if (300 - 7) == 293
yes
#else
no
#endif

#if ('0' + 3) == '3'
yes
#else
no
#endif

#if (3 * 4) == 12
yes
#else
no
#endif

#if(77/11)==7
yes
#else
no
#endif

#if (29 % 9) == 2
yes
#else
no
#endif

#if +6 == 6
yes
#else
no
#endif

#if (15 - 26) == -11
yes
#else
no
#endif

#if (~5 & 0xf) == 10
yes
#else
no
#endif

#if !1
yes
#else
no
#endif

#if !0
yes
#else
no
#endif

#if (901 == 901L) && (7u == 7l) && (12U == 12Lu) && (5Ul == 5)
yes
#else
no
#endif

#define average(a,b) (((a) + (b))/2)
#define abs(a) (a) ? (a) : -(a)

#if abs(average(2 - 100, -12)) == average(abs(100 - 2), abs(-12))
yes
#else
no
#endif
