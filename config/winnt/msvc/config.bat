mkdir ..\..\..\bin
copy makefile.top ..\..\..\makefile
copy DEFINE.H ..\..\..\src\h
copy PATH.H ..\..\..\src\h
copy RSWITCH.C ..\..\..\src\common
#copy RSWITCH.ASM ..\..\..\src\common
copy NTICONT.LNK ..\..\..\src\icont
copy ICONX.LNK ..\..\..\src\runtime
echo CONSOLE=NTConsole >..\..\..\src\common\makefile
type MAKEFILE.CMN >>..\..\..\src\common\makefile
copy MAKEFILE.RTT ..\..\..\src\rtt\makefile
echo ICONX=nticonx >..\..\..\src\runtime\makefile
type MAKEFILE.RUN >>..\..\..\src\runtime\makefile
echo ICONT=nticont >..\..\..\src\icont\makefile
echo CONSOLE=NTConsole >>..\..\..\src\icont\makefile
type MAKEFILE.T >>..\..\..\src\icont\makefile
copy RTT.LNK ..\..\..\src\rtt
del ..\..\..\src\icont\*.obj
del ..\..\..\src\common\*.obj
del ..\..\..\src\runtime\*.obj
