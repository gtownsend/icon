#!/bin/sh
#
#  setup.sh -- invoked by top-level Makefile

USAGE="usage: setup.sh configname [No]Graphics [pthreads]"

NAME=$1
GPX=$2
CSW=$3
TOP=..
SRC=$TOP/src

# check parameters
case "$GPX" in
   Graphics)	XL='-L../../bin -lIgpx $(XLIBS)';;
   NoGraphics)	XL= ;;
   *)		echo "$USAGE" 1>&2; exit 1;;
esac
case "$CSW" in
   custom | "")	;;
   pthreads)	;;
   *)		echo "$USAGE" 1>&2; exit 1;;
esac

# check that configuration exists
if [ ! -d "$NAME" ]; then
   echo "no configuration directory for $NAME" 1>&2 
   exit 1
fi

# find and copy the context switch code.
# use pthreads version if specified, or as a last resort.
# first try `uname -p`.[cs] or `uname -m`.[cs] and then rswitch.[cs].
ARCH=`uname -p 2>/dev/null || echo unknown`
if [ "$ARCH" = "unknown" ]; then
   ARCH=`uname -m`
fi
if [ "$CSW" = "pthreads" ]; then
   RSW=pthreads.c
   COCLEAN="#define CoClean"
elif [ -f "$NAME/$ARCH.c" ]; then
   RSW="$NAME/$ARCH.c"
   COCLEAN=
elif [ -f "$NAME/$ARCH.s" ]; then
   RSW="$NAME/$ARCH.s"
   COCLEAN=
elif [ -f $NAME/rswitch.[cs] ]; then
   RSW=`echo $NAME/rswitch.[cs]`
   COCLEAN=
else
   RSW=pthreads.c
   COCLEAN="#define CoClean"
fi
case $RSW in
   *.c)  DRSW=rswitch.c;;
   *.s)  DRSW=rswitch.s;;
esac
cp $RSW $SRC/common/$DRSW

if [ "$RSW" = "pthreads.c" ]; then
   TL='$(TLIBS)'
else
   TL=
fi

RSN=`echo $RSW | sed 's=.*/=='`

# build the "define.h" file
echo "#define Config \"$NAME, $RSN\""		 > $SRC/h/define.h
echo "#define $GPX 1"				>> $SRC/h/define.h
echo "$COCLEAN"					>> $SRC/h/define.h
echo ""						>> $SRC/h/define.h
cat  $NAME/define.h				>> $SRC/h/define.h

# build the "Makedefs" file
echo "#  from config/$NAME"			 > $TOP/Makedefs
echo ""						>> $TOP/Makedefs
cat $NAME/Makedefs				>> $TOP/Makedefs
echo ""						>> $TOP/Makedefs
echo "RSW = $DRSW"				>> $TOP/Makedefs
echo "TL = $TL"					>> $TOP/Makedefs
echo ""						>> $TOP/Makedefs
echo "#  $GPX"					>> $TOP/Makedefs
echo "XL = $XL"					>> $TOP/Makedefs

# report actions
echo "   configured $NAME"
echo "   with $GPX"
echo "   using $RSW"

# run customization script, if one exists
if [ -f $NAME/custom.sh ]; then
   cd $NAME
   sh custom.sh
fi
