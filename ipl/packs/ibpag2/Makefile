##########################################################################
#
   PROGNAME = ibpag2
#
##########################################################################
#
#  User-modifiable section.  Read carefully!  You will almost
#  certainly have to change some settings here.
#

#
# Destination directory for binaries files.  Owner and group for
# public executables.  Leave the trailing slash off of directory
# names.
#
OWNER = richard # root
GROUP = group   # root
DESTDIR = /usr/local/bin
# Put this path into your LPATH variable (on which, see the Icon
# documentation).  Make sure that the directory exists.
LIBDIR = /usr/local/lib/icon/data

#
# Name of your icon compiler and compiler flags.
#
ICONC = icont
IFLAGS = -u -s #-Sc 400 -Sg 400 -Si 2000 -Sn 4000 -SF 40

SHAR = /usr/local/bin/shar
COMPRESS = /usr/bin/compress
# COMPRESS = /usr/local/bin/gzip

###########################################################################
#
#  Don't change anything below this line unless you're really sure of
#  what you're doing.
#

AUX = slshupto.icn rewrap.icn outbits.icn sortff.icn itokens.icn
SRC = $(PROGNAME).icn $(AUX) slrtbls.icn slritems.icn follow.icn \
	ibutil.icn iohno.icn ibreader.icn ibwriter.icn shrnktbl.icn \
	version.icn
PARSER = iiparse.lib
GLRPARSER = iiglrpar.lib
SHARFILES = $(SRC) $(PARSER) $(GLRPARSER) sample.ibp beta2ref.ibp \
	iacc.ibp Makefile.dist README

all: $(PROGNAME)

$(PROGNAME): $(SRC)
	$(ICONC) $(IFLAGS) -o $(PROGNAME) $(SRC)


##########################################################################
#
#  Pseudo-target names (shar, install, clean, clobber)
#

#
# Assumes you have a shar program like mine.
#
shar: $(SHARFILES)
	@echo ""
	@echo "Removing any old shars in this directory."
	@echo ""
	-rm -f $(PROGNAME).[0-9][0-9].Z
	@echo ""
	$(SHAR) -fVc -o$(PROGNAME) -L32 $(SHARFILES)
	$(COMPRESS) -f $(PROGNAME).[0-9][0-9]
	@echo ""
	@echo "Shell archive finished."
	@echo ""

# Pessimistic assumptions regarding the environment (in particular,
# I don't assume you have the BSD "install" shell script).
install: all
	@echo ""
	-test -d $(DESTDIR) || mkdir $(DESTDIR) && chmod 755 $(DESTDIR)
	cp $(PROGNAME) $(DESTDIR)/$(PROGNAME)
	-chgrp $(GROUP) $(DESTDIR)/$(PROGNAME)
	-chown $(OWNER) $(DESTDIR)/$(PROGNAME)
	-chmod 755 $(DESTDIR)/$(PROGNAME)
	-test -d $(LIBDIR) || mkdir $(LIBDIR) && chmod 755 $(LIBDIR)
	cp $(PARSER) $(LIBDIR)/$(PARSER)
	cp $(GLRPARSER) $(LIBDIR)/$(GLRPARSER)
	-chgrp $(GROUP) $(LIBDIR)/$(PARSER)
	-chown $(OWNER) $(LIBDIR)/$(PARSER)
	-chgrp $(GROUP) $(LIBDIR)/$(GLRPARSER)
	-chown $(OWNER) $(LIBDIR)/$(GLRPARSER)
	-chmod 644 $(LIBDIR)/$(PARSER)
	-chmod 644 $(LIBDIR)/$(GLRPARSER)
	@echo ""
	@echo "Done installing."
	@echo ""

#  Build executable and copy to ../../iexe.
#  Nothing done in this case because the executable doesn't stand alone.
Iexe:	


#
# Cleanup
#
clean:
	-rm -f *~ #*# core *.u[12] $(PROGNAME).output
Clean clobber: clean
	-rm -f $(PROGNAME)
