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

#ifndef __CO_EPOLL_H__
#define __CO_EPOLL_H__
#include <stdint.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <sys/types.h>
#include <time.h>
#include <time.h>

#if !defined(__APPLE__) && !defined(__FreeBSD__)

#include <sys/epoll.h>

/*libco对epoll和kqueue进行了统一接口的封装，接口的方式与epoll类似，这里为了适应epoll和kqueue这两种IO模型，
*co_epoll_res就把epoll和kqueue的结果包装在同一个结构体里面
*/
struct co_epoll_res
{
	int size;
	struct epoll_event *events; //epoll_event是glibc定义的结构体
	struct kevent *eventlist;	//TODO: kevent的定义找不到，百度之后发现好像是和kqueue有关。参考链接：https://www.freebsd.org/cgi/man.cgi?query=kevent&sektion=2
								/*
	struct kevent {
		uintptr_t   ident;      // identifier for this event 
		int16_t     filter;     // filter for event 
		uint16_t    flags;      // general flags 
		uint32_t    fflags;     // filter-specific flags 
		intptr_t    data;       // filter-specific data 
		void        *udata;     // opaque user data identifier 
	};
	ident 是事件唯一的 key，在 socket 使用中，它是 socket 的 fd 句柄
	filter 是事件的类型，有 15 种，其中几种常用的是 
		EVFILT_READ   socket 可读事件
		EVFILT_WRITE  socket 可写事件
		EVFILT_SIGNAL  unix 系统的各种信号
		EVFILT_TIMER  定时器事件
		EVFILT_USER  用户自定义的事件
	flags 操作方式，EV_ADD 添加，EV_DELETE 删除，EV_ENABLE 激活，EV_DISABLE 不激活
	fflags 第二种操作方式，NOTE_TRIGGER 立即激活等等
	data int 型的用户数据，socket 里面它是可读写的数据长度
	udata 指针类型的数据，你可以携带任何想携带的附加数据。比如对象
	*/
};
int co_epoll_wait(int epfd, struct co_epoll_res *events, int maxevents, int timeout);
int co_epoll_ctl(int epfd, int op, int fd, struct epoll_event *);
int co_epoll_create(int size);
struct co_epoll_res *co_epoll_res_alloc(int n);
void co_epoll_res_free(struct co_epoll_res *);

#else

#include <sys/event.h>
enum EPOLL_EVENTS
{
	EPOLLIN = 0X001,
	EPOLLPRI = 0X002,
	EPOLLOUT = 0X004,

	EPOLLERR = 0X008,
	EPOLLHUP = 0X010,

	EPOLLRDNORM = 0x40,
	EPOLLWRNORM = 0x004,
};
#define EPOLL_CTL_ADD 1
#define EPOLL_CTL_DEL 2
#define EPOLL_CTL_MOD 3
typedef union epoll_data
{
	void *ptr;
	int fd;
	uint32_t u32;
	uint64_t u64;

} epoll_data_t;

struct epoll_event
{
	uint32_t events;
	epoll_data_t data;
};

struct co_epoll_res
{
	int size;
	struct epoll_event *events;
	struct kevent *eventlist;
};
int co_epoll_wait(int epfd, struct co_epoll_res *events, int maxevents, int timeout);
int co_epoll_ctl(int epfd, int op, int fd, struct epoll_event *);
int co_epoll_create(int size);
struct co_epoll_res *co_epoll_res_alloc(int n);
void co_epoll_res_free(struct co_epoll_res *);

#endif
#endif
