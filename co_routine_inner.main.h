#ifndef __CO_ROUTINE_INNER_H__
#define __CO_ROUTINE_INNER_H__

#include "co_routine.h"
#include "coctx.h"
#include <sys/types.h>
#include <sys/socket.h>
struct stCoRoutineEnv_t;
struct stCoSpec_t
{
	void *value;
};
struct stStackMem_t;
struct stStackMemEnv_t;
struct stCoRoutine_t
{
	stCoRoutineEnv_t *env;
	pfn_co_routine_t pfn;
	void *arg;
	//ucontext_t ctx;
	coctx_t ctx;
	char cStart;
	char cEnd;

	//stCoSpec_t aSpecInner[ 128 ];
	//stCoSpec_t aSpecInner2[ 128 ];
	//stCoSpec_t *aSpecPtr[ 4096 / 128 ];// 2 ^ 7
	stCoSpec_t **aSpecPtr;

	char cIsMain;
	char cEnableSysHook;

	void *pvEnv;

	char cNoProtectStack;
	
	stStackMem_t* pDynamicStack;

	//for copy stack;
	stStackMemEnv_t* pStackEnv;
	char *pRunStackSP;
	char *pBuffer;
	int  iStackCapacity;
	int  iStackLen;

	//char sRunStack[ 1 ];  //the last field!!
	/*************/
	/* forbidden */
	/*************/
	//in the hell
	char *pRunStack;
};



//1.env
void 				co_init_curr_thread_env();
stCoRoutineEnv_t *	co_get_curr_thread_env();

//2.coroutine
void    co_free( stCoRoutine_t * co );
void    co_yield_env(  stCoRoutineEnv_t *env );

//3.func



//-----------------------------------------------------------------------------------------------

struct stTimeout_t;
struct stTimeoutItem_t ;

stTimeout_t *AllocTimeout( int iSize );
void 	FreeTimeout( stTimeout_t *apTimeout );
int  	AddTimeout( stTimeout_t *apTimeout,stTimeoutItem_t *apItem ,uint64_t allNow );

struct stCoEpoll_t;
stCoEpoll_t * AllocEpoll();
void 		FreeEpoll( stCoEpoll_t *ctx );

stCoRoutine_t *		GetCurrThreadCo();
void 				SetEpoll( stCoRoutineEnv_t *env,stCoEpoll_t *ev );

typedef void (*pfnCoRoutineFunc_t)();

//event
struct stCoEvent_t;
typedef void* (*pfn_co_event_call_back)(int fd, int revent, void* args);
stCoEvent_t* co_alloc_event();
int co_free_event(stCoEvent_t* coevent);
int co_get_eventfd(stCoEvent_t* coevent);
int co_add_event(stCoEvent_t* coevent, int fd, pfn_co_event_call_back pfn, void* args, unsigned int events, int timeout);


int co_accept( int fd, struct sockaddr *addr, socklen_t *len );
#endif

