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

#ifndef __CO_ROUTINE_INNER_H__

#include "co_routine.h"
#include "coctx.h"
struct stCoRoutineEnv_t;
struct stCoSpec_t
{
	void *value;
};

/**
* 指向一个共享栈的内存空间
* 一个进程或者线程栈的地址，是从高位到低位安排数据的，所以stack_bp是栈底，stack_buffer是栈顶
*/
struct stStackMem_t
{
	stCoRoutine_t *occupy_co; // 当前正在使用该共享栈的协程
	int stack_size;			  // 栈的大小
	char *stack_bp;			  //stack_buffer + stack_size 栈底
	char *stack_buffer;		  // 栈顶
};

struct stShareStack_t
{
	unsigned int alloc_idx; // 当前正在使用的共享栈的index，这个的值可能大于count，大于则取模从头开始循环
	int stack_size;			// 共享栈的大小，这里的大小指的是一个stStackMem_t*的大小，和stStackMem_t里的一样
	int count;				// 共享栈的个数，共享栈可以为多个，所以以下为共享栈的数组
	stStackMem_t **stack_array;
};

//协程
struct stCoRoutine_t
{
	stCoRoutineEnv_t *env; // 协程所在的运行环境
	pfn_co_routine_t pfn;  // 协程所对应的函数
	void *arg;			   // 函数参数
	coctx_t ctx;		   // 协程上下文，包括寄存器和栈

	char cStart; // 是否开始执行
	char cEnd;
	char cIsMain;		 // 是否是主协程
	char cEnableSysHook; // 是否开启hook
	char cIsShareStack;	 // 是否开启共享栈

	void *pvEnv;

	//char sRunStack[ 1024 * 128 ];
	stStackMem_t *stack_mem;

	//save satck buffer while confilct on same stack_buffer;
	char *stack_sp;
	unsigned int save_size;
	char *save_buffer;

	stCoSpec_t aSpec[1024];
};

//1.env
void co_init_curr_thread_env();
stCoRoutineEnv_t *co_get_curr_thread_env();

//2.coroutine
void co_free(stCoRoutine_t *co);
void co_yield_env(stCoRoutineEnv_t *env);

//3.func

//-----------------------------------------------------------------------------------------------

struct stTimeout_t;
struct stTimeoutItem_t;

stTimeout_t *AllocTimeout(int iSize);
void FreeTimeout(stTimeout_t *apTimeout);
int AddTimeout(stTimeout_t *apTimeout, stTimeoutItem_t *apItem, uint64_t allNow);

struct stCoEpoll_t;
stCoEpoll_t *AllocEpoll();
void FreeEpoll(stCoEpoll_t *ctx);

stCoRoutine_t *GetCurrThreadCo();
void SetEpoll(stCoRoutineEnv_t *env, stCoEpoll_t *ev);

typedef void (*pfnCoRoutineFunc_t)();

#endif

#define __CO_ROUTINE_INNER_H__
