/*
############################################################################
#
#	File:     fpoll.c
#
#	Subject:  Function to poll file for input
#
#	Author:   Gregg M. Townsend
#
#	Date:     November 27, 2001
#
############################################################################
#
#   This file is in the public domain.
#
############################################################################
#
#  fpoll(f, msec) waits until data is available for input from file f,
#  and then returns.  It also returns when end-of-file is reached.
#  If msec is specified, and no data is available after waiting that
#  many milliseconds, then fpoll fails.  If msec is omitted, fpoll
#  waits indefinitely.
#
############################################################################
#
#  Requires:  UNIX, dynamic loading
#
############################################################################
*/

#include <stdio.h>
#include <sys/types.h>
#include <sys/time.h>

#include "icall.h"

int fpoll(int argc, descriptor *argv)	/*: await data from file */
   {
   FILE *f;
   int msec, r;
   fd_set fds;
   struct timeval tv, *tvp;

   /* check arguments */
   if (argc < 1)
      Error(105);
   if ((IconType(argv[1]) != 'f') || (FileStat(argv[1]) & Fs_Window))
      ArgError(1, 105);
   if (!(FileStat(argv[1]) & Fs_Read))
      ArgError(1, 212);
   f = FileVal(argv[1]);

   if (argc < 2)
      msec = -1;
   else {
      ArgInteger(2);
      msec = IntegerVal(argv[2]);
      }

   /* check for data already in buffer */
   /* there's no legal way to do this in C; we cheat */
#if defined(__GLIBC__) && defined(_STDIO_USES_IOSTREAM)	/* new GCC library */
   if (f->_IO_read_ptr < f->_IO_read_end)
      RetArg(1);
#elif defined(__GLIBC__)				/* old GCC library */
   if (f->__bufp < f->__get_limit)
      RetArg(1);
#elif defined(_FSTDIO)					/* new BSD library */
   if (f->_r > 0)
      RetArg(1);
#else							/* old AT&T library */
   if (f->_cnt > 0)
      RetArg(1);
#endif

   /* set up select(2) structure */
   FD_ZERO(&fds);			/* clear file bits */
   FD_SET(fileno(f), &fds);		/* set bit of interest */

   /* set up timeout and pointer */
   if (msec < 0)
      tvp = NULL;
   else {
      tv.tv_sec = msec / 1000;
      tv.tv_usec = (msec % 1000) * 1000;
      tvp = &tv;
      }

   /* poll the file using select(2) */
   r = select(fileno(f) + 1, &fds, (fd_set*)NULL, (fd_set*)NULL, tvp);

   if (r > 0)
      RetArg(1);			/* success */
   else if (r == 0)			
      Fail;				/* timeout */
   else
      ArgError(1, 214);			/* I/O error */

}
