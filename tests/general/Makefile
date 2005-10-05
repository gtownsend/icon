#  Makefile for testing Icon


SAMPLES = btrees diffwrds kross meander prefix recogn roman sieve wordcnt



# default is to run all tests, using icont

Test Test-icont:  Test-programs Test-preproc Test-options


# test programs

Programs Programs-icont Test-programs:
		IC=icont sh Test-icon

Samples Samples-icont:
		IC=icont sh Test-icon $(SAMPLES)


Test-iconc:
		IC=iconc sh Test-icon

Samples-iconc:
		IC=iconc sh Test-icon $(SAMPLES)


# test preprocessor

Test-preproc:
		../../bin/icont -E tpp.icn tpp9.icn >tpp.out 2>tpp.err || :
		cat tpp.err tpp.out >tpp.all
		cmp tpp.ok tpp.all
		: preprocessor test passed


# test various command options

Test-options:
		sh Test-opts >options.out
		cmp options.ok options.out
		: options test passed


Clean:
		-rm -f *.u? *.c *.h foo.baz tmp1 tmp2 *.out *.err *.all
		-rm -f xx `find * -type f -perm -100 ! -name 'Test-*' -print`
