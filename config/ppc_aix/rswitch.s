#  coswitch(old, new, first)
#          GPR3  GPR4  GPR5

	.file	"rswitch.s"
	.extern	.new_context{PR}
	.extern	.syserr{PR}
	.globl	.coswitch[PR]
	.csect	.coswitch[PR]

	.set	r0, 0
	.set	SP, 1
	.set	TOC, 2
	.set	OLD, 3
	.set	NEW, 4
	.set	FIRST, 5
	.set	RSIZE, 80		# room for regs 13-31, rounded up mod16

.coswitch:
	stu	SP, -RSIZE(SP)		# allocate stack frame

					# Save Old Context:
	st	SP, 0(OLD)		# SP
	st	TOC, 4(OLD)		# TOC
	mflr	r0
	st	r0, 8(OLD)		# LR (return address)
	mfcr	r0
	st	r0, 12(OLD)		# CR
	stm	13, -RSIZE(SP)		# GPRs 13-31 (save on stack)

	cmpi	0, FIRST, 0
	beq	first			# if first time

					# Restore new context:
	l	SP, 0(NEW)		# SP
	l	TOC, 4(NEW)		# TOC
	l	r0, 8(NEW)		# LR
	mtlr	r0		
	l	r0, 12(NEW)		# CR
	mtcr	r0
	lm	13, -RSIZE(SP)		# GPRs 13-31 (from stack)
	
	ai	SP, SP, RSIZE		# deallocate stack frame
	brl				# return into new context

first:					# First-time call:
	l	SP, 0(NEW)		# SP as figured by Icon
	ai	SP, SP, -64(SP)		# save area for callee
	cal	OLD, 0(r0)		# arg1
	cal	NEW, 0(r0)		# arg2
	bl	.new_context{PR}	# new_context(0,0)
	cal	OLD, 0(r0)
	bl	.syserr{PR}
