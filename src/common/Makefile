include ../../Makedefs
CFLAGS = $(CFCOMM)


OBJS =	long.o getopt.o time.o filepart.o identify.o strtbl.o rtdb.o\
	munix.o literals.o rswitch.o alloc.o long.o getopt.o time.o\
	save.o rswitch.o redirerr.o xwindow.o dlrgint.o ipp.o

common:		doincl patchstr $(OBJS) xpmaybe

doincl:		doincl.c
		$(CC) $(CFLAGS) -o doincl doincl.c
		-./doincl -o ../../bin/rt.h ../h/rt.h

patchstr:	patchstr.c
		$(CC) $(CFLAGS) -o patchstr patchstr.c
		cp patchstr ../../bin

xpmaybe:
		-if [ "$(XL)" != "" ]; then $(MAKE) xpm; fi

xpm:
		cd ../xpm; $(MAKE) libXpm.a
		cp ../xpm/libXpm.a ../../bin
		-(test -f ../../NoRanlib) || (ranlib ../../bin/libXpm.a)

$(OBJS): ../h/define.h ../h/config.h ../h/cstructs.h ../h/mproto.h \
	  ../h/typedefs.h ../h/proto.h ../h/cpuconf.h

identify.o: ../h/version.h

ipp.o: ../h/features.h

literals.o: ../h/esctab.h

rtdb.o: ../h/version.h icontype.h

dlrgint.o: ../h/rproto.h ../h/rexterns.h ../h/rmacros.h ../h/rstructs.h

xwindow.o: ../h/graphics.h ../h/xwin.h

rswitch.o: $(RSWITCH)
	$(CC) -c $(CFLAGS) $(RSWITCH)


#  The following section is needed if changes are made to the Icon grammar,
#  but it is not run as part of the normal installation process.  If it is
#  needed, it is run by changing ../icont/Makefile and/or ../iconc/Makefile;
#  see the comments there for details.  icont must be in the search path
#  for this section to work.
 
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
