//
// Created by dell-pc on 2018/4/5.
//

#ifndef LIBCO_M_ROUTINE_H
#define LIBCO_M_ROUTINE_H

#include "coctx.h"
#include <map>

using namespace std;

typedef void *(*pfn_n_routine_t)( void * );


int cm_create(coctx_t **ctx, coctx_pfn_t pfn, const void *s1);

int cm_resume(coctx_t *ctx);

int cm_yield_ct();


#endif //LIBCO_M_ROUTINE_H
