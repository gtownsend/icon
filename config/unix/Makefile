#  Makefile for configuring a Unix build.
#
#  (make Setup name=xxx_yyy)

name = undefined
TOP = ../..
SRC = $(TOP)/src


Setup-Common:
	cp $(name)/define.h $(SRC)/h
	cp $(name)/rswitch.[csS] $(SRC)/common
	-if grep -s NoRanlib $(name)/define.h; \
	   then touch $(TOP)/NoRanlib; \
	   else rm -f $(TOP)/NoRanlib; fi
	cp $(name)/Makedefs $(TOP)
	echo ''					>> $(TOP)/Makedefs

Setup-NoGraphics: Setup-Common
	echo '# Graphics not enabled'		>> $(TOP)/Makedefs

Setup-Graphics:  Setup-Common
	echo '# Graphics enabled'		>> $(TOP)/Makedefs
	echo 'XI = $$(XINCL)'			>> $(TOP)/Makedefs
	echo 'XL = -L../../bin -lXpm $$(XLINK)'	>> $(TOP)/Makedefs
	echo '#define Graphics 1'		>> $(SRC)/h/define.h


Clean:


Pure:
	rm -f $(TOP)/Makedefs
	rm -f $(SRC)/h/define.h
	rm -f $(SRC)/common/rswitch.[csS]
	rm -f $(TOP)/NoRanlib
