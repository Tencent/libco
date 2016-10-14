#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/epoll.h>
#include <errno.h>
#include <string.h>
#include "coctx.h"
#include "co_routine.h"
#include "co_routine_inner.h"

void* RoutineFunc(void* args)
{
	co_enable_hook_sys();
	int* routineid = (int*)args;
	while (true)
	{
		char sBuff[128];
		sprintf(sBuff, "from routineid %d stack addr %p\n", *routineid, sBuff);

		printf("%s", sBuff);
		poll(NULL, 0, 1000); //sleep 1s
	}
	return NULL;
}

int main()
{
	stShareStack_t* share_stack= co_alloc_sharestack(1, 1024 * 128);
	stCoRoutineAttr_t attr;
	attr.stack_size = 0;
	attr.share_stack = share_stack;

	stCoRoutine_t* co[2];
	int routineid[2];
	for (int i = 0; i < 2; i++)
	{
		routineid[i] = i;
		co_create(&co[i], &attr, RoutineFunc, routineid + i);
		co_resume(co[i]);
	}
	co_eventloop(co_get_epoll_ct(), NULL, NULL);
	return 0;
}
