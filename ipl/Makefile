#  make library distribution (portable ucode and include files)
Ilib ilib/.complete:
	test -d ilib || mkdir ilib
	rm -f ilib/* ilib/.complete
	cp incl/*icn gincl/*icn mincl/*icn ilib
	cd cfuncs; LPATH= make ICONT=../../bin/icont cfunc.u2; mv *.u? ../ilib
	cd procs;  LPATH=../ilib ../../bin/icont -usc *icn; mv *.u? ../ilib
	cd gprocs; LPATH=../ilib ../../bin/icont -usc *icn; mv *.u? ../ilib
	cd mprocs; LPATH=../ilib ../../bin/icont -usc *icn; mv *.u? ../ilib
	touch ilib/.complete

# make program binaries (platform-dependent icode), given that ilib is ready
Iexe iexe/.complete:  ilib/.complete
	test -d iexe || mkdir iexe
	rm -f iexe/* iexe/.complete
	./BuildExe
	touch iexe/.complete
	

# check for undefined identifiers in ilib
# (many of these are known to exist outside the core modules)
Undef:
	cd ilib; for f in *.u2; do (echo $$f; icont -us -fs $$f); done

# check for stray files
Strays:
	for d in *procs *progs *incl; do (cd $$d; pwd; gcomp CVS *.icn); done

# verify that all procedures and programs build, including packs,
# and perform some other sanity checks
Check:
	./CheckAll


# clean up
Clean:
	rm -rf ilib iexe */*.u[12] */*.zip
	rm -f `find *procs *progs -type f -perm -100`
	for d in cfuncs *packs/[a-z]*; do (cd $$d; make Clean); done
