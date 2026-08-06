#ifndef MEMCPY_DEF_H
#define MEMCPY_DEF_H

#define CACHE_LINE 64
#define BLOCK_SIZE (2 * CACHE_LINE)

#define memcpy_blk(temp_dst, temp_src, len_count, memcpy_func) \
    do                                                         \
    {                                                          \
        for (int i = 0; i < (len_count); i++)                  \
        {                                                      \
            memcpy_func(temp_dst, temp_src);                   \
            temp_dst += BLOCK_SIZE;                            \
            temp_src += BLOCK_SIZE;                            \
        }                                                      \
    } while (0)

#define memcpy_rem(dst, src, len)                                             \
    do                                                                        \
    {                                                                         \
        asm volatile("add    x4, %1, %2\n"                                    \
                     "add    x5, %0, %2\n"                                    \
                     "cmp    %2, #32\n"                                       \
                     "b.hi   .copy32_127%=\n"                                 \
                     "cmp    %2, #16\n"                                       \
                     "b.lo   .copy16%=\n"                                     \
                     "ldr    q0, [%1]\n"                                      \
                     "ldr    q1, [x4, #-16]\n"                                \
                     "str    q0, [%0]\n"                                      \
                     "str    q1, [x5, #-16]\n"                                \
                     "b      .exit%=\n"                                       \
                     ".copy16%=: \n"                                          \
                     "tbz    %2, #3, .copy8%=\n"                              \
                     "ldr    x6, [%1]\n"                                      \
                     "ldr    x7, [x4, #-8]\n"                                 \
                     "str    x6, [%0]\n"                                      \
                     "str    x7, [x5, #-8]\n"                                 \
                     "b      .exit%=\n"                                       \
                     ".copy8%=: \n"                                           \
                     "tbz    %2, #2, .copy4%=\n"                              \
                     "ldr    w6, [%1]\n"                                      \
                     "ldr    w8, [x4, #-4]\n"                                 \
                     "str    w6, [%0]\n"                                      \
                     "str    w8, [x5, #-4]\n"                                 \
                     "b      .exit%=\n"                                       \
                     ".copy4%=: \n"                                           \
                     "cbz     %2, .copy0%=\n"                                 \
                     "lsr     x14, %2, #1\n"                                  \
                     "ldrb    w6, [%1]\n"                                     \
                     "ldrb    w10, [x4, -1]\n"                                \
                     "ldrb    w8, [%1, x14]\n"                                \
                     "strb    w6, [%0]\n"                                     \
                     "strb    w8, [%0, x14]\n"                                \
                     "strb    w10, [x5, -1]\n"                                \
                     ".copy0%=: \n"                                           \
                     "b      .exit%=\n"                                       \
                     ".p2align 4\n"                                           \
                     ".copy32_127%=: \n"                                      \
                     "ldp    q0, q1, [%1]\n"                                  \
                     "ldp    q2, q3, [x4, #-32]\n"                            \
                     "cmp    %2, #64\n"                                       \
                     "b.hi   .copy127%=\n"                                    \
                     "stp    q0, q1, [%0]\n"                                  \
                     "stp    q2, q3, [x5, #-32]\n"                            \
                     "b      .exit%=\n"                                       \
                     ".p2align 4\n"                                           \
                     ".copy127%=: \n"                                         \
                     "ldp    q4, q5, [%1, #32]\n"                             \
                     "cmp    %2, #96\n"                                       \
                     "b.ls   .copy96%=\n"                                     \
                     "ldp    q6, q7, [x4, #-64]\n"                            \
                     "stp    q6, q7, [x5, #-64]\n"                            \
                     ".copy96%=: \n"                                          \
                     "stp    q0, q1, [%0]\n"                                  \
                     "stp    q4, q5, [%0, #32]\n"                             \
                     "stp    q2, q3, [x5, #-32]\n"                            \
                     ".exit%=: \n"                                            \
                     :: "r"(dst), "r"(src), "r"(len)                          \
                     : "x4", "x5", "x6", "x7", "x14", "q0", "q1", "q2", "q3", \
                       "q4", "q5", "q6", "q7", "w6", "w8", "w10");     \
    } while (0)

#define memcpy_blk_save(dst, src)    memcpy_blk_neon(dst, src, "ldp",  "stnp")
#define memcpy_blk_swap(dst, src)    memcpy_blk_neon(dst, src, "ldnp", "stp")

#define memcpy_256B_save(dst, src, len, rem)  memcpy_256B_neon(dst, src, len, rem, "ldp",  "stnp")
#define memcpy_256B_swap(dst, src, len, rem)  memcpy_256B_neon(dst, src, len, rem, "ldnp", "stp")

#define memcpy_384B_save(dst, src, len, rem)  memcpy_384B_neon(dst, src, len, rem, "ldp",  "stnp")
#define memcpy_384B_swap(dst, src, len, rem)  memcpy_384B_neon(dst, src, len, rem, "ldnp", "stp")

#define memcpy_512B_save(dst, src, len, rem)  memcpy_512B_neon(dst, src, len, rem, "ldp",  "stnp")
#define memcpy_512B_swap(dst, src, len, rem)  memcpy_512B_neon(dst, src, len, rem, "ldnp", "stp")

#define memcpy_1024B_save(dst, src, len, rem)  memcpy_1024B_neon(dst, src, len, rem, "ldp",  "stnp")
#define memcpy_1024B_swap(dst, src, len, rem)  memcpy_1024B_neon(dst, src, len, rem, "ldnp", "stp")

#define memcpy_1536B_save(dst, src, len, rem)  memcpy_1536B_neon(dst, src, len, rem, "ldp",  "stnp")
#define memcpy_1536B_swap(dst, src, len, rem)  memcpy_1536B_neon(dst, src, len, rem, "ldnp", "stp")

#endif /* MEMCPY_DEF_H */