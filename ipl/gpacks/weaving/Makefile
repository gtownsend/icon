#  The programs listed in this Makefile (there are more in the
#  directory) are those that are not labeled AD HOC that have
#  been verified to build cleanly.


PROCS = cells.u2 tdialog.u2 tieutils.u2 tpath.u2 \
   weavegif.u2 weavutil.u2 wifcvt.u2

PROGS = comb draw2gmr drawdown drawup gif2geom gif2html heddle lindpath \
   mtrxedit pfd2gif pfd2gmr pfd2ill pfd2wif plexity randweav \
   seqdraft shadow shadpapr showrav tieimage unravel wallpapr weaver wif2pfd


IC = icont
IFLAGS = -us

.SUFFIXES: 	.icn .u2
.icn.u2:        ; $(IC) $(IFLAGS) -c $<
.icn:		; $(IC) $(IFLAGS) $<


all:		$(PROGS)

$(PROGS): 	$(PROCS)

Iexe:		$(PROGS)
		cp $(PROGS) ../../iexe/

clean Clean:
		rm -f $(PROGS) *.u?
