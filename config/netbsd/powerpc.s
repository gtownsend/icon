#
#	coswitch for the PowerPC architecture
#

	.file   "rswitch.s"

	.data
errmsg:		.string "new_context() returned in coswitch\n"

	.text
	.align  2
	.globl  coswitch
	.type	coswitch,@function

coswitch:
	stwu	1, -80(1)		# allocate stack frame

					# Save Old Context:
	stw	1, 0(3)			# SP
	mflr	0
	stw	0, 4(3)			# LR (return address)
	stw	14, 0(1)		# GPRs 14-31 (save on stack)
	stw	15, 4(1)
	stw	16, 8(1)
	stw	17, 12(1)
	stw	18, 16(1)
	stw	19, 20(1)
	stw	20, 24(1)
	stw	21, 28(1)
	stw	22, 32(1)
	stw	23, 36(1)
	stw	24, 40(1)
	stw	25, 44(1)
	stw	26, 48(1)
	stw	27, 52(1)
	stw	28, 56(1)
	stw	29, 60(1)
	stw	30, 64(1)
	stw	31, 68(1)

	cmpi	0, 5, 0
	beq	first			# if first time

					# Restore new context:
	lwz	1, 0(4)			# SP
	lwz	0, 4(4)			# LR
	mtlr	0
	lwz	14, 0(1)		# GPRs 14-31 (from stack)
	lwz	15, 4(1)
	lwz	16, 8(1)
	lwz	17, 12(1)
	lwz	18, 16(1)
	lwz	19, 20(1)
	lwz	20, 24(1)
	lwz	21, 28(1)
	lwz	22, 32(1)
	lwz	23, 36(1)
	lwz	24, 40(1)
	lwz	25, 44(1)
	lwz	26, 48(1)
	lwz	27, 52(1)
	lwz	28, 56(1)
	lwz	29, 60(1)
	lwz	30, 64(1)
	lwz	31, 68(1)

	addic	1, 1, 80		# deallocate stack frame
	blr				# return into new context

first:					# First-time call:
	lwz	1, 0(4)			# SP as figured by Icon
	addic	1, 1, -64		# save area for callee
	addi	3, 0, 0			# arg1
	addi	4, 0, 0			# arg2
	bl	new_context		# new_context(0,0)
	lis	3, errmsg@ha
	la	3, errmsg@l(3)
	bl	syserr
