SHELL=/bin/sh
MAKE=make

what:
		@echo "what do you want to make?"


##########################################################
#
#  Test iconc.
#

test-iconc:
		make test-opt
		make test-noopt

test-icont:
		sh Test-icont icont
		sh Test-icont large

test-opt:
		sh Test-iconc iconc
		sh Test-iconc invoke -fs
		sh Test-iconc errcnv -fen
		sh Test-iconc large -fdl

test-noopt:
		sh Test-iconc iconc -na -p -w
		sh Test-iconc debug -fde -na -p -w
		sh Test-iconc large -fdl -na -p -w

test-coexpr-iconc:
		sh Test-iconc coexpr -fd
		sh Test-iconc coexpr -fd -na -p -w

test-coexpr-icont:
		sh Test-icont coexpr

test-large:
		sh Test-icont large


##########################################################
#
#  Clean up.
#
Clean:
		rm -f local/*
		touch local/.placeholder


# Entries beyond this point are for use at Arizona only.
#   *** Do not delete the line above; it is used in trimming Makefiles
#   for distribution ***

Dist-clean:
		rm -f local/*
		touch local/.placeholder
		rm -rf `gcomp README Makefile Test* local stand *.icn *.dat \
		  *.lst`

Dist-vms:
		rm -f Makefile README Test-icont Test-icont
