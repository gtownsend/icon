SHELL=/bin/sh
MAKE=make

what:
		@echo "what do you want to make?"

Clean:
		cd extcall; $(MAKE) Clean
		cd loadfunc; $(MAKE) Clean
		rm -f foo

# Entries beyond this point are for use at Arizona only.
#   *** Do not delete the line above; it is used in trimming Makefiles
#   for distribution ***

Dist-clean:
		rm -f .*K* .emacs*
