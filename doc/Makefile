#  Makefile for FAQ installation and webpage validation
#  <<ARIZONA-ONLY>>


MADE = icont.txt faq.txt

HFAQ = /cs/www/icon/faq.htm
TFAQ = /cs/www/icon/ftp/doc/faq.txt
FFAQ = /cs/ftp/icon/doc/faq.txt


default:  $(MADE)

icont.txt: icont.1 clnroff.sed
	nroff -man icont.1 | sed -f clnroff.sed | uniq >icont.txt

faq.txt:  faq.htm
	lynx -nolist -dump faq.htm >faq.txt

lint:
	weblint -x netscape *.htm
	weblinks -t *.htm

diff:
	diff -c $(HFAQ) faq.htm
	
install:  faq.txt
	cp faq.htm $(HFAQ)
	cp faq.txt $(TFAQ)
	cp -p $(TFAQ) $(FFAQ)


Clean clean:
	rm -f $(MADE)
