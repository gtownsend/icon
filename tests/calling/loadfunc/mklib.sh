#!/bin/sh
#
#  mklib libname.so obj.o ...

CC=${CC-cc}

LIBNAME=${1?"usage: $0 libname obj..."}
shift

SYS=`uname -sr | sed 's/ //'`
case "$SYS" in
   SunOS4*)
      ld -o $LIBNAME "$@";;
   SunOS5*)
      $CC $CFLAGS -G -K pic -o $LIBNAME "$@" -lc -lsocket;;
   IRIX*)
      ld -shared -o $LIBNAME "$@";;
   OSF*)
      ld -shared -expect_unresolved '*' -o $LIBNAME "$@" -lc;;
   Linux*)
      gcc -shared -o $LIBNAME "$@";;
   FreeBSD*)
      ld -Bshareable -o $LIBNAME "$@" -lc;;
   *)
      set -
      echo 1>&2 "don't know how to make libraries under $SYS"
      exit 1;;
esac
