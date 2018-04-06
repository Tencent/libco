//
// Created by dell-pc on 2018/4/5.
//

#include <iostream>
#include <ucontext.h>
#include <malloc.h>
#include <string.h>

using namespace std;

#define __LOG_PLACE  __FILE__<<":"<<__LINE__<<" at "<<__FUNCTION__

ucontext_t main_ctx, cli_ctx,  cli_ctx1;

int SC_COUNT = 1;

void foo1()
{
    cout << __LOG_PLACE << " start" << endl;
    swapcontext(&cli_ctx1, &cli_ctx);
    cout << __LOG_PLACE << " end " << endl;

}

void foo()
{
    getcontext(&cli_ctx1);

    cli_ctx1.uc_link = &cli_ctx;
    cli_ctx1.uc_stack.ss_sp = malloc(1024);
    cli_ctx1.uc_stack.ss_size = 1024;
    cli_ctx1.uc_stack.ss_flags = 0;
    makecontext(&cli_ctx1, &foo1,  0);

    for (int i = 0; i < SC_COUNT*2; ++i) {
        cout << __LOG_PLACE << " entry i="<< i << endl;
        int ret = swapcontext(&cli_ctx, &cli_ctx1);
        if (ret < 0) {
            cout << __LOG_PLACE << " swapcontext err ret="<<ret<<" err="<<strerror(errno) << endl;
        }

        ret = swapcontext(&cli_ctx, &main_ctx);
        if (ret < 0) {
            cout << __LOG_PLACE << " swapcontext err ret="<<ret<<" err="<<strerror(errno) << endl;
        }
        cout << __LOG_PLACE << " end i="<< i << endl;
    }
    cout << __LOG_PLACE << " exit i="<< -1 << endl;

}

int main()
{
    memset(&main_ctx, 0, sizeof(main_ctx));
    memset(&cli_ctx, 0, sizeof(cli_ctx));
    memset(&cli_ctx1, 0, sizeof(cli_ctx1));

    getcontext(&cli_ctx);

    cli_ctx.uc_link = &main_ctx;
    cli_ctx.uc_stack.ss_sp = malloc(1024);
    cli_ctx.uc_stack.ss_size = 1024;
    cli_ctx.uc_stack.ss_flags = 0;
    makecontext(&cli_ctx, &foo,  0);
    int ret;
    for (int i = 0; i < SC_COUNT*2; ++i) {
        cout << __LOG_PLACE << " entry i="<< i << endl;
        ret = swapcontext(&main_ctx, &cli_ctx);
        if (ret < 0) {
            cout << __LOG_PLACE << " swapcontext err ret="<<ret<<" err="<<strerror(errno) << endl;
        }
        cout << __LOG_PLACE << " end i="<< i << endl;
    }

   /* ret = swapcontext(&main_ctx, &cli_ctx);
    if (ret < 0) {
        cout << __LOG_PLACE << " swapcontext err ret="<<ret<<" err="<<strerror(errno) << endl;
    }
    ret = swapcontext(&main_ctx, &cli_ctx);
    if (ret < 0) {
        cout << __LOG_PLACE << " swapcontext err ret="<<ret<<" err="<<strerror(errno) << endl;
    }
    cout << __LOG_PLACE << " end" << endl;*/
}

