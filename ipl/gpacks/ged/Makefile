ICONT=icont
IFLAGS=-us

ged:	ged.icn control.icn textedit.icn
	$(ICONT) $(IFLAGS) ged control textedit

Iexe:	ged
	cp ged ../../iexe/

Clean:
	rm -f ged *.u?
