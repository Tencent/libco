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

#include "co_routine.h"
#include "co_routine_inner.h"
#include "co_epoll.h"

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <string>
#include <map>

#include <poll.h>
#include <sys/time.h>
#include <errno.h>

#include <assert.h>

#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/syscall.h>
#include <unistd.h>
#include <limits.h>

extern "C"
{
	// TODO: 保存当前上下文到第一个参数，并激活第二个参数的上下文
	extern void coctx_swap(coctx_t *, coctx_t *) asm("coctx_swap"); // TODO: 这里没看懂，extern本身不就实现了函数声明吗，为什么还要加asm汇编
};
using namespace std;
stCoRoutine_t *GetCurrCo(stCoRoutineEnv_t *env); // 根据线程的运行环境取当前协程
struct stCoEpoll_t;

/*
* 线程所管理的协程的运行环境
* 一个线程只有一个这个属性
* 包括协程调用栈、epoll协程调度器、
*/
struct stCoRoutineEnv_t
{
	// 这里实际上维护的是个调用栈
	// 最后一位是当前运行的协程，前一位是当前协程的父协程(即resume该协程的协程)
	// 可以看出，libco只能支持128层协程的嵌套调用
	stCoRoutine_t *pCallStack[128];
	int iCallStackSize; // 当前调用栈长度
	//主要是epoll，作为协程的调度器，每个线程只有一个env，所以每个线程只有一个pEpoll，通过pEpoll将事件组成到epoll中，同时通过时间轮来管理这些事件
	stCoEpoll_t *pEpoll;

	//for copy stack log lastco and nextco
	stCoRoutine_t *pending_co;
	stCoRoutine_t *occupy_co;
};
//int socket(int domain, int type, int protocol);
void co_log_err(const char *fmt, ...)
{
}

#if defined(__LIBCO_RDTSCP__)
static unsigned long long counter(void)
{
	register uint32_t lo, hi;
	register unsigned long long o;
	__asm__ __volatile__(
		"rdtscp"
		: "=a"(lo), "=d"(hi)::"%rcx");
	o = hi;
	o <<= 32;
	return (o | lo);
}
static unsigned long long getCpuKhz()
{
	FILE *fp = fopen("/proc/cpuinfo", "r");
	if (!fp)
		return 1;
	char buf[4096] = {0};
	fread(buf, 1, sizeof(buf), fp);
	fclose(fp);

	char *lp = strstr(buf, "cpu MHz");
	if (!lp)
		return 1;
	lp += strlen("cpu MHz");
	while (*lp == ' ' || *lp == '\t' || *lp == ':')
	{
		++lp;
	}

	double mhz = atof(lp);
	unsigned long long u = (unsigned long long)(mhz * 1000);
	return u;
}
#endif

static unsigned long long GetTickMS()
{
#if defined(__LIBCO_RDTSCP__)
	static uint32_t khz = getCpuKhz();
	return counter() / khz;
#else
	struct timeval now = {0};
	gettimeofday(&now, NULL);
	unsigned long long u = now.tv_sec;
	u *= 1000;
	u += now.tv_usec / 1000;
	return u;
#endif
}

/* no longer use
static pid_t GetPid()
{
    static __thread pid_t pid = 0;
    static __thread pid_t tid = 0;
    if( !pid || !tid || pid != getpid() )
    {
        pid = getpid();
#if defined( __APPLE__ )
		tid = syscall( SYS_gettid );
		if( -1 == (long)tid )
		{
			tid = pid;
		}
#elif defined( __FreeBSD__ )
		syscall(SYS_thr_self, &tid);
		if( tid < 0 )
		{
			tid = pid;
		}
#else 
        tid = syscall( __NR_gettid );
#endif

    }
    return tid;

}
static pid_t GetPid()
{
	char **p = (char**)pthread_self();
	return p ? *(pid_t*)(p + 18) : getpid();
}
*/
/* 将一个节点从其所在的链表中删除
*参考stTimeoutItemLink_t的定义，libco中，用Link_t/TLink结构体来表示一个一条链表，
*里面包含了头节点、尾节点指针，为stTimeoutItem_t*型，每个Item_t都有一个stTimeoutItemLink_t*的变量，指向它所在的链表
*/
template <class T, class TLink>
void RemoveFromLink(T *ap) //TODO:变量名ap是什么意思？
{
	TLink *lst = ap->pLink;
	if (!lst)
		return;
	assert(lst->head && lst->tail);

	if (ap == lst->head)
	{
		lst->head = ap->pNext;
		if (lst->head)
		{
			lst->head->pPrev = NULL;
		}
	}
	else
	{
		if (ap->pPrev)
		{
			ap->pPrev->pNext = ap->pNext;
		}
	}

	if (ap == lst->tail)
	{
		lst->tail = ap->pPrev;
		if (lst->tail)
		{
			lst->tail->pNext = NULL;
		}
	}
	else
	{
		ap->pNext->pPrev = ap->pPrev; // TODO: 这里和从头部删除怎么处理不同？是上面判断多余了，还是下面有nullptr风险？ TODO:感觉是上面多余了
	}

	ap->pPrev = ap->pNext = NULL;
	ap->pLink = NULL;
}

// 向链表尾部插入一个节点
template <class TNode, class TLink>
void inline AddTail(TLink *apLink, TNode *ap)
{
	if (ap->pLink) //pLink不为空，表示ap不是一个单独的节点，已经属于某条链表，所以直接返回
	{
		return;
	}
	if (apLink->tail)
	{
		apLink->tail->pNext = (TNode *)ap;
		ap->pNext = NULL;
		ap->pPrev = apLink->tail;
		apLink->tail = ap;
	}
	else
	{
		apLink->head = apLink->tail = ap;
		ap->pNext = ap->pPrev = NULL;
	}
	ap->pLink = apLink;
}
// 将头节点从双向链表中删除（但是不释放空间，因为在实现上并非真正的链表）
template <class TNode, class TLink>
void inline PopHead(TLink *apLink)
{
	if (!apLink->head)
	{
		return;
	}
	TNode *lp = apLink->head;
	if (apLink->head == apLink->tail)
	{
		apLink->head = apLink->tail = NULL;
	}
	else
	{
		apLink->head = apLink->head->pNext;
	}

	lp->pPrev = lp->pNext = NULL;
	lp->pLink = NULL; //这里必须置NULL，如果不置NULL会导致将lp插入另一个链表时失败

	if (apLink->head)
	{
		apLink->head->pPrev = NULL;
	}
}

/*将apOther合并到apLink之后
*/
template <class TNode, class TLink>
void inline Join(TLink *apLink, TLink *apOther)
{
	//printf("apOther %p\n",apOther);
	if (!apOther->head)
	{
		return;
	}
	TNode *lp = apOther->head;
	while (lp)
	{
		lp->pLink = apLink;
		lp = lp->pNext;
	}
	lp = apOther->head;
	if (apLink->tail)
	{
		apLink->tail->pNext = (TNode *)lp;
		lp->pPrev = apLink->tail;
		apLink->tail = apOther->tail;
	}
	else
	{
		apLink->head = apOther->head;
		apLink->tail = apOther->tail;
	}

	apOther->head = apOther->tail = NULL;
}

/////////////////for copy stack //////////////////////////
// 分配共享栈内存 TODO：共享栈和非共享栈模式下都可以调用这个函数
stStackMem_t *co_alloc_stackmem(unsigned int stack_size)
{
	stStackMem_t *stack_mem = (stStackMem_t *)malloc(sizeof(stStackMem_t));
	stack_mem->occupy_co = NULL; // 一开始没协程
	stack_mem->stack_size = stack_size;
	stack_mem->stack_buffer = (char *)malloc(stack_size);
	stack_mem->stack_bp = stack_mem->stack_buffer + stack_size; // 栈底，高地址
	return stack_mem;
}

// 分配count个共享栈的空间，每个栈空间大小为stack_size TODO：什么情况下要使用多个共享栈？云风的库不是只有schedule有一个共享栈吗？
stShareStack_t *co_alloc_sharestack(int count, int stack_size)
{
	stShareStack_t *share_stack = (stShareStack_t *)malloc(sizeof(stShareStack_t));
	share_stack->alloc_idx = 0; // TODO: 默认从第0个共享栈开始使用？
	share_stack->stack_size = stack_size;

	//alloc stack array
	share_stack->count = count;
	/*C语言的标准内存分配函数：malloc，calloc，realloc。
	*malloc与calloc的区别为1块与n块的区别：
	*malloc调用形式为(类型*)malloc(size)：在内存的动态存储区中分配一块长度为“size”字节的连续区域，返回该区域的首地址。
	*calloc调用形式为(类型*)calloc(n，size)：在内存的动态存储区中分配n块长度为“size”字节的连续区域，返回首地址。calloc会将所分配的内存空间中的每一位都初始化为零,也就是说,如果你是为字符类型或整数类型的元素分配内存,那麽这些元素将保证会被初始化为0;如果你是为指针类型的元素分配内存,那麽这些元素通常会被初始化为空指针;如果你为实型数据分配内存,则这些元素会被初始化为浮点型的零。 
	*realloc调用形式为(类型*)realloc(ptr，size)：将ptr内存大小增大到size。
	*/
	stStackMem_t **stack_array = (stStackMem_t **)calloc(count, sizeof(stStackMem_t *));
	for (int i = 0; i < count; i++)
	{
		stack_array[i] = co_alloc_stackmem(stack_size); //每个栈指针指向一块stackmem
	}
	share_stack->stack_array = stack_array;
	return share_stack;
}

// 在共享栈中，获取协程的栈内存, TODO:libco中的共享栈是什么意思？和云风schedule中的栈是同一种含义吗？
// TODO: 为什么index要递增？
static stStackMem_t *co_get_stackmem(stShareStack_t *share_stack)
{
	if (!share_stack)
	{
		return NULL;
	}
	int idx = share_stack->alloc_idx % share_stack->count;
	share_stack->alloc_idx++;

	return share_stack->stack_array[idx];
}

// ----------------------------------------------------------------------------
struct stTimeoutItemLink_t;
struct stTimeoutItem_t;

// TODO: epoll结构体？
struct stCoEpoll_t
{
	/*epollfd对应内核中的一个epoll instance，epoll instance从用户态的角度来看可以看成有两条队列的容器，interest list和ready list
	*/
	int iEpollFd;							  // epoll的ID，epollfd是一个file descriptor
	static const int _EPOLL_SIZE = 1024 * 10; // epoll大小固定为10k ? 从Linux2.6.8起，epoll_create的size参数被忽略了，但必须大于0.

	struct stTimeout_t *pTimeout; // 超时管理器

	struct stTimeoutItemLink_t *pstTimeoutList; // 目前已超时的事件，仅仅作为中转使用，最后会合并到active上

	struct stTimeoutItemLink_t *pstActiveList; // 正在处理的事件

	co_epoll_res *result;
};

typedef void (*OnPreparePfn_t)(stTimeoutItem_t *, struct epoll_event &ev, stTimeoutItemLink_t *active);
typedef void (*OnProcessPfn_t)(stTimeoutItem_t *);

// 超时链表中的一项
struct stTimeoutItem_t
{

	enum
	{
		eMaxTimeout = 40 * 1000 //40s
	};
	stTimeoutItem_t *pPrev;		// 前驱
	stTimeoutItem_t *pNext;		// 后继
	stTimeoutItemLink_t *pLink; // 该链表项的首指针，也就代表该链表项所在链表

	unsigned long long ullExpireTime; //超时事件的到期时间

	OnPreparePfn_t pfnPrepare;
	OnProcessPfn_t pfnProcess;

	void *pArg; // routine

	bool bTimeout; // 是否已经超时
};
// 超时链表
struct stTimeoutItemLink_t
{
	stTimeoutItem_t *head; // 头指针
	stTimeoutItem_t *tail; // 尾指针
};
/*
* 毫秒级的超时管理器
* 使用时间轮实现，结构和hash表类似，在散列冲突时，采用链表挂载事件
* 但是是有限制的，最长超时时间不可以超过iItemSize毫秒
*/
struct stTimeout_t
{
	/*
	   时间轮
	   超时事件数组，总长度为iItemSize,每一项代表1毫秒，为一个链表，代表这个时间所超时的事件。
	   这个数组在使用的过程中，会使用取模的方式，把它当做一个循环数组来使用，虽然并不是用循环链表来实现的
	*/
	stTimeoutItemLink_t *pItems; //链表，初始化时指向一块 sizeof(stTimeoutItemLink_t) * lp->iItemSize 大小的内存

	/* TODO: 默认为60*1000ms，也就是1min 
	*iItemSize可以理解为时间轮的刻度数，每1ms占用一个刻度，同一过期时间的事件放在同一个刻度的链表里
	*/
	int iItemSize;

	unsigned long long ullStart; //目前的超时管理器最早的时间
	long long llStartIdx;		 //目前最早的时间所对应的pItems上的索引，是不断递增的，通过取模获取真正的刻度
};
stTimeout_t *AllocTimeout(int iSize)
{
	stTimeout_t *lp = (stTimeout_t *)calloc(1, sizeof(stTimeout_t));

	lp->iItemSize = iSize;
	//TODO:感觉有点绕这里，pItems指向一块内存，这块内存由iItemSize 个 stTimeoutItemLink_t（链表）组成，pItems+1指向第1条链表（从0开始计数）
	lp->pItems = (stTimeoutItemLink_t *)calloc(1, sizeof(stTimeoutItemLink_t) * lp->iItemSize);

	lp->ullStart = GetTickMS();
	lp->llStartIdx = 0;

	return lp;
}

// 释放超时管理器内存空间，先释放数组的空间
void FreeTimeout(stTimeout_t *apTimeout)
{
	//TODO:发现libco经常用malloc、free，为什么不用new、delete，配合析构函数，不是更简略吗？
	free(apTimeout->pItems);
	free(apTimeout);
}

/*
* 返回值用来标识是否正常将事件添加到超时链表，0表示正常添加，非0为添加失败
*/
int AddTimeout(stTimeout_t *apTimeout, stTimeoutItem_t *apItem, unsigned long long allNow)
{
	if (apTimeout->ullStart == 0)
	{
		apTimeout->ullStart = allNow;
		apTimeout->llStartIdx = 0;
	}

	/*标准C语言预处理要求定义某些对象宏，每个预定义宏的名称一两个下划线字符开头和结尾，这些预定义宏不能被取消定义（#undef）或由编程人员重新定义。下面预定义宏表，被我抄了下来。
	*__LINE__  ：当前程序行的行号，表示为十进制整型常量
	*__FILE__  ：当前源文件名，表示字符串型常量
	*__DATE__ ：转换的日历日期，表示为Mmm dd yyyy 形式的字符串常量，Mmm是由asctime产生的。
	*/

	//当前时间比超时链表的开始时间还小，没法添加时间轮的某个“刻度”
	if (allNow < apTimeout->ullStart)
	{
		co_log_err("CO_ERR: AddTimeout line %d allNow %llu apTimeout->ullStart %llu",
				   __LINE__, allNow, apTimeout->ullStart);

		return __LINE__;
	}
	//超时时间的过期时间比当前时间还小，说明已经过期了
	if (apItem->ullExpireTime < allNow)
	{
		co_log_err("CO_ERR: AddTimeout line %d apItem->ullExpireTime %llu allNow %llu apTimeout->ullStart %llu",
				   __LINE__, apItem->ullExpireTime, allNow, apTimeout->ullStart);

		return __LINE__;
	}
	unsigned long long diff = apItem->ullExpireTime - apTimeout->ullStart;

	//过期时间差超过了时间轮刻度
	if (diff >= (unsigned long long)apTimeout->iItemSize)
	{
		diff = apTimeout->iItemSize - 1;
		co_log_err("CO_ERR: AddTimeout line %d diff %d",
				   __LINE__, diff);

		//return __LINE__;
	}
	AddTail(apTimeout->pItems + (apTimeout->llStartIdx + diff) % apTimeout->iItemSize, apItem);

	return 0;
}
inline void TakeAllTimeout(stTimeout_t *apTimeout, unsigned long long allNow, stTimeoutItemLink_t *apResult)
{
	if (apTimeout->ullStart == 0)
	{
		apTimeout->ullStart = allNow;
		apTimeout->llStartIdx = 0;
	}

	if (allNow < apTimeout->ullStart)
	{
		return;
	}
	int cnt = allNow - apTimeout->ullStart + 1;
	if (cnt > apTimeout->iItemSize)
	{
		cnt = apTimeout->iItemSize;
	}
	if (cnt < 0)
	{
		return;
	}
	for (int i = 0; i < cnt; i++)
	{
		int idx = (apTimeout->llStartIdx + i) % apTimeout->iItemSize;
		//将两个链表合并
		Join<stTimeoutItem_t, stTimeoutItemLink_t>(apResult, apTimeout->pItems + idx);
	}
	apTimeout->ullStart = allNow;
	apTimeout->llStartIdx += cnt - 1;
}
static int CoRoutineFunc(stCoRoutine_t *co, void *)
{
	if (co->pfn)
	{
		//执行对应的函数
		co->pfn(co->arg);
	}
	co->cEnd = 1;

	stCoRoutineEnv_t *env = co->env;

	//执行完就让出CPU
	co_yield_env(env);

	return 0;
}

/**
* 根据协程管理器env, 新建一个协程，在co_create函数中进行调用
* 
* @param env - (input) 协程所在线程的环境，每个线程都有一个env
* @param attr - (input) 协程属性，目前主要是共享栈 
* @param pfn - (input) 协程所运行的函数
* @param arg - (input) 协程运行函数的参数
*/
struct stCoRoutine_t *co_create_env(stCoRoutineEnv_t *env, const stCoRoutineAttr_t *attr,
									pfn_co_routine_t pfn, void *arg)
{

	// 初始化属性。并且给默认值
	stCoRoutineAttr_t at;
	if (attr) // 用外部传入的参数来初始化协程参数
	{
		memcpy(&at, attr, sizeof(at));
	}
	if (at.stack_size <= 0) // 外部参数未给栈大小赋值，赋128K
	{
		at.stack_size = 128 * 1024;
	}
	else if (at.stack_size > 1024 * 1024 * 8) // 栈大小最大不能超过8M
	{
		at.stack_size = 1024 * 1024 * 8;
	}

	// TODO: 相当于低12位向上取整进位，为啥这么做？
	if (at.stack_size & 0xFFF)
	{
		at.stack_size &= ~0xFFF;
		at.stack_size += 0x1000;
	}

	stCoRoutine_t *lp = (stCoRoutine_t *)malloc(sizeof(stCoRoutine_t));

	memset(lp, 0, (long)(sizeof(stCoRoutine_t)));

	lp->env = env;
	lp->pfn = pfn;
	lp->arg = arg;

	stStackMem_t *stack_mem = NULL;
	if (at.share_stack)
	{
		stack_mem = co_get_stackmem(at.share_stack);
		at.stack_size = at.share_stack->stack_size;
	}
	else
	{
		stack_mem = co_alloc_stackmem(at.stack_size);
	}
	lp->stack_mem = stack_mem;

	lp->ctx.ss_sp = stack_mem->stack_buffer;
	lp->ctx.ss_size = at.stack_size;

	lp->cStart = 0;
	lp->cEnd = 0;
	lp->cIsMain = 0;
	lp->cEnableSysHook = 0;
	lp->cIsShareStack = at.share_stack != NULL;

	lp->save_size = 0;
	lp->save_buffer = NULL;

	return lp;
}

/**
* 创建一个协程对象
* 
* @param ppco - (output) 协程的地址，未初始化，需要在此函数中将其申请内存空间以及初始化工作
* @param attr - (input) 协程属性，目前主要是共享栈 
* @param pfn - (input) 协程所运行的函数
* @param arg - (input) 协程运行函数的参数
*/
int co_create(stCoRoutine_t **ppco, const stCoRoutineAttr_t *attr, pfn_co_routine_t pfn, void *arg)
{
	// 查找当前线程的管理环境
	if (!co_get_curr_thread_env())
	{
		// 如果找不到，则初始化协程
		co_init_curr_thread_env();
	}
	// 根据协程的运行环境，来创建一个协程
	stCoRoutine_t *co = co_create_env(co_get_curr_thread_env(), attr, pfn, arg);
	*ppco = co;
	return 0;
}
void co_free(stCoRoutine_t *co)
{
	if (!co->cIsShareStack)
	{
		free(co->stack_mem->stack_buffer);
		free(co->stack_mem);
	}
	//walkerdu fix at 2018-01-20
	//存在内存泄漏
	else
	{
		if (co->save_buffer)
			free(co->save_buffer);

		if (co->stack_mem->occupy_co == co)
			co->stack_mem->occupy_co = NULL;
	}

	free(co);
}
void co_release(stCoRoutine_t *co)
{
	co_free(co);
}

void co_swap(stCoRoutine_t *curr, stCoRoutine_t *pending_co);

void co_resume(stCoRoutine_t *co)
{
	stCoRoutineEnv_t *env = co->env;
	stCoRoutine_t *lpCurrRoutine = env->pCallStack[env->iCallStackSize - 1];
	if (!co->cStart)
	{
		coctx_make(&co->ctx, (coctx_pfn_t)CoRoutineFunc, co, 0);
		co->cStart = 1;
	}
	env->pCallStack[env->iCallStackSize++] = co;
	co_swap(lpCurrRoutine, co);
}

// walkerdu 2018-01-14
// 用于reset超时无法重复使用的协程
void co_reset(stCoRoutine_t *co)
{
	if (!co->cStart || co->cIsMain)
		return;

	co->cStart = 0;
	co->cEnd = 0;

	// 如果当前协程有共享栈被切出的buff，要进行释放
	if (co->save_buffer)
	{
		free(co->save_buffer);
		co->save_buffer = NULL;
		co->save_size = 0;
	}

	// 如果共享栈被当前协程占用，要释放占用标志，否则被切换，会执行save_stack_buffer()
	if (co->stack_mem->occupy_co == co)
		co->stack_mem->occupy_co = NULL;
}

void co_yield_env(stCoRoutineEnv_t *env)
{

	stCoRoutine_t *last = env->pCallStack[env->iCallStackSize - 2];
	stCoRoutine_t *curr = env->pCallStack[env->iCallStackSize - 1];

	//TODO:协程调用yield表示当前协程已经执行完任务，所以iCallStackSize减1，libco是非抢占式的协程？
	env->iCallStackSize--;

	co_swap(curr, last);
}

void co_yield_ct()
{

	co_yield_env(co_get_curr_thread_env());
}
void co_yield(stCoRoutine_t *co)
{
	co_yield_env(co->env);
}

void save_stack_buffer(stCoRoutine_t *occupy_co)
{
	///copy out
	stStackMem_t *stack_mem = occupy_co->stack_mem;
	int len = stack_mem->stack_bp - occupy_co->stack_sp;

	if (occupy_co->save_buffer)
	{
		free(occupy_co->save_buffer), occupy_co->save_buffer = NULL;
	}

	occupy_co->save_buffer = (char *)malloc(len); //malloc buf;
	occupy_co->save_size = len;

	memcpy(occupy_co->save_buffer, occupy_co->stack_sp, len);
}

void co_swap(stCoRoutine_t *curr, stCoRoutine_t *pending_co)
{
	stCoRoutineEnv_t *env = co_get_curr_thread_env();

	//get curr stack sp
	char c;
	curr->stack_sp = &c;

	if (!pending_co->cIsShareStack)
	{
		env->pending_co = NULL;
		env->occupy_co = NULL;
	}
	else
	{
		env->pending_co = pending_co;
		//get last occupy co on the same stack mem
		stCoRoutine_t *occupy_co = pending_co->stack_mem->occupy_co;
		//set pending co to occupy thest stack mem;
		pending_co->stack_mem->occupy_co = pending_co;

		env->occupy_co = occupy_co;
		if (occupy_co && occupy_co != pending_co)
		{
			save_stack_buffer(occupy_co);
		}
	}

	//swap context
	coctx_swap(&(curr->ctx), &(pending_co->ctx));

	//stack buffer may be overwrite, so get again;
	stCoRoutineEnv_t *curr_env = co_get_curr_thread_env();
	stCoRoutine_t *update_occupy_co = curr_env->occupy_co;
	stCoRoutine_t *update_pending_co = curr_env->pending_co;

	if (update_occupy_co && update_pending_co && update_occupy_co != update_pending_co)
	{
		//resume stack buffer
		if (update_pending_co->save_buffer && update_pending_co->save_size > 0)
		{
			memcpy(update_pending_co->stack_sp, update_pending_co->save_buffer, update_pending_co->save_size);
		}
	}
}

//int poll(struct pollfd fds[], nfds_t nfds, int timeout);
// { fd,events,revents }
struct stPollItem_t;
struct stPoll_t : public stTimeoutItem_t
{
	struct pollfd *fds;
	nfds_t nfds; // typedef unsigned long int nfds_t;

	stPollItem_t *pPollItems;

	int iAllEventDetach;

	int iEpollFd;

	int iRaiseCnt;
};
struct stPollItem_t : public stTimeoutItem_t
{
	struct pollfd *pSelf;
	stPoll_t *pPoll;

	struct epoll_event stEvent;
};
/*
 *   EPOLLPRI 		POLLPRI    // There is urgent data to read.  
 *   EPOLLMSG 		POLLMSG
 *
 *   				POLLREMOVE
 *   				POLLRDHUP
 *   				POLLNVAL
 *
 * */
static uint32_t PollEvent2Epoll(short events)
{
	uint32_t e = 0;
	if (events & POLLIN)
		e |= EPOLLIN;
	if (events & POLLOUT)
		e |= EPOLLOUT;
	if (events & POLLHUP)
		e |= EPOLLHUP;
	if (events & POLLERR)
		e |= EPOLLERR;
	if (events & POLLRDNORM)
		e |= EPOLLRDNORM;
	if (events & POLLWRNORM)
		e |= EPOLLWRNORM;
	return e;
}
static short EpollEvent2Poll(uint32_t events)
{
	short e = 0;
	if (events & EPOLLIN)
		e |= POLLIN;
	if (events & EPOLLOUT)
		e |= POLLOUT;
	if (events & EPOLLHUP)
		e |= POLLHUP;
	if (events & EPOLLERR)
		e |= POLLERR;
	if (events & EPOLLRDNORM)
		e |= POLLRDNORM;
	if (events & EPOLLWRNORM)
		e |= POLLWRNORM;
	return e;
}

// 线程私有变量，一个线程维护一个独立的
static __thread stCoRoutineEnv_t *gCoEnvPerThread = NULL;

void co_init_curr_thread_env()
{
	gCoEnvPerThread = (stCoRoutineEnv_t *)calloc(1, sizeof(stCoRoutineEnv_t));
	stCoRoutineEnv_t *env = gCoEnvPerThread;

	env->iCallStackSize = 0;
	struct stCoRoutine_t *self = co_create_env(env, NULL, NULL, NULL);
	self->cIsMain = 1;

	env->pending_co = NULL;
	env->occupy_co = NULL;

	coctx_init(&self->ctx);

	env->pCallStack[env->iCallStackSize++] = self;

	stCoEpoll_t *ev = AllocEpoll();
	SetEpoll(env, ev);
}
stCoRoutineEnv_t *co_get_curr_thread_env()
{
	return gCoEnvPerThread;
}

void OnPollProcessEvent(stTimeoutItem_t *ap)
{
	stCoRoutine_t *co = (stCoRoutine_t *)ap->pArg;
	co_resume(co);
}

void OnPollPreparePfn(stTimeoutItem_t *ap, struct epoll_event &e, stTimeoutItemLink_t *active)
{
	stPollItem_t *lp = (stPollItem_t *)ap;
	lp->pSelf->revents = EpollEvent2Poll(e.events);

	stPoll_t *pPoll = lp->pPoll;
	pPoll->iRaiseCnt++;

	if (!pPoll->iAllEventDetach)
	{
		pPoll->iAllEventDetach = 1;

		RemoveFromLink<stTimeoutItem_t, stTimeoutItemLink_t>(pPoll);

		AddTail(active, pPoll);
	}
}

void co_eventloop(stCoEpoll_t *ctx, pfn_co_eventloop_t pfn, void *arg)
{
	if (!ctx->result)
	{
		ctx->result = co_epoll_res_alloc(stCoEpoll_t::_EPOLL_SIZE);
	}
	co_epoll_res *result = ctx->result;

	for (;;)
	{
		int ret = co_epoll_wait(ctx->iEpollFd, result, stCoEpoll_t::_EPOLL_SIZE, 1);

		stTimeoutItemLink_t *active = (ctx->pstActiveList);
		stTimeoutItemLink_t *timeout = (ctx->pstTimeoutList);

		memset(timeout, 0, sizeof(stTimeoutItemLink_t));

		for (int i = 0; i < ret; i++)
		{
			stTimeoutItem_t *item = (stTimeoutItem_t *)result->events[i].data.ptr;
			if (item->pfnPrepare)
			{
				item->pfnPrepare(item, result->events[i], active);
			}
			else
			{
				AddTail(active, item);
			}
		}

		unsigned long long now = GetTickMS();
		TakeAllTimeout(ctx->pTimeout, now, timeout);

		stTimeoutItem_t *lp = timeout->head;
		while (lp)
		{
			//printf("raise timeout %p\n",lp);
			lp->bTimeout = true;
			lp = lp->pNext;
		}

		Join<stTimeoutItem_t, stTimeoutItemLink_t>(active, timeout);

		lp = active->head;
		while (lp)
		{

			PopHead<stTimeoutItem_t, stTimeoutItemLink_t>(active);
			if (lp->bTimeout && now < lp->ullExpireTime)
			{
				int ret = AddTimeout(ctx->pTimeout, lp, now);
				if (!ret)
				{
					lp->bTimeout = false;
					lp = active->head;
					continue;
				}
			}
			if (lp->pfnProcess)
			{
				lp->pfnProcess(lp);
			}

			lp = active->head;
		}
		if (pfn)
		{
			if (-1 == pfn(arg))
			{
				break;
			}
		}
	}
}
void OnCoroutineEvent(stTimeoutItem_t *ap)
{
	stCoRoutine_t *co = (stCoRoutine_t *)ap->pArg;
	co_resume(co);
}

stCoEpoll_t *AllocEpoll()
{
	stCoEpoll_t *ctx = (stCoEpoll_t *)calloc(1, sizeof(stCoEpoll_t));

	ctx->iEpollFd = co_epoll_create(stCoEpoll_t::_EPOLL_SIZE);
	ctx->pTimeout = AllocTimeout(60 * 1000);

	ctx->pstActiveList = (stTimeoutItemLink_t *)calloc(1, sizeof(stTimeoutItemLink_t));
	ctx->pstTimeoutList = (stTimeoutItemLink_t *)calloc(1, sizeof(stTimeoutItemLink_t));

	return ctx;
}

void FreeEpoll(stCoEpoll_t *ctx)
{
	if (ctx)
	{
		free(ctx->pstActiveList);
		free(ctx->pstTimeoutList);
		FreeTimeout(ctx->pTimeout);
		co_epoll_res_free(ctx->result);
	}
	free(ctx);
}

// 从协程的运行环境中取当前协程
stCoRoutine_t *GetCurrCo(stCoRoutineEnv_t *env)
{
	return env->pCallStack[env->iCallStackSize - 1]; // 调用栈的栈顶即为当前协程
}
stCoRoutine_t *GetCurrThreadCo()
{
	stCoRoutineEnv_t *env = co_get_curr_thread_env();
	if (!env)
		return 0;
	return GetCurrCo(env);
}

typedef int (*poll_pfn_t)(struct pollfd fds[], nfds_t nfds, int timeout);
int co_poll_inner(stCoEpoll_t *ctx, struct pollfd fds[], nfds_t nfds, int timeout, poll_pfn_t pollfunc)
{
	if (timeout == 0)
	{
		return pollfunc(fds, nfds, timeout);
	}
	if (timeout < 0)
	{
		timeout = INT_MAX;
	}
	int epfd = ctx->iEpollFd;
	stCoRoutine_t *self = co_self();

	//1.struct change
	stPoll_t &arg = *((stPoll_t *)malloc(sizeof(stPoll_t)));
	memset(&arg, 0, sizeof(arg));

	arg.iEpollFd = epfd;
	arg.fds = (pollfd *)calloc(nfds, sizeof(pollfd));
	arg.nfds = nfds;

	stPollItem_t arr[2];
	if (nfds < sizeof(arr) / sizeof(arr[0]) && !self->cIsShareStack)
	{
		arg.pPollItems = arr;
	}
	else
	{
		arg.pPollItems = (stPollItem_t *)malloc(nfds * sizeof(stPollItem_t));
	}
	memset(arg.pPollItems, 0, nfds * sizeof(stPollItem_t));

	arg.pfnProcess = OnPollProcessEvent;
	arg.pArg = GetCurrCo(co_get_curr_thread_env());

	//2. add epoll
	for (nfds_t i = 0; i < nfds; i++)
	{
		arg.pPollItems[i].pSelf = arg.fds + i;
		arg.pPollItems[i].pPoll = &arg;

		arg.pPollItems[i].pfnPrepare = OnPollPreparePfn;
		struct epoll_event &ev = arg.pPollItems[i].stEvent;

		if (fds[i].fd > -1)
		{
			ev.data.ptr = arg.pPollItems + i;
			ev.events = PollEvent2Epoll(fds[i].events);

			int ret = co_epoll_ctl(epfd, EPOLL_CTL_ADD, fds[i].fd, &ev);
			if (ret < 0 && errno == EPERM && nfds == 1 && pollfunc != NULL)
			{
				if (arg.pPollItems != arr)
				{
					free(arg.pPollItems);
					arg.pPollItems = NULL;
				}
				free(arg.fds);
				free(&arg);
				return pollfunc(fds, nfds, timeout);
			}
		}
		//if fail,the timeout would work
	}

	//3.add timeout

	unsigned long long now = GetTickMS();
	arg.ullExpireTime = now + timeout;
	int ret = AddTimeout(ctx->pTimeout, &arg, now);
	int iRaiseCnt = 0;
	if (ret != 0)
	{
		co_log_err("CO_ERR: AddTimeout ret %d now %lld timeout %d arg.ullExpireTime %lld",
				   ret, now, timeout, arg.ullExpireTime);
		errno = EINVAL;
		iRaiseCnt = -1;
	}
	else
	{
		co_yield_env(co_get_curr_thread_env());
		iRaiseCnt = arg.iRaiseCnt;
	}

	{
		//clear epoll status and memory
		RemoveFromLink<stTimeoutItem_t, stTimeoutItemLink_t>(&arg);
		for (nfds_t i = 0; i < nfds; i++)
		{
			int fd = fds[i].fd;
			if (fd > -1)
			{
				co_epoll_ctl(epfd, EPOLL_CTL_DEL, fd, &arg.pPollItems[i].stEvent);
			}
			fds[i].revents = arg.fds[i].revents;
		}

		if (arg.pPollItems != arr)
		{
			free(arg.pPollItems);
			arg.pPollItems = NULL;
		}

		free(arg.fds);
		free(&arg);
	}

	return iRaiseCnt;
}

int co_poll(stCoEpoll_t *ctx, struct pollfd fds[], nfds_t nfds, int timeout_ms)
{
	return co_poll_inner(ctx, fds, nfds, timeout_ms, NULL);
}

void SetEpoll(stCoRoutineEnv_t *env, stCoEpoll_t *ev)
{
	env->pEpoll = ev;
}
stCoEpoll_t *co_get_epoll_ct()
{
	if (!co_get_curr_thread_env())
	{
		co_init_curr_thread_env();
	}
	return co_get_curr_thread_env()->pEpoll;
}
struct stHookPThreadSpec_t
{
	stCoRoutine_t *co;
	void *value;

	enum
	{
		size = 1024
	};
};
void *co_getspecific(pthread_key_t key)
{
	stCoRoutine_t *co = GetCurrThreadCo();
	if (!co || co->cIsMain)
	{
		return pthread_getspecific(key);
	}
	return co->aSpec[key].value;
}
int co_setspecific(pthread_key_t key, const void *value)
{
	stCoRoutine_t *co = GetCurrThreadCo();
	if (!co || co->cIsMain)
	{
		return pthread_setspecific(key, value);
	}
	co->aSpec[key].value = (void *)value;
	return 0;
}

void co_disable_hook_sys()
{
	stCoRoutine_t *co = GetCurrThreadCo();
	if (co)
	{
		co->cEnableSysHook = 0;
	}
}
bool co_is_enable_sys_hook()
{
	stCoRoutine_t *co = GetCurrThreadCo();
	return (co && co->cEnableSysHook);
}

stCoRoutine_t *co_self()
{
	return GetCurrThreadCo();
}

//co cond
struct stCoCond_t;
struct stCoCondItem_t
{
	stCoCondItem_t *pPrev;
	stCoCondItem_t *pNext;
	stCoCond_t *pLink;

	stTimeoutItem_t timeout;
};
struct stCoCond_t
{
	stCoCondItem_t *head;
	stCoCondItem_t *tail;
};
static void OnSignalProcessEvent(stTimeoutItem_t *ap)
{
	stCoRoutine_t *co = (stCoRoutine_t *)ap->pArg;
	co_resume(co);
}

stCoCondItem_t *co_cond_pop(stCoCond_t *link); // 提前把空结构体声明放前面是防止编译报错
//
int co_cond_signal(stCoCond_t *si)
{
	stCoCondItem_t *sp = co_cond_pop(si);
	if (!sp)
	{
		return 0;
	}
	RemoveFromLink<stTimeoutItem_t, stTimeoutItemLink_t>(&sp->timeout);

	AddTail(co_get_curr_thread_env()->pEpoll->pstActiveList, &sp->timeout);

	return 0;
}
int co_cond_broadcast(stCoCond_t *si)
{
	for (;;)
	{
		stCoCondItem_t *sp = co_cond_pop(si);
		if (!sp)
			return 0;

		RemoveFromLink<stTimeoutItem_t, stTimeoutItemLink_t>(&sp->timeout);

		AddTail(co_get_curr_thread_env()->pEpoll->pstActiveList, &sp->timeout);
	}

	return 0;
}

int co_cond_timedwait(stCoCond_t *link, int ms)
{
	stCoCondItem_t *psi = (stCoCondItem_t *)calloc(1, sizeof(stCoCondItem_t));
	psi->timeout.pArg = GetCurrThreadCo();
	psi->timeout.pfnProcess = OnSignalProcessEvent;

	if (ms > 0)
	{
		unsigned long long now = GetTickMS();
		psi->timeout.ullExpireTime = now + ms;

		int ret = AddTimeout(co_get_curr_thread_env()->pEpoll->pTimeout, &psi->timeout, now);
		if (ret != 0)
		{
			free(psi);
			return ret;
		}
	}
	AddTail(link, psi);

	co_yield_ct();

	RemoveFromLink<stCoCondItem_t, stCoCond_t>(psi);
	free(psi);

	return 0;
}
stCoCond_t *co_cond_alloc()
{
	return (stCoCond_t *)calloc(1, sizeof(stCoCond_t));
}
int co_cond_free(stCoCond_t *cc)
{
	free(cc);
	return 0;
}

// 将信号量结构体链表中的头节点弹出，并返回弹出的头指针
stCoCondItem_t *co_cond_pop(stCoCond_t *link)
{
	stCoCondItem_t *p = link->head;
	if (p)
	{
		PopHead<stCoCondItem_t, stCoCond_t>(link);
	}
	return p;
}
