CC=cc
CFLAGS= -DVarTran
LDFLAGS=
SHELL=/bin/sh
MAKE=make

SRC=../../../src

all:
		@echo "what do you want to make"

Setup:
		make -f ../Config/Makefile Clean
		make -f ../Config/Makefile Localcode
		make -f ../Config/Makefile Makefiles

Setup-X-Icon: Setup
		echo "#define Graphics 1"	       >>$(SRC)/h/define.h
		-if [ -f xiconc.def ]; \
		    then cat xiconc.def		>>$(SRC)/h/define.h; fi
		echo "XPMLIB=-L../../bin -lXpm"		>$(SRC)/runtime/Makefile
		-if [ -f xiconx.mak ]; \
		    then cat xiconx.mak		>>$(SRC)/runtime/Makefile; \
		    else echo "XLIB= -lX11"	>>$(SRC)/runtime/Makefile; fi
		echo "XPM = xpm"		       >>$(SRC)/runtime/Makefile
		cat runtime.hdr ../Config/runtime.make >>$(SRC)/runtime/Makefile
		cat xpm.hdr ../Config/xpm.make		>$(SRC)/xpm/Makefile
		echo "XPM = xpm"			>$(SRC)/iconc/Makefile
		cat iconc.hdr ../Config/iconc.make     >>$(SRC)/iconc/Makefile

Clean:
		rm -f $(SRC)/common/rswitch.[csS]

Makefiles:
		cat rtt.hdr	../Config/rtt.make	>$(SRC)/rtt/Makefile
		cat common.hdr	../Config/common.make	>$(SRC)/common/Makefile
		cat iconc.hdr	../Config/iconc.make	>$(SRC)/iconc/Makefile
		cat preproc.hdr	../Config/preproc.make	>$(SRC)/preproc/Makefile
		cat icont.hdr	../Config/icont.make	>$(SRC)/icont/Makefile
		cat runtime.hdr	../Config/runtime.make	>$(SRC)/runtime/Makefile

Localcode:
		cp define.h $(SRC)/h
		cp rswitch.[csS] $(SRC)/common
		-if grep -s NoRanlib define.h; \
		   then touch ../../../NoRanlib; \
		   else rm -rf ../../../NoRanlib; fi
