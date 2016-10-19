#include "co_routine.h"
#include "co_routine_inner.h"

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <string>

#include <poll.h>
#include <unistd.h>
#include <sys/epoll.h>
#include <sys/time.h>
#include <errno.h>

#include <assert.h>

#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/syscall.h>

#include <sys/mman.h>
#include <execinfo.h>
#include <sys/time.h>
extern "C"
{
	//extern void co_swapcontext(ucontext_t *, ucontext_t *) asm("co_swapcontext");
	extern void coctx_swap(coctx_t*, coctx_t*) asm("coctx_swap");
}
using namespace std;
static pfn_sys_getspecific_t g_pfnSysGetSpecific = NULL;
static pfn_sys_setspecific_t g_pfnSysSetSpecific = NULL;

static pfn_co_eventloop_start_t g_pfnEventLoopStart = NULL;
static pfn_co_eventloop_end_t g_pfnEventLoopEnd = NULL;

struct stBeforeYieldCb_t
{
	pfn_before_yield_cb_t cb;
	void *arg;
};
static stBeforeYieldCb_t g_arrBeforeYieldCb[ 100 ];
static int g_iBeforeYieldCbLen = 0;
void fire_before_yield_cb()
{
	for( int i=0;i<g_iBeforeYieldCbLen;i++ )
	{
		stBeforeYieldCb_t & c = g_arrBeforeYieldCb[i];
		if( !c.cb ) break;
		c.cb( c.arg );
	}
}

stCoRoutine_t *GetCurrCo( stCoRoutineEnv_t *env );
struct stCoEpoll_t;
struct stStackMemEnv_t;

struct stCoRoutineEnv_t
{
	stCoRoutine_t *pCallStack[ 128 ];
	int iCallStackSize;
	stCoEpoll_t *pEpoll;

	//for copy stack
	stStackMemEnv_t* pMemEnv;
	//for stat
	stCoRoutineStat_t* pStat;

	//for copy stack log lastco and nextco
	stCoRoutine_t* ptNextCo;
	stCoRoutine_t* ptLastCo;

	stCoRoutine_t *arrCoRoutine[ 20480 ];
	int iCoRoutineCnt;
};
//int socket(int domain, int type, int protocol);
void co_log_err( const char *fmt,... )
{
}


static unsigned long long counter(void);
static unsigned long long getCpuKhz()
{
	FILE *fp = fopen("/proc/cpuinfo","r");
	if(!fp) return 1;
	char buf[4096] = {0};
	fread(buf,1,sizeof(buf),fp);
	fclose(fp);

	char *lp = strstr(buf,"cpu MHz");
	if(!lp) return 1;
	lp += strlen("cpu MHz");
	while(*lp == ' ' || *lp == '\t' || *lp == ':')
	{
		++lp;
	}

	double mhz = atof(lp);
	unsigned long long u = (unsigned long long)(mhz * 1000);
	return u;
}

/*  
* Notice!
* Can't use GetTimeOfDay for time adjusted!
*/
static unsigned long long GetTickMS()
{
	unsigned long long result = 0;
	static uint32_t khz = getCpuKhz();
	result = counter() / khz;
	return result;
}

static unsigned long long counter(void)
{
	register uint32_t lo, hi;
	register unsigned long long o;
    __asm__ __volatile__ (
        "rdtscp" : "=a"(lo), "=d"(hi)
        );
	o = hi;
	o <<= 32;
	return (o | lo);
}
static pid_t GetPid()
{
 	static __thread pid_t pid = 0;
	static __thread pid_t tid = 0;
	if( !pid || !tid || pid != getpid() )
	{
		pid = getpid();
		tid = syscall( __NR_gettid );
	}
	return tid;

}
/*
static pid_t GetPid()
{
	char **p = (char**)pthread_self();
	return p ? *(pid_t*)(p + 18) : getpid();
}
*/
template <class T,class TLink>
void RemoveFromLink(T *ap)
{
	TLink *lst = ap->pLink;
	if(!lst) return ;
	assert( lst->head && lst->tail );

	if( ap == lst->head )
	{
		lst->head = ap->pNext;
		if(lst->head)
		{
			lst->head->pPrev = NULL;
		}
	}
	else
	{
		if(ap->pPrev)
		{
			ap->pPrev->pNext = ap->pNext;
		}
	}

	if( ap == lst->tail )
	{
		lst->tail = ap->pPrev;
		if(lst->tail)
		{
			lst->tail->pNext = NULL;
		}
	}
	else
	{
		ap->pNext->pPrev = ap->pPrev;
	}

	ap->pPrev = ap->pNext = NULL;
	ap->pLink = NULL;
}

template <class TNode,class TLink>
void inline AddTail(TLink*apLink,TNode *ap)
{
	if( ap->pLink )
	{
		return ;
	}
	if(apLink->tail)
	{
		apLink->tail->pNext = (TNode*)ap;
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
template <class TNode,class TLink>
void inline PopHead( TLink*apLink )
{
	if( !apLink->head ) 
	{
		return ;
	}
	TNode *lp = apLink->head;
	if( apLink->head == apLink->tail )
	{
		apLink->head = apLink->tail = NULL;
	}
	else
	{
		apLink->head = apLink->head->pNext;
	}

	lp->pPrev = lp->pNext = NULL;
	lp->pLink = NULL;

	if( apLink->head )
	{
		apLink->head->pPrev = NULL;
	}
}

template <class TNode,class TLink>
void inline Join( TLink*apLink,TLink *apOther )
{
	//printf("apOther %p\n",apOther);
	if( !apOther->head )
	{
		return ;
	}
	TNode *lp = apOther->head;
	while( lp )
	{
		lp->pLink = apLink;
		lp = lp->pNext;
	}
	lp = apOther->head;
	if(apLink->tail)
	{
		apLink->tail->pNext = (TNode*)lp;
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
static bool g_bEnableShareStack = false;
void co_enable_share_stack(bool bEnableShareStack)
{
	g_bEnableShareStack = bEnableShareStack;
}

struct stStackMem_t
{
	char* stack;
	char* stack_bp;
	stCoRoutine_t* co;
};
struct stStackMemEnv_t
{
	int count;
	unsigned int alloc_idx;
	int stack_size;
	stStackMem_t* mem;
};
stStackMemEnv_t* co_alloc_stackmemenv(int iCount, int iStackSize)
{
	stStackMemEnv_t* memenv = (stStackMemEnv_t*)malloc(sizeof(stStackMemEnv_t));
	if (!memenv)
	{
		return NULL;
	}
	memenv->count = iCount;
	memenv->alloc_idx = 0;
	memenv->stack_size = iStackSize;
	memenv->mem = (stStackMem_t*)malloc(iCount * sizeof(stStackMem_t));
	if (!memenv->mem)
	{
		free(memenv);
		return NULL;
	}
	for (int i = 0; i < iCount; i++)
	{
		memenv->mem[i].stack = (char*)malloc(iStackSize);
		if (!memenv->mem[i].stack)
		{
			free(memenv->mem);
			free(memenv);
			return NULL;
		}
		memenv->mem[i].stack_bp = memenv->mem[i].stack + iStackSize;
		memenv->mem[i].co = NULL;
	}
	return memenv;
}
void co_copy_stack_out(stCoRoutine_t* last, stCoRoutine_t* next, stCoRoutineEnv_t* env)
{
	///copy out
	char* pRunStackBP = last->pDynamicStack->stack_bp;
	int len = pRunStackBP - last->pRunStackSP;

	if (!last->pBuffer || last->iStackCapacity < len)
	{
		//align size to pow(2,n);
		int align_len= 128;
		while (align_len < len)
		{
			align_len *= 2;
		}
		if (last->pBuffer)
		{
			free(last->pBuffer), last->pBuffer = NULL;
			env->pStat->alloc_buffer_size -= last->iStackCapacity;
		}
		last->pBuffer = (char*)malloc(align_len); //malloc buf;
		last->iStackCapacity = align_len;
		env->pStat->alloc_buffer_size += align_len;
	}

	memcpy(last->pBuffer, last->pRunStackSP, len);
	//printf("stack_capacity %llu copy out %llu\n", last->iStackCapacity, len);
	last->iStackLen = len;
	env->pStat->copy_out_size += len;
	env->pStat->copy_out_count++;

}

void co_swap_wrapper(stCoRoutine_t* curr, stCoRoutine_t* next, stCoRoutineEnv_t* env)
{
	if (!g_bEnableShareStack)
	{
		env->pStat->normal_resume_count++;
		coctx_swap(&curr->ctx, &next->ctx);
		return;
	}
	char c;
	curr->pRunStackSP = &c;

	env->ptNextCo = NULL;
	env->ptLastCo = NULL;

	do 
	{
		if (!next->pDynamicStack)
		{
			env->pStat->normal_resume_count++;
			break;
		}
		env->pStat->copy_resume_count++;

		env->ptNextCo = next;
		//get current co stack;

		//save stack to mem;
		stCoRoutine_t* last = next->pDynamicStack->co;
		env->ptLastCo = last;

		if (!last || last == next)
		{
			env->pStat->hit_count++;
		}
		else
		{
			env->pStat->not_hit_count++;
			//resume stack from mem;
			co_copy_stack_out(last, next, env);
		}
		next->pDynamicStack->co = next;
	} while (0);

	//swap context
	coctx_swap(&(curr->ctx),&(next->ctx) );
	//co_swapcontext( &(curr->ctx),&(next->ctx) );

	volatile stCoRoutineEnv_t* last_stack_env = co_get_curr_thread_env();

	if (last_stack_env->ptLastCo && last_stack_env->ptNextCo && last_stack_env->ptLastCo != last_stack_env->ptNextCo)
	{
		//copy in
		if (last_stack_env->ptNextCo->pBuffer && last_stack_env->ptNextCo->iStackLen > 0)
		{
			memcpy(last_stack_env->ptNextCo->pRunStackSP, last_stack_env->ptNextCo->pBuffer, last_stack_env->ptNextCo->iStackLen);
			volatile stCoRoutineEnv_t* curr_stack_env = co_get_curr_thread_env();
			curr_stack_env->pStat->copy_in_size += curr_stack_env->ptNextCo->iStackLen;
			curr_stack_env->pStat->copy_in_count++;
		}
	}
}

stStackMem_t* co_get_stackmem(stCoRoutine_t* co, stStackMemEnv_t* memenv)
{
	if (!memenv || !co)
	{
		return NULL;
	}

	int idx = memenv->alloc_idx % memenv->count;
	memenv->alloc_idx++;

	co->pStackEnv = memenv;
	co->pDynamicStack = (memenv->mem + idx);

	return (memenv->mem + idx);
}

// ----------------------------------------------------------------------------
struct stTimeoutItemLink_t;
struct stTimeoutItem_t;
struct stCoEpoll_t
{
	int iEpollFd;
	static const int _EPOLL_SIZE = 1024 * 10;

	struct stTimeout_t *pTimeout;

	struct stTimeoutItemLink_t *pstTimeoutList;

	struct stTimeoutItemLink_t *pstActiveList;

};
typedef void (*OnPreparePfn_t)( stTimeoutItem_t *,struct epoll_event &ev, stTimeoutItemLink_t *active );
typedef void (*OnProcessPfn_t)( stTimeoutItem_t *);
struct stTimeoutItem_t
{

	enum
	{
		eMaxTimeout = 40 * 1000 //40s
	};
	stTimeoutItem_t *pPrev;
	stTimeoutItem_t *pNext;
	stTimeoutItemLink_t *pLink;

	unsigned long long ullExpireTime;

	OnPreparePfn_t pfnPrepare;
	OnProcessPfn_t pfnProcess;

	void *pArg; // routine 
	bool bTimeout;
};
struct stTimeoutItemLink_t
{
	stTimeoutItem_t *head;
	stTimeoutItem_t *tail;

};
struct stTimeout_t
{
	stTimeoutItemLink_t *pItems;
	int iItemSize;

	unsigned long long ullStart;
	long long llStartIdx;
};
stTimeout_t *AllocTimeout( int iSize )
{
	stTimeout_t *lp = (stTimeout_t*)calloc( 1,sizeof(stTimeout_t) );	

	lp->iItemSize = iSize;
	lp->pItems = (stTimeoutItemLink_t*)calloc( 1,sizeof(stTimeoutItemLink_t) * lp->iItemSize );

	lp->ullStart = GetTickMS();
	lp->llStartIdx = 0;

	return lp;
}
void FreeTimeout( stTimeout_t *apTimeout )
{
	free( apTimeout->pItems );
	free ( apTimeout );
}
int AddTimeout( stTimeout_t *apTimeout,stTimeoutItem_t *apItem ,unsigned long long allNow )
{
	if( apTimeout->ullStart == 0 )
	{
		apTimeout->ullStart = allNow;
		apTimeout->llStartIdx = 0;
	}
	if( allNow < apTimeout->ullStart )
	{
		co_log_err("CO_ERR: AddTimeout line %d allNow %llu apTimeout->ullStart %llu",
					__LINE__,allNow,apTimeout->ullStart);

		return __LINE__;
	}
	if( apItem->ullExpireTime < allNow )
	{
		co_log_err("CO_ERR: AddTimeout line %d apItem->ullExpireTime %llu allNow %llu apTimeout->ullStart %llu",
					__LINE__,apItem->ullExpireTime,allNow,apTimeout->ullStart);

		return __LINE__;
	}
	int diff = apItem->ullExpireTime - apTimeout->ullStart;

	if( diff >= apTimeout->iItemSize )
	{
		diff = apTimeout->iItemSize - 1;
		co_log_err("CO_ERR: AddTimeout line %d diff %d",
					__LINE__,diff);
		//return __LINE__;
	}
	AddTail( apTimeout->pItems + ( apTimeout->llStartIdx + diff ) % apTimeout->iItemSize , apItem );

	return 0;
}
inline void TakeAllTimeout( stTimeout_t *apTimeout,unsigned long long allNow,stTimeoutItemLink_t *apResult )
{
	if( apTimeout->ullStart == 0 )
	{
		apTimeout->ullStart = allNow;
		apTimeout->llStartIdx = 0;
	}

	if( allNow < apTimeout->ullStart )
	{
		return ;
	}
	int cnt = allNow - apTimeout->ullStart + 1;
	if( cnt > apTimeout->iItemSize )
	{
		cnt = apTimeout->iItemSize;
	}
	if( cnt < 0 )
	{
		return;
	}
	for( int i = 0;i<cnt;i++)
	{
		int idx = ( apTimeout->llStartIdx + i) % apTimeout->iItemSize;
		Join<stTimeoutItem_t,stTimeoutItemLink_t>( apResult,apTimeout->pItems + idx  );
	}
	apTimeout->ullStart = allNow;
	apTimeout->llStartIdx += cnt - 1;


}


static void CoRoutineFunc( stCoRoutine_t* co, void* )
{
	/*
	unsigned long p = (uint32_t)co_high;
	p <<= 32;
	p |= (uint32_t)co_low;

	stCoRoutine_t *co = (stCoRoutine_t*)p;
	*/
	
	if( co->pfn )
	{
		co->pfn( co->arg );
	}
	co->cEnd = 1;

	stCoRoutineEnv_t *env = co->env;

	co_yield_env( env );
}
/*
struct stCoRoutine_t *co_create_env( stCoRoutineEnv_t * env,
									 const stCoRoutineAttr_t *attr,
									 pfn_co_routine_t pfn,void *arg )
{
	stCoRoutineAttr_t at = { 0 };
	if( attr )
	{
		memcpy( &at,attr,sizeof(at) );
	}
	if( at.stack_size <= 0 )
	{
		at.stack_size = 128 * 1024;
	}
	else if( at.stack_size > 1024 * 1024 * 8 )
	{
		at.stack_size = 1024 * 1024 * 8;
	}
	int len = (int)sizeof( stCoRoutine_t ) + at.stack_size;
	stCoRoutine_t *lp = (stCoRoutine_t*)malloc( len );

	printf("malloc len %d stack %d\n",len,at.stack_size );

	memset( lp,0,(long)((stCoRoutine_t*)0)->sRunStack );

	lp->env = env;
	lp->pfn = pfn;
	lp->arg = arg;

	lp->aSpecPtr[0] = lp->aSpecInner;
	lp->aSpecPtr[ 1024 >> 7 ] = lp->aSpecInner2;

	lp->ctx.uc_stack.ss_sp = lp->sRunStack ;
	lp->ctx.uc_stack.ss_size = len - sizeof(stCoRoutine_t) ;


	return lp;
}
*/
static bool g_bUseProtect = true;
void SetUseProtect(bool bUseProtect)
{
	g_bUseProtect = bUseProtect;
}



struct stCoRoutine_t *co_create_env( stCoRoutineEnv_t * env,
									 const stCoRoutineAttr_t *attr,
									 pfn_co_routine_t pfn,void *arg )
{
	stCoRoutineAttr_t at = { 0 };
	if( attr )
	{
		memcpy( &at,attr,sizeof(at) );
	}
	if( at.stack_size <= 0 )
	{
		at.stack_size = 128 * 1024;
	}
	else if( at.stack_size > 1024 * 1024 * 8 )
	{
		at.stack_size = 1024 * 1024 * 8;
	}

	if( at.stack_size & 0xFFF ) 
	{
		at.stack_size &= ~0xFFF;
		at.stack_size += 0x1000;
	}

	const int iPageSize = 4096;

	stCoRoutine_t *lp = (stCoRoutine_t*)malloc( sizeof(stCoRoutine_t) );

	memset( lp,0,(long)(sizeof(stCoRoutine_t)));

	char *pBuffer = 0;
	char *pStackBuffer = 0;
	if (at.use_share_stack && g_bEnableShareStack)
	{
		stStackMem_t* sharestack = NULL;
		if (at.stack_env)
		{
			sharestack = co_get_stackmem(lp, at.stack_env);
			at.stack_size = at.stack_env->stack_size;
		}
		else
		{
			if (!env->pMemEnv)
			{
				env->pMemEnv = co_alloc_stackmemenv(50, 128 * 1024);
			}
			sharestack = co_get_stackmem(lp, env->pMemEnv);
			at.stack_size = env->pMemEnv->stack_size;
		}
		assert(sharestack != NULL);
		pStackBuffer = sharestack->stack;
		env->pStat->use_share_stack_cnt++;
	}
	else if( at.no_protect_stack || !g_bUseProtect)
	{
		pBuffer = (char*)malloc( at.stack_size );
		assert( pBuffer );
		pStackBuffer = pBuffer;
	}
	else
	{
		pBuffer = (char*)mmap(NULL, 2 * iPageSize + at.stack_size , 
					PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_PRIVATE , -1, 0 );

		if( pBuffer == (void*)-1 || pBuffer == 0 )
		{
			assert( false );
		}
		mprotect( pBuffer, iPageSize , PROT_NONE );
		mprotect( pBuffer + iPageSize + at.stack_size, iPageSize , PROT_NONE );

		pStackBuffer = pBuffer + iPageSize;

	}

	lp->cNoProtectStack = at.no_protect_stack;
	lp->pRunStack = pStackBuffer;
	lp->pRunStackSP = pStackBuffer + (at.stack_size - sizeof(long));

	lp->env = env;
	lp->pfn = pfn;
	lp->arg = arg;

	/*
	lp->aSpecPtr[0] = lp->aSpecInner;
	lp->aSpecPtr[ 1024 >> 7 ] = lp->aSpecInner2;
	*/

	// ucontext_t 
	//lp->ctx.uc_stack.ss_sp = lp->pRunStack;
	//lp->ctx.uc_stack.ss_size = at.stack_size - sizeof(long);

	//use coctx_t
	lp->ctx.ss_sp = lp->pRunStack;
	lp->ctx.ss_size = at.stack_size - sizeof(long);

	if( env->iCoRoutineCnt < (int)(sizeof(env->arrCoRoutine)/sizeof(env->arrCoRoutine[0])) ) 
	{
		env->arrCoRoutine[ env->iCoRoutineCnt ] = lp;
		env->iCoRoutineCnt++;
	}

	return lp;
}

int co_create( stCoRoutine_t **ppco,const stCoRoutineAttr_t *attr,pfn_co_routine_t pfn,void *arg )
{
	if( !co_get_curr_thread_env() ) 
	{
		co_init_curr_thread_env();
	}

	stCoRoutine_t *co = co_create_env( co_get_curr_thread_env(),attr,pfn,arg );
	*ppco = co;
	return 0;
}
void co_free( stCoRoutine_t *co )
{
	if( co->cNoProtectStack )
	{
		free( co->pRunStack );
	}
	else
	{
		//munmap( co->pRunStack - 4096,co->ctx.uc_stack.ss_size + 8192 );
		munmap( co->pRunStack - 4096,co->ctx.ss_size + 8192 );
	}
	//free( co->pRunStack - 4096 );//,co->ctx.uc_stack.ss_size + 8192 );
	free( co );
}
void co_release( stCoRoutine_t *co )
{
	if( co->cEnd )
	{
		free( co );
	}
}
void co_resume( stCoRoutine_t *co )
{
	stCoRoutineEnv_t *env = co->env;
	stCoRoutine_t *lpCurrRoutine = env->pCallStack[ env->iCallStackSize - 1 ];
	if( !co->cStart )
	{
		/*
		unsigned long p = (unsigned long)co;
		getcontext(&co->ctx);
		makecontext( &co->ctx, (pfnCoRoutineFunc_t)CoRoutineFunc, 2,(int)(p & 0xffffffff), (int)((p >> 32) & 0xffffffff) );
		co->cStart = 1;
		*/
		coctx_make(&co->ctx, (coctx_pfn_t)CoRoutineFunc, co, 0);
		co->cStart = 1;
	}

	//co->ctx.uc_link = &lpCurrRoutine->ctx; 
	env->pCallStack[ env->iCallStackSize++ ] = co;

	co_swap_wrapper(lpCurrRoutine, co, env);
	
	//swapcontext( &(lpCurrRoutine->ctx),&(co->ctx) );
}
void co_yield_env( stCoRoutineEnv_t *env )
{
	fire_before_yield_cb();

	stCoRoutine_t *last = env->pCallStack[ env->iCallStackSize - 2 ];
	stCoRoutine_t *curr = env->pCallStack[ env->iCallStackSize - 1 ];

	env->iCallStackSize--;

	env->pStat->yield_cnt++;

	//co_swapcontext( &curr->ctx, &last->ctx );

	co_swap_wrapper(curr, last, env);
}

void co_yield_resume_env( stCoRoutineEnv_t *env,stCoRoutine_t *dest )
{
	
	stCoRoutine_t *last = env->pCallStack[ env->iCallStackSize - 2 ];
	stCoRoutine_t *curr = env->pCallStack[ env->iCallStackSize - 1 ];

	if( last == dest )
	{
		env->iCallStackSize--;
		//co_swapcontext( &curr->ctx, &last->ctx );
		co_swap_wrapper(curr, last, env);
	}
	else
	{
		env->pCallStack[ env->iCallStackSize - 1 ] = dest;	
		//co_swapcontext( &curr->ctx, &dest->ctx );
		co_swap_wrapper(curr, dest, env);

	}
}

void co_yield_resume_ct( stCoRoutine_t *dest )
{
	co_yield_resume_env( co_get_curr_thread_env(),dest );
}
void co_yield_ct()
{
	co_yield_env( co_get_curr_thread_env() );
}
void co_yield( stCoRoutine_t *co )
{
	co_yield_env( co->env );
}



//int poll(struct pollfd fds[], nfds_t nfds, int timeout);
// { fd,events,revents }
struct stPollItem_t ;
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
struct stCoEvent_t : public stTimeoutItem_t
{
	int fd;
	struct epoll_event stEvent;
	struct epoll_event stREvent;
	pfn_co_event_call_back pfn;
	void* args;
};

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
static void OnSignalProcessEvent( stTimeoutItem_t * ap )
{
	stCoRoutine_t *co = (stCoRoutine_t*)ap->pArg;
	co_resume( co );
}

stCoCondItem_t *co_cond_pop( stCoCond_t *link );
int co_cond_signal( stCoCond_t *si )
{
	stCoCondItem_t * sp = co_cond_pop( si );
	if( !sp ) 
	{
		return 0;
	}
	RemoveFromLink<stTimeoutItem_t,stTimeoutItemLink_t>( &sp->timeout );

	AddTail( co_get_curr_thread_env()->pEpoll->pstActiveList,&sp->timeout );

	return 0;
}
int co_cond_broadcast( stCoCond_t *si )
{
	for(;;)
	{
		stCoCondItem_t * sp = co_cond_pop( si );
		if( !sp ) return 0;

		RemoveFromLink<stTimeoutItem_t,stTimeoutItemLink_t>( &sp->timeout );

		AddTail( co_get_curr_thread_env()->pEpoll->pstActiveList,&sp->timeout );
	}

	return 0;
}


int co_cond_timedwait( stCoCond_t *link,int ms )
{
	stCoCondItem_t* psi = (stCoCondItem_t*)calloc(1, sizeof(stCoCondItem_t));
	psi->timeout.pArg = GetCurrThreadCo();
	psi->timeout.pfnProcess = OnSignalProcessEvent;

	if( ms > 0 )
	{
		unsigned long long now = GetTickMS();
		psi->timeout.ullExpireTime = now + ms;

		int ret = AddTimeout( co_get_curr_thread_env()->pEpoll->pTimeout,&psi->timeout,now );
		if( ret != 0 )
		{
			free(psi);
			return ret;
		}
	}
	AddTail( link, psi);

	co_yield_ct();


	RemoveFromLink<stCoCondItem_t,stCoCond_t>( psi );
	free(psi);

	return 0;
}
stCoCond_t *co_cond_alloc()
{
	return (stCoCond_t*)calloc( 1,sizeof(stCoCond_t) );
}
int co_cond_free( stCoCond_t * cc )
{
	free( cc );
	return 0;
}


stCoCondItem_t *co_cond_pop( stCoCond_t *link )
{
	stCoCondItem_t *p = link->head;
	if( p )
	{
		PopHead<stCoCondItem_t,stCoCond_t>( link );
	}
	return p;
}
/*
 *   EPOLLPRI 		POLLPRI    // There is urgent data to read.  
 *   EPOLLMSG 		POLLMSG
 *
 *   				POLLREMOVE
 *   				POLLRDHUP
 *   				POLLNVAL
 *
 * */
static uint32_t PollEvent2Epoll( short events )
{
	uint32_t e = 0;	
	if( events & POLLIN ) 	e |= EPOLLIN;
	if( events & POLLOUT )  e |= EPOLLOUT;
	if( events & POLLHUP ) 	e |= EPOLLHUP;
	if( events & POLLERR )	e |= EPOLLERR;
	if( events & POLLRDNORM ) e |= EPOLLRDNORM;
	if( events & POLLWRNORM ) e |= EPOLLWRNORM;
	return e;
}
static short EpollEvent2Poll( uint32_t events )
{
	short e = 0;	
	if( events & EPOLLIN ) 	e |= POLLIN;
	if( events & EPOLLOUT ) e |= POLLOUT;
	if( events & EPOLLHUP ) e |= POLLHUP;
	if( events & EPOLLERR ) e |= POLLERR;
	if( events & EPOLLRDNORM ) e |= POLLRDNORM;
	if( events & EPOLLWRNORM ) e |= POLLWRNORM;
	return e;
}

static stCoRoutineEnv_t* g_arrCoEnvPerThread[ 204800 ] = { 0 };
void co_init_curr_thread_env()
{
	pid_t pid = GetPid();	
	g_arrCoEnvPerThread[ pid ] = (stCoRoutineEnv_t*)calloc( 1,sizeof(stCoRoutineEnv_t) );
	stCoRoutineEnv_t *env = g_arrCoEnvPerThread[ pid ];

	env->pStat = (stCoRoutineStat_t*)calloc(1, sizeof(stCoRoutineStat_t));

	env->iCallStackSize = 0;
	struct stCoRoutine_t *self = co_create_env( env,NULL,NULL,NULL );
	self->cIsMain = 1;

	/*
	getcontext( &self->ctx );
	makecontext( &self->ctx, NULL, 0 );
	*/
	coctx_init( &self->ctx );
	//coctx_make(&self->ctx, (coctx_pfn_t)CoRoutineFunc, self, 0);

	env->pCallStack[ env->iCallStackSize++ ] = self;

	stCoEpoll_t *ev = AllocEpoll();
	SetEpoll( env,ev );
}
stCoRoutineEnv_t *co_get_curr_thread_env()
{
	return g_arrCoEnvPerThread[ GetPid() ];
}

void OnPollProcessEvent( stTimeoutItem_t * ap )
{
	//printf("OnPollProcessEvent\n");
	stCoRoutine_t *co = (stCoRoutine_t*)ap->pArg;
	co_resume( co );
}

void OnPollPreparePfn( stTimeoutItem_t * ap,struct epoll_event &e,stTimeoutItemLink_t *active )
{
	stPollItem_t *lp = (stPollItem_t *)ap;
	lp->pSelf->revents = EpollEvent2Poll( e.events );


	stPoll_t *pPoll = lp->pPoll;
	pPoll->iRaiseCnt++;

	if( !pPoll->iAllEventDetach )
	{
		pPoll->iAllEventDetach = 1;

		RemoveFromLink<stTimeoutItem_t,stTimeoutItemLink_t>( pPoll );

		AddTail( active,pPoll );

	}
}


void co_eventloop( stCoEpoll_t *ctx,pfn_co_eventloop_t pfn,void *arg )
{
	epoll_event *result = (epoll_event*)calloc(1, sizeof(epoll_event) * stCoEpoll_t::_EPOLL_SIZE );

	stTimeoutItemLink_t *active = (ctx->pstActiveList);
	//memset( active,0,sizeof(stTimeoutItemLink_t) );  

	for(;;)
	{
		int ret = epoll_wait( ctx->iEpollFd,result,stCoEpoll_t::_EPOLL_SIZE, active->head ? 0 : 1 );

		if (g_pfnEventLoopStart)
		{
			g_pfnEventLoopStart(ret);
		}

		stTimeoutItemLink_t *timeout = (ctx->pstTimeoutList);

		memset( timeout,0,sizeof(stTimeoutItemLink_t) );

		for(int i=0;i<ret;i++)
		{
			stTimeoutItem_t *item = (stTimeoutItem_t*)result[i].data.ptr;
			if( item->pfnPrepare )
			{
				item->pfnPrepare( item,result[i],active );
			}
			else
			{
				AddTail( active,item );
			}
		}


		unsigned long long now = GetTickMS();
		TakeAllTimeout( ctx->pTimeout,now,timeout );

		stTimeoutItem_t *lp = timeout->head;
		while( lp )
		{
			//printf("raise timeout %p\n",lp);
			lp->bTimeout = true;
			lp = lp->pNext;
		}

		Join<stTimeoutItem_t,stTimeoutItemLink_t>( active,timeout );

		lp = active->head;
		while( lp )
		{

			PopHead<stTimeoutItem_t,stTimeoutItemLink_t>( active );
			if( lp->pfnProcess )
			{
				lp->pfnProcess( lp );
			}

			lp = active->head;
		}
		if( pfn )
		{
			if( -1 == pfn( arg ) )
			{
				break;
			}
		}
		//memset( active,0,sizeof(stTimeoutItemLink_t) );

		if (g_pfnEventLoopEnd)
		{
			g_pfnEventLoopEnd();
		}
	}
	free( result );
}
void OnCoroutineEvent( stTimeoutItem_t * ap )
{
	stCoRoutine_t *co = (stCoRoutine_t*)ap->pArg;
	co_resume( co );
}


stCoEpoll_t *AllocEpoll()
{
	stCoEpoll_t *ctx = (stCoEpoll_t*)calloc( 1,sizeof(stCoEpoll_t) );

	ctx->iEpollFd = epoll_create( stCoEpoll_t::_EPOLL_SIZE );
	ctx->pTimeout = AllocTimeout( 60 * 1000 );
	
	ctx->pstActiveList = (stTimeoutItemLink_t*)calloc( 1,sizeof(stTimeoutItemLink_t) );
	ctx->pstTimeoutList = (stTimeoutItemLink_t*)calloc( 1,sizeof(stTimeoutItemLink_t) );


	return ctx;
}

void FreeEpoll( stCoEpoll_t *ctx )
{
	if( ctx )
	{
		free( ctx->pstActiveList );
		free( ctx->pstTimeoutList );
		FreeTimeout( ctx->pTimeout );
	}
	free( ctx );
}

stCoRoutine_t *GetCurrCo( stCoRoutineEnv_t *env )
{
	return env->pCallStack[ env->iCallStackSize - 1 ];
}
stCoRoutine_t *GetCurrThreadCo( )
{
	stCoRoutineEnv_t *env = co_get_curr_thread_env();
	if( !env ) return 0;
	return GetCurrCo(env);
}


struct stCoPersistEvent_t: public stTimeoutItem_t
{
	struct epoll_event stEvent;
};
static void OnCoPersistEvent( stTimeoutItem_t * ap )
{
	stCoRoutine_t *co = (stCoRoutine_t*)ap->pArg;
	co_resume( co );
}
int co_add_persist_event( stCoEpoll_t *ctx, int fd,int events )
{
	int epfd = ctx->iEpollFd;

	stCoPersistEvent_t * arg = (stCoPersistEvent_t*)calloc( 1,sizeof(stCoPersistEvent_t) );

	arg->pfnProcess = OnCoPersistEvent;
	arg->pArg = GetCurrCo( co_get_curr_thread_env() );

	arg->stEvent.events = PollEvent2Epoll( events );
	arg->stEvent.data.ptr = arg;

	return epoll_ctl( epfd,EPOLL_CTL_ADD, fd, &(arg->stEvent) );

}

int co_yield_timeout(int timeout)
{
	//add timeout
	stCoEpoll_t* ctx = co_get_epoll_ct();
	stTimeoutItem_t* ptTimeoutItem = (stTimeoutItem_t*)calloc(1, sizeof(stTimeoutItem_t));
	//memset(&tTimeoutItem, 0, sizeof(stTimeoutItem_t));
	unsigned long long start = GetTickMS();
	ptTimeoutItem->ullExpireTime = start + timeout;
	ptTimeoutItem->pfnProcess = OnPollProcessEvent;
	ptTimeoutItem->pArg = GetCurrCo( co_get_curr_thread_env() );
	if (timeout > 0)
	{
		int ret = AddTimeout( ctx->pTimeout,ptTimeoutItem, start);
		if( ret != 0 )
		{
			errno = EINVAL;
			if ( ret < 0) 
			{
				free(ptTimeoutItem);
				return ret;
			} 
			return -ret;
		}
	}
	co_yield_env( co_get_curr_thread_env() );
	RemoveFromLink<stTimeoutItem_t,stTimeoutItemLink_t>( ptTimeoutItem);
	free(ptTimeoutItem);
	return 0;
}

typedef int (*poll_pfn_t)(struct pollfd fds[], nfds_t nfds, int timeout);
int co_poll_inner( stCoEpoll_t *ctx,struct pollfd fds[], nfds_t nfds, int timeout, poll_pfn_t pollfunc)
{
	if( timeout > stTimeoutItem_t::eMaxTimeout )
	{
		timeout = stTimeoutItem_t::eMaxTimeout;
	}
	int epfd = ctx->iEpollFd;

	//1.struct change
	stCoRoutine_t* self = co_self();
	stPoll_t& arg = *((stPoll_t*)malloc(sizeof(stPoll_t)));
	
	memset( &arg,0,sizeof(arg) );

	arg.iEpollFd = epfd;
	arg.fds = (pollfd*)calloc(nfds, sizeof(pollfd));
	arg.nfds = nfds;

	stPollItem_t arr[2];
	if( nfds < sizeof(arr) / sizeof(arr[0]) && !self->pDynamicStack)
	{
		arg.pPollItems = arr;
	}	
	else
	{
		arg.pPollItems = (stPollItem_t*)malloc( nfds * sizeof( stPollItem_t ) );
	}
	memset( arg.pPollItems,0,nfds * sizeof(stPollItem_t) );

	arg.pfnProcess = OnPollProcessEvent;
	arg.pArg = GetCurrCo( co_get_curr_thread_env() );
	
	for(nfds_t i=0;i<nfds;i++)
	{
		arg.pPollItems[i].pSelf = arg.fds + i;
		arg.pPollItems[i].pPoll = &arg;

		arg.pPollItems[i].pfnPrepare = OnPollPreparePfn;
		struct epoll_event &ev = arg.pPollItems[i].stEvent;

		if( fds[i].fd > -1 )
		{
			ev.data.ptr = arg.pPollItems + i;
			ev.events = PollEvent2Epoll( fds[i].events );

			int ret = epoll_ctl( epfd,EPOLL_CTL_ADD, fds[i].fd, &ev );
			if (ret < 0 && errno == EPERM && nfds == 1 && pollfunc != NULL)
			{
				if( arg.pPollItems != arr )
				{
					free( arg.pPollItems );
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
	if (timeout > 0)
	{
		int ret = AddTimeout( ctx->pTimeout,&arg,now );
		if( ret != 0 )
		{
			co_log_err("CO_ERR: AddTimeout ret %d now %lld timeout %d arg.ullExpireTime %lld",
					ret,now,timeout,arg.ullExpireTime);
			errno = EINVAL;
			if( arg.pPollItems != arr )
			{
				free( arg.pPollItems );
				arg.pPollItems = NULL;
			}
			free(arg.fds);
			free(&arg);

			if ( ret < 0) 
			{
				return ret;
			} 
			return -ret;
		}
	}

	co_yield_env( co_get_curr_thread_env() );

	RemoveFromLink<stTimeoutItem_t,stTimeoutItemLink_t>( &arg );
	for(nfds_t i = 0;i < nfds;i++)
	{
		int fd = fds[i].fd;
		if( fd > -1 )
		{
			epoll_ctl( epfd,EPOLL_CTL_DEL,fd,&arg.pPollItems[i].stEvent );
		}
		fds[i].revents = arg.fds[i].revents;
	}

	if( arg.pPollItems != arr )
	{
		free( arg.pPollItems );
		arg.pPollItems = NULL;
	}
	int ret  = arg.iRaiseCnt > 0 ? arg.iRaiseCnt : 
		arg.bTimeout == true ? 0 : -1;

	free(arg.fds);
	free(&arg);

	return ret ;

}
int	co_poll( stCoEpoll_t *ctx,struct pollfd fds[], nfds_t nfds, int timeout_ms )
{
	return co_poll_inner(ctx, fds, nfds, timeout_ms, NULL);
}
void SetEpoll( stCoRoutineEnv_t *env,stCoEpoll_t *ev )
{
	env->pEpoll = ev;
}
stCoEpoll_t *co_get_epoll_ct()
{
	if( !co_get_curr_thread_env() )
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
void co_set_sys_specific_func( pfn_sys_getspecific_t get, pfn_sys_setspecific_t set)
{
	g_pfnSysGetSpecific = get;
	g_pfnSysSetSpecific = set;
}
void co_set_eventloop_stat(pfn_co_eventloop_start_t start, pfn_co_eventloop_end_t end)
{
	g_pfnEventLoopStart = start;
	g_pfnEventLoopEnd = end;
}

void co_set_before_yield_cb( pfn_before_yield_cb_t cb,void *arg )
{
	int idx = __sync_fetch_and_add( &g_iBeforeYieldCbLen,1 );

	g_arrBeforeYieldCb[ idx ].cb = cb;
	g_arrBeforeYieldCb[ idx ].arg = arg;
}
void *co_getspecific(pthread_key_t key)
{
	stCoRoutine_t *co = GetCurrThreadCo();
	if( !co || co->cIsMain )
	{
		if ( g_pfnSysGetSpecific )
		{
			return g_pfnSysGetSpecific ( key );
		}
		else
		{
			return pthread_getspecific( key );
		}
	}

	if( key >= 4096 ) return  NULL;
	
	if (!co->aSpecPtr)
	{
		co->aSpecPtr = (stCoSpec_t**)calloc(4096 / 128, sizeof(stCoSpec_t*));
	}

	stCoSpec_t * arr = co->aSpecPtr[ key >> 7 ];
	if( !arr )	
	{
		return NULL;
	}

	return arr[ key & 127 ].value;

}
int co_setspecific(pthread_key_t key, const void *value)
{
	stCoRoutine_t *co = GetCurrThreadCo();
	if( !co || co->cIsMain )
	{
		if ( g_pfnSysSetSpecific )
		{
			return g_pfnSysSetSpecific( key , value );
		}
		else
		{
			return pthread_setspecific( key, value );
		}
	}
	if( key >= 4096 ) return  -__LINE__;

	if (!co->aSpecPtr)
	{
		co->aSpecPtr = (stCoSpec_t**)calloc(4096 / 128, sizeof(stCoSpec_t*));
	}

	stCoSpec_t ** slot = co->aSpecPtr + ( key >> 7 );
	if( !*slot )	
	{
		*slot = (stCoSpec_t*)calloc( 1,sizeof(stCoSpec_t) * 128 );
	}
	(*slot)[ key & 127 ].value = (void *)value;

	return 0;
}



void co_disable_hook_sys()
{
	stCoRoutine_t *co = GetCurrThreadCo();
	if( co )
	{
		co->cEnableSysHook = 0;
	}
}
bool co_is_enable_sys_hook()
{
	stCoRoutine_t *co = GetCurrThreadCo();
	return ( co && co->cEnableSysHook );
}



stCoRoutine_t *co_self()
{
	return GetCurrThreadCo();
}
bool co_is_main()
{
	stCoRoutine_t* co = co_self();
	if (!co || (co && co->cIsMain))
	{
		return true;
	}
	return false;
}

void co_reset_epolltimeout()
{
	stCoEpoll_t* ctx = co_get_epoll_ct();
	ctx->pTimeout->ullStart = 0;
}

void OnEventPrepare( stTimeoutItem_t * ap,struct epoll_event &e,stTimeoutItemLink_t *active )
{
	stCoEvent_t* lp = (stCoEvent_t*)ap;
	lp->stREvent = e;

	RemoveFromLink<stTimeoutItem_t,stTimeoutItemLink_t>( ap );

	AddTail( active, ap );

}
void OnEventProcess( stTimeoutItem_t * ap )
{
	//printf("OnPollProcessEvent\n");
	stCoEvent_t* lp = (stCoEvent_t*)ap;
	short revent = EpollEvent2Poll(lp->stREvent.events);
	lp->pfn(lp->fd, revent, lp->args);
}

stCoEvent_t* co_alloc_event()
{
	stCoEvent_t* coevent = (stCoEvent_t*)calloc(1, sizeof(stCoEvent_t));
	return coevent;
}

int co_add_event(stCoEvent_t* coevent, int fd, pfn_co_event_call_back pfn, void* args, unsigned int events, int timeout)
{
	//init coevent
	coevent->fd = fd;
	coevent->pfn = pfn;
	coevent->args = args;

	coevent->stREvent.events = 0;

	coevent->pfnPrepare = OnEventPrepare;
	coevent->pfnProcess = OnEventProcess;

	if( timeout > stTimeoutItem_t::eMaxTimeout )
	{
		timeout = stTimeoutItem_t::eMaxTimeout;
	}
	int ret = 0;
	stCoEpoll_t* ctx = co_get_epoll_ct();
	int epfd = ctx->iEpollFd;

	if (events == 0)
	{
		if (coevent->stEvent.events != 0)
		{
			coevent->stEvent.events = PollEvent2Epoll(events);
			coevent->stEvent.data.ptr = (void*)coevent;
			ret = epoll_ctl(epfd, EPOLL_CTL_DEL, coevent->fd, &coevent->stEvent);
		}
	}
	else if (coevent->stEvent.events == 0)
	{
		coevent->stEvent.events = PollEvent2Epoll(events);
		coevent->stEvent.data.ptr = (void*)coevent;
		ret = epoll_ctl(epfd, EPOLL_CTL_ADD, coevent->fd, &coevent->stEvent);
	}
	else if (coevent->stEvent.events != events)
	{
		coevent->stEvent.events = PollEvent2Epoll(events);
		coevent->stEvent.data.ptr = (void*)coevent;
		ret = epoll_ctl(epfd, EPOLL_CTL_MOD, coevent->fd, &coevent->stEvent);
	}
	if (ret)
	{
		return ret;
	}
	
	//2.add timeout
	RemoveFromLink<stTimeoutItem_t,stTimeoutItemLink_t>( coevent );
	unsigned long long now = GetTickMS();
	coevent->ullExpireTime = now + timeout;
	if (timeout > 0)
	{
		int ret = AddTimeout( ctx->pTimeout, coevent, now );
		if( ret != 0 )
		{
			epoll_ctl(epfd, EPOLL_CTL_DEL, coevent->fd, &coevent->stEvent);
			co_log_err("CO_ERR: AddTimeout ret %d now %lld timeout %d arg.ullExpireTime %lld",
					ret, now, timeout, coevent->ullExpireTime);
			errno = EINVAL;
			if ( ret < 0) 
			{
				return ret;
			} 
			return -ret;
		}
	}

	return ret;
}

int co_free_event(stCoEvent_t* coevent)
{
	stCoEpoll_t* ctx = co_get_epoll_ct();
	int epfd = ctx->iEpollFd;
	//remove event
	epoll_ctl(epfd, EPOLL_CTL_DEL, coevent->fd, &coevent->stEvent);
	//remove timeout
	RemoveFromLink<stTimeoutItem_t,stTimeoutItemLink_t>( coevent );
	if (coevent->fd > 0)
	{
		close(coevent->fd);
		coevent->fd = -1;
	}
	free(coevent);
	return 0;
}

int co_get_eventfd(stCoEvent_t* coevent)
{
	return coevent->fd;
}

stCoRoutineStat_t* co_get_curr_stat()
{
	stCoRoutineEnv_t* env = co_get_curr_thread_env();
	if (!env)
	{
		return NULL;
	}
	stCoRoutineStat_t* stat =  env->pStat;
	return stat;
}





