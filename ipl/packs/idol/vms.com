$ ! A script used internally by Idol on VMS
$ set default [.idolenv]
$ icont -c 'P1'
$ set default [-]
