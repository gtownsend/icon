#  Makefile for configuring a Unix build.
#
#  (make Setup-[No]Graphics name=xxx_yyy)

name = undefined
TOP = ../..
SRC = $(TOP)/src



Setup-NoGraphics:
	sh setup.sh $(name) NoGraphics

Setup-Graphics:
	sh setup.sh $(name) Graphics


Clean:


Pure:
	rm -f $(TOP)/Makedefs
	rm -f $(SRC)/h/define.h
	rm -f $(SRC)/common/rswitch.[csS]
