SHELL=/bin/sh
MAKE=make
what:
	@echo "What do you want to make?"

Clean:
		cd iconc; rm -f *.o iconc
		cd common; rm -f *.o doincl patchstr
		cd preproc; rm -f *.o pp
		cd rtt; rm -f *.o rtt
		cd runtime; rm -f *.c *.o rt.db rt.a rttcur.lst\
		  rttfull.lst iconx
		cd icont; rm -f *.o icont
		-cd xpm; rm -f *.o *.a
# force full rebuild of runtime system
		touch h/define.h

Pure:		Clean
		rm -f common/rswitch.* h/path.h h/define.h */Makefile

# Entries beyond this point are for use at Arizona only.
#   *** Do not delete the line above; it is used in trimming Makefiles
#   for distribution ***

Dist-clean:
		rm -f */Make*
		cd h; rm -f define.h; rm -rf `gcomp *.[h]`
		cd iconc; rm -f `gcomp *.[ch] *.icn *.txt`
		cd runtime; rm -f *.c; rm -f `gcomp *.icn *.[ch] *.r *.ri`
		cd rtt; rm -f `gcomp *.[chy]`
		cd common;rm -f rswitch.*; rm -f `gcomp *.[ch] *.icn *.txt`
		cd preproc; rm -f `gcomp README *.[ch]`
		cd icont; rm -f y.tab.c\
		   hdr.h; rm -rf `gcomp *.[ch] *.icn *.txt`
		rm -rf `gcomp Makefile h iconc rtt preproc common runtime \
		   icont xpm`
		cd xpm; rm -f *.o *.a

#  The following entry forces rebuilding of everthing from first-generation
#  files, even files not normally recreated.  Doing this requires uncommenting
#  some lines in common/Makefile, icont/Makefile, and iconc/Makefile.

Force-rebuild: Clean
	cd h; rm -f kdefs.h
	cd common; rm -f *.o yacctok.h lextab.h icontype.h \
		doincl fixgram mktoktab patchstr pscript typespec
	cd icont; rm -f *.o icont mkkwd trash \
		hdr.h keyword.h tgram.g ttoken.h tparse.c
	cd iconc; rm -f *.o iconc cgram.g ctoken.h cparse.h
