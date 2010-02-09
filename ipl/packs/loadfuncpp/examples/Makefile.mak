
ifndef TARGET
ifneq ($(strip $(shell g++ -v 2>&1 | grep "cygwin")),)
TARGET=cygwin
else
TARGET=other
endif
endif

FLAGS_cygwin = /opt/icon/bin/iload.a -Wl,--enable-auto-import
FLAGS_other = 

PIC_other = -fPIC

EXAMPLES = #exe#
DYNAMICS = #so#

%.so : %.cpp loadfuncpp.h loadfuncpp.u1
	g++ -shared $(PIC_$(TARGET)) -o $@ $< $(FLAGS_$(TARGET))

%.exe : %.icn %.so iload.so
	icont -so $@ $*

default: $(DYNAMICS) $(EXAMPLES)

.PHONY : iload.so loadfuncpp.h loadfuncpp.u1

loadfuncpp.h : ../loadfuncpp.h
	cp ../loadfuncpp.h ./

test : clean default 

clean :
	rm -f *.exe *.so *.o *% *~ core .#*
