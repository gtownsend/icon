# Symantec C++ 7.5 icont

CFLAGS= -c -A -o+speed -mx -wx
CC=sc

LHDRS=        ../h/sys.h ../h/rmacros.h ../h/rstructs.h ../h/rexterns.h \
      link.h lfile.h ../h/rt.h

TRANS=      trans.obj tcode.obj tlex.obj lnklist.obj \
        tparse.obj tsym.obj tmem.obj tree.obj

LINKR= link.obj lglob.obj lcode.obj llex.obj lmem.obj lsym.obj opcode.obj

COMMON= ../common/getopt.obj ../common/long.obj  ../common/alloc.obj \
           ../common/filepart.obj ../common/strtbl.obj ../common/ipp.obj

OBJS=   tmain.obj util.obj tlocal.obj $(TRANS) $(LINKR) $(COMMON)

.c.obj:
        $(CC) $(CFLAGS) -o$*.obj $*.c

# main program

icont.exe:      ixhdr.h $(OBJS)
                link /M @icont.lnk
  copy icont.exe ..\..\bin

# translator files

$(TRANS):

trans.obj: tglobals.h tsym.h ttoken.h tree.h ../h/version.h ../h/kdefs.h
lnklist.obj: lfile.h
tparse.obj: ../h/lexdef.h  tglobals.h tsym.h tree.h keyword.h
tlex.obj: ../h/lexdef.h ../h/parserr.h ttoken.h tree.h ../h/esctab.h \
    ../common/lextab.h ../common/yylex.h
tmem.obj: tglobals.h tsym.h tree.h
tree.obj: tree.h
tsym.obj: lfile.h tglobals.h tsym.h ttoken.h keyword.h ../h/kdefs.h

# linker files

$(LINKR): $(HDRS) $(LHDRS)

link.obj: tglobals.h ixhdr.h ../h/header.h
lcode.obj: tglobals.h opcode.h ../h/header.h ../h/opdefs.h ../h/version.h
lglob.obj: opcode.h ../h/opdefs.h ../h/version.h
llex.obj: tglobals.h opcode.h ../h/opdefs.h
lmem.obj: tglobals.h
lsym.obj: tglobals.h
opcode.obj: opcode.h ../h/opdefs.h

# ixhdr files

ixhdr.h:        ixhdrmk.exe ixhdr.exe
                ixhdrmk

ixhdrmk.exe:    ixhdrmk.c
                sc ixhdrmk.c

#ixhdr.exe:      ixhdr.c
#                sc -Jm -o+space ixhdr.c

#
#Use the following dependency instead if you have TASM
# NOTE: Using link386 instead of tlink produces a smaller executable.
#
ixhdr.exe:     ixhdr.asm
               tasm ixhdr
               link ixhdr


# clean dependency for rebuilding everything

clean:
        del *.obj            > nul
        del ..\common\*.obj  > nul
        del icont.exe        > nul
        del icont.map        > nul
        del ixhdr.h
        del ixhdr.exe        > nul
        del ixhdrmk.exe      > nul
        del *.bak            > nul
