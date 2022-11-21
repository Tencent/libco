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

#ifndef __CO_ROUTINE_H__
#define __CO_ROUTINE_H__

#include <stdint.h>
#include <sys/poll.h>
#include <pthread.h>

#ifdef __cplusplus
extern "C"
{
#endif
	// 1.struct

	struct stCoRoutine_t;
	struct stShareStack_t;

	struct stCoRoutineAttr_t
	{
		int stack_size;
		struct stShareStack_t *share_stack;
#ifdef __cplusplus
		stCoRoutineAttr_t()
		{
			stack_size = 128 * 1024;
			share_stack = NULL;
		}
#endif

	}
	__attribute__((packed));

	struct stCoEpoll_t;
	typedef int (*pfn_co_eventloop_t)(void *);
	typedef void *(*pfn_co_routine_t)(void *);

	// 2.co_routine
	// 获取当前协程
	struct stCoRoutine_t *co_get_current();

	int co_create(struct stCoRoutine_t **co, const struct stCoRoutineAttr_t *attr, void *(*routine)(void *), void *arg);
	void co_resume(struct stCoRoutine_t *co);
	void co_yield (struct stCoRoutine_t *co);
	void co_yield_ct(); // ct = current thread
	void co_release(struct stCoRoutine_t *co);
	void co_reset(struct stCoRoutine_t *co);

	struct stCoRoutine_t *co_self();

	int co_poll(struct stCoEpoll_t *ctx, struct pollfd fds[], nfds_t nfds, int timeout_ms);
	void co_eventloop(struct stCoEpoll_t *ctx, pfn_co_eventloop_t pfn, void *arg);

	// 3.specific

	int co_setspecific(pthread_key_t key, const void *value);
	void *co_getspecific(pthread_key_t key);

	// 4.event

	struct stCoEpoll_t *co_get_epoll_ct(); // ct = current thread

	// 5.hook syscall ( poll/read/write/recv/send/recvfrom/sendto )

	void co_enable_hook_sys();
	void co_disable_hook_sys();
	bool co_is_enable_sys_hook();

	// 6.sync
	struct stCoCond_t;

	struct stCoCond_t *co_cond_alloc();
	int co_cond_free(struct stCoCond_t *cc);

	int co_cond_signal(struct stCoCond_t *);
	int co_cond_broadcast(struct stCoCond_t *);
	int co_cond_timedwait(struct stCoCond_t *, int timeout_ms);

	// 7.share stack
	struct stShareStack_t *co_alloc_sharestack(int iCount, int iStackSize);

	// 8.init envlist for hook get/set env
	void co_set_env_list(const char *name[], size_t cnt);

	void co_log_err(const char *fmt, ...);
#ifdef __cplusplus
}
#endif
#endif
