# Symantec C++ runtime
#
# Note: Some of the larger .r files may cause make to run out of memory.
#       Use the r2obj.bat utility to build the offending file, and then
#       restart make. Rebuilding rtt with DOSX also solves the problem.
#

#
# Note: AFLAGS is used by rswitch.c, which is mostly assembly language.
#       Do not use optimization on rswitch.c or it will not work!
#       Use AFLAGS to define all compiler switches except optimization.
#       Then add desired optimization switches to CFLAGS.
#
AFLAGS= -c -mn -a4 -wx -D_CONSOLE=1
CFLAGS= $(AFLAGS) -A -o+speed
CC=sc

ICONDIR=e:\bin

HDRS = ../h/define.h ../h/config.h ../h/typedefs.h ../h/sys.h ../h/proto.h \
        ../h/cstructs.h ../h/cpuconf.h ../h/rmacros.h ../h/rexterns.h \
        ../h/rstructs.h ../h/rproto.h ../h/mproto.h ../h/fgevents.h

####################################################################
#
# Make entries for iconx
#

COMMON= ..\common\long.obj ..\common\memory.obj ..\common\mlocal.obj \
        ..\common\time.obj ..\common\filepart.obj

COMMON2= ..\common\redirerr.obj ..\common\rswitch.obj

OBJ=    xcnv.obj     xdata.obj    xdef.obj     xerrmsg.obj  xextcall.obj \
        xfconv.obj   xfmath.obj   xfmisc.obj   xfmonitr.obj xfscan.obj   \
        xfstr.obj    xfstranl.obj xfstruct.obj xfsys.obj    xfxtra.obj   \
        ximain.obj   ximisc.obj   xinit.obj    xinterp.obj  xinvoke.obj  \
        xistart.obj  xkeyword.obj xlmisc.obj   xoarith.obj  xoasgn.obj   \
        xocat.obj    xocomp.obj   xomisc.obj   xoref.obj    xoset.obj    \
        xovalue.obj  xralc.obj    xrcoexpr.obj xrcomp.obj   xrdebug.obj  \
        xrlocal.obj  xrlrgint.obj xrmemmgt.obj xrmisc.obj   xrstruct.obj \
        xrsys.obj

.c.obj:
        $(CC) $(CFLAGS) -o$*.obj $*.c

iconx.exe: icont.exe setsize.exe $(OBJ) $(COMMON) $(COMMON2)
        link /M @iconxcns.lnk
        $(ICONDIR)\iconx setsize.exe
        copy iconx.exe ..\..\bin

icont.exe:
        @cd ..\icont
        @make
        @cd ..\runtime
        @copy ..\icont\icont.exe >nul

setsize.exe: setsize.icn
        $(ICONDIR)\icont setsize

..\common\rswitch.obj: ..\common\rswitch.c
        $(CC) $(AFLAGS) -o..\common\rswitch.obj ..\common\rswitch.c

..\common\redirerr.obj: ..\common\redirerr.c
        $(CC) $(AFLAGS) -o..\common\redirerr.obj ..\common\redirerr.c

clean:
        del *.obj
        del ..\common\*.obj
        del ..\icont\*.obj
        del icon?.exe
        del iconx.map
        del *.bak

include iconx.dep
