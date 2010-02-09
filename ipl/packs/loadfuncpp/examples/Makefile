
#Automatically generated from Makefile.mak and examples.txt by ../savex.icn

ifndef TARGET

ifneq ($(strip $(shell g++ -v 2>&1 | grep "darwin")),)
TARGET=mac
else
ifneq ($(strip $(shell g++ -v 2>&1 | grep "cygwin")),)
TARGET=cygwin
else
TARGET=other
endif
endif

endif


FLAGS_cygwin = /opt/icon/bin/iload.a -Wl,--enable-auto-import
FLAGS_other = 

SHARED_mac = -bundle -undefined suppress
SHARED_cygwin = -shared
SHARED_other = -shared

IMPLIB_cygwin =  -Wl,--out-implib=iload.a
PIC_other = -fPIC
PIC_mac = -flat_namespace



EXAMPLES = callicon.exe coexp.exe extwidget.exe iterate.exe iterate2.exe iterate3.exe jmexample.exe kwd_vbl.exe methodcall.exe mkexternal.exe runerr.exe stop.exe 
DYNAMICS = callicon.so coexp.so extwidget.so iterate.so iterate2.so iterate3.so jmexample.so kwd_vbl.so methodcall.so mkexternal.so runerr.so stop.so 

%.so : %.cpp loadfuncpp.h loadfuncpp.u1
	g++ $(SHARED_$(TARGET)) $(PIC_$(TARGET)) -o $@ $< $(FLAGS_$(TARGET))

%.exe : %.icn %.so iload.so
	icont -so $@ $*

default: $(DYNAMICS) $(EXAMPLES)

.PHONY : iload.so loadfuncpp.h loadfuncpp.u1

loadfuncpp.h : ../loadfuncpp.h
	cp ../loadfuncpp.h ./

test : clean default 

clean :
	rm -f *.exe *.so *.o *% *~ core .#* *.u?
