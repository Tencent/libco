#include <gtest/gtest.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "coctx.h"
#include "co_routine.h"
#include "co_routine_inner.h"
#include <unistd.h>
#include <sys/time.h>
#include <sys/epoll.h>
#include <errno.h>
#include <string.h>
extern "C"
{
	extern void coctx_swap(coctx_t*, coctx_t*) asm("coctx_swap");
	extern void test_xmm(char*, char*) asm("test_xmm");
}


void Test1()
{
	char a16[16] __attribute__ ((aligned (16)))= "0123456789abcde";
	char b16[16]  __attribute__ ((aligned (16))) = "abcde012345789"; 
	test_xmm( a16, b16);
	printf("(%p : %s) (%p : %s) \n", &a16, a16, &b16, b16);
	return;
}

void* RoutineFunc(void* args1, void* args2)
{
	coctx_t* main = (coctx_t*)(args1);
	coctx_t* self = (coctx_t*)args2;
	printf("%s:%d args1 %p args2 %p main %p self %p\n", __func__, __LINE__, &args1, &args2, main ,self);
	coctx_swap(self, main);
	return NULL;
}

void* TestXMMFunc(void* args1, void* args2)
{
	Test1();

	coctx_t* main = (coctx_t*)(args1);
	coctx_t* self = (coctx_t*)args2;
	char s = 's';
	printf("%s:%d args1 %p args2 %p main %p self %p\n", __func__, __LINE__, &args1, &args2, main ,self);

	coctx_swap(self, main);

}


TEST(Coctx, normal)
{
	coctx_t main;
	coctx_init(&main);

	int i = 5;
	coctx_t ctx;
	coctx_init(&ctx);
	ctx.ss_size = 1024 * 128;
	ctx.ss_sp = (char*)malloc(ctx.ss_size);
	coctx_make(&ctx, RoutineFunc, (void*)&main, (void*)&ctx);

	coctx_swap(&main, &ctx);
	return;
}

TEST(Coctx, xmm)
{
	coctx_t main;
	coctx_init(&main);

	coctx_t self;
	coctx_init(&self);
	self.ss_size = 1024 * 128;
	self.ss_sp = (char*)malloc(self.ss_size);
	coctx_make(&self, TestXMMFunc, (void*)&main, (void*)&self);
	coctx_swap(&main, &self);

	printf("%s:%d end\n", __func__, __LINE__);
}
int g_iExitEventLoop = 0;
int TickFunc(void* args)
{
	if (g_iExitEventLoop == 0)
	{
		return -1;
	}
	return 0;
}
void* RoutineFunc1(void* args)
{
	co_enable_hook_sys();
	int* piRoutineID = (int*)args;
	int iRoutinedID = *piRoutineID;
	for (int i = 0; i < 2; i++)
	{
		poll(NULL, 0, 1000);
		printf("%s:%d iRoutinedID (%p:%d)\n", __func__, __LINE__, &iRoutinedID, iRoutinedID);
	}
	g_iExitEventLoop--;
	return NULL;
}

TEST(CopyStack, ResumeFromMain)
{
	stShareStack_t* env = co_alloc_sharestack(1, 1024 * 128);
	int iCount = 2;
	g_iExitEventLoop = iCount;
	stCoRoutine_t* co[2];
	int iRoutineID[2];
	for (int i = 0; i < iCount; i++)
	{
		stCoRoutineAttr_t attr;
		attr.share_stack = env;
		iRoutineID[i] = i;
		co_create(&co[0], &attr, RoutineFunc1, iRoutineID + i);
		co_resume(co[0]);
	}
	co_eventloop(co_get_epoll_ct(), TickFunc, NULL);
}

void* Sender(void* ptArgs)
{
	co_enable_hook_sys();
	stCoRoutine_t* recever = (stCoRoutine_t*)ptArgs;
	while (true)
	{
		char s[128];
		strcpy(s, "from sender\n");
		co_resume(recever);
		poll(NULL, 0, 1000);
		printf("%s:%d %p %s", __func__, __LINE__, s, s);
		g_iExitEventLoop--;
		if (g_iExitEventLoop == 0)
		{
			break;
		}
	}
	return NULL;
}
void* Recever(void* ptArgs)
{
	co_enable_hook_sys();
	while (true)
	{
		char s[128];
		strcpy(s, "from recever\n");
		co_yield_ct();
		printf("%s:%d %p %s", __func__, __LINE__, s, s);
	}
}

TEST(CopyStack, ResumeFromSender)
{
	g_iExitEventLoop = 2;

	stShareStack_t* env = co_alloc_sharestack(1, 1024 * 128);

	stCoRoutineAttr_t attr;
	attr.share_stack = env;

	stCoRoutine_t* recever;
	co_create(&recever, &attr, Recever, NULL);
	co_resume(recever);

	stCoRoutine_t* sender;
	co_create(&sender, &attr, Sender, (void*)recever);
	co_resume(sender);

	co_eventloop(co_get_epoll_ct(), TickFunc, NULL);
}
TEST(NoCopyStack, ResumeFromMain)
{
	stShareStack_t* env = co_alloc_sharestack(1, 1024 * 128);
	int iCount = 2;
	g_iExitEventLoop = iCount;
	stCoRoutine_t* co[2];
	int iRoutineID[2];
	for (int i = 0; i < iCount; i++)
	{
		stCoRoutineAttr_t attr;
		attr.share_stack = env;
		iRoutineID[i] = i;
		co_create(&co[0], &attr, RoutineFunc1, iRoutineID + i);
		co_resume(co[0]);
	}
	co_eventloop(co_get_epoll_ct(), TickFunc, NULL);
}
TEST(NoCopyStack, ResumeFromSender)
{
	g_iExitEventLoop = 2;

	stShareStack_t* env = co_alloc_sharestack(1, 1024 * 128);

	stCoRoutineAttr_t attr;
	attr.share_stack = env;

	stCoRoutine_t* recever;
	co_create(&recever, &attr, Recever, NULL);
	co_resume(recever);

	stCoRoutine_t* sender;
	co_create(&sender, &attr, Sender, (void*)recever);
	co_resume(sender);

	co_eventloop(co_get_epoll_ct(), TickFunc, NULL);
}


int main(int argc, char* argv[])
{
	printf("size %d\n", sizeof(stCoRoutine_t));
	testing::InitGoogleTest(&argc, argv);
	return RUN_ALL_TESTS();
}

//gzrd_Lib_CPP_Version_ID--start
#ifndef GZRD_SVN_ATTR
#define GZRD_SVN_ATTR "0"
#endif
static char gzrd_Lib_CPP_Version_ID[] __attribute__((used))="$HeadURL: http://scm-gy.tencent.com/gzrd/gzrd_mail_rep/QQMailcore_proj/trunk/basic/colib/test/test_colib.cpp $ $Id: test_colib.cpp 1650485 2016-06-28 06:34:38Z leiffyli $ " GZRD_SVN_ATTR "__file__";
// gzrd_Lib_CPP_Version_ID--end

