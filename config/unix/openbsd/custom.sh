#!/bin/sh
#
#  custom setup script for openbsd

SRC=../../../src
ARCH=`uname -m`

if [ -f rswitch.$ARCH ]; then
	cp rswitch.$ARCH $SRC/common/rswitch.c
else
	echo "#define NoCoexpr" >>$SRC/h/define.h
fi
