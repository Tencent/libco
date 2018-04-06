//
// Created by dell-pc on 2018/4/5.
//
#include <iostream>
#include <stddef.h>
#include "coctx.h"
#include "cm_routine.h"

using namespace std;

void* foo2(void* s1, void *s2 ) {
    cout << __func__<<" start" << endl;
    cm_yield_ct();
    cout << __func__<<" end" << endl;
}


void *foo1(void *s1, void* s2 )
{
    cout << __func__<<" start" << endl;
    coctx_t *ctx = NULL;
    cm_create(&ctx, foo2, 0);
    cm_resume(ctx);
    cm_yield_ct();

    cm_resume(ctx);
    cout << __func__<<" end" << endl;
}

int main()
{
    cout << __func__<<" start" << endl;

    int ret = 0;
    coctx_t *ctx = NULL;
    ret = cm_create(&ctx, foo1, 0);
    cm_resume(ctx);
    cout << __func__<<" end" << endl;
    cm_resume(ctx);
    cout << __func__<<" end1" << endl;

}
