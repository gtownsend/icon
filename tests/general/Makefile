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


# test various command options

Test-options:
		sh Test-opts >options.out
		cmp options.ok options.out


Clean:
		-rm -f *.u? *.c *.h foo.baz tmp1 tmp2 *.out
		-rm -f `find * -type f -perm -100 ! -name 'Test-*' -print`
