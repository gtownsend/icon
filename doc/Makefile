#  Makefile for FAQ installation
#  <<ARIZONA-ONLY>>


HFAQ = /cs/www/icon/faq.htm
TFAQ = /cs/www/icon/ftp/doc/faq.txt
FFAQ = /cs/ftp/icon/doc/faq.txt


default:  faq.txt

faq.txt:  faq.htm
	lynx -nolist -dump faq.htm >faq.txt

diff:
	diff -c $(HFAQ) faq.htm
	
install:  faq.txt
	cp faq.htm $(HFAQ)
	cp faq.txt $(TFAQ)
	cp -p $(TFAQ) $(FFAQ)


Clean clean:
	rm -f faq.txt
