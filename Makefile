SHELL=/bin/sh
MAKE=make
name=unspecified

default:	Icon Ilib Ibin

src/h/define.h:
	:
	: To configure Icon, run either
	:
	:	make Configure name=xxxx     [for no graphics]
	: or	make X-Configure name=xxxx   [with graphics]
	:
	: where xxxx is one of
	:
	@(cd config/unix; ls -d [a-z]*)
	:
	@exit 1

##################################################################
#
# Code configuration.

#
# Configure the code for a specific system.
#

Configure:
		make Clean
		cp config/unix/Common/Makefile config/unix/$(name)/
		cd config/unix/$(name);	$(MAKE) 
		rm -f config/unix/$(name)/Makefile

X-Configure:
		make Clean
		cp config/unix/Common/Makefile config/unix/$(name)/
		cd config/unix/$(name);	$(MAKE) X-Icon
		rm -f config/unix/$(name)/Makefile

#
# Check to see what systems have configuration information.
#

Supported:
		@echo "There is configuration information for"
		@echo " the following systems:"
		@echo ""
		@cd config/unix;		ls -d [a-z]*

#
# Get the status information for a specific system.
#

Status:
		@cat config/unix/$(name)/status

#
# Build a prototype configuration for a new system.
#

Platform:
		mkdir config/unix/$(name)
		cp config/unix/Common/* config/unix/$(name)

#
# Copy default header files.
#

Headers:
		cp config/unix/Common/*.hdr config/unix/$(name)

##################################################################
#
# Compilation and installation.
#

#
# The interpreter: icont and iconx.
#

Icon Icon-icont bin/icont: Common
		cd src/icont;		$(MAKE)
		cd src/runtime;		$(MAKE) 

#
# The compiler: rtt, the run-time system, and iconc.
# (NO LONGER SUPPORTED OR MAINTAINED.)
#

Icon-iconc:	Common
		cd src/runtime;		$(MAKE) comp_all
		cd src/iconc;		$(MAKE)

#
# Common components.
#

Common:		src/h/define.h
		cd src/common;		$(MAKE)
		cd src/rtt;		$(MAKE)

#
# Library.
#

Ilib:		bin/icont
		cd ipl;			$(MAKE)

Ibin:		bin/icont
		cd ipl;			$(MAKE) Ibin

##################################################################
#
# 
#

CopyLib:
		cp bin/dlrgint.o bin/rt.db bin/rt.h bin/*.a $(Target)
		-(test -f NoRanlib) || (ranlib $(Target)/*.a)

##################################################################
#
# Tests.
#

#
# Some simple tests to be sure Icon works.
#

Samples:
		$(MAKE) Samples-icont

Samples-iconc:
		cd tests/samples;	$(MAKE) Samples-iconc

Samples-icont:
		cd tests/samples;	$(MAKE) Samples-icont

#
# More exhaustive tests of various features of Icon and larger programs.
#

#
# Basic tests. Should show only insignificant differences.
#

Test:
		$(MAKE) Test-icont

Test-iconc:
		cd tests/general;	$(MAKE) test-iconc

Test-icont:
		cd tests/general;	$(MAKE) test-icont

Test-opt:
		cd tests/general;	$(MAKE) test-opt

Test-noopt:
		cd tests/general;	$(MAKE) test-noopt

#
# Tests of co-expressions. Should not show differences if co-expressions
# are implemented.
#

Test-coexpr:
		$(MAKE) Test-coexpr-icont

Test-coexpr-iconc:
		cd tests/general;	$(MAKE) test-coexpr-iconc

Test-coexpr-icont:
		cd tests/general;	$(MAKE) test-coexpr-icont

Test-large:
		cd tests/general;	$(MAKE) test-large

#################################################################
#
# Run benchmarks.
#

Benchmark:
		$(MAKE) Benchmark-icont

Benchmark-iconc:
		cd tests/bench;		$(MAKE) benchmark-iconc

Benchmark-icont:
		cd tests/bench;		$(MAKE) benchmark-icont


##################################################################
#
# Clean-up.
#
# "make Clean" removes intermediate files, leaving executables and library.
# "make Pure"  also removes binaries, library, and configured files.

Clean:
		cd src;			$(MAKE) Clean
		cd ipl;			$(MAKE) Clean
		cd tests;		$(MAKE) Clean

Pure:
		rm -f bin/[a-z]* lib/[a-z]* NoRanlib
		cd ipl;			$(MAKE) Clean
		cd src;			$(MAKE) Pure
		cd tests;		$(MAKE) Clean

Dist-Clean:
		rm -f `find * -type f | xargs grep -l '<<ARIZONA-ONLY>>'`
		

##################################################################
#Entries beyond this point are for use at Arizona only.
#   *** Do not delete the line above; it is used in trimming Makefiles
#   for distribution ***

Dist-clean:
		rm -rf `gcomp \
		   Makefile \
		   README \
		   config \
		   src \
		   tests`
		mkdir bin
		touch bin/.placeholder
		-cd config;		$(MAKE) Dist-clean
		-cd src;		$(MAKE) Dist-clean
		-cd tests;		$(MAKE) Dist-clean
		rm -rf config/unix/Ppdef

Dist-dist:
		cd src/preproc;		rm -f Makefile
		cd config;		rm -f mac_lsc

Dist-unix:
		cd config/unix;		$(MAKE)
		cd config;		rm -rf `gcomp Makefile unix`
		cd src/runtime;		rm -f fx*.ri rmac*.ri rms*.ri rpm*.ri
		cd src/h;		rm -f macgraph.h mswin.h pmwin.h

Dist-vms:
		-cd tests/general;	$(MAKE) Dist-vms
		cd config/vms;		$(MAKE)
		cd src/runtime;		rm -f fx*.ri rmac*.ri rms*.ri rpm*.ri
		cd src/h;		rm -f macgraph.h mswin.h pmwin.h
		rm -rf config
		find . -name Makefile -exec rm {} \;
