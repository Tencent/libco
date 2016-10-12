#include "co_routine_specific.h"
#include "co_routine.h"
#include <unistd.h>
#include <stdio.h>
#include <vector>
#include <iostream>
using namespace std;
struct stRoutineArgs_t
{
	stCoRoutine_t* co;
	int routine_id;
};
struct stRoutineSpecificData_t
{
	int idx;
};

CO_ROUTINE_SPECIFIC(stRoutineSpecificData_t, __routine);

void* RoutineFunc(void* args)
{
	co_enable_hook_sys();
	stRoutineArgs_t* routine_args = (stRoutineArgs_t*)args;
	__routine->idx = routine_args->routine_id;
	while (true)
	{
		printf("%s:%d routine specific data idx %d\n", __func__, __LINE__, __routine->idx);
		poll(NULL, 0, 1000);
	}
	return NULL;
}
int main()
{
	stRoutineArgs_t args[10];
	for (int i = 0; i < 10; i++)
	{
		args[i].routine_id = i;
		co_create(&args[i].co, NULL, RoutineFunc, (void*)&args[i]);
		co_resume(args[i].co);
	}
	co_eventloop(co_get_epoll_ct(), NULL, NULL);
	return 0;
}
