#  Makefile for FAQ/doc installation and webpage validation
#  <<ARIZONA-ONLY>>
#
#  This Makefile is for Icon maintenance and FAQ installation at Arizona.
#  Because of that it is targeted for the Icon development environment
#  (Mac OS X plus some specialized tools) and is less portable than
#  other Makefiles.

MADE = icon.txt icont.txt

SERVER = lectura.cs.arizona.edu
PUBDIR = /cs/www/icon/uguide


#  (default:) rebuild icon.txt, icont.txt, and faq.txt from nroff/html sources.

default:  $(MADE)

icon.txt: ../man/man1/icon.1 clnroff.sed
	nroff -Tascii -man ../man/man1/icon.1 | \
	    sed -f clnroff.sed | uniq >icon.txt

icont.txt: ../man/man1/icont.1 clnroff.sed
	nroff -Tascii -man ../man/man1/icont.1 | \
	    sed -f clnroff.sed | uniq >icont.txt


#  install updated files on the Icon website.

publish: $(MADE)
	scp *.css *.gif *.jpg *.txt *.htm $(SERVER):$(PUBDIR)


#  remove built files to force a rebuild.
#  do NOT do this before preparing an Icon distribution.

Clean clean:
	rm -f $(MADE)
