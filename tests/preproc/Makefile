SHELL=/bin/sh
MAKE=make

.SILENT:
.IGNORE:

test: cond errif macros errmac include errinc line errlin misc_dir dir_delm\
   gen_line str_cat misc cmd_opts errcmd

# test conditional compilation
cond:
	Test-pp condexpr.out condexpr.c
	Test-pp elif.out elif.c
	Test-pp emptyif.out emptyif.c
	Test-pp nestif.out nestif.c
	Test-pp variants.out variants.c

# test errors in conditional compilation
errif:
	Test-pp errif1.out errif1.c
	Test-pp errif2.out errif2.c
	Test-pp errif3.out errif3.c
	Test-pp errif4.out errif4.c
	Test-pp errif5.out errif5.c
	Test-pp errif6.out errif6.c
	Test-pp errif7.out errif7.c
	Test-pp errif8.out errif8.c
	Test-pp errif9.out errif9.c
	Test-pp errif10.out errif10.c
	Test-pp errif11.out errif11.c
	Test-pp errif12.out errif12.c

# test macros
macros:
	Test-pp define.out define.c
	Test-pp begdef.out begdef.c
	Test-pp undef.out undef.c
	Test-pp macargs.out macargs.c
	Test-pp stringiz.out stringiz.c
	Test-pp paste.out paste.c
	Test-pp macnest.out macnest.c
	Test-pp special.out special.c
	Test-pp recurse.out recurse.c
	Test-pp redef.out redef.c
	Test-pp std1.out std1.c
	Test-pp std2.out std2.c

# test macro errors
errmac:
	Test-pp errmac1.out errmac1.c
	Test-pp errmac2.out errmac2.c
	Test-pp errmac3.out errmac3.c
	Test-pp errmac4.out errmac4.c
	Test-pp errmac5.out errmac5.c
	Test-pp errmac6.out errmac6.c
	Test-pp errmac7.out errmac7.c
	Test-pp errmac8.out errmac8.c
	Test-pp errmac9.out errmac9.c
	Test-pp errmac10.out errmac10.c
	Test-pp errmac11.out errmac11.c
	Test-pp errmac12.out errmac12.c
	Test-pp errmac13.out errmac13.c
	Test-pp errmac14.out errmac14.c
	Test-pp errmac15.out errmac15.c
	Test-pp errmac16.out errmac16.c
	Test-pp errmac17.out errmac17.c
	Test-pp errmac18.out errmac18.c
	Test-pp errmac19.out errmac19.c
	Test-pp errmac20.out errmac20.c
	Test-pp errmac21.out errmac21.c

# test #include directives
include:
	Test-pp  sys.out sys.c
	Test-pp  include.out include.c

# test errors in #include directives
errinc:
	Test-pp errinc1.out errinc1.c
	Test-pp errinc2.out errinc2.c
	Test-pp errinc3.out errinc3.c
	Test-pp errinc4.out errinc4.c

# test #line directive
line:
	Test-pp line.out line.c
	Test-pp linemac.out linemac.c

# test errors in #line directive
errlin:
	Test-pp errlin1.out errlin1.c
	Test-pp errlin2.out errlin2.c
	Test-pp errlin3.out errlin3.c
	Test-pp errlin4.out errlin4.c

# test misc directives: #error, #pragma, and #
misc_dir:
	Test-pp error1.out error1.c
	Test-pp error2.out error2.c
	Test-pp pragma.out pragma.c

# test how various combinations of white space, including new lines
#  delimit preprocessor directives
dir_delm:
	Test-pp delim1.out delim1.c
	Test-pp delim2.out delim2.c

# test the generation of #line directives
gen_line:
	Test-pp glnargs.out glnargs.c
	Test-pp glncond.out glncond.c
	Test-pp glnincl.out glnincl.c
	Test-pp glnmulti.out glnmulti.c

# test string concatenation
str_cat:
	Test-pp concat.out concat.c
	Test-pp wh_sp.out wh_sp.c
	Test-pp oct_hex.out oct_hex.c

# test misc: trigraphs, empty file, misssing final new line, and comments
misc:
	Test-pp trigraph.out trigraph.c
	Test-pp empty.out empty.c
	Test-pp miss_nl.out miss_nl.c
	Test-pp comments.ou1 comments.c
	Test-pp comments.ou2 -C comments.c

# test command line options and arguments
cmd_opts:
	Test-pp cmdopt.ou1 cmdopt.c
	Test-pp cmdopt.ou2 -C -U__RCRS__ cmdopt.c
	Test-pp cmdopt.ou3 -P -Dtesting= cmdopt.c
	Test-pp cmdopt.ou4 -P -C "'-Dtesting=   test of -D  '"<cmdopt.c
	Test-pp cmdopt.ou5 -Dtesting cmdopt1.c - cmdopt1.c <cmdopt.c
	Test-pp inclsrch.out -Idir1 -Idir2 inclsrch.c

# test erroneous command line options and arguments
errcmd:
	Test-pp errcmd.ou1 missing.c
	Test-pp errcmd.ou2 -a dummy.c
	Test-pp errcmd.ou3 -D dummy.c
	Test-pp errcmd.ou4 -D7 dummy.c
	Test-pp errcmd.ou5 -Dx- dummy.c
	Test-pp errcmd.ou6 -U
	Test-pp errcmd.ou7 -U- dummy.c
	Test-pp errcmd.ou8 -U__RCRS__= dummy.c
	Test-pp errcmd.ou9 dummy.c -Dx

Clean:
		rm -f foo

# Entries beyond this point are for use at Arizona only.
#   *** Do not delete the line above; it is used in trimming Makefiles
#   for distribution ***

Dist-clean:
		rm -f .*K* .emacs*
		rm -f local/*
		touch local/.placeholder
		rm -rf `gcomp Test-pp README Makefile local stand *.[ch] \
		   dir1 dir2`
