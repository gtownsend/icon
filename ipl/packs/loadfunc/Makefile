#  Makefile for testing dynamic loading of C functions from Icon
#
#  "make" builds everything.
#  "make kr" builds the programs that don't require ANSI C.
#
#  If building with the compiler (instead of the interpreter)
#  use the "-fs" option to avoid problems.

SHELL = /bin/sh

FUNCLIB = libfunc.so
KRLIB = libkr.so

ICONT = icont
IFLAGS = -us
ITRAN = $(ICONT) $(IFLAGS)

CC = cc
CFLAGS = -O -w

AFUNCS = argdump.o cspgen.o fpoll.o tconnect.o
KRFUNCS = bitcount.o ddump.o
FUNCS = $(AFUNCS) $(KRFUNCS)

APROGS = dldemo cspace tnet newsgrp
KRPROGS = btest ddtest
PROGS = $(APROGS) $(KRPROGS)


default: all
all:		 $(PROGS)
kr:		 $(KRPROGS)


# header file
$(PROGS):	libnames.icn
libnames.icn:	Makefile
		@echo 'Creating libnames.icn:'
		echo '# library names from Makefile'	 >libnames.icn
		echo '$$define FUNCLIB "./$(FUNCLIB)"'	>>libnames.icn
		echo '$$define KRLIB "./$(KRLIB)"'	>>libnames.icn


# full library
$(APROGS):	$(FUNCLIB)
$(FUNCLIB):	$(FUNCS) mklib.sh
		CC="$(CC)" CFLAGS="$(CFLAGS)" sh mklib.sh $(FUNCLIB) $(FUNCS)
$(FUNCS):	icall.h

# library of K&R functions
$(KRPROGS):	$(KRLIB)
$(KRLIB):	$(KRFUNCS) mklib.sh
		CC="$(CC)" CFLAGS="$(CFLAGS)" sh mklib.sh $(KRLIB) $(KRFUNCS)


# ANSI C programs
btest:		btest.icn	; $(ITRAN) btest
ddtest:		ddtest.icn	; $(ITRAN) ddtest

# K&R programs
cspace:		cspace.icn	; $(ITRAN) cspace
dldemo:		dldemo.icn	; $(ITRAN) dldemo
tnet:		tnet.icn	; $(ITRAN) tnet
newsgrp:	newsgrp.icn	; $(ITRAN) newsgrp


# cleanup
clean Clean:
	-rm -f $(PROGS) $(FUNCLIB) $(KRLIB) *.o *.so libnames.icn
	-rm -f `grep -l '^/\*ICONC\*/$$' *.[ch] /dev/null` core so_locations
