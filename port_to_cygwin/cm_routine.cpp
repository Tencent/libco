//
// Created by dell-pc on 2018/4/5.
//

#include <stdlib.h>
#include <malloc.h>
#include <ucontext.h>
#include <iostream>
#include <string.h>
#include "cm_routine.h"

using  namespace std;

#define __LOG_PLACE  __FILE__<<":"<<__LINE__<<" at "<<__FUNCTION__

extern "C"
{
    extern void coctx_swap( coctx_t *,coctx_t* ) asm("coctx_swap");
};

static coctx_t* g_ctx_arr[100];
static int g_ctx_size = 0;
static int g_ctx_init = 0;

void *my_alloc(size_t n)
{
#if 1
    void *pp =  malloc(n);
    memset(pp, 0, n);
    return pp;
#else
    static char memory[1024*1024*10];
    static int alloc_size = 0;
    void *p = &memory[alloc_size];
    alloc_size+=n;
    return p;
#endif
}

static int __init_env()
{
    coctx_t *ctx = (coctx_t *) my_alloc(sizeof(ctx));
    coctx_t *pctx = ctx;
//    coctx_init(pctx);

    pctx->ss_sp = (char *) my_alloc(1024);
    pctx->ss_size = 1024;
    pctx->ucontext = my_alloc(sizeof(ucontext_t));

//    coctx_make(pctx, NULL, NULL, NULL);
    g_ctx_arr[g_ctx_size++] = pctx;
}

int cm_create(coctx_t **ctx, coctx_pfn_t pfn, const void *s)
{
    if (!g_ctx_init) {
        g_ctx_init = 1;
        __init_env();
    }
    *ctx = (coctx_t *) my_alloc(sizeof(coctx_t));
    coctx_t *pctx = *ctx;
//    coctx_init(pctx);
    pctx->ss_sp = (char *) my_alloc(1024);
    pctx->ss_size = 1024;

    pctx->ucontext = my_alloc(sizeof(ucontext_t));

    getcontext((ucontext_t *) pctx->ucontext);
    ((ucontext_t*)pctx->ucontext)->uc_stack.ss_sp = my_alloc(1024);
    ((ucontext_t*)pctx->ucontext)->uc_stack.ss_size = 1024;
    ((ucontext_t*)pctx->ucontext)->uc_stack.ss_flags = 0;
    ((ucontext_t*)pctx->ucontext)->uc_link = (__ucontext *) g_ctx_arr[g_ctx_size - 1]->ucontext;
    makecontext((ucontext_t *) pctx->ucontext, pfn, 2,  pctx->ucontext, s);

//    coctx_make(pctx, pfn, pctx, s);
//    g_ctx_arr[g_ctx_size++] = pctx;
    return 0;
}

int cm_resume(coctx_t *ctx)
{
    coctx_t *curr_ctx = g_ctx_arr[g_ctx_size - 1];
    g_ctx_arr[g_ctx_size++] = ctx;
//    coctx_swap( curr_ctx, ctx );
    swapcontext((ucontext_t *) curr_ctx->ucontext, (const ucontext_t *) ctx->ucontext);
//    cout << __LOG_PLACE << " here " << endl;
}

int cm_yield_ct()
{
    coctx_t *last = g_ctx_arr[g_ctx_size - 2];
    coctx_t *curr = g_ctx_arr[g_ctx_size - 1];
    g_ctx_size--;
    int ret = swapcontext((ucontext_t *) curr->ucontext, (const ucontext_t *) last->ucontext);
    if (ret < 0) {
        cout << __LOG_PLACE << "swapcontext err ret="<<ret << " err=" << strerror(errno) << endl;
    }

//    coctx_swap(curr, last);

}
