# Makefile for WINCAP
# Copyright (c) 1991 Microsoft Corporation. All rights reserved.

OBJS = wincap.obj dialogs.obj file.obj errors.obj  \
    dibutil.obj copy.obj print.obj

APP_compile = -c -AS -Gsw -Zipe -W3 -Od
APP_link = /CO /NOD /NOE

lib: $(OBJS)

all: wincap.exe

.c.obj:
    cl -c /D_X86_ /DWIN32 /DMSNT /O1 $*.c

wincap.obj: wincap.c wincap.h dialogs.h dibapi.h

dialogs.obj: dialogs.c dialogs.h

file.obj: file.c dibapi.h

errors.obj: errors.c errors.h

dibutil.obj: dibutil.c dibutil.h

copy.obj: copy.c dibapi.h

print.obj: print.c dibapi.h dialogs.h

wincap.res: $*.rc wincap.h dialogs.h dialogs.dlg wincap.rc dibapi.h wincap.ico
     rc -r $*.rc

wincap.exe: $(OBJS) wincap.def wincap.res
     link $(APP_link) $(OBJS),,/align:16, libw slibcew, wincap.def
     rc $*.res

