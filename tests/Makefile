#  Makefile for testing Icon


# for old versions of "make"
SHELL=/bin/sh
MAKE=make


# default is to run all tests, using icont

Test    Test-icont:	; cd general; $(MAKE) Test
Samples Samples-icont:	; cd general; $(MAKE) Samples

Test-iconc:		; cd general; $(MAKE) Test-iconc
Samples-iconc:		; cd general; $(MAKE) Samples-iconc


Clean:
		cd bench;	$(MAKE) Clean
		cd general;	$(MAKE) Clean
		cd preproc;	$(MAKE) Clean
		cd special;	$(MAKE) Clean
