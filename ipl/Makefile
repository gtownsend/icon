#  Makefile for the Icon Program Library


#  Make a library distribution (portable ucode and include files).

Ilib:	../bin/libcfunc.so
../bin/libcfunc.so:  ../bin/icont
	cp incl/*.icn gincl/*.icn cfuncs/icall.h ../lib
	cd procs;  LPATH= ../../bin/icont -usc *.icn; mv *.u? ../../lib
	cd gprocs; LPATH= ../../bin/icont -usc *.icn; mv *.u? ../../lib
	cd cfuncs; LPATH= $(MAKE) ICONT=../../bin/icont
	mv cfuncs/*.u? ../lib
	mv cfuncs/libcfunc.so ../bin


#  Make selected program binaries (platform-dependent icode) for ../bin,
#  given that ../lib is ready

Ibin:	../bin/ipatch
../bin/ipatch:  ../bin/icont
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
	rm -rf ilib iexe *.zip */*.u[12] */*.zip
	-rm -f `find *procs *progs -type f -perm -100 -print`
	for d in cfuncs *packs/[a-z]*; do (cd $$d; $(MAKE) Clean); done
