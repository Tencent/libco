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
    movq %rax, 13 * 8(%rdi) // rsp
    movq %rbx, 12 * 8(%rdi)
    movq %rcx, 11 * 8(%rdi)
    movq %rdx, 10 * 8(%rdi)
	  movq (%rax), %rax
	  movq %rax,  9 * 8(%rdi) // ret func
    movq %rsi,  8 * 8(%rdi)
	  movq %rdi,  7 * 8(%rdi)
    movq %rbp,  6 * 8(%rdi)
    movq %r8,   5 * 8(%rdi)
    movq %r9,   4 * 8(%rdi)
    movq %r12,  3 * 8(%rdi)
    movq %r13,  2 * 8(%rdi)
    movq %r14,  1 * 8(%rdi)
    movq %r15,  0 * 8(%rdi)
	  xorq %rax, %rax

    movq 13 * 8(%rsi), %rsp
    movq 12 * 8(%rsi), %rbx
    movq 11 * 8(%rsi), %rcx
    movq 10 * 8(%rsi), %rdx
    movq  7 * 8(%rsi), %rdi
    movq  6 * 8(%rsi), %rbp
    movq  5 * 8(%rsi), %r8
    movq  4 * 8(%rsi), %r9
    movq  3 * 8(%rsi), %r12
    movq  2 * 8(%rsi), %r13
    movq  1 * 8(%rsi), %r14
    movq  0 * 8(%rsi), %r15
		leaq  1 * 8(%rsp), %rsp
		pushq 9 * 8(%rsi)

    movq  8 * 8(%rsi), %rsi
	ret
#endif
