#  make library distribution (portable ucode and include files)
Ilib:	.ldone
.ldone:
	cp incl/*.icn gincl/*.icn mincl/*.icn cfuncs/icall.h ../lib
	cd procs;  LPATH= ../../bin/icont -usc *.icn; mv *.u? ../../lib
	cd gprocs; LPATH= ../../bin/icont -usc *.icn; mv *.u? ../../lib
	cd mprocs; LPATH= ../../bin/icont -usc *.icn; mv *.u? ../../lib
	cd cfuncs; LPATH= $(MAKE) ICONT=../../bin/icont
	mv cfuncs/*.u? ../lib
	mv cfuncs/libcfunc.so ../bin
	touch .ldone

# make program binaries (platform-dependent icode), given that ../lib is ready
Iexe:	.xdone
.xdone:	.ldone
	rm -f .xdone iexe/*
	./BuildExe
	touch .xdone
	

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
	rm -rf .ldone .xdone iexe */*.u[12] */*.zip
	rm -f `find *procs *progs -type f -perm -100`
	for d in cfuncs *packs/[a-z]*; do (cd $$d; make Clean); done
