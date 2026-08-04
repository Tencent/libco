#ifndef CO_MEMCPY_H
#define CO_MEMCPY_H

#ifdef __cplusplus
extern "C" {
#endif

void *co_memcpy_save(void *__restrict dst, const void *__restrict src, unsigned long len);
void *co_memcpy_swap(void *__restrict dst, const void *__restrict src, unsigned long len);

#ifdef __cplusplus
}
#endif

#endif /* CO_MEMCPY_H */