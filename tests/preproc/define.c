/* tests of #define */

/* object-like */
#define empty
#define one 1
#define many_tokens (2 3 4 "x" 'y' 7.7e-10)
>empty<   >one<   >many_tokens<

/* function-like */
#define nothing()
#define something() "everything"
#define echo(x) x
#define list_3(a, b, c) (a,b,c)
>nothing()<   >something()<   >echo("hi")<    >list_3((1,1),2,two)<

/* nested macros */
#define x 1
#define y x
#define z y
z

/* nested calls */
#define m(a) 3 + a
m(m(7))
