/*
############################################################################
#
#	File:     tconnect.c
#
#	Subject:  Function to open TCP connection
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
#  tconnect(hostname, portnum) establishes a TCP connection to the given
#  host and port, returning an Icon file f.
#
#  Note that seek(f) must be called when switching between input and output
#  on this bidirectional file.  Additionally, the DEC Alpha requires a call
#  to flush(f), after the seek, when switching from input to output.
#
############################################################################
#
#  See also:  fpoll.c
#
############################################################################
#
#  Requires:  Unix, dynamic loading
#
############################################################################
*/

#include <string.h>
#include <stdio.h>

#include <fcntl.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include "icall.h"


int tconnect(int argc, descriptor *argv)	/*: connect to TCP socket */
   {
   char *hostname, filename[1000];
   unsigned char *p;
   int port, fd, i, d[4];
   FILE *fp;
   struct hostent *h;
   struct sockaddr_in sin;

   memset(&sin, 0, sizeof(sin));

   /* check arguments */
   ArgString(1);
   hostname = StringVal(argv[1]);

   ArgInteger(2);
   port = IntegerVal(argv[2]);

   /* get host address */
   if (sscanf(hostname, "%d.%d.%d.%d", &d[0], &d[1], &d[2], &d[3]) == 4) {
      p = (unsigned char *) &sin.sin_addr;
      for (i = 0; i < 4; i++)
         p[i] = d[i];
      }
   else {
      h = gethostbyname(hostname);
      if (!h)
         Fail;
      memcpy(&sin.sin_addr, h->h_addr, sizeof(struct in_addr));
      endhostent();
      }

   /* create socket and connect */
   sin.sin_family = AF_INET;
   sin.sin_port = htons(port);
   if ((fd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
      Fail;
   if (connect(fd, (struct sockaddr *) &sin, sizeof(sin)) < 0)
      Fail;

   /* create stdio file pointer */
   fp = fdopen(fd, "r+");
   if (!fp)
      Fail;

   /* return Icon file */
   sprintf(filename, "%s:%d", hostname, port);
   RetFile(fp, Fs_Read | Fs_Write, filename);
   }
