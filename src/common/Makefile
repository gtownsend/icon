include ../../Makedefs


OBJS =	long.o getopt.o time.o filepart.o identify.o strtbl.o rtdb.o\
	munix.o literals.o rswitch.o alloc.o long.o getopt.o time.o\
	xwindow.o ipp.o

common:		$(OBJS) gpxmaybe

patchstr:	patchstr.c
		$(CC) $(CFLAGS) -o patchstr patchstr.c

gpxmaybe:
		-if [ "x$(XL)" != "x" ]; then $(MAKE) $(GDIR); fi

xpm:
		cd ../xpm; $(MAKE) libXpm.a
		cp -p ../xpm/libXpm.a ../../bin/libIgpx.a

wincap:		
		cd ../wincap; $(MAKE) libWincap.a
		cp -u ../wincap/libWincap.a ../../bin/libIgpx.a

$(OBJS): ../h/define.h ../h/arch.h ../h/config.h ../h/cstructs.h \
	  ../h/typedefs.h ../h/mproto.h ../h/cpuconf.h

../h/arch.h:	infer.c
		$(CC) $(CFLAGS) -o infer infer.c
		./infer >../h/arch.h

identify.o: ../h/version.h

ipp.o: ../h/features.h

literals.o: ../h/esctab.h

rtdb.o: ../h/version.h icontype.h

xwindow.o: ../h/graphics.h ../h/xwin.h


#  The following section is needed if changes are made to the Icon grammar,
#  but it is not run as part of the normal installation process.  If it is
#  needed, it is run by changing ../icont/Makefile; see the comments there
#  for details.  icont must be in the search path for this section to work.

gfiles:			lextab.h yacctok.h fixgram pscript

lextab.h yacctok.h:	tokens.txt op.txt mktoktab
			./mktoktab

mktoktab:		mktoktab.icn
			icont -s mktoktab.icn

fixgram:		fixgram.icn
			icont -s fixgram.icn

pscript:		pscript.icn
			icont -s pscript.icn



#  The following section is commented out because it does not need to be
#  performed unless changes are made to typespec.txt. Such changes
#  and are not part of the installation process.  However, if the
#  distribution files are unloaded in a fashion such that their dates
#  are not set properly, the following section would be attempted.
#
#  Note that if any changes are made to the file mentioned above, the
#  comment characters at the beginning of the following lines should be
#  removed.
#
#  Note that icont must be on your search path for this.
#
#
#icontype.h: typespec.txt typespec
#	typespec <typespec.txt >icontype.h
#
#typespec: typespec.icn
#	icont typespec
