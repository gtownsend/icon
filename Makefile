#  configuration parameters
VER=9.4.d
name=unspecified
dest=/must/specify/dest/

# for old versions of "make"
SHELL=/bin/sh
MAKE=make

##################################################################
#
# Default targets.
#

default:	Icon Ilib Ibin

config/unix/$(name)/status src/h/define.h:
	:
	: To configure Icon, run either
	:
	:	make Configure name=xxxx     [for no graphics]
	: or	make X-Configure name=xxxx   [with X-Windows graphics]
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

Configure:	config/unix/$(name)/status
		make Clean
		cp config/unix/Common/Makefile config/unix/$(name)/
		cd config/unix/$(name);	$(MAKE) 
		rm -f config/unix/$(name)/Makefile

X-Configure:	config/unix/$(name)/status
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
# Compilation.
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
# Installation and packaging.
#

#
# Installation:  "make Install dest=new-parent-directory"
#
D=$(dest)
Install:
		test -d $D || mkdir $D
		test -d $D/bin || mkdir $D/bin
		test -d $D/lib || mkdir $D/lib
		test -d $D/doc || mkdir $D/doc
		test -d $D/man || mkdir $D/man
		test -d $D/man/man1 || mkdir $D/man/man1
		cp bin/[a-qs-z]* $D/bin
		rm -f $D/bin/libXpm*
		cp lib/*.* $D/lib
		cp doc/*.* $D/doc
		cp man/man1/icont.1 $D/man/man1

#
# Package for binary distribution.
#
DIR=icon.$(VER)
Package:
		rm -rf $(DIR)
		umask 002; $(MAKE) Install dest=$(DIR)
		cp README $(DIR)
		tar cf - icon.$(VER) | gzip -9 >icon.$(VER).tgz
		rm -rf $(DIR)
		


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

Test    Test-icont:	; cd tests; $(MAKE) Test
Samples Samples-icont:	; cd tests; $(MAKE) Samples

Test-iconc:		; cd tests; $(MAKE) Test-iconc
Samples-iconc:		; cd tests; $(MAKE) Samples-iconc



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
		rm -rf icon.*
		cd src;			$(MAKE) Clean
		cd ipl;			$(MAKE) Clean
		cd tests;		$(MAKE) Clean

Pure:
		rm -f icon.* bin/[a-z]* lib/[a-z]* NoRanlib
		cd ipl;			$(MAKE) Clean
		cd src;			$(MAKE) Pure
		cd tests;		$(MAKE) Clean

Dist-Clean:
		rm -rf `find * -type d -name CVS`
		rm -f `find * -type f | xargs grep -l '<<ARIZONA-[O]NLY>>'`
