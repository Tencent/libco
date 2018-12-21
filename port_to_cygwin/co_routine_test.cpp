//
// Created by dell-pc on 2018/4/5.
//
#include <iostream>
#include "co_routine.h"

using namespace std;

void* foo2(void* s1 )
{
    cout << __func__<<" start" << endl;
    co_yield_ct();
    cout << __func__<<" end" << endl;
}


void *foo1(void *s1)
{
    cout << __func__<<" start" << endl;
    stCoRoutine_t *ctx = NULL;
    co_create(&ctx, NULL, foo2, 0);
    co_resume(ctx);
    co_yield_ct();

    co_resume(ctx);
    cout << __func__<<" end" << endl;
}

int main()
{
    cout << __func__<<" start" << endl;

    int ret = 0;
    stCoRoutine_t *ctx = NULL;
    ret = co_create(&ctx,NULL, foo1, 0);
    co_resume(ctx);
    cout << __func__<<" end0" << endl;
    co_resume(ctx);
    cout << __func__<<" end1" << endl;

}
