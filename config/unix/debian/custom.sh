#!/bin/sh
#
#  Custom setup script for Linux

SRC=../../../src
ARCH=`dpkg --print-architecture||uname -m`

if [ -f rswitch.$ARCH.s ]; then
	cp -f rswitch.$ARCH.s $SRC/common/rswitch.s
elif [ -f rswitch.$ARCH.c ]; then
	cp -f rswitch.$ARCH.c $SRC/common/rswitch.c
else
	echo "#define NoCoexpr" >>$SRC/h/define.h
fi

if [ -f define.$ARCH ]; then
        cat define.$ARCH >>$SRC/h/define.h
fi
