# make binaries for library distribution (ucode only)
Ilib:
	test -d ilib || mkdir ilib
	cp incl/*icn gincl/*icn mincl/*icn ilib
	(cd cfuncs; LPATH=        make cfunc.u2;             mv *.u? ../ilib)
	(cd procs;  LPATH=../ilib ../../bin/icont -usc *icn; mv *.u? ../ilib)
	(cd gprocs; LPATH=../ilib ../../bin/icont -usc *icn; mv *.u? ../ilib)
	(cd mprocs; LPATH=../ilib ../../bin/icont -usc *icn; mv *.u? ../ilib)

# check for undefined identifiers and externals  (requires ilib)
Undef:
	cd ilib; for f in *.u2; do (echo $$f; icont -us -fs $$f); done

# check for stray files
Strays:
	for d in *procs *progs *incl; do (cd $$d; pwd; gcomp CVS *.icn); done

# build all programs including packs, to verify that they do build,
# and perform some other sanity checks
Check:
	BuildAll

# clean up
Clean:
	rm -rf ilib */*.u[12] */*.zip
	rm -f `find *procs *progs -type f -perm -100`
	cd cfuncs; $(MAKE) Clean
	for d in *packs/[a-z]*; do (cd $$d; make Clean); done
