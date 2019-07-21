#  Makefile for the dynamically loaded C function library.
#
#  If building with the compiler (instead of the interpreter)
#  use the "-fs" option to avoid problems.


include ../../Makedefs

IC = icont
IFLAGS = -us

FUNCLIB = libcfunc.so

.SUFFIXES: .c .o .icn .u2
.c.o:			; $(CC) $(CFLAGS) $(CFDYN) -c $<
.icn.u2:		; $(IC) $(IFLAGS) -c $<

FUNCS = bitcount.o external.o files.o fpoll.o internal.o lgconv.o osf.o \
	pack.o ppm.o process.o tconnect.o
CSRC = $(FUNCS:.o=.c)


default:	cfunc.u2 $(FUNCLIB)


# library

$(FUNCLIB):	$(FUNCS) mklib.sh
		CC="$(CC)" CFLAGS="$(CFLAGS)" LDFLAGS="$(LDFLAGS)" BIN="../../bin" \
		    sh mklib.sh $(FUNCLIB) $(FUNCS)
$(FUNCS):	icall.h


# Icon interface

cfunc.u2:	cfunc.icn
cfunc.icn:	$(CSRC) mkfunc.sh
		sh mkfunc.sh $(FUNCLIB) $(FUNCS) >$@


# cleanup

clean Clean:
		rm -f $(FUNCLIB) *.o *.u? *.so so_locations cfunc.icn
