#!/bin/sh
#
#  custom setup script for openbsd

SRC=../../../src
ARCH=`uname -m`

if [ -f rswitch.$ARCH ]; then
	cp -f rswitch.$ARCH $SRC/common/rswitch.c
else
	echo "#define NoCoexpr" >>$SRC/h/define.h
fi

if [ -f define.$ARCH ]; then
        cat define.$ARCH >>$SRC/h/define.h
fi
