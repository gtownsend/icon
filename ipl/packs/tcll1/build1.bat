rem 	These are from the Icon Program Library:

icont -c xcode  escape ebcdic 

rem 	These form the parser generator proper

icont -c gramanal ll1 semstk readll1 parsell1 scangram semgram
icont -fs tcll1

