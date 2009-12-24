default:	benchmark


micro microbench microbenchmark:
		../../bin/icon micro.icn >micro.out 
		../../bin/icon micsum.icn micro.out >micro.sum
		cat micro.sum

benchmark:
		$(MAKE) benchmark-icont

benchmark-icont:
		$(MAKE) translate-icont compile-icont run-icont check-icont

translate-icont:
		sh Trans-icont post options shuffle

compile-icont:
		sh Comp-icont concord deal ipxref queens rsg

run-icont:
		sh Execute-icont

rerun-icont:
		sh ReRun-icont

check-icont:
		grep elapsed *-t.out

Clean:
		rm -f *.out *.sum *-t *.u?
