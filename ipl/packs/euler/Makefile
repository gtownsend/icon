euler: 
	icont -s -c xcode  escape ebcdic 
	icont -s -c parsell1 readll1 semstk eulerscn
	icont -s -fs euler eulersem eulerint \
		parsell1.u1 readll1.u1 semstk.u1 eulerscn.u1

Iexe:	euler
	cp euler ../../iexe/

Clean:
	rm -f euler *.u[12]
