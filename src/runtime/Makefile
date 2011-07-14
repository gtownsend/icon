#  Makefile for the Icon run-time system.

include ../../Makedefs


HDRS = ../h/define.h ../h/arch.h ../h/config.h ../h/typedefs.h \
	  ../h/cstructs.h ../h/cpuconf.h ../h/grttin.h\
	  ../h/rmacros.h ../h/rexterns.h ../h/rstructs.h \
	  ../h/rproto.h ../h/mproto.h ../h/sys.h

GHDRS = ../h/graphics.h ../h/xwin.h ../h/mswin.h

COBJS =	../common/long.o ../common/time.o \
	../common/rswitch.o ../common/xwindow.o \
	../common/alloc.o ../common/filepart.o ../common/munix.o

XOBJS =	cnv.o data.o def.o errmsg.o fconv.o fload.o fmath.o\
	fmisc.o fscan.o fstr.o fstranl.o fstruct.o fsys.o\
	fwindow.o imain.o imisc.o init.o interp.o invoke.o\
	keyword.o lmisc.o oarith.o oasgn.o ocat.o ocomp.o\
	omisc.o oref.o oset.o ovalue.o profile.o ralc.o rcoexpr.o rcomp.o\
	rdebug.o rexternal.o rlrgint.o rmemmgt.o rmisc.o rstruct.o \
	rsys.o rwinrsc.o rwinsys.o rwindow.o rcolor.o rimage.o

RTT = ../rtt/rtt
SUFFIXES = .r .c .o
.r.o:	; $(RTT) -x $*.r && $(CC) -o $*.o -c $(CFLAGS) x$*.c && rm x$*.c
.r.c:	; $(RTT) -x $*.r


iconx: $(COBJS) $(XOBJS)
	cd ../common; $(MAKE)
	$(CC) $(RLINK) -o iconx $(XOBJS) $(COBJS) $(XL) $(RLIBS) $(TLIBS)
	cp iconx ../../bin
	strip $(SFLAGS) ../../bin/iconx$(EXE)

$(COBJS):
	cd ../common; $(MAKE)


$(XOBJS): $(HDRS) $(GHDRS)

data.o:  ../h/fdefs.h ../h/odefs.h ../h/kdefs.h

fmisc.o: ../h/opdefs.h 

imain.o: ../h/version.h ../h/header.h ../h/opdefs.h ../h/version.h

init.o:  ../h/header.h ../h/odefs.h ../h/version.h

keyword.o:  ../h/kdefs.h ../h/features.h ../h/version.h

rdebug.o:   ../h/opdefs.h

rwinrsc.o:  rxrsc.ri

rwinsys.o:  rxwin.ri rmswin.ri
