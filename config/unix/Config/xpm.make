#
# Simple makefile for the C library part of XPM needed by Icon
#
# This file is a simplification of XPM's standard Makefile
#

SHELL = /bin/sh
MAKE = make
RM = rm -f

## if your system doesn't provide strcasecmp add -DNEED_STRCASECMP
## if your system doesn't provide pipe remove -DZPIPE
AR = ar qc
OBJS1 = data.o create.o misc.o rgb.o scan.o parse.o hashtable.o \
	  XpmWrFFrP.o XpmRdFToP.o XpmCrPFData.o XpmCrDataFP.o \
	  XpmWrFFrI.o XpmRdFToI.o XpmCrIFData.o XpmCrDataFI.o

.c.o:
	$(CC) -c $(CFLAGS) $(DEFINES) $*.c


libXpm.a: $(OBJS1)
	$(RM) $@
	$(AR) $@ $(OBJS1)

$(OBJS1): xpmP.h xpm.h

Clean:
	rm *.o *.a
