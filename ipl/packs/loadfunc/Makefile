#  Makefile for testing dynamic loading of C functions from Icon
#  It is assumed that the standard C functions exist on $FPATH


CC = cc
CFLAGS = -O -I../../cfuncs

IC = icont
IFLAGS = -us

.SUFFIXES: 	.icn
.icn:		; $(IC) $(IFLAGS) $<

MKLIB = ../../cfuncs/mklib.sh


PROGS = btest ddtest dldemo cspace tnet newsgrp
FUNCS = argdump.o cspgen.o ddump.o
FUNCLIB = libdemo.so



default: 	$(PROGS) $(FUNCLIB)

$(PROGS):	libnames.icn

libnames.icn:	Makefile
		echo '$$define FUNCLIB "./$(FUNCLIB)"'	>libnames.icn

$(FUNCLIB):	$(FUNCS)
		CC="$(CC)" CFLAGS="$(CFLAGS)" sh $(MKLIB) $(FUNCLIB) $(FUNCS)



clean Clean:
		rm -f $(PROGS) $(FUNCLIB) *.o *.so *.u[12] libnames.icn
