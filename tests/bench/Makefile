what:
		@echo "What do you want to make?"

benchmark:
		$(MAKE) benchmark-icont


benchmark-iconc:
		$(MAKE) compile-iconc run-iconc check-iconc

compile-iconc:
		sh Comp-iconc concord deal ipxref queens rsg

run-iconc:
		sh Run-iconc

rerun-iconc:
		sh ReRun-iconc

check-iconc:
		grep elapsed *-c.out


benchmark-icont:
		$(MAKE) translate-icont compile-icont run-icont check-icont

translate-icont:
		sh Trans-icont post options shuffle

compile-icont:
		sh Comp-icont concord deal ipxref queens rsg

run-icont:
		sh Run-icont

rerun-icont:
		sh ReRun-icont

check-icont:
		grep elapsed *-t.out

Clean:
		rm -f *.out concord-[ct] deal-[ct] ipxref-[ct] queens-[ct] \
		  rsg-[ct] *.u?
