#  loadfuncpp -- a C++ interface for icon.  See doc/index.htm.

#  Requires GNU make and g++.

CC=g++

ifneq ($(strip $(shell g++ -v 2>&1 | grep "darwin")),)
TARGET=mac
else
ifneq ($(strip $(shell g++ -v 2>&1 | grep "cygwin")),)
TARGET=cygwin
else
ifneq ($(strip $(shell g++ -v 2>&1 | grep "solaris")),)
#TARGET=sun
#CC=cc
TARGET=other
else
TARGET=other
endif
endif
endif



#ICON_PATH = $(shell cd $(PWD)/../../..; pwd)
ICON_PATH = ../../..

ICON_BUILD_PATH = $(ICON_PATH)

ICON_BIN_PATH = $(ICON_PATH)/bin
ICON_LIB_PATH = $(ICON_PATH)/lib
ICON_HDR_PATH = $(ICON_BUILD_PATH)/src/h

ICON_HDR_FILE = \"$(ICON_HDR_PATH)/rt.h\"

FLAGS_cygwin = -Wl,--enable-auto-import
FLAGS_cygwin_default = $(ICON_BIN_PATH)/iconx.a
FLAGS_cygwin_iexample = $(ICON_BIN_PATH)/iload.a

SHARED_mac = -bundle -undefined suppress
SHARED_cygwin = -shared
SHARED_other = -shared

IMPLIB_cygwin =  -Wl,--out-implib=iload.a
PIC_other = -fPIC
PIC_mac = -flat_namespace

COPY_cygwin =cp iload.a $(ICON_BIN_PATH)/

COPY_PACKAGE_cygwin=cp iload.a package/bin

DEPS_cygwin_default = $(ICON_BIN_PATH)/iconx.a
DEPS_cygwin_iexample = $(ICON_BIN_PATH)/iload.a

DEPS_default = $(ICON_BIN_PATH)/iload.so $(ICON_BIN_PATH)/iloadgpx.so $(ICON_BIN_PATH)/iloadnogpx.so
SLIBS = iload.so iloadgpx.so iloadnogpx.so	

.PHONY : default clean iconsrc iexample package

	
default : $(DEPS_default) $(DEPS_$(TARGET)_default) $(ICON_LIB_PATH)/loadfuncpp.u1

iload.so : %.so : %.cpp loadfuncpp.h iload.h
	$(CC) $(PIC_$(TARGET)) $(SHARED_$(TARGET)) -o $@ $< $(FLAGS_$(TARGET)_default) $(FLAGS_$(TARGET)) $(IMPLIB_$(TARGET)) -DRTT=$(ICON_HDR_FILE)

iloadgpx.so iloadnogpx.so : %.so : %.cpp loadfuncpp.h iload.h $(DEPS_$(TARGET)_iexample)
	$(CC) $(PIC_$(TARGET)) $(SHARED_$(TARGET)) -o $@ $< $(FLAGS_$(TARGET)_default) $(FLAGS_$(TARGET)_iexample) $(FLAGS_$(TARGET)) -DRTT=$(ICON_HDR_FILE)

$(DEPS_default) : $(ICON_BIN_PATH)/%.so : %.so
	cp $< $(ICON_BIN_PATH)

$(ICON_BIN_PATH)/iload.a : iload.a
	cp $< $(ICON_BIN_PATH)
	
iload.a : iload.so

$(ICON_LIB_PATH)/loadfuncpp.u1 : loadfuncpp.u1
	cp loadfuncpp.u? $(ICON_LIB_PATH)
	
loadfuncpp.u1 : loadfuncpp.icn
	icont -cs loadfuncpp.icn
	
clean Clean:
	rm -f iexample *.exe *.u? *.so *.o *% *~ core .#*

iconsrc: $(ICON_BIN_PATH) $(ICON_LIB_PATH) $(ICON_HDR_PATH)
	@ echo "install Icon 9.5+ from source in $(ICON_PATH)" 
	@ exit 1

iexample: iexample.so $(DEPS_$(TARGET)_iexample)
	icont -s iexample.icn

iexample.so : iexample.cpp loadfuncpp.h
	$(CC) $(PIC_$(TARGET)) $(SHARED_$(TARGET)) -o $@ $< $(FLAGS_$(TARGET)_iexample) $(FLAGS_$(TARGET))

package : $(SLIBS) loadfuncpp.u1
	mkdir package
	mkdir package/bin
	cp iload*.so package/bin
	$(COPY_PACKAGE_$(TARGET))
	mkdir package/lib
	cp loadfuncpp.u? package/lib
	mkdir package/h
	cp loadfuncpp.h package/h
	tar -cf $(TARGET).tar package
	gzip $(TARGET).tar
	rm -rf package/
