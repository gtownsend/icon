/* test treatment of octal and hexidecimal constants by string concatentation */

"\001" "1",
"\01" "1",
"\18" "1",
"\01" "\01",
"\01" "8",
"abc\2d\0" "0",
"\77" "0\1",

"\xff" "f\xff" "g",
"\x0a\x0" "0\x0" "x"
