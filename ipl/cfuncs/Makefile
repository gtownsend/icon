#  Makefile for dynamically loaded C function library
#
#  If building with the compiler (instead of the interpreter)
#  use the "-fs" option to avoid problems.

SHELL = /bin/sh

FUNCLIB = libcfunc.so

ICONT = icont
IFLAGS = -us
ITRAN = $(ICONT) $(IFLAGS)

CC = cc
CFLAGS = -O -w

FUNCS = bitcount.o files.o fpoll.o internal.o lgconv.o osf.o \
	pack.o ppm.o process.o tconnect.o


default:	cfunc.u2 $(FUNCLIB)

# library
$(FUNCLIB):	$(FUNCS) mklib.sh
		CC="$(CC)" CFLAGS="$(CFLAGS)" sh mklib.sh $(FUNCLIB) $(FUNCS)
$(FUNCS):	icall.h

# Icon interface
cfunc.u2:	cfunc.icn
		$(ITRAN) -c cfunc.icn
cfunc.icn:	$(FUNCS) mkfunc.sh
		sh mkfunc.sh $(FUNCLIB) $(FUNCS) >cfunc.icn


# cleanup
clean Clean:
	-rm -f $(FUNCLIB) *.o *.u? *.so so_locations cfunc.icn
