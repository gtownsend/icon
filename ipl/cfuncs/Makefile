#  Makefile for the dynamically loaded C function library.
#
#  If building with the compiler (instead of the interpreter)
#  use the "-fs" option to avoid problems.


include ../../Makedefs

ICONT = icont
IFLAGS = -us

FUNCLIB = libcfunc.so

.SUFFIXES: .c .o
.c.o:			; $(CC) $(CFLAGS) $(CFDYN) -c $<

FUNCS = bitcount.o files.o fpoll.o internal.o lgconv.o osf.o \
	pack.o ppm.o process.o tconnect.o
CSRC = $(FUNCS:.o=.c)


default:	cfunc.u2 $(FUNCLIB)


# library

$(FUNCLIB):	$(FUNCS) mklib.sh
		CC="$(CC)" CFLAGS="$(CFLAGS)" sh mklib.sh $(FUNCLIB) $(FUNCS)
$(FUNCS):	icall.h


# Icon interface

cfunc.u2:	cfunc.icn
		$(ICONT) $(IFLAGS) -c cfunc.icn
cfunc.icn:	$(CSRC) mkfunc.sh
		sh mkfunc.sh $(FUNCLIB) $(FUNCS) >cfunc.icn


# cleanup

clean Clean:
		rm -f $(FUNCLIB) *.o *.u? *.so so_locations cfunc.icn
