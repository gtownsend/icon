#  Icon source Makefile, normally used only for cleanup.


what:
	@echo "What do you want to make?"

Clean Pure:
	cd iconc;	rm -f *.o iconc
	cd common;	rm -f *.o doincl patchstr infer
	cd preproc;	rm -f *.o pp
	cd rtt;		rm -f *.o rtt
	cd runtime;	rm -f *.o *.c rt.db rt.a rttcur.lst rttfull.lst iconx
	cd icont;	rm -f *.o icont hdr.h ixhdr.hdr newhdr
	cd wincap;	rm -f *.o *.a
	cd xpm;		rm -f *.o *.a
# force full runtime system rebuild
	touch h/define.h
	rm -f h/arch.h


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
