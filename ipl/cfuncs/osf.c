/*
############################################################################
#
#	File:     osf.c
#
#	Subject:  Function to return OSF system table value
#
#	Author:   Gregg M. Townsend
#
#	Date:     November 17, 1997
#
############################################################################
#
#   This file is in the public domain.
#
############################################################################
#
#  osftable(id, index, len) returns one element from an OSF table() call.
#  This function is for the OSF operating system, and fails on other systems.
#
#  See "man table" for a detailed description of the "table" system call
#  and the formats of the structures returned; see /usr/include/table.h
#  for a list of allowed ID values.
#
#  Defaults: index    0
#            len    100
#
############################################################################
#
#  Requires:  OSF or Digital UNIX, dynamic loading
#
############################################################################
*/

#include "icall.h"
#include <stdlib.h>

#define DEFLENGTH 100

#ifndef __osf__
int osftable (int argc, descriptor argv[])  { Fail; }
#else

int osftable (int argc, descriptor argv[])	/*: query OSF system table */
   {
   int id, index, len;
   static void *buf;
   static int bufsize;

   if (argc == 0)
      Error(101);
   ArgInteger(1);
   id = IntegerVal(argv[1]);

   if (argc > 1)  {
      ArgInteger(2);
      index = IntegerVal(argv[2]);
      }
   else
      index = 0;

   if (argc > 2)  {
      ArgInteger(3);
      len = IntegerVal(argv[3]);
      }
   else
      len = DEFLENGTH;

   if (len > bufsize) {
      buf = realloc(buf, bufsize = len);
      if (len > 0 && !buf)
         Error(305);
      }

   if ((id = table(id, index, buf, 1, len)) != 1)
      Fail;
   RetStringN(buf, len);
   }

#endif
