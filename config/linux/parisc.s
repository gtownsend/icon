; coexpression code for HP PA-RISC architecture for Icon 8.10
;
; n.b.  two of the three coexpression tests work, but coexpression
;	*transmission*, a rarely used feature, does not

	.CODE
        .IMPORT syserr
	.EXPORT	coswitch
coswitch
        .PROC
        .CALLINFO
        .ENTRY
	; store old registers
	STW	%sp,0(%arg0)
	; not used: 4(%arg0)
	STW	%rp,8(%arg0)
	STW	%r3,12(%arg0)
	STW	%r4,16(%arg0)
	STW	%r5,20(%arg0)
	STW	%r6,24(%arg0)
	STW	%r7,28(%arg0)
	STW	%r8,32(%arg0)
	STW	%r9,36(%arg0)
	STW	%r10,40(%arg0)
	STW	%r11,44(%arg0)
	STW	%r12,48(%arg0)
	STW	%r13,52(%arg0)
	STW	%r14,56(%arg0)
	STW	%r15,60(%arg0)
	STW	%r16,64(%arg0)
	STW	%r17,68(%arg0)
	STW	%r18,72(%arg0)

        COMIB,=,N      0,%arg2,L$isfirst

	; this is not a first-time call; reload old context
	LDW	0(%arg1),%sp
	LDW	8(%arg1),%rp
	LDW	12(%arg1),%r3
	LDW	16(%arg1),%r4
	LDW	20(%arg1),%r5
	LDW	24(%arg1),%r6
	LDW	28(%arg1),%r7
	LDW	32(%arg1),%r8
	LDW	36(%arg1),%r9
	LDW	40(%arg1),%r10
	LDW	44(%arg1),%r11
	LDW	48(%arg1),%r12
	LDW	52(%arg1),%r13
	LDW	56(%arg1),%r14
	LDW	60(%arg1),%r15
	LDW	64(%arg1),%r16
	LDW	68(%arg1),%r17
	LDW	72(%arg1),%r18
	BV,N	(%rp)			; return

L$isfirst
	LDW	0(%arg1),%sp
	LDI	0,%arg0
	LDI	0,%arg1
        .CALL   ARGW0=GR,ARGW1=GR
	BL,N	new_context,%rp		; call new_context(0,0)
	SUBI	1,%r0,%rp
	BV,N	(%rp)			; abort w/ illegal jump
        .EXIT
        .PROCEND 
        .IMPORT new_context,CODE
        .END
