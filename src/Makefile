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
# force full rebuild of runtime system by touching define.h if present
		touch /dev/null `find h -name define.h`

Pure:		Clean
		rm -f common/rswitch.* h/define.h

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
