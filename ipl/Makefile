#  Makefile for the Icon Program Library


All:	Ilib Ibin


#  Make a library distribution (portable ucode and include files).

Ilib:	cfuncs/libcfunc.so
cfuncs/libcfunc.so:  ../bin/icont
	cp incl/*.icn gincl/*.icn cfuncs/icall.h ../lib
	cd procs;  LPATH= ../../bin/icont -usc *.icn; mv *.u? ../../lib
	cd gprocs; LPATH= ../../bin/icont -usc *.icn; mv *.u? ../../lib
	if grep '^ *# *define LoadFunc' ../src/h/define.h >/dev/null; \
	   then $(MAKE) Cfunctions; fi


#  Make C functions.  Only called if LoadFunc is defined.

Cfunctions:
	cd cfuncs; LPATH= $(MAKE) ICONT=../../bin/icont
	cp cfuncs/*.u? ../lib
	cp cfuncs/libcfunc.so ../bin


#  Make selected graphics program binaries (platform-dependent icode)
#  for ../bin, given that ../lib is ready

Ibin:	gpacks/vib/vib
gpacks/vib/vib:  ../bin/icont
	MAKE=$(MAKE) ./BuildBin


#  Make a full set of program binaries (not usually done) in ./iexe,
#  given that ../lib is ready

Iexe:
	rm -f iexe/*
	MAKE=$(MAKE) ./BuildExe


#  Check for undefined identifiers in ../lib.
#  (A few are expected: references to Mp, program, init, goal).

Undef:
	cd ../lib; for f in *.u2; do (echo $$f; icont -us -fs $$f); done


#  Check for stray files

Strays:
	for d in *procs *progs *incl; do (cd $$d; pwd; gcomp CVS *.icn); done


#  Verify that all procedures and programs build, including packs,
#  and perform some other sanity checks

Check:
	./CheckAll


#  Make Zip files for separate distribution of the library

ZipFiles:  Ilib
	rm -rf ilib *.zip
	zip -qrX9 bipl.zip docs incl procs progs packs data cfuncs -x '*/CVS/*'
	zip -qrX9 gipl.zip gdocs gincl gprocs gprogs gpacks gdata  -x '*/CVS/*'
	zip -qrX9 mipl.zip mincl mprocs mprogs                     -x '*/CVS/*'
	mkdir ilib
	cp ../lib/*.* ilib
	zip -qrX9 ilib.zip ilib
	rm -rf ilib


#  Clean up.

Clean Pure:
	-rm -rf ilib iexe *.zip */*.u[12] */*.zip */*.so
	-rm -f xx `find *procs *progs -type f -perm -100 -print`
	for d in cfuncs *packs/[abcdefghijklmnopqrstuvwxyz]*; do \
	    (cd $$d; $(MAKE) Clean); done
