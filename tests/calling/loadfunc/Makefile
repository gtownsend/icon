SHELL=/bin/sh
MAKE=make

ISRCH = ../../../src/h
CFLAGS = -I$(ISRCH)


what:
		@echo "What do you want to make?"


test-loadfunc:	test-loadfunc-iconc test-loadfunc-icont

test-loadfunc-iconc:  lib.so
		@ echo Compiling btest
		@ iconc -us -fa btest.icn
		@ echo Running btest
		@ ./btest >btest.out
		@ echo Checking btest
		@ diff btest.out btest.std

test-loadfunc-icont:  lib.so
		@ echo Translating btest
		@ icont -us btest.icn
		@ echo Running btest
		@ ./btest >btest.out
		@ echo Checking btest
		@ diff btest.out btest.std

lib.so:	bitcount.o
		sh mklib.sh lib.so bitcount.o


Clean:
		rm -f *.o *.so *.out so_locations btest


# Entries beyond this point are for use at Arizona only.
#   *** Do not delete the line above; it is used in trimming Makefiles
#   for distribution ***

Dist-clean:
		rm -rf `gcomp README Makefile *.c *.icn *.sh *.std`
