
#Automatically generated from Makefile.mak and examples.txt by ../savex.icn

# icont -ucs file.icn -> u1, u2, goes in the /opt/icon/lib/.
# g++ stuff -> .os, goes in the /opt/icon/bin/.


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

PIC_other = -fPIC
PIC_mac = -flat_namespace

EXAMPLES = bang.exe divide.exe divide2.exe dull.exe generator.exe isexternal.exe iterate.exe keyword.exe makelist.exe object.exe 
DYNAMICS = bang.so divide.so divide2.so dull.so generator.so isexternal.so iterate.so keyword.so makelist.so object.so 

%.so : %.cpp loadfuncpp.h
	g++ $(SHARED_$(TARGET)) $(PIC_$(TARGET)) -o $@ $< $(FLAGS_$(TARGET))

%.exe : %.icn %.so
	icont -so $@ $*

default: $(DYNAMICS) $(EXAMPLES)

.PHONY : loadfuncpp.h

loadfuncpp.h : ../loadfuncpp.h
	cp ../loadfuncpp.h ./

test : clean default 

clean :
	rm -f *.exe *.so *.o *% *~ core .#*
