#  Makefile for the Icon compiler, iconc.
#
#  This is no longer supported and may not work.

include ../../Makedefs


OBJS =		cmain.o ctrans.o dbase.o clex.o\
		cparse.o csym.o cmem.o ctree.o ccode.o ccomp.o\
                ivalues.o codegen.o fixcode.o inline.o chkinv.o\
		typinfer.o types.o lifetime.o incheck.o

COBJS =		../common/long.o ../common/getopt.o ../common/time.o\
		  ../common/filepart.o ../common/identify.o ../common/munix.o\
		  ../common/strtbl.o ../common/rtdb.o ../common/literals.o \
		  ../common/alloc.o ../common/ipp.o



iconc:		$(OBJS) $(COBJS)
		$(CC) -o iconc $(OBJS) $(COBJS)
		cp iconc ../../bin
		strip ../../bin/iconc$(EXE)

$(OBJS):	../h/config.h ../h/cpuconf.h ../h/cstructs.h ../h/define.h\
		../h/mproto.h ../h/typedefs.h ../h/gsupport.h \
		ccode.h cglobals.h cproto.h csym.h ctrans.h ctree.h

$(COBJS):	../h/mproto.h
		cd ../common; $(MAKE); $(MAKE) xpm

ccode.o:	../h/lexdef.h ctoken.h
chkinv.o:	ctoken.h
clex.o:		../h/lexdef.h ../h/parserr.h ctoken.h \
		   ../common/lextab.h ../common/yylex.h ../common/error.h
clocal.o:	../h/config.h
cparse.o:	../h/lexdef.h
ctrans.o:	ctoken.h
ctree.o:	../h/lexdef.h ctoken.h
csym.o:		ctoken.h
dbase.o:	../h/lexdef.h
lifetime.o:	../h/lexdef.h ctoken.h
typinfer.o:	../h/lexdef.h ctoken.h
types.o:	../h/lexdef.h ctoken.h



#  The following sections are commented out because they do not need to
#  be performed unless changes are made to cgrammar.c, ../h/grammar.h,
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
#cparse.c ctoken.h:	cgram.g ../common/pscript
## expect 218 shift/reduce conflicts
#		yacc -d cgram.g
#		../common/pscript <y.tab.c >cparse.c
#		mv y.tab.h ctoken.h
#		rm -f y.tab.c
#
#cgram.g:	cgrammar.c ../h/define.h ../h/grammar.h \
#			../common/yacctok.h ../common/fixgram
#		$(CC) -E -C cgrammar.c | ../common/fixgram >cgram.g
