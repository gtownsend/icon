# make binaries for library distribution (ucode only)
Ilib:
	test -d ilib || mkdir ilib
	cp incl/*icn gincl/*icn mincl/*icn ilib
	(cd cfuncs; LPATH=        make cfunc.u2;             mv *.u? ../ilib)
	(cd procs;  LPATH=../ilib ../../bin/icont -usc *icn; mv *.u? ../ilib)
	(cd gprocs; LPATH=../ilib ../../bin/icont -usc *icn; mv *.u? ../ilib)
	(cd mprocs; LPATH=../ilib ../../bin/icont -usc *icn; mv *.u? ../ilib)

# build programs, just to verify that they compile and link
Progs:
	BuildAll


# check for undefined identifiers and externals
Undef:
	cd ilib; for f in *.u2; do (echo $$f; icont -us $$f); done

# check for stray files
Strays:
	for d in *procs *progs *incl; do (cd $$d; pwd; gcomp CVS *.icn); done

# clean up
Clean:
	rm -rf ilib */*.u[12] */*.zip
	rm -f `find *procs *progs -type f -perm -100`
	cd cfuncs; $(MAKE) Clean
	for d in *packs/[a-z]*; do (cd $$d; make Clean); done
