SHELL=/bin/sh
MAKE=make

what:
	@echo 'What do you want to make?'

##########################################################
#
#  Clean up.
#
Clean:
		-cd bench;	$(MAKE) Clean
		-cd calling;	$(MAKE) Clean
		-cd general;	$(MAKE) Clean
		-cd graphics;	$(MAKE) Clean
		-cd preproc;	$(MAKE) Clean
		-cd samples;	$(MAKE) Clean
		-cd special;	$(MAKE) Clean


# Entries beyond this point are for use at Arizona only.
#   *** Do not delete the line above; it is used in trimming Makefiles
#   for distribution ***

Dist-clean:
		-cd bench;	$(MAKE) Dist-clean
		-cd calling;	$(MAKE) Dist-clean
		-cd general;	$(MAKE) Dist-clean
		-cd graphics;	$(MAKE) Dist-clean
		-cd preproc;	$(MAKE) Dist-clean
		-cd samples;	$(MAKE) Dist-clean
		-cd special;	$(MAKE) Dist-clean
		rm -rf `gcomp bench calling general samples \
		  graphics special preproc Makefile`
