#
# Sample makefile for compiling Idol
#
idol: idol.iol idolmain.u1 unix.u1 idolboot
	idolboot idol unix.u1 idolmain.u1

idolboot: idolboot.icn unix.u1
	icont -Sr1000 -SF30 -Si1000 -Sn3000 idolboot unix.u1

unix.u1: unix.icn
	icont -c unix

idolmain.u1: idolmain.icn
	icont -c idolmain
