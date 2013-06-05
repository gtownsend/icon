#  Makefile for Version 9.5 of Icon
#
#  See doc/install.htm for instructions.


#  configuration parameters
VERSION=v951
name=unspecified
dest=/must/specify/dest/


##################################################################
#
# Default targets.

All:	Icont Ilib Ibin
	bin/icon -V

config/$(name)/status src/h/define.h:
	:
	: To configure Icon, run either
	:
	:	make Configure name=xxxx     [for no graphics]
	: or	make X-Configure name=xxxx   [with X-Windows graphics]
	:
	: where xxxx is one of
	:
	@cd config; ls -d `find * -type d -prune -print`
	:
	@exit 1


##################################################################
#
# Code configuration.


# Configure the code for a specific system.

Configure:	config/$(name)/status
		$(MAKE) Pure >/dev/null
		cd config; sh setup.sh $(name) NoGraphics

X-Configure:	config/$(name)/status
		$(MAKE) Pure >/dev/null
		cd config; sh setup.sh $(name) Graphics


# Get the status information for a specific system.

Status:
		@cat config/$(name)/status


##################################################################
#
# Compilation.


# The interpreter: icont and iconx.

Icont bin/icont: src/h/define.h
		uname -a
		pwd
		cd src/common;		$(MAKE)
		cd src/rtt;		$(MAKE)
		cd src/icont;		$(MAKE)
		cd src/runtime;		$(MAKE) 


# The Icon program library.

Ilib:		bin/icont
		cd ipl;			$(MAKE) Ilib

Ibin:		bin/icont
		cd ipl;			$(MAKE) Ibin


##################################################################
#
# Installation and packaging.


# Installation:  "make Install dest=new-icon-directory"

D=$(dest)
Install:
		mkdir $D
		mkdir $D/bin $D/lib $D/doc $D/man $D/man/man1
		cp README $D
		cp bin/[cflpvwx]* $D/bin
		cp bin/icon[tx]* $D/bin
		rm -f $D/bin/libI*
		(cd $D/bin; ln -s icont icon)
		cp lib/*.* $D/lib
		cp doc/*.* $D/doc
		cp man/man1/*.* $D/man/man1


# Bundle up for binary distribution.

DIR=icon-$(VERSION)
Package:
		rm -rf $(DIR)
		umask 002; $(MAKE) Install dest=$(DIR)
		tar cf - $(DIR) | gzip -9 >$(DIR).tgz
		rm -rf $(DIR)


##################################################################
#
# Tests.

Test    Test-icont:	; cd tests; $(MAKE) Test
Samples Samples-icont:	; cd tests; $(MAKE) Samples


#################################################################
#
# Run benchmarks.

Benchmark Benchmark-icont:
		cd tests/bench;		$(MAKE) benchmark-icont

Micro Microbench Microbenchmark:
		cd tests/bench;		$(MAKE) microbenchmark


##################################################################
#
# Cleanup.
#
# "make Clean" removes intermediate files, leaving executables and library.
# "make Pure"  also removes binaries, library, and configured files.

Clean:
		touch Makedefs
		rm -rf icon-*
		cd src;			$(MAKE) Clean
		cd ipl;			$(MAKE) Clean
		cd tests;		$(MAKE) Clean

Pure:
		touch Makedefs
		rm -rf icon-*
		rm -rf bin/[abcdefghijklmnopqrstuvwxyz]*
		rm -rf lib/[abcdefghijklmnopqrstuvwxyz]*
		cd ipl;			$(MAKE) Pure
		cd src;			$(MAKE) Pure
		cd tests;		$(MAKE) Pure
		cd config; 		$(MAKE) Pure



#  (This is used at Arizona to prepare source distributions.)

Dist-Clean:
		rm -rf xx `find * -type d -name CVS`
		rm -f  xx `find * -type f | xargs grep -l '<<ARIZONA-[O]NLY>>'`
		rm -f  xx `find . -type f -name '.??*' ! -name .placeholder`
		find . -type d | xargs chmod u=rwx,g=rwsx,o=rx
		find . -type f | xargs chmod ug=rw+X,o=r+X
