#  Makefile for configuring a Unix build.
#
#  (make Setup name=xxx_yyy)

name = undefined
TOP = ../..
SRC = $(TOP)/src


Setup-Common:
	echo '#define Config "$(name)"'		 > $(SRC)/h/define.h
	echo ''					>> $(SRC)/h/define.h
	cat  $(name)/define.h			>> $(SRC)/h/define.h
	echo ''					>> $(SRC)/h/define.h
	cp $(name)/rswitch.[cs] $(SRC)/common
	echo '#  from config/unix/$(name)'	 > $(TOP)/Makedefs
	echo ''					>> $(TOP)/Makedefs
	cat $(name)/Makedefs			>> $(TOP)/Makedefs
	echo ''					>> $(TOP)/Makedefs
	if [ -f $(name)/custom.sh ]; then cd $(name); sh custom.sh; fi

Setup-NoGraphics: Setup-Common
	echo '#  Graphics not enabled'		>> $(TOP)/Makedefs
	echo '/* no Graphics */'		>> $(SRC)/h/define.h

Setup-Graphics:  Setup-Common
	echo '#  Graphics enabled'		>> $(TOP)/Makedefs
	echo 'XL = -L../../bin -lXpm $$(XLIBS)'	>> $(TOP)/Makedefs
	echo '#define Graphics 1'		>> $(SRC)/h/define.h


Clean:


Pure:
	rm -f $(TOP)/Makedefs
	rm -f $(SRC)/h/define.h
	rm -f $(SRC)/common/rswitch.[csS]
