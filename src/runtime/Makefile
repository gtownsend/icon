#  Makefile for the Icon run-time system.

include ../../Makedefs


HDRS = ../h/define.h ../h/config.h ../h/typedefs.h \
	  ../h/cstructs.h ../h/cpuconf.h ../h/grttin.h\
	  ../h/rmacros.h ../h/rexterns.h ../h/rstructs.h \
	  ../h/rproto.h ../h/mproto.h ../h/sys.h

GRAPHICSHDRS = ../h/graphics.h ../h/xwin.h ../h/mswin.h

COBJS =	../common/long.o ../common/time.o \
	../common/rswitch.o ../common/xwindow.o \
	../common/alloc.o ../common/filepart.o ../common/munix.o


default: iconx

$(COBJS):
	cd ../common; $(MAKE)


XOBJS =	xcnv.o xdata.o xdef.o xerrmsg.o xfconv.o xfload.o xfmath.o\
	xfmisc.o xfscan.o xfstr.o xfstranl.o xfstruct.o xfsys.o\
	xfwindow.o ximain.o ximisc.o xinit.o xinterp.o xinvoke.o\
	xkeyword.o xlmisc.o xoarith.o xoasgn.o xocat.o xocomp.o\
	xomisc.o xoref.o xoset.o xovalue.o xralc.o xrcoexpr.o xrcomp.o\
	xrdebug.o xrexternal.o xrlrgint.o xrmemmgt.o xrmisc.o xrstruct.o \
	xrsys.o xrwinrsc.o xrwinsys.o xrwindow.o xrcolor.o xrimage.o

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

xrexternal.o: rexternal.r $(HDRS)
	../../bin/rtt -x rexternal.r
	$(CC) -c $(CFLAGS) xrexternal.c
	rm xrexternal.c

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

xrwinsys.o: rwinsys.r $(HDRS) $(GRAPHICSHDRS) rxwin.ri rmswin.ri
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
