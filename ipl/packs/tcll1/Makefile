tcll1:
	icont -s -c xcode  escape ebcdic 
	icont -s -c gramanal ll1 semstk readll1 parsell1 scangram semgram
	icont -s -fs tcll1

Iexe:	tcll1
	cp tcll1 ../../iexe/

Clean:
	rm -f *.u[12] tcll1
