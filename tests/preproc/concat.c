/* test simple string concatenation */

"isolated",
"one, " "two, " 		/* */
"three",
"1" "2" "3" "4" "5" '6'
"abc" L"def" "ghi" L"jkl",
L"mno" L"qrs" "tuv"
.
#define str(x) "" #x ""
str(1 + 2 - 3)
