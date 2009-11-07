#  Icon source Makefile, normally used only for cleanup.


what:
	@echo "What do you want to make?"

Clean Pure:
	cd common;	rm -f *.o *.exe patchstr infer
	cd preproc;	rm -f *.o *.exe pp
	cd rtt;		rm -f *.o *.exe rtt
	cd runtime;	rm -f *.o *.exe *.c rt.db rt.a rtt*.lst iconx
	cd icont;	rm -f *.o *.exe icont hdr.h ixhdr.hdr newhdr
	cd wincap;	rm -f *.o *.exe *.a
	cd xpm;		rm -f *.o *.exe *.a
# force full runtime system rebuild
	touch h/define.h
	rm -f h/arch.h


#  The following entry forces rebuilding of everthing from first-generation
#  files, even files not normally recreated.  Doing this requires uncommenting
#  some lines in common/Makefile and icont/Makefile.

Force-rebuild: Clean
	cd h; rm -f kdefs.h
	cd common; rm -f *.o yacctok.h lextab.h icontype.h \
		fixgram mktoktab patchstr pscript typespec
	cd icont; rm -f *.o icont mkkwd trash \
		hdr.h keyword.h tgram.g ttoken.h tparse.c
