/*
############################################################################
#
#	File:     files.c
#
#	Subject:  Functions to manipulate file attributes
#
#	Author:   Gregg M. Townsend
#
#	Date:     November 17, 2004
#
############################################################################
#
#   This file is in the public domain.
#
############################################################################
#
#  chmod(filename, mode) changes the file permission modes of a file to
#  those specified.
#
#  umask(mask) sets the process "umask" to the specified value.
#  If mask is omitted, the current process mask is returned.
#
############################################################################
#
#  Requires:  UNIX, dynamic loading
#
############################################################################
*/

#include "icall.h"

#include <sys/types.h>
#include <sys/stat.h>

int icon_chmod (int argc, descriptor argv[]) /*: change UNIX file permissions */
   {
   ArgString(1);
   ArgInteger(2);
   if (chmod(StringVal(argv[1]), IntegerVal(argv[2])) == 0)
      RetNull();
   else
      Fail;
   }

int icon_umask (int argc, descriptor argv[]) /*: change UNIX permission mask */
   {
   int n;

   if (argc == 0) {
      umask(n = umask(0));
      RetInteger(n);
      }
   ArgInteger(1);
   umask(IntegerVal(argv[1]));
   RetArg(1);
   }
