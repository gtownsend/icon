/* tests of #begdef - #enddef */

/*
 * object-like macros
 */
#begdef empty
#enddef

#begdef one
1
#enddef

#begdef many_lines
testing
multi-line
macro with
several
lines
#enddef

>empty<
>one<
>many_lines<

/*
 * function-like macros
 */
#begdef nothing()
#enddef

#begdef something()
"everything"
#enddef

#begdef echo(x)
x
#enddef

#begdef abc(a, b, c)
a
-
b
-
c
#enddef

>nothing()<   >something()<   >echo("hi")<    >abc((1,1),2,two)<
