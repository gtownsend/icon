SHELL=/bin/sh
MAKE=make

what:
		@echo "what do you want to make?"

Samples-iconc:
		sh Test-iconc

Samples-icont:
		sh Test-icont

Clean:
		rm -f local/*
		touch local/.placeholder

# Entries beyond this point are for use at Arizona only.
#   *** Do not delete the line above; it is used in trimming Makefiles
#   for distribution ***

Dist-clean:
		rm -f .*K* .emacs*
		rm -f local/*
		touch local/.placeholder
		rm -rf `gcomp Test* Makefile local stand *.icn *.dat`
