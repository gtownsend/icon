#  Makefile for the Icon translator, icont.

include ../../Makedefs


HFILES =	../h/define.h ../h/config.h ../h/cpuconf.h ../h/gsupport.h \
		   ../h/mproto.h ../h/typedefs.h ../h/cstructs.h

TRANS =		trans.o tcode.o tlex.o lnklist.o tparse.o tsym.o tmem.o tree.o

LINKR =		link.o lglob.o lcode.o llex.o lmem.o lsym.o opcode.o

OBJS =		tunix.o tglobals.o util.o $(TRANS) $(LINKR)

COBJS =		../common/long.o ../common/getopt.o ../common/alloc.o \
		   ../common/filepart.o ../common/strtbl.o ../common/ipp.o \
		   ../common/munix.o



icont:		$(OBJS) $(COBJS)
		$(CC) $(CFLAGS) $(LDFLAGS) -o icont $(OBJS) $(COBJS)
		cp icont ../../bin
		strip ../../bin/icont$(EXE)
		(cd ../../bin; rm -f icon; ln -s icont icon)

$(OBJS):	$(HFILES) tproto.h

$(COBJS):	$(HFILES)
		cd ../common; $(MAKE)

tunix.o:	tglobals.h ../h/version.h
tglobals.o:	tglobals.h
util.o:		tglobals.h tree.h ../h/fdefs.h

# translator files
trans.o:	tglobals.h tsym.h ttoken.h tree.h ../h/version.h ../h/kdefs.h
lnklist.o:	lfile.h
tparse.o:	../h/lexdef.h tglobals.h tsym.h tree.h keyword.h
tcode.o:	tglobals.h tsym.h ttoken.h tree.h
tlex.o:		../h/lexdef.h ../h/parserr.h ttoken.h tree.h ../h/esctab.h \
		   ../common/lextab.h ../common/yylex.h ../common/error.h
tmem.o:		tglobals.h tsym.h tree.h
tree.o:		tree.h
tsym.o:		tglobals.h tsym.h ttoken.h lfile.h keyword.h ../h/kdefs.h

# linker files
$(LINKR):	link.h lfile.h ../h/rt.h ../h/sys.h ../h/monitor.h \
		   ../h/rstructs.h ../h/rmacros.h ../h/rexterns.h

link.o:		tglobals.h hdr.h ../h/header.h
lcode.o:	tglobals.h opcode.h keyword.h ../h/header.h \
			../h/opdefs.h ../h/version.h
lglob.o:	tglobals.h opcode.h ../h/opdefs.h ../h/version.h
llex.o:		tglobals.h opcode.h ../h/opdefs.h
lmem.o:		tglobals.h
lsym.o:		tglobals.h
opcode.o:	opcode.h ../h/opdefs.h

#  hdr.h is always built, to simplify the Makefile,
#  but it is only actually used if BinHeader is define.
hdr.h:		newhdr ixhdr.hdr
		./newhdr -o hdr.h ixhdr.hdr
newhdr:		newhdr.c ../h/define.h ../h/config.h ../h/gsupport.h
		$(CC) $(CFLAGS) $(LDFLAGS) -o newhdr newhdr.c
ixhdr.hdr:	ixhdr.c ../h/define.h ../h/config.h ../h/header.h $(COBJS)
		$(CC) $(CFLAGS) $(LDFLAGS) -o ixhdr.hdr \
			ixhdr.c ../common/alloc.o ../common/munix.o
		strip ixhdr.hdr




#  The following sections are commented out because they do not need to be
#  performed unless changes are made to cgrammar.c, ../h/grammar.h,
#  ../common/tokens.txt, or ../common/op.txt.  Such changes involve
#  modifications to the syntax of Icon and are not part of the installation
#  process. However, if the distribution files are unloaded in a fashion
#  such that their dates are not set properly, the following sections would
#  be attempted.
#
#  Note that if any changes are made to the files mentioned above, the comment
#  characters at the beginning of the following lines should be removed.
#  icont must be on your search path for these actions to work.
#
#../common/lextab.h ../common/yacctok.h ../common/fixgram ../common/pscript: \
#			../common/tokens.txt ../common/op.txt
#		cd ../common; $(MAKE) gfiles
#
#tparse.c ttoken.h:	tgram.g trash ../common/pscript
## expect 218 shift/reduce conflicts
#		yacc -d tgram.g
#		./trash <y.tab.c | ../common/pscript >tparse.c
#		mv y.tab.h ttoken.h
#		rm -f y.tab.c
#
#tgram.g:	tgrammar.c ../h/define.h ../h/grammar.h \
#			../common/yacctok.h ../common/fixgram 
#		$(CC) -E -C tgrammar.c | ../common/fixgram >tgram.g
#
#../h/kdefs.h keyword.h:	../runtime/keyword.r mkkwd
#		./mkkwd <../runtime/keyword.r
#
#trash:		trash.icn
#		icont -s trash.icn
#
#mkkwd:		mkkwd.icn
#		icont -s mkkwd.icn
