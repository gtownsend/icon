#  Makefile for vib, the Visual Interface Builder

ICONT = icont
IFLAGS = -us
ITRAN = $(ICONT) $(IFLAGS)

OBJ = vib.u2 vibbttn.u2 vibedit.u2 vibfile.u2 vibglbl.u2 \
	viblabel.u2 vibline.u2 viblist.u2 vibmenu.u2 vibradio.u2 \
	vibrect.u2 vibsizer.u2 vibslidr.u2 vibtalk.u2 vibtext.u2

.SUFFIXES: .icn .u2 .gif .ps

.icn.u2:	; $(ITRAN) -c $<
.icn:		; $(ITRAN) $<

.gif.ps:
	giftoppm $< | ppmtopgm | pnmtops -scale .75 >$@

vib:	$(OBJ)
	$(ITRAN) -o vib $(OBJ)

$(OBJ):	vibdefn.icn


ipd doc: ipd265.ps

ipd265.ps:	ipd265.bibl fig1.ps fig2.ps
	bib -t stdn -p /r/che/usr/ralph/docs/reg.index <ipd265.bibl | \
	    psfig | psroff -t >ipd265.ps

Iexe:	vib
	cp vib ../../iexe/

clean Clean:
	rm -f vib *.ps *.u[12] app vibpro* core busy dlog
