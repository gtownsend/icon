# sed(1) directives for cleaning up nroff(1) formatted man page

/^User Commands/d
/^ICONT/d
/^University/d
s/.//g
s/’/'/g

