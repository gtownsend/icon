#  Makefile for FAQ/doc installation and webpage validation
#  <<ARIZONA-ONLY>>
#
#  This Makefile is for Icon maintenance and FAQ installation at Arizona.
#  Because of that it is targeted for the Icon development environment
#  (Red Hat Linux and some specialized tools) and is less portable than
#  other Makefiles.


MADE = icon.txt icont.txt faq.txt

HFAQ = /cs/www/icon/faq.htm
TFAQ = /cs/www/icon/ftp/doc/faq.txt
FFAQ = /cs/ftp/icon/doc/faq.txt

RLSDIR = /cs/www/icon/v940


#  (default:) rebuild icon.txt, icont.txt, and faq.txt from nroff/html sources.

default:  $(MADE)

icon.txt: ../man/man1/icon.1 clnroff.sed
	nroff -Tascii -man ../man/man1/icon.1 | \
	    sed -f clnroff.sed | uniq >icon.txt

icont.txt: ../man/man1/icont.1 clnroff.sed
	nroff -Tascii -man ../man/man1/icont.1 | \
	    sed -f clnroff.sed | uniq >icont.txt

faq.txt:  faq.htm
	lynx -nolist -dump -justify=off faq.htm >faq.txt


#  check integrity of HTML files.
#  n.b. weblinks generates some false negatives.

lint:
	weblint -x netscape *.htm
	weblinks -t *.htm


#  compare FAQ with currently installed version.

diff:
	diff -c $(HFAQ) faq.htm


#  install updated files on Icon web and FTP sites.

install:  faq.txt
	cp faq.htm $(HFAQ)
	cp faq.txt $(TFAQ)
	cp -p $(TFAQ) $(FFAQ)
	: OTHER REVISED FILES NOT INSTALLED ON WEBSITE:
	: cp *.css *.gif *.jpg *.txt $(RLSDIR)
	: cp `ls *.htm | grep -v index` $(RLSDIR)


#  remove built files to force a rebuild.
#  do NOT do this before preparing an Icon distribution.

Clean clean:
	rm -f $(MADE)
