#
# Sample makefile for compiling Idol
#
idol: idol.iol idolmain.u1 unix.u1 idolboot
	idolboot idol unix.u1 idolmain.u1

idolboot: idolboot.icn unix.u1
	icont -s idolboot unix.u1

unix.u1: unix.icn
	icont -s -c unix

idolmain.u1: idolmain.icn
	icont -s -c idolmain

Clean:
	rm -rf *.u[12] idol idolboot idolmain unix idolcode.env
