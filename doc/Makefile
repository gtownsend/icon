#  Makefile for FAQ/doc installation and webpage validation
#  <<ARIZONA-ONLY>>


MADE = icont.txt faq.txt

HFAQ = /cs/www/icon/faq.htm
TFAQ = /cs/www/icon/ftp/doc/faq.txt
FFAQ = /cs/ftp/icon/doc/faq.txt

RLSDIR = /cs/www/icon/v940

MANPAGE = ../man/man1/icont.1


default:  $(MADE)

icont.txt: $(MANPAGE) clnroff.sed
	nroff -man $(MANPAGE) | sed -f clnroff.sed | uniq >icont.txt

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
	cp *.css *.gif *.jpg *.txt $(RLSDIR)
	cp `ls *.htm | grep -v index` $(RLSDIR)


Clean clean:
	rm -f $(MADE)
