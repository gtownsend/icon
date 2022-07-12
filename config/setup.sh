#!/bin/sh
#
#  setup.sh -- invoked by top-level Makefile

USAGE="usage: setup.sh configname [No]Graphics"

NAME=$1
GPX=$2
TOP=..
SRC=$TOP/src

# check parameters
case "$GPX" in
   Graphics)	XL='$(XLIBS)';;
   NoGraphics)	XL= ;;
   *)		echo "$USAGE" 1>&2; exit 1;;
esac

# check that configuration exists
if [ ! -d "$NAME" ]; then
   echo "no configuration directory for $NAME" 1>&2 
   exit 1
fi

# build the "define.h" file
echo "#define Config \"$NAME\""			 > $SRC/h/define.h
echo "#define $GPX 1"				>> $SRC/h/define.h
echo ""						>> $SRC/h/define.h
cat  $NAME/define.h				>> $SRC/h/define.h

# build the "Makedefs" file
echo "#  from config/$NAME"			 > $TOP/Makedefs
echo ""						>> $TOP/Makedefs
cat $NAME/Makedefs				>> $TOP/Makedefs
echo ""						>> $TOP/Makedefs
echo "#  $GPX"					>> $TOP/Makedefs
echo "XL = $XL"					>> $TOP/Makedefs

# report actions
echo "   configured $NAME"
echo "   with $GPX"

# run customization script, if one exists
if [ -f $NAME/custom.sh ]; then
   cd $NAME
   sh custom.sh
fi
