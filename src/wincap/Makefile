# Makefile for creating a library of the parts of Wincap used by Icon.

include ../../Makedefs

W32DEFS = -mwin32
OBJS = copy.o dibutil.o errors.o file.o

.c.o:
	$(CC) -c $(CFLAGS) $(W32DEFS) $*.c

libWincap.a: $(OBJS)
	rm -f $@
	ar qc $@ $(OBJS)

copy.o: copy.c dibapi.h dibutil.h errors.h

dibutil.o: dibutil.c dibutil.h

errors.o: errors.c errors.h

file.o: file.c  dibapi.h dibutil.h errors.h

Clean:
	rm -f *.o *.a
