#  Makefile for testing Icon


SAMPLES = btrees diffwrds kross meander prefix recogn roman sieve wordcnt



# default is to run all tests, using icont

Test Test-icont:
		IC=icont sh Test-icon

Samples Samples-icont:
		IC=icont sh Test-icon $(SAMPLES)


Test-iconc:
		IC=iconc sh Test-icon

Samples-iconc:
		IC=iconc sh Test-icon $(SAMPLES)



Clean:
		-rm -f *.u? foo.baz *.out
		-rm -f `find [a-z]* -type f -perm -100 -print`
