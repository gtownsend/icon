#!/bin/sh
#
#  mklib libname.so obj.o ...

CC=${CC-cc}
BIN=${BIN-../../bin}

LIBNAME=${1?"usage: $0 libname obj..."}
shift

SYS=`uname -s`
set -x
case "$SYS" in
   Linux*|*BSD*|GNU*)
      $CC $CFLAGS $LDFLAGS -shared -fPIC -o $LIBNAME "$@";;
   CYGWIN*)
      # move the win32 import library for iconx.exe callbacks
      # created when iconx.exe was built
      if [ -e $BIN/../src/runtime/iconx.a ]; then
         mv $BIN/../src/runtime/iconx.a $BIN
      fi
      $CC $CFLAGS $LDFLAGS -shared -Wl,--enable-auto-import -o $LIBNAME "$@" $BIN/iconx.a;;
   Darwin*)
      $CC $CFLAGS $LDFLAGS -bundle -undefined suppress -flat_namespace -o $LIBNAME "$@";;
   SunOS*)
      $CC $CFLAGS $LDFLAGS -G -o $LIBNAME "$@" -lc -lsocket;;
   HP-UX*)
      ld $LDFLAGS -b -o $LIBNAME "$@";;
   IRIX*)
      ld $LDFLAGS -shared -o $LIBNAME "$@";;
   OSF*)
      ld $LDFLAGS -shared -expect_unresolved '*' -o $LIBNAME "$@" -lc;;
   AIX*)
      # this may not be quite right; it doesn't seem to work yet...
      ld $LDFLAGS -bM:SRE -berok -bexpall -bnoentry -bnox -bnogc -brtl -o $LIBNAME "$@";;
   *)
      set -
      echo 1>&2 "don't know how to make libraries under $SYS"
      exit 1;;
esac
