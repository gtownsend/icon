ICONT=icont
IFLAGS=-us

SRC = skeem.icn skbasic.icn skcontrl.icn skdebug.icn skextra.icn skfun.icn \
      skin.icn skio.icn sklist.icn skmisc.icn sknumber.icn skout.icn \
      skstring.icn skuser.icn skutil.icn llist.icn

skeem:	$(SRC)
	$(ICONT) $(IFLAGS) $(SRC)


Iexe:	skeem
	cp skeem ../../iexe/

Clean:	
	rm -f skeem *.u?
