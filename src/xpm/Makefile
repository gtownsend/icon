# Makefile for the C library part of XPM needed by Icon.
# This file is a simplification of XPM's standard Makefile.

include ../../Makedefs

## if your system doesn't provide strcasecmp add -DNEED_STRCASECMP
## if your system doesn't provide pipe remove -DZPIPE

RM = rm -f
AR = ar qc
RANLIB = ranlib
OBJS1 = data.o create.o misc.o rgb.o scan.o parse.o hashtable.o \
	  XpmWrFFrP.o XpmRdFToP.o XpmCrPFData.o XpmCrDataFP.o \
	  XpmWrFFrI.o XpmRdFToI.o XpmCrIFData.o XpmCrDataFI.o

.c.o:
	$(CC) -c $(CFLAGS) $(XPMDEFS) $*.c


libXpm.a: $(OBJS1)
	$(RM) $@
	$(AR) $@ $(OBJS1)
	$(RANLIB) $@ 2>/dev/null || :

$(OBJS1): xpmP.h xpm.h

Clean:
	rm *.o *.a
