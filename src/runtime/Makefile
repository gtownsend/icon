#  Makefile for the Icon run-time system.

include ../../Makedefs


HDRS = ../h/define.h ../h/config.h ../h/typedefs.h ../h/monitor.h\
	  ../h/cstructs.h ../h/cpuconf.h ../h/grttin.h\
	  ../h/rmacros.h ../h/rexterns.h ../h/rstructs.h \
	  ../h/rproto.h ../h/mproto.h ../h/sys.h

GRAPHICSHDRS = ../h/graphics.h ../h/xwin.h ../h/mswin.h

COBJS =	../common/long.o ../common/time.o \
	../common/rswitch.o ../common/xwindow.o \
	../common/alloc.o ../common/filepart.o ../common/munix.o


default: iconx
all:	 iconx comp_all

$(COBJS):
	cd ../common; $(MAKE)


####################################################################
#
# Make entries for iconx
#

XOBJS =	xcnv.o xdata.o xdef.o xerrmsg.o xextcall.o xfconv.o xfload.o xfmath.o\
	xfmisc.o xfmonitr.o xfscan.o xfstr.o xfstranl.o xfstruct.o xfsys.o\
	xfwindow.o ximain.o ximisc.o xinit.o xinterp.o xinvoke.o\
	xkeyword.o xlmisc.o xoarith.o xoasgn.o xocat.o xocomp.o\
	xomisc.o xoref.o xoset.o xovalue.o xralc.o xrcoexpr.o xrcomp.o\
	xrdebug.o xrlrgint.o xrmemmgt.o xrmisc.o xrstruct.o xrsys.o\
	xrwinrsc.o xrwinsys.o xrwindow.o xrcolor.o xrimage.o

OBJS =	$(XOBJS) $(COBJS)

iconx: $(OBJS)
	cd ../common; $(MAKE)
	$(CC) $(RLINK) -o iconx  $(OBJS) $(XL) $(RLIBS) $(TL)
	cp iconx ../../bin
	strip $(SFLAGS) ../../bin/iconx$(EXE)

xcnv.o: cnv.r $(HDRS)
	../../bin/rtt -x cnv.r
	$(CC) -c $(CFLAGS) xcnv.c
	rm xcnv.c

xdata.o: data.r $(HDRS) ../h/kdefs.h ../h/fdefs.h ../h/odefs.h
	../../bin/rtt -x data.r
	$(CC) -c $(CFLAGS) xdata.c
	rm xdata.c

xdef.o: def.r $(HDRS)
	../../bin/rtt -x def.r
	$(CC) -c $(CFLAGS) xdef.c
	rm xdef.c

xerrmsg.o: errmsg.r $(HDRS)
	../../bin/rtt -x errmsg.r
	$(CC) -c $(CFLAGS) xerrmsg.c
	rm xerrmsg.c

xextcall.o: extcall.r $(HDRS)
	../../bin/rtt -x extcall.r
	$(CC) -c $(CFLAGS) xextcall.c
	rm xextcall.c

xfconv.o: fconv.r $(HDRS)
	../../bin/rtt -x fconv.r
	$(CC) -c $(CFLAGS) xfconv.c
	rm xfconv.c

xfload.o: fload.r $(HDRS)
	../../bin/rtt -x fload.r
	$(CC) -c $(CFLAGS) xfload.c
	rm xfload.c

xfmath.o: fmath.r $(HDRS)
	../../bin/rtt -x fmath.r
	$(CC) -c $(CFLAGS) xfmath.c
	rm xfmath.c

xfmisc.o: fmisc.r $(HDRS)
	../../bin/rtt -x fmisc.r
	$(CC) -c $(CFLAGS) xfmisc.c
	rm xfmisc.c

xfmonitr.o: fmonitr.r $(HDRS)
	../../bin/rtt -x fmonitr.r
	$(CC) -c $(CFLAGS) xfmonitr.c
	rm xfmonitr.c

xfscan.o: fscan.r $(HDRS)
	../../bin/rtt -x fscan.r
	$(CC) -c $(CFLAGS) xfscan.c
	rm xfscan.c

xfstr.o: fstr.r $(HDRS)
	../../bin/rtt -x fstr.r
	$(CC) -c $(CFLAGS) xfstr.c
	rm xfstr.c

xfstranl.o: fstranl.r $(HDRS)
	../../bin/rtt -x fstranl.r
	$(CC) -c $(CFLAGS) xfstranl.c
	rm xfstranl.c

xfstruct.o: fstruct.r $(HDRS)
	../../bin/rtt -x fstruct.r
	$(CC) -c $(CFLAGS) xfstruct.c
	rm xfstruct.c

xfsys.o: fsys.r $(HDRS)
	../../bin/rtt -x fsys.r
	$(CC) -c $(CFLAGS) xfsys.c
	rm xfsys.c

xfwindow.o: fwindow.r $(HDRS) $(GRAPHICSHDRS)
	../../bin/rtt -x fwindow.r
	$(CC) -c $(CFLAGS) xfwindow.c
	rm xfwindow.c

ximain.o: imain.r $(HDRS) ../h/version.h
	../../bin/rtt -x imain.r
	$(CC) -c $(CFLAGS) ximain.c
	rm ximain.c

ximisc.o: imisc.r $(HDRS)
	../../bin/rtt -x imisc.r
	$(CC) -c $(CFLAGS) ximisc.c
	rm ximisc.c

xinit.o: init.r $(HDRS) ../h/odefs.h ../h/version.h
	../../bin/rtt -x init.r
	$(CC) -c $(CFLAGS) xinit.c
	rm xinit.c

xinterp.o: interp.r $(HDRS)
	../../bin/rtt -x interp.r
	$(CC) -c $(CFLAGS) xinterp.c
	rm xinterp.c

xinvoke.o: invoke.r $(HDRS)
	../../bin/rtt -x invoke.r
	$(CC) -c $(CFLAGS) xinvoke.c
	rm xinvoke.c

xkeyword.o: keyword.r $(HDRS) ../h/features.h ../h/version.h
	../../bin/rtt -x keyword.r
	$(CC) -c $(CFLAGS) xkeyword.c
	rm xkeyword.c

xlmisc.o: lmisc.r $(HDRS)
	../../bin/rtt -x lmisc.r
	$(CC) -c $(CFLAGS) xlmisc.c
	rm xlmisc.c

xoarith.o: oarith.r $(HDRS)
	../../bin/rtt -x oarith.r
	$(CC) -c $(CFLAGS) xoarith.c
	rm xoarith.c

xoasgn.o: oasgn.r $(HDRS)
	../../bin/rtt -x oasgn.r
	$(CC) -c $(CFLAGS) xoasgn.c
	rm xoasgn.c

xocat.o: ocat.r $(HDRS)
	../../bin/rtt -x ocat.r
	$(CC) -c $(CFLAGS) xocat.c
	rm xocat.c

xocomp.o: ocomp.r $(HDRS)
	../../bin/rtt -x ocomp.r
	$(CC) -c $(CFLAGS) xocomp.c
	rm xocomp.c

xomisc.o: omisc.r $(HDRS)
	../../bin/rtt -x omisc.r
	$(CC) -c $(CFLAGS) xomisc.c
	rm xomisc.c

xoref.o: oref.r $(HDRS)
	../../bin/rtt -x oref.r
	$(CC) -c $(CFLAGS) xoref.c
	rm xoref.c

xoset.o: oset.r $(HDRS)
	../../bin/rtt -x oset.r
	$(CC) -c $(CFLAGS) xoset.c
	rm xoset.c

xovalue.o: ovalue.r $(HDRS)
	../../bin/rtt -x ovalue.r
	$(CC) -c $(CFLAGS) xovalue.c
	rm xovalue.c

xralc.o: ralc.r $(HDRS)
	../../bin/rtt -x ralc.r
	$(CC) -c $(CFLAGS) xralc.c
	rm xralc.c

xrcoexpr.o: rcoexpr.r $(HDRS)
	../../bin/rtt -x rcoexpr.r
	$(CC) -c $(CFLAGS) xrcoexpr.c
	rm xrcoexpr.c

xrcomp.o: rcomp.r $(HDRS)
	../../bin/rtt -x rcomp.r
	$(CC) -c $(CFLAGS) xrcomp.c
	rm xrcomp.c

xrdebug.o: rdebug.r $(HDRS)
	../../bin/rtt -x rdebug.r
	$(CC) -c $(CFLAGS) xrdebug.c
	rm xrdebug.c

xrlrgint.o: rlrgint.r $(HDRS)
	../../bin/rtt -x rlrgint.r
	$(CC) -c $(CFLAGS) xrlrgint.c
	rm xrlrgint.c

xrmemmgt.o: rmemmgt.r $(HDRS)
	../../bin/rtt -x rmemmgt.r
	$(CC) -c $(CFLAGS) xrmemmgt.c
	rm xrmemmgt.c

xrmisc.o: rmisc.r $(HDRS)
	../../bin/rtt -x rmisc.r
	$(CC) -c $(CFLAGS) xrmisc.c
	rm xrmisc.c

xrstruct.o: rstruct.r $(HDRS)
	../../bin/rtt -x rstruct.r
	$(CC) -c $(CFLAGS) xrstruct.c
	rm xrstruct.c

xrsys.o: rsys.r $(HDRS)
	../../bin/rtt -x rsys.r
	$(CC) -c $(CFLAGS) xrsys.c
	rm xrsys.c

xrwinrsc.o: rwinrsc.r $(HDRS) $(GRAPHICSHDRS) rxrsc.ri
	../../bin/rtt -x rwinrsc.r
	$(CC) -c $(CFLAGS) xrwinrsc.c
	rm xrwinrsc.c

xrwinsys.o: rwinsys.r $(HDRS) $(GRAPHICSHDRS) rxwin.ri
	../../bin/rtt -x rwinsys.r
	$(CC) -c $(CFLAGS) xrwinsys.c
	rm xrwinsys.c

xrwindow.o: rwindow.r $(HDRS) $(GRAPHICSHDRS)
	../../bin/rtt -x rwindow.r
	$(CC) -c $(CFLAGS) xrwindow.c
	rm xrwindow.c

xrcolor.o: rcolor.r $(HDRS) $(GRAPHICSHDRS)
	../../bin/rtt -x rcolor.r
	$(CC) -c $(CFLAGS) xrcolor.c
	rm xrcolor.c

xrimage.o: rimage.r $(HDRS) $(GRAPHICSHDRS)
	../../bin/rtt -x rimage.r
	$(CC) -c $(CFLAGS) xrimage.c
	rm xrimage.c


####################################################################
#
# Make entries for the compiler library
#

comp_all: $(COBJS) db_lib

db_lib: rt.db rt.a

#
# if rt.db is missing or any header files have been updated, recreate
# rt.db from scratch along with the .o files.
#
rt.db: $(HDRS)
	rm -f rt.db rt.a
	../../bin/rtt cnv.r data.r def.r errmsg.r fconv.r fload.r fmath.r\
	  fmisc.r fmonitr.r fscan.r fstr.r fstranl.r fstruct.r\
	  fsys.r fwindow.r init.r invoke.r keyword.r\
	  lmisc.r oarith.r oasgn.r ocat.r ocomp.r omisc.r\
	  oref.r oset.r ovalue.r ralc.r rcoexpr.r rcomp.r\
	  rdebug.r rlrgint.r rmemmgt.r rmisc.r rstruct.r\
	  rsys.r rwinrsc.r rwinsys.r rwindow.r rcolor.r rimage.r
	$(CC) -c $(CFLAGS) `sed 's/$$/.c/' rttcur.lst`
	rm `sed 's/$$/.c/' rttcur.lst`

rt.a: ../common/rswitch.o ../common/long.o ../common/time.o\
      cnv.o data.o def.o errmsg.o fconv.o fload.o fmath.o fmisc.o fmonitr.o \
      fscan.o fstr.o fstranl.o fstruct.o fsys.o fwindow.o init.o invoke.o\
      keyword.o lmisc.o oarith.o oasgn.o ocat.o ocomp.o omisc.o oref.o oset.o\
      ovalue.o ralc.o rcoexpr.o rcomp.o rdebug.o rlrgint.o rmemmgt.o\
      rmisc.o rstruct.o rsys.o rwinrsc.o rwinsys.o\
      rwindow.o rcolor.o rimage.o ../common/xwindow.o ../common/alloc.o
	rm -f rt.a
	ar qc rt.a `sed 's/$$/.o/' rttcur.lst` ../common/rswitch.o\
	    ../common/long.o ../common/time.o\
	    ../common/xwindow.o ../common/alloc.o
	ranlib rt.a 2>/dev/null || :
	cp -p rt.a rt.db ../common/dlrgint.o ../../bin

cnv.o: cnv.r $(HDRS)
	../../bin/rtt cnv.r
	$(CC) -c $(CFLAGS) `sed 's/$$/.c/' rttcur.lst`
	rm `sed 's/$$/.c/' rttcur.lst`

data.o: data.r $(HDRS)
	../../bin/rtt data.r
	$(CC) -c $(CFLAGS) `sed 's/$$/.c/' rttcur.lst`
	rm `sed 's/$$/.c/' rttcur.lst`

def.o: def.r $(HDRS)
	../../bin/rtt def.r
	$(CC) -c $(CFLAGS) `sed 's/$$/.c/' rttcur.lst`
	rm `sed 's/$$/.c/' rttcur.lst`

errmsg.o: errmsg.r $(HDRS)
	../../bin/rtt errmsg.r
	$(CC) -c $(CFLAGS) `sed 's/$$/.c/' rttcur.lst`
	rm `sed 's/$$/.c/' rttcur.lst`

fconv.o: fconv.r $(HDRS)
	../../bin/rtt fconv.r
	$(CC) -c $(CFLAGS) `sed 's/$$/.c/' rttcur.lst`
	rm `sed 's/$$/.c/' rttcur.lst`

fload.o: fload.r $(HDRS)
	../../bin/rtt fload.r
	$(CC) -c $(CFLAGS) `sed 's/$$/.c/' rttcur.lst`
	rm `sed 's/$$/.c/' rttcur.lst`

fmath.o: fmath.r $(HDRS)
	../../bin/rtt fmath.r
	$(CC) -c $(CFLAGS) `sed 's/$$/.c/' rttcur.lst`
	rm `sed 's/$$/.c/' rttcur.lst`

fmisc.o: fmisc.r $(HDRS)
	../../bin/rtt fmisc.r
	$(CC) -c $(CFLAGS) `sed 's/$$/.c/' rttcur.lst`
	rm `sed 's/$$/.c/' rttcur.lst`

fmonitr.o: fmonitr.r $(HDRS)
	../../bin/rtt fmonitr.r
	$(CC) -c $(CFLAGS) `sed 's/$$/.c/' rttcur.lst`
	rm `sed 's/$$/.c/' rttcur.lst`

fscan.o: fscan.r $(HDRS)
	../../bin/rtt fscan.r
	$(CC) -c $(CFLAGS) `sed 's/$$/.c/' rttcur.lst`
	rm `sed 's/$$/.c/' rttcur.lst`

fstr.o: fstr.r $(HDRS)
	../../bin/rtt fstr.r
	$(CC) -c $(CFLAGS) `sed 's/$$/.c/' rttcur.lst`
	rm `sed 's/$$/.c/' rttcur.lst`

fstranl.o: fstranl.r $(HDRS)
	../../bin/rtt fstranl.r
	$(CC) -c $(CFLAGS) `sed 's/$$/.c/' rttcur.lst`
	rm `sed 's/$$/.c/' rttcur.lst`

fstruct.o: fstruct.r $(HDRS)
	../../bin/rtt fstruct.r
	$(CC) -c $(CFLAGS) `sed 's/$$/.c/' rttcur.lst`
	rm `sed 's/$$/.c/' rttcur.lst`

fsys.o: fsys.r $(HDRS)
	../../bin/rtt fsys.r
	$(CC) -c $(CFLAGS) `sed 's/$$/.c/' rttcur.lst`
	rm `sed 's/$$/.c/' rttcur.lst`

fwindow.o: fwindow.r $(HDRS) $(GRAPHICSHDRS)
	../../bin/rtt fwindow.r
	$(CC) -c $(CFLAGS) `sed 's/$$/.c/' rttcur.lst`
	rm `sed 's/$$/.c/' rttcur.lst`

init.o: init.r $(HDRS)
	../../bin/rtt init.r
	$(CC) -c $(CFLAGS) `sed 's/$$/.c/' rttcur.lst`
	rm `sed 's/$$/.c/' rttcur.lst`

invoke.o: invoke.r $(HDRS)
	../../bin/rtt invoke.r
	$(CC) -c $(CFLAGS) `sed 's/$$/.c/' rttcur.lst`
	rm `sed 's/$$/.c/' rttcur.lst`

keyword.o: keyword.r $(HDRS)
	../../bin/rtt keyword.r
	$(CC) -c $(CFLAGS) `sed 's/$$/.c/' rttcur.lst`
	rm `sed 's/$$/.c/' rttcur.lst`

lmisc.o: lmisc.r $(HDRS)
	../../bin/rtt lmisc.r
	$(CC) -c $(CFLAGS) `sed 's/$$/.c/' rttcur.lst`
	rm `sed 's/$$/.c/' rttcur.lst`

oarith.o: oarith.r $(HDRS)
	../../bin/rtt oarith.r
	$(CC) -c $(CFLAGS) `sed 's/$$/.c/' rttcur.lst`
	rm `sed 's/$$/.c/' rttcur.lst`

oasgn.o: oasgn.r $(HDRS)
	../../bin/rtt oasgn.r
	$(CC) -c $(CFLAGS) `sed 's/$$/.c/' rttcur.lst`
	rm `sed 's/$$/.c/' rttcur.lst`

ocat.o: ocat.r $(HDRS)
	../../bin/rtt ocat.r
	$(CC) -c $(CFLAGS) `sed 's/$$/.c/' rttcur.lst`
	rm `sed 's/$$/.c/' rttcur.lst`

ocomp.o: ocomp.r $(HDRS)
	../../bin/rtt ocomp.r
	$(CC) -c $(CFLAGS) `sed 's/$$/.c/' rttcur.lst`
	rm `sed 's/$$/.c/' rttcur.lst`

omisc.o: omisc.r $(HDRS)
	../../bin/rtt omisc.r
	$(CC) -c $(CFLAGS) `sed 's/$$/.c/' rttcur.lst`
	rm `sed 's/$$/.c/' rttcur.lst`

oref.o: oref.r $(HDRS)
	../../bin/rtt oref.r
	$(CC) -c $(CFLAGS) `sed 's/$$/.c/' rttcur.lst`
	rm `sed 's/$$/.c/' rttcur.lst`

oset.o: oset.r $(HDRS)
	../../bin/rtt oset.r
	$(CC) -c $(CFLAGS) `sed 's/$$/.c/' rttcur.lst`
	rm `sed 's/$$/.c/' rttcur.lst`

ovalue.o: ovalue.r $(HDRS)
	../../bin/rtt ovalue.r
	$(CC) -c $(CFLAGS) `sed 's/$$/.c/' rttcur.lst`
	rm `sed 's/$$/.c/' rttcur.lst`

ralc.o: ralc.r $(HDRS)
	../../bin/rtt ralc.r
	$(CC) -c $(CFLAGS) `sed 's/$$/.c/' rttcur.lst`
	rm `sed 's/$$/.c/' rttcur.lst`

rcoexpr.o: rcoexpr.r $(HDRS)
	../../bin/rtt rcoexpr.r
	$(CC) -c $(CFLAGS) `sed 's/$$/.c/' rttcur.lst`
	rm `sed 's/$$/.c/' rttcur.lst`

rcomp.o: rcomp.r $(HDRS)
	../../bin/rtt rcomp.r
	$(CC) -c $(CFLAGS) `sed 's/$$/.c/' rttcur.lst`
	rm `sed 's/$$/.c/' rttcur.lst`

rdebug.o: rdebug.r $(HDRS)
	../../bin/rtt rdebug.r
	$(CC) -c $(CFLAGS) `sed 's/$$/.c/' rttcur.lst`
	rm `sed 's/$$/.c/' rttcur.lst`

rlrgint.o: rlrgint.r $(HDRS)
	../../bin/rtt rlrgint.r
	$(CC) -c $(CFLAGS) `sed 's/$$/.c/' rttcur.lst`
	rm `sed 's/$$/.c/' rttcur.lst`

rmemmgt.o: rmemmgt.r $(HDRS)
	../../bin/rtt rmemmgt.r
	$(CC) -c $(CFLAGS) `sed 's/$$/.c/' rttcur.lst`
	rm `sed 's/$$/.c/' rttcur.lst`

rmisc.o: rmisc.r $(HDRS)
	../../bin/rtt rmisc.r
	$(CC) -c $(CFLAGS) `sed 's/$$/.c/' rttcur.lst`
	rm `sed 's/$$/.c/' rttcur.lst`

rstruct.o: rstruct.r $(HDRS)
	../../bin/rtt rstruct.r
	$(CC) -c $(CFLAGS) `sed 's/$$/.c/' rttcur.lst`
	rm `sed 's/$$/.c/' rttcur.lst`

rsys.o: rsys.r $(HDRS)
	../../bin/rtt rsys.r
	$(CC) -c $(CFLAGS) `sed 's/$$/.c/' rttcur.lst`
	rm `sed 's/$$/.c/' rttcur.lst`

rwinrsc.o: rwinrsc.r $(HDRS) $(GRAPHICSHDRS)
	../../bin/rtt rwinrsc.r
	$(CC) -c $(CFLAGS) `sed 's/$$/.c/' rttcur.lst`
	rm `sed 's/$$/.c/' rttcur.lst`

rwinsys.o: rwinsys.r $(HDRS) $(GRAPHICSHDRS)
	../../bin/rtt rwinsys.r
	$(CC) -c $(CFLAGS) `sed 's/$$/.c/' rttcur.lst`
	rm `sed 's/$$/.c/' rttcur.lst`

rwindow.o: rwindow.r $(HDRS) $(GRAPHICSHDRS)
	../../bin/rtt rwindow.r
	$(CC) -c $(CFLAGS) `sed 's/$$/.c/' rttcur.lst`
	rm `sed 's/$$/.c/' rttcur.lst`

rcolor.o: rcolor.r $(HDRS) $(GRAPHICSHDRS)
	../../bin/rtt rcolor.r
	$(CC) -c $(CFLAGS) `sed 's/$$/.c/' rttcur.lst`
	rm `sed 's/$$/.c/' rttcur.lst`

rimage.o: rimage.r $(HDRS) $(GRAPHICSHDRS)
	../../bin/rtt rimage.r
	$(CC) -c $(CFLAGS) `sed 's/$$/.c/' rttcur.lst`
	rm `sed 's/$$/.c/' rttcur.lst`
