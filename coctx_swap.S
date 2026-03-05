/*
* Tencent is pleased to support the open source community by making Libco available.

* Copyright (C) 2014 THL A29 Limited, a Tencent company. All rights reserved.
*
* Licensed under the Apache License, Version 2.0 (the "License"); 
* you may not use this file except in compliance with the License. 
* You may obtain a copy of the License at
*
*	http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, 
* software distributed under the License is distributed on an "AS IS" BASIS, 
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. 
* See the License for the specific language governing permissions and 
* limitations under the License.
*/

.globl coctx_swap
#if !defined( __APPLE__ )
.type  coctx_swap, @function
#endif
coctx_swap:

#if defined(__i386__)
    movl 4(%esp), %eax
    movl %esp,  28(%eax)
    movl %ebp, 24(%eax)
    movl %esi, 20(%eax)
    movl %edi, 16(%eax)
    movl %edx, 12(%eax)
    movl %ecx, 8(%eax)
    movl %ebx, 4(%eax)


    movl 8(%esp), %eax
    movl 4(%eax), %ebx
    movl 8(%eax), %ecx
    movl 12(%eax), %edx
    movl 16(%eax), %edi
    movl 20(%eax), %esi
    movl 24(%eax), %ebp
    movl 28(%eax), %esp

	ret

#elif defined(__x86_64__)
	leaq (%rsp),%rax
    movq %rax, 104(%rdi)
    movq %rbx, 96(%rdi)
    movq %rcx, 88(%rdi)
    movq %rdx, 80(%rdi)
	  movq 0(%rax), %rax
	  movq %rax, 72(%rdi) 
    movq %rsi, 64(%rdi)
	  movq %rdi, 56(%rdi)
    movq %rbp, 48(%rdi)
    movq %r8, 40(%rdi)
    movq %r9, 32(%rdi)
    movq %r12, 24(%rdi)
    movq %r13, 16(%rdi)
    movq %r14, 8(%rdi)
    movq %r15, (%rdi)
	  xorq %rax, %rax

    movq 48(%rsi), %rbp
    movq 104(%rsi), %rsp
    movq (%rsi), %r15
    movq 8(%rsi), %r14
    movq 16(%rsi), %r13
    movq 24(%rsi), %r12
    movq 32(%rsi), %r9
    movq 40(%rsi), %r8
    movq 56(%rsi), %rdi
    movq 80(%rsi), %rdx
    movq 88(%rsi), %rcx
    movq 96(%rsi), %rbx
		leaq 8(%rsp), %rsp
		pushq 72(%rsi)

    movq 64(%rsi), %rsi
	ret
#elif defined(__riscv) && __riscv_xlen == 64
    /* Save curr context registers to regs[] pointed by a0 */
    /* regs layout: [0]=ra [1]=sp [2]=s0 [3]=s1 ... [13]=s11 [14]=a0 [15]=a1 */
    sd  ra,    0(a0)
    sd  sp,    8(a0)
    sd  s0,   16(a0)
    sd  s1,   24(a0)
    sd  s2,   32(a0)
    sd  s3,   40(a0)
    sd  s4,   48(a0)
    sd  s5,   56(a0)
    sd  s6,   64(a0)
    sd  s7,   72(a0)
    sd  s8,   80(a0)
    sd  s9,   88(a0)
    sd  s10,  96(a0)
    sd  s11, 104(a0)
    sd  a0,  112(a0)
    sd  a1,  120(a0)

    /* Restore pending context registers from regs[] pointed by a1 */
    ld  ra,    0(a1)
    ld  sp,    8(a1)
    ld  s0,   16(a1)
    ld  s1,   24(a1)
    ld  s2,   32(a1)
    ld  s3,   40(a1)
    ld  s4,   48(a1)
    ld  s5,   56(a1)
    ld  s6,   64(a1)
    ld  s7,   72(a1)
    ld  s8,   80(a1)
    ld  s9,   88(a1)
    ld  s10,  96(a1)
    ld  s11, 104(a1)
    ld  a0,  112(a1)
    ld  a1,  120(a1)
    ret
#endif
