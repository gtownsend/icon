/*
############################################################################
#
#	File:     internal.c
#
#	Subject:  Functions to access Icon internals
#
#	Author:   Gregg M. Townsend
#
#	Date:     October 3, 1995
#
############################################################################
#
#   This file is in the public domain.
#
############################################################################
#
#  These functions provide some access to the internal machinery of the
#  Icon interpreter.  Some knowledge of the interpreter is needed to use
#  these profitably; misuse can lead to memory violations.
#
#  dword(x)		return d-word of descriptor
#  vword(x)		return v-word of descriptor
#  descriptor(d,v)	construct descriptor from d-word and v-word
#  peek(addr,n)		return contents of memory as n-character string
#			(if n is omitted, return Icon integer at addr)
#  spy(addr,n)		return string pointer to memory, without copying
#
############################################################################
#
#  Requires:  Dynamic loading
#
############################################################################
*/

#include "icall.h"

int dword(int argc, descriptor argv[])		/*: return descriptor d-word */
   {
   if (argc == 0)
      Fail;
   else
      RetInteger(argv[1].dword);
   }

int vword(int argc, descriptor argv[])		/*: return descriptor v-word */
   {
   if (argc == 0)
      Fail;
   else
      RetInteger(argv[1].vword);
   }

int icon_descriptor(int argc, descriptor argv[])  /*: construct descriptor */
   {
   ArgInteger(1);
   ArgInteger(2);
   argv[0].dword = argv[1].vword;
   argv[0].vword = argv[2].vword;
   Return;
   }

int peek(int argc, descriptor argv[])		/*: load value from memory */
   {
   ArgInteger(1);
   if (argc > 1) {
      ArgInteger(2);
      RetStringN((void *)IntegerVal(argv[1]), IntegerVal(argv[2]));
      }
   else
      RetInteger(*(word *)IntegerVal(argv[1]));
   }

int spy(int argc, descriptor argv[])		/*: create spy-port to memory */
   {
   ArgInteger(1);
   ArgInteger(2);
   RetConstStringN((void *)IntegerVal(argv[1]), IntegerVal(argv[2]));
   }
