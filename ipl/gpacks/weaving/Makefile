#  The programs listed in this Makefile (there are more in the directory)
#  are known to build cleanly.

PROGS = randweav showrav unravel heddle \
   mtrxedit

IC = icont

.SUFFIXES: .icn

.icn:	; $(IC) -us $<



all:	$(PROGS)

clean:	
	rm -f *.u? `find . -type f -perm -100`
