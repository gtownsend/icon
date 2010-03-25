#  icondb -- Icon database interface contributed by Carl Sturtivant.

#  Requires GNU make, gcc, mysql utilities, and mysql development package.

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

endif #TARGET


ICON_PATH=../../..


SHARED_mac = -bundle -undefined suppress
SHARED_cygwin = -shared
SHARED_other = -shared

PIC_mac = -flat_namespace
PIC_other = -fPIC

EXTRA_cygwin = $(ICON_PATH)/bin/iload.a -Wl,--enable-auto-import
EXTRA_other = -I./


default:
	icont -ucs icondb.icn
	cp icondb.u1 icondb.u2 $(ICON_PATH)/lib
	sh -c "gcc -I../../cfuncs $(SHARED_$(TARGET)) -o mysqldb.so $(PIC_$(TARGET)) `mysql_config --cflags` mysqldb.c `mysql_config --libs`"
	cp mysqldb.so $(ICON_PATH)/bin

clean Clean:
	rm -f *.u? *.o *.so */*.o */*.u? */*.so
