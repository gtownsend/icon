#  Makefile for TIGER mapping programs


IC = icont
IFLAGS = -us
DEST = /unspecified/destination/

PROGS = tgrprep tgrlink tgrmap tgrmerge tgrquant tgrtrack
SCRIPTS = tgrsort tgrstats tgrclean


.SUFFIXES:	.icn
.icn:		; $(IC) $(IFLAGS) $<



default:	$(PROGS)


test:


install:	$(PROGS) $(SCRIPTS)
		cp $(PROGS) $(SCRIPTS) $(DEST)

Iexe:		
		$(MAKE) DEST=../../iexe install


clean Clean:		
		rm -f $(PROGS) *.u[12] *.out*
