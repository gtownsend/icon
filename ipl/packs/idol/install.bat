rem msdos Idol installation
rem This compiles Idol in order to to test the system
icont -Sr1000 -SF30 -Si1000 idolboot msdos
mkdir idolcode.env
iconx idolboot -t -install
chdir idolcode.env
icont -c i_object
chdir ..
iconx idolboot idol idolmain msdos
idolt
