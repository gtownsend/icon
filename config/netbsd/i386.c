/*
 * coswitch for the i386 architecture
 */

int
coswitch (int *old_cs, int *new_cs, int first)
{
	asm ("movl 8(%ebp),%eax");
	asm ("movl %esp,0(%eax)");
	asm ("movl %ebp,4(%eax)");
	asm ("movl 12(%ebp),%eax");

	if (first == 0) {                    /* this is the first activation */
		asm ("movl 0(%eax),%esp");
		asm ("movl $0,%ebp");
		new_context (0, 0);
		syserr ("new_context() returned in coswitch");
	}
	else {
		asm ("movl 0(%eax),%esp");
		asm ("movl 4(%eax),%ebp");
	}
}
