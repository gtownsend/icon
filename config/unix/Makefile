#  Makefile for configuring a Unix build.
#
#  (make Setup name=xxx_yyy)

name = undefined
TOP = ../..
SRC = $(TOP)/src


Setup:
	cp $(name)/Makedefs $(TOP)
	cp $(name)/define.h $(SRC)/h
	cp $(name)/rswitch.[csS] $(SRC)/common
	-if grep -s NoRanlib define.h; \
	   then touch $(TOP)/NoRanlib; \
	   else rm -f $(TOP)/NoRanlib; fi


Clean:


Pure:
	rm -f $(TOP)/Makedefs
	rm -f $(SRC)/h/define.h
	rm -f $(SRC)/common/rswitch.[csS]
	rm -f $(TOP)/NoRanlib
