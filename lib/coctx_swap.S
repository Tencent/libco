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

#elif defined(__aarch64__)
    stp x0,  x1,  [x0]
    stp x2,  x3,  [x0, #16]
    stp x4,  x5,  [x0, #32]
    stp x6,  x7,  [x0, #48]
    stp x8,  x9,  [x0, #64]
    stp x10, x11, [x0, #80]
    stp x12, x13, [x0, #96]
    stp x14, x15, [x0, #112]
    stp x16, x17, [x0, #128]
    stp x18, x19, [x0, #144]
    stp x20, x21, [x0, #160]
    stp x22, x23, [x0, #176]
    stp x24, x25, [x0, #192]
    stp x26, x27, [x0, #208]
    stp x28, x29, [x0, #224]
    mov x2, sp
    stp x30, x2,  [x0, #240]

    ldp x2,  x3,  [x1, #16]
    ldp x4,  x5,  [x1, #32]
    ldp x6,  x7,  [x1, #48]
    ldp x8,  x9,  [x1, #64]
    ldp x10, x11, [x1, #80]
    ldp x12, x13, [x1, #96]
    ldp x14, x15, [x1, #112]
    ldp x16, x17, [x1, #128]
    ldp x18, x19, [x1, #144]
    ldp x20, x21, [x1, #160]
    ldp x22, x23, [x1, #176]
    ldp x24, x25, [x1, #192]
    ldp x26, x27, [x1, #208]
    ldp x28, x29, [x1, #224]
    ldp x30, x0,  [x1, #240]
    mov sp,  x0
    ldp x0,  x1,  [x1]

    ret

.globl get_sp
get_sp:
    mov x1, sp
    str x1, [x0]
    ret

#endif
