############################################################################ 
#
# Unix Makefile for installing itweak and running a sample debugging session.
#
# $Id$
# updated 4-aug-2000/gmt
#
# 'make' or 'make install'
# does the necessary compilations to get the itweak package ready to use.
# Note, however, that it leaves the resulting files in the current directory.
# You must move or copy them yourself if you want them any other place.
# (See the documentation.)
#
# 'make sample-debug'
# compiles, tweaks, and links a sample program to make it ready for a
# debugging session.
# Assumes the 'dbg_run.u?' files are on your IPATH or in the current directory
# which is the case if you haven't moved things around since 'make install'.
#
# The sample executable is named 'sample'.
# The program is, however, identical 'ipxref' copied from the Icon Library.
# It also requires 'options.icn' (included), so the program is built from two
# source files.
#
# 'make demo'
# runs a debugging session with the sample program.
# It is uncommon to run debugging sessions from a Makefile.
# This is only for demo purposes.
#
# This makefile is in itself an example of how to construct makefiles.
# It provides a simple way to switch between a clean (untweaked) version
# and a tweaked version of the sample program without duplicating a lot of
# makefile code.
# Use 'make sample-clean' to force compilation of a clean (untweaked) copy of
# 'sample'.
#
############################################################################ 
# 
# Copyright (c) 1994 Hakan Soderstrom and
# Soderstrom Programvaruverkstad AB, Sweden
# 
# Permission to use, copy, modify, distribute, and sell this software
# and its documentation for any purpose is hereby granted without fee,
# provided that the above copyright notice and this permission notice
# appear in all copies of the software and related documentation.
# 
# THE SOFTWARE IS PROVIDED "AS-IS" AND WITHOUT WARRANTY OF ANY KIND,
# EXPRESS, IMPLIED OR OTHERWISE, INCLUDING WITHOUT LIMITATION, ANY
# WARRANTY OF MERCHANTABILITY OR FITNESS FOR A PARTICULAR PURPOSE.
#
# IN NO EVENT SHALL HAKAN SODERSTROM OR SODERSTROM PROGRAMVARUVERKSTAD
# AB BE LIABLE FOR ANY SPECIAL, INCIDENTAL, INDIRECT OR CONSEQUENTIAL
# DAMAGES OF ANY KIND, OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS
# OF USE, DATA OR PROFITS, WHETHER OR NOT ADVISED OF THE POSSIBILITY
# OF DAMAGE, AND ON ANY THEORY OF LIABILITY, ARISING OUT OF OR IN
# CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
#
############################################################################ 

ICONT=icont -s
ITWEAK=itweak


MAKEFILE=Makefile
SAMPLE_INIT=samp_ini.icn
CMD=demo.cmd

##### 'install' targets

install : itweak dbg_run.u1

itweak : itweak.icn
	$(ICONT) itweak.icn

dbg_run.u1 : dbg_run.icn
	$(ICONT) -c dbg_run.icn

##### 'sample' targets: first the plain ones
##### The program is built from source files 'ipxref.icn' and 'options.icn'.
##### The name of the resulting program is 'sample'.

sample : ipxref.u1 options.u1 $(DEBUG)
	$(ICONT) -u -o sample ipxref.u1 options.u1

ipxref.u1 : ipxref.icn
	$(ICONT) -cu ipxref.icn

options.u1 : options.icn
	$(ICONT) -cu options.icn

##### 'sample' targets: the debugging stuff

sample-debug :
	make -f $(MAKEFILE) sample DEBUG=$(SAMPLE_INIT)

$(SAMPLE_INIT) : ipxref.u1 options.u1
	@echo '*** This is how the program files are tweaked...'
	$(ITWEAK) -o $(SAMPLE_INIT) ipxref options
	@echo '*** ... and don't forget to compile the generated file.'
	$(ICONT) -cu $(SAMPLE_INIT)

sample-clean :
	rm -f ipxref.u? options.u?
	make -f $(MAKEFILE) sample

##### demo session

demo : sample-debug
	@echo 'We will now start a sample debugging session.'
	@echo 'Debugging commands will be taken from the file $(CMD).'
	@echo 'Please open an editor on this file -- the commands will'
	@echo 'not appear in the debugger output.'
	@echo '-------------- session start --------------------------'
	@(DBG_INPUT=$(CMD); export DBG_INPUT; sample ipxref.icn)
	@echo '-------------- session end ----------------------------'

##### build executable and copy to ../../iexe
##### (nothing done in this case because the executable doesn't stand alone)

Iexe :

##### cleanup

Clean :
	rm -f $(ITWEAK) *.u[12]
