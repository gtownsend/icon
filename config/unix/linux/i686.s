#
#  Assembler source for context switch using gas 1.38.1 + gcc 1.40 on
#  Xenix/386, revamped slightly for use with Linux by me (Richard Goer-
#  witz) on 7/25/94.
#

.file	"rswitch.s"
.data 1
.LC0:
	.byte 0x6e,0x65,0x77,0x5f,0x63,0x6f,0x6e,0x74,0x65,0x78
	.byte 0x74,0x28,0x29,0x20,0x72,0x65,0x74,0x75,0x72,0x6e
	.byte 0x65,0x64,0x20,0x69,0x6e,0x20,0x63,0x6f,0x73,0x77
	.byte 0x69,0x74,0x63,0x68,0x0
.text
	.align 4
.globl coswitch


coswitch:
	pushl %ebp
	movl %esp,%ebp
	movl 8(%ebp),%eax
	movl %esp,0(%eax)
	movl %ebp,4(%eax)
	movl 12(%ebp),%eax
	cmpl $0,16(%ebp)
	movl 0(%eax),%esp
	je .L2

	movl 4(%eax),%ebp
	jmp .L1

.L2:
	movl $0,%ebp
	pushl $0
	pushl $0
	call new_context
	pushl $.LC0
	call syserr
	addl $12,%esp

.L1:
	leave
	ret
