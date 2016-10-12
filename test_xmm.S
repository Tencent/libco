.globl test_xmm 
test_xmm:
#if defined(__i386__)
	movl 4(%esp), %eax
	movl 8(%esp), %ecx
	movaps (%eax),%xmm0
	movaps %xmm0,(%ecx)
#elif defined(__x86_64__)
	movaps (%rdi),%xmm0
	movaps %xmm0,(%rsi)
#endif
	ret


