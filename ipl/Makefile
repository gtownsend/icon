#  Makefile for Icon Program Library


#  make library distribution (portable ucode and include files)
Ilib:	../bin/libcfunc.so
../bin/libcfunc.so:  ../bin/icont
	cp incl/*.icn gincl/*.icn mincl/*.icn cfuncs/icall.h ../lib
	cd procs;  LPATH= ../../bin/icont -usc *.icn; mv *.u? ../../lib
	cd gprocs; LPATH= ../../bin/icont -usc *.icn; mv *.u? ../../lib
	cd mprocs; LPATH= ../../bin/icont -usc *.icn; mv *.u? ../../lib
	cd cfuncs; LPATH= $(MAKE) ICONT=../../bin/icont
	mv cfuncs/*.u? ../lib
	mv cfuncs/libcfunc.so ../bin

# make selected program binaries (platform-dependent icode) for ../bin,
# given that ../lib is ready
Ibin:	../bin/ipatch
../bin/ipatch:  ../bin/icont
	./BuildBin


# make full set of program binaries (not usually done) in ./iexe,
# given that ../lib is ready
Iexe:
	rm -f iexe/*
	./BuildExe

# check for undefined identifiers in ../lib
# (outside the core modules, there are many of these)
Undef:
	cd ../lib; for f in *.u2; do (echo $$f; icont -us -fs $$f); done

# check for stray files
Strays:
	for d in *procs *progs *incl; do (cd $$d; pwd; gcomp CVS *.icn); done

# verify that all procedures and programs build, including packs,
# and perform some other sanity checks
Check:
	./CheckAll


# clean up
Clean:
	rm -rf iexe */*.u[12] */*.zip
	rm -f `find *procs *progs -type f -perm -100`
	for d in cfuncs *packs/[a-z]*; do (cd $$d; make Clean); done
