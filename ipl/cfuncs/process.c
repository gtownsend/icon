/*
############################################################################
#
#	File:     process.c
#
#	Subject:  Functions to manipulate UNIX processes
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
#  kill(pid, signal)	kill process (defaults: pid=0, signal=SIGTERM)
#  getpid()		return process ID
#  getuid()		return user ID
#  getgid()		return group ID
#
############################################################################
#
#  Requires:  UNIX, dynamic loading
#
############################################################################
*/

#include <signal.h>
#include <sys/types.h>
#include <unistd.h>

#include "icall.h"

int icon_kill (int argc, descriptor argv[])	/*: kill process */
   {
   int pid, sig;

   if (argc > 0)  {
      ArgInteger(1);
      pid = IntegerVal(argv[1]);
      }
   else
      pid = 0;

   if (argc > 1)  {
      ArgInteger(2);
      sig = IntegerVal(argv[2]);
      }
   else
      sig = SIGTERM;

   if (kill(pid, sig) == 0)
      RetNull();
   else
      Fail;
   }

int icon_getpid (int argc, descriptor argv[])	/*: query process ID */
   {
   RetInteger(getpid());
   }

int icon_getuid (int argc, descriptor argv[])	/*: query user ID */
   {
   RetInteger(getuid());
   }

int icon_getgid (int argc, descriptor argv[])	/*: query group ID */
   {
   RetInteger(getgid());
   }
