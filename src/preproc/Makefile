include ../../Makedefs

POBJS = pout.o pchars.o perr.o pmem.o bldtok.o macro.o preproc.o evaluate.o\
	files.o gettok.o pinit.o

COBJS= ../common/getopt.o ../common/time.o ../common/strtbl.o ../common/alloc.o

ICOBJS=	getopt.o time.o strtbl.o alloc.o

OBJS= $(POBJS) $(COBJS)

DOT_H = preproc.h pproto.h ptoken.h ../h/define.h ../h/config.h\
        ../h/typedefs.h ../h/mproto.h

common:
	cd ../common; $(MAKE) $(ICOBJS)
	$(MAKE) pp

pp: pmain.o $(OBJS)
	$(CC) -o pp pmain.o $(OBJS)

pmain.o:	$(DOT_H)
p_out.o:	$(DOT_H)
pchars.o:	$(DOT_H)
p_err.o:	$(DOT_H)
pmem.o:		$(DOT_H)
pstring.o:	$(DOT_H)
bldtok.o:	$(DOT_H)
macro.o:	$(DOT_H)
preproc.o:	$(DOT_H)
evaluate.o:	$(DOT_H)
files.o:	$(DOT_H)
gettok.o:	$(DOT_H)
p_init.o:	$(DOT_H)
