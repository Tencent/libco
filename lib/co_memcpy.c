#include "co_memcpy.h"
#include <string.h>
#include "memcpy_def.h"
#include "memcpy_neon_impl.h"

#define SIZE_128B 128
#define SIZE_1KB 1024

static void *co_memcpy_save_aarch64(void *restrict dst, const void *restrict src, unsigned long len)
{
    void *dst_ret = dst;
#ifdef __aarch64__
    if (len < SIZE_128B) {
        memcpy_rem(dst, src, len);
        return dst_ret;
    }

    unsigned long align_offset = (unsigned long)dst & 0x3f;
    if (align_offset) {
        memcpy_rem(dst, src, align_offset);
        dst += align_offset;
        src += align_offset;
        len -= align_offset;
    }

    int len_count = len / BLOCK_SIZE;
    int rem = len % BLOCK_SIZE;
    if (len <= SIZE_1KB) {
        memcpy_blk(dst, src, len_count, memcpy_blk_save);
        if (rem) {
            memcpy_rem(dst, src, rem);
        }
    } else {
        memcpy_384B_save(dst, src, len_count, rem);
        if (rem) {
            memcpy_rem(dst, src, rem);
        }
    }
#endif
    return dst_ret;
}

static void *co_memcpy_swap_aarch64(void *restrict dst, const void *restrict src, unsigned long len)
{
    void *dst_ret = dst;
#ifdef __aarch64__
    if (len < SIZE_128B) {
        memcpy_rem(dst, src, len);
        return dst_ret;
    }

    unsigned long align_offset = (unsigned long)dst & 0x3f;
    if (align_offset) {
        memcpy_rem(dst, src, align_offset);
        dst += align_offset;
        src += align_offset;
        len -= align_offset;
    }

    int len_count = len / BLOCK_SIZE;
    int rem = len % BLOCK_SIZE;
    if (len <= SIZE_1KB) {
        memcpy_blk(dst, src, len_count, memcpy_blk_swap);
        if (rem) {
            memcpy_rem(dst, src, rem);
        }
    } else {
        memcpy_384B_swap(dst, src, len_count, rem);
        if (rem) {
            memcpy_rem(dst, src, rem);
        }
    }
#endif
    return dst_ret;
}

typedef void *(*memcpy_func_name)(void *restrict dst, const void *restrict src, unsigned long len);

static memcpy_func_name memcpy_save_func = (memcpy_func_name)memcpy;
static memcpy_func_name memcpy_swap_func = (memcpy_func_name)memcpy;

static void __attribute__((constructor)) model_init(void)
{
#ifdef __aarch64__
    memcpy_save_func = (memcpy_func_name)co_memcpy_save_aarch64;
    memcpy_swap_func = (memcpy_func_name)co_memcpy_swap_aarch64;
#endif
    return;
}

void *co_memcpy_save(void *restrict dst, const void *restrict src, unsigned long len)
{
    return memcpy_save_func(dst, src, len);
}

void *co_memcpy_swap(void *restrict dst, const void *restrict src, unsigned long len)
{
    return memcpy_swap_func(dst, src, len);
}
