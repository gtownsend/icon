#  Makefile for the Icon run-time translator, rtt,
#  which is used to build the Icon run-time system.

include ../../Makedefs


ROBJS = rttparse.o rttmain.o rttlex.o rttsym.o rttnode.o rttout.o rttmisc.o\
	  rttdb.o rttinlin.o rttilc.o

PP_DIR = ../preproc/
P_DOT_H = $(PP_DIR)preproc.h $(PP_DIR)pproto.h ltoken.h ../h/mproto.h\
        ../h/define.h ../h/config.h ../h/typedefs.h\
        ../h/cstructs.h ../h/cpuconf.h
POBJS = pout.o pchars.o  perr.o pmem.o  bldtok.o macro.o preproc.o\
	evaluate.o files.o gettok.o pinit.o

COBJS = ../common/getopt.o ../common/time.o ../common/filepart.o \
	  ../common/identify.o ../common/strtbl.o ../common/alloc.o \
	  ../common/rtdb.o ../common/munix.o ../common/literals.o

OBJ = $(ROBJS) $(POBJS) $(COBJS)


rtt:	$(OBJ)
	$(CC) $(LDFLAGS) -o rtt $(OBJ)
	cp rtt ../../bin
	strip ../../bin/rtt$(EXE)

library:	$(OBJ)
		rm -rf rtt.a
		ar qc rtt.a $(OBJ)

$(COBJS):
	cd ../common; $(MAKE)

$(ROBJS): rtt.h rtt1.h rttproto.h $(P_DOT_H)

rttdb.o: ../h/version.h
rttparse.o : ../h/gsupport.h ../h/config.h ../h/cstructs.h \
	../h/mproto.h ../h/typedefs.h ../h/cpuconf.h ../h/define.h

pout.o: $(PP_DIR)pout.c $(P_DOT_H)
	$(CC) -c $(CFLAGS) $(PP_DIR)pout.c 

pchars.o: $(PP_DIR)pchars.c $(P_DOT_H)
	$(CC) -c $(CFLAGS) $(PP_DIR)pchars.c 

perr.o: $(PP_DIR)perr.c $(P_DOT_H)
	$(CC) -c $(CFLAGS) $(PP_DIR)perr.c

pmem.o: $(PP_DIR)pmem.c $(P_DOT_H)
	$(CC) -c $(CFLAGS) $(PP_DIR)pmem.c

bldtok.o: $(PP_DIR)bldtok.c $(P_DOT_H)
	$(CC) -c $(CFLAGS) $(PP_DIR)bldtok.c

macro.o: $(PP_DIR)macro.c $(P_DOT_H)
	$(CC) -c $(CFLAGS) $(PP_DIR)macro.c

preproc.o: $(PP_DIR)preproc.c $(P_DOT_H)
	$(CC) -c $(CFLAGS) $(PP_DIR)preproc.c

evaluate.o: $(PP_DIR)evaluate.c $(P_DOT_H)
	$(CC) -c $(CFLAGS) $(PP_DIR)evaluate.c

files.o: $(PP_DIR)files.c $(P_DOT_H)
	$(CC) -c $(CFLAGS) $(PP_DIR)files.c

gettok.o: $(PP_DIR)gettok.c $(P_DOT_H)
	$(CC) -c $(CFLAGS) $(PP_DIR)gettok.c

pinit.o: $(PP_DIR)pinit.c $(P_DOT_H)
	$(CC) -c $(CFLAGS) $(PP_DIR)pinit.c

#
# The following entry is commented out because it is not normally
# necessary to recreate rttparse.c and ltoken.h unless the grammar
# in rttgram.y for the run-time langauge is changed. Recreating these
# files is not normally a part of the installation process. Note that
# on some systems, yacc may not have large enough internal tables to
# translate this grammar.
#
#rttparse.c ltoken.h: rttgram.y
#	yacc -d rttgram.y
#	fgrep -v -x "extern char *malloc(), *realloc();" y.tab.c > rttparse.c
#	rm y.tab.c
#	mv y.tab.h ltoken.h
