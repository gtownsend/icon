call config
echo #define MSWindows 1 >> ..\..\..\src\h\define.h
echo #define FAttrib 1 >> ..\..\..\src\h\define.h
echo ICONX=wiconx >..\..\..\src\runtime\makefile
type MAKEFILE.RUN >>..\..\..\src\runtime\makefile
echo ICONT=wicont >..\..\..\src\icont\makefile
echo CONSOLE=NOTHING >>..\..\..\src\icont\makefile
type MAKEFILE.T >>..\..\..\src\icont\makefile
echo CONSOLE=NOTHING >..\..\..\src\common\makefile
echo DCONSOLE=dconsole.obj >>..\..\..\src\common\makefile
type MAKEFILE.CMN >>..\..\..\src\common\makefile
