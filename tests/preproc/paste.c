/* test token pasting */
#define two_toks  www xxx ## yy zzz
two_toks
#define two_args(a,b) a ## b
two_args(12, .7)
two_args(1 2 3 , 4 5 6)
two_args(1 2,3)
two_args(1,2 3)
two_args(x,)
two_args(,one two)
(two_args(,))
two_args(-, 7)
#begdef one_arg(a)
a ## a##a
L ## #a
b ## a ## c
a ## b ## a
#enddef
one_arg(x y)
one_arg()
