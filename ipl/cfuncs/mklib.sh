#!/bin/sh
#
#  mklib libname.so obj.o ...

CC=${CC-cc}

LIBNAME=${1?"usage: $0 libname obj..."}
shift

SYS=`uname -s`
set -x
case "$SYS" in
   Linux*|*BSD*|GNU*)
      gcc -shared -o $LIBNAME -fPIC "$@";;
   Darwin*)
      cc -bundle -undefined suppress -flat_namespace -o $LIBNAME "$@";;
   SunOS*)
      $CC $CFLAGS -G -o $LIBNAME "$@" -lc -lsocket;;
   HP-UX*)
      ld -b -o $LIBNAME "$@";;
   IRIX*)
      ld -shared -o $LIBNAME "$@";;
   OSF*)
      ld -shared -expect_unresolved '*' -o $LIBNAME "$@" -lc;;
   AIX*)
      # this may not be quite right; it doesn't seem to work yet...
      ld -bM:SRE -berok -bexpall -bnoentry -bnox -bnogc -brtl -o $LIBNAME "$@";;
   *)
      set -
      echo 1>&2 "don't know how to make libraries under $SYS"
      exit 1;;
esac
