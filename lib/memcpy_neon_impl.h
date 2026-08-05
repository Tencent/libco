#ifndef MEMCPY_NEON_IMPL_H
#define MEMCPY_NEON_IMPL_H

#ifdef __aarch64__
#include <arm_neon.h>
#ifdef __cplusplus
extern "C"
{
#endif

#define memcpy_blk_neon(dst, src, ld_ins, st_ins)                               \
    do                                                                         \
    {                                                                          \
        asm volatile(                                                          \
            ld_ins "   q0, q1, [%0]\n"                                         \
            ld_ins "   q2, q3, [%0, #32]\n"                                    \
            ld_ins "   q4, q5, [%0, #64]\n"                                    \
            ld_ins "   q6, q7, [%0, #96]\n"                                    \
            st_ins "    q0, q1, [%1]\n"                                        \
            st_ins "    q2, q3, [%1, #32]\n"                                   \
            st_ins "    q4, q5, [%1, #64]\n"                                   \
            st_ins "    q6, q7, [%1, #96]\n"                                   \
            :                                                                  \
            : "r"(src), "r"(dst)                                               \
            : "memory", "q0", "q1", "q2", "q3", "q4", "q5", "q6", "q7");      \
    } while (0)

#define memcpy_384B_neon(dst, src, len, rem, ld_ins, st_ins)                   \
    do                                                                         \
    {                                                                          \
        asm volatile(                                                          \
            "0:\n"                                                             \
            "cmp    %2, #3\n"                                                  \
            "b.lt   1f\n"                                                      \
            ld_ins "   q0, q1, [%1]\n"                                         \
            ld_ins "   q2, q3, [%1, #32]\n"                                    \
            ld_ins "   q4, q5, [%1, #64]\n"                                    \
            ld_ins "   q6, q7, [%1, #96]\n"                                    \
            st_ins "    q0, q1, [%0]\n"                                        \
            st_ins "    q2, q3, [%0, #32]\n"                                   \
            ld_ins "   q16, q17, [%1, #128]\n"                                 \
            ld_ins "   q18, q19, [%1, #160]\n"                                 \
            st_ins "    q4, q5, [%0, #64]\n"                                   \
            st_ins "    q6, q7, [%0, #96]\n"                                   \
            ld_ins "   q20, q21, [%1, #192]\n"                                 \
            ld_ins "   q22, q23, [%1, #224]\n"                                 \
            st_ins "    q16, q17, [%0, #128]\n"                                \
            st_ins "    q18, q19, [%0, #160]\n"                                \
            ld_ins "   q24, q25, [%1, #256]\n"                                 \
            ld_ins "   q26, q27, [%1, #288]\n"                                 \
            st_ins "    q20, q21, [%0, #192]\n"                                \
            st_ins "    q22, q23, [%0, #224]\n"                                \
            ld_ins "   q28, q29, [%1, #320]\n"                                 \
            ld_ins "   q30, q31, [%1, #352]\n"                                 \
            st_ins "    q24, q25, [%0, #256]\n"                                \
            st_ins "    q26, q27, [%0, #288]\n"                                \
            "sub    %2, %2, #3\n"                                              \
            st_ins "    q28, q29, [%0, #320]\n"                                \
            st_ins "    q30, q31, [%0, #352]\n"                                \
            "add    %1, %1, #384\n"                                            \
            "add    %0, %0, #384\n"                                            \
            "b      0b\n"                                                      \
            "1:\n"                                                             \
            "cmp    %2, #1\n"                                                  \
            "b.lt   2f\n"                                                      \
            ld_ins "   q0, q1, [%1]\n"                                         \
            ld_ins "   q2, q3, [%1, #32]\n"                                    \
            ld_ins "   q4, q5, [%1, #64]\n"                                    \
            st_ins "    q0, q1, [%0]\n"                                        \
            st_ins "    q2, q3, [%0, #32]\n"                                   \
            ld_ins "   q6, q7, [%1, #96]\n"                                    \
            st_ins "    q4, q5, [%0, #64]\n"                                   \
            st_ins "    q6, q7, [%0, #96]\n"                                   \
            "sub    %2, %2, #1\n"                                              \
            "add    %1, %1, #128\n"                                            \
            "add    %0, %0, #128\n"                                            \
            "b      1b\n"                                                      \
            "2:\n"                                                             \
            "cbz    %3, 5f\n"                                                  \
            "3:\n"                                                             \
            "cmp    %3, #32\n"                                                 \
            "b.lt   5f\n"                                                      \
            ld_ins "   q0, q1, [%1]\n"                                         \
            st_ins "    q0, q1, [%0]\n"                                        \
            "add    %1, %1, #32\n"                                             \
            "add    %0, %0, #32\n"                                             \
            "subs   %3, %3, #32\n"                                             \
            "b.ge   3b\n"                                                      \
            "5:\n"                                                             \
            : "+r"(dst), "+r"(src), "+r"(len), "+r"(rem)                       \
            :                                                                  \
            : "memory", "q0", "q1", "q2", "q3", "q4", "q5", "q6", "q7",       \
              "q16", "q17", "q18", "q19", "q20", "q21", "q22", "q23",          \
              "q24", "q25", "q26", "q27", "q28", "q29", "q30", "q31");         \
    } while (0)

#define memcpy_512B_neon(dst, src, len, rem, ld_ins, st_ins)                   \
    do                                                                         \
    {                                                                          \
        asm volatile(                                                          \
            "0:\n"                                                             \
            "cmp    %2, #4\n"                                                  \
            "b.lt   1f\n"                                                      \
            ld_ins "   q0, q1, [%1]\n"                                         \
            ld_ins "   q2, q3, [%1, #32]\n"                                    \
            ld_ins "   q4, q5, [%1, #64]\n"                                    \
            ld_ins "   q6, q7, [%1, #96]\n"                                    \
            st_ins "    q0, q1, [%0]\n"                                        \
            st_ins "    q2, q3, [%0, #32]\n"                                   \
            ld_ins "   q16, q17, [%1, #128]\n"                                 \
            ld_ins "   q18, q19, [%1, #160]\n"                                 \
            st_ins "    q4, q5, [%0, #64]\n"                                   \
            st_ins "    q6, q7, [%0, #96]\n"                                   \
            ld_ins "   q20, q21, [%1, #192]\n"                                 \
            ld_ins "   q22, q23, [%1, #224]\n"                                 \
            st_ins "    q16, q17, [%0, #128]\n"                                \
            st_ins "    q18, q19, [%0, #160]\n"                                 \
            ld_ins "   q0, q1, [%1, #256]\n"                                   \
            ld_ins "   q2, q3, [%1, #288]\n"                                   \
            st_ins "    q20, q21, [%0, #192]\n"                                \
            st_ins "    q22, q23, [%0, #224]\n"                                \
            ld_ins "   q4, q5, [%1, #320]\n"                                   \
            ld_ins "   q6, q7, [%1, #352]\n"                                   \
            st_ins "    q0, q1, [%0, #256]\n"                                  \
            st_ins "    q2, q3, [%0, #288]\n"                                  \
            st_ins "    q4, q5, [%0, #320]\n"                                  \
            st_ins "    q6, q7, [%0, #352]\n"                                  \
            "sub    %2, %2, #4\n"                                              \
            "add    %1, %1, #512\n"                                            \
            "add    %0, %0, #512\n"                                            \
            "b      0b\n"                                                      \
            "1:\n"                                                             \
            "cmp    %2, #1\n"                                                  \
            "b.lt   2f\n"                                                      \
            ld_ins "   q0, q1, [%1]\n"                                         \
            ld_ins "   q2, q3, [%1, #32]\n"                                    \
            ld_ins "   q4, q5, [%1, #64]\n"                                    \
            ld_ins "   q6, q7, [%1, #96]\n"                                    \
            st_ins "    q0, q1, [%0]\n"                                        \
            st_ins "    q2, q3, [%0, #32]\n"                                   \
            st_ins "    q4, q5, [%0, #64]\n"                                   \
            st_ins "    q6, q7, [%0, #96]\n"                                   \
            "sub    %2, %2, #1\n"                                              \
            "add    %1, %1, #128\n"                                            \
            "add    %0, %0, #128\n"                                            \
            "b      1b\n"                                                      \
            "2:\n"                                                             \
            "cbz    %3, 5f\n"                                                  \
            "3:\n"                                                             \
            "cmp    %3, #32\n"                                                 \
            "b.lt   5f\n"                                                      \
            ld_ins "   q0, q1, [%1]\n"                                         \
            st_ins "    q0, q1, [%0]\n"                                        \
            "add    %1, %1, #32\n"                                             \
            "add    %0, %0, #32\n"                                             \
            "subs   %3, %3, #32\n"                                             \
            "b.ge   3b\n"                                                      \
            "5:\n"                                                             \
            : "+r"(dst), "+r"(src), "+r"(len), "+r"(rem)                       \
            :                                                                  \
            : "memory", "q0", "q1", "q2", "q3", "q4", "q5", "q6", "q7",       \
              "q16", "q17", "q18", "q19", "q20", "q21", "q22", "q23");         \
    } while (0)

#define memcpy_1024B_neon(dst, src, len, rem, ld_ins, st_ins)                  \
    do                                                                         \
    {                                                                          \
        asm volatile(                                                          \
            "0:\n"                                                             \
            "cmp    %2, #8\n"                                                  \
            "b.lt   1f\n"                                                      \
            ld_ins "   q0, q1, [%1]\n"                                         \
            ld_ins "   q2, q3, [%1, #32]\n"                                    \
            ld_ins "   q4, q5, [%1, #64]\n"                                    \
            ld_ins "   q6, q7, [%1, #96]\n"                                    \
            st_ins "    q0, q1, [%0]\n"                                        \
            st_ins "    q2, q3, [%0, #32]\n"                                   \
            ld_ins "   q16, q17, [%1, #128]\n"                                 \
            ld_ins "   q18, q19, [%1, #160]\n"                                 \
            st_ins "    q4, q5, [%0, #64]\n"                                   \
            st_ins "    q6, q7, [%0, #96]\n"                                   \
            ld_ins "   q20, q21, [%1, #192]\n"                                 \
            ld_ins "   q22, q23, [%1, #224]\n"                                 \
            st_ins "    q16, q17, [%0, #128]\n"                                \
            st_ins "    q18, q19, [%0, #160]\n"                                \
            ld_ins "   q0, q1, [%1, #256]\n"                                   \
            ld_ins "   q2, q3, [%1, #288]\n"                                   \
            st_ins "    q20, q21, [%0, #192]\n"                                \
            st_ins "    q22, q23, [%0, #224]\n"                                \
            ld_ins "   q4, q5, [%1, #320]\n"                                   \
            ld_ins "   q6, q7, [%1, #352]\n"                                   \
            st_ins "    q0, q1, [%0, #256]\n"                                  \
            st_ins "    q2, q3, [%0, #288]\n"                                  \
            ld_ins "   q16, q17, [%1, #384]\n"                                 \
            ld_ins "   q18, q19, [%1, #416]\n"                                 \
            st_ins "    q4, q5, [%0, #320]\n"                                  \
            st_ins "    q6, q7, [%0, #352]\n"                                  \
            ld_ins "   q20, q21, [%1, #448]\n"                                 \
            ld_ins "   q22, q23, [%1, #480]\n"                                 \
            st_ins "    q16, q17, [%0, #384]\n"                                \
            st_ins "    q18, q19, [%0, #416]\n"                                \
            st_ins "    q20, q21, [%0, #448]\n"                                \
            st_ins "    q22, q23, [%0, #480]\n"                                \
            "sub    %2, %2, #8\n"                                              \
            "add    %1, %1, #1024\n"                                           \
            "add    %0, %0, #1024\n"                                           \
            "b      0b\n"                                                      \
            "1:\n"                                                             \
            "cmp    %2, #4\n"                                                  \
            "b.lt   2f\n"                                                      \
            ld_ins "   q0, q1, [%1]\n"                                         \
            ld_ins "   q2, q3, [%1, #32]\n"                                    \
            ld_ins "   q4, q5, [%1, #64]\n"                                    \
            ld_ins "   q6, q7, [%1, #96]\n"                                    \
            st_ins "    q0, q1, [%0]\n"                                        \
            st_ins "    q2, q3, [%0, #32]\n"                                   \
            ld_ins "   q16, q17, [%1, #128]\n"                                 \
            ld_ins "   q18, q19, [%1, #160]\n"                                 \
            st_ins "    q4, q5, [%0, #64]\n"                                   \
            st_ins "    q6, q7, [%0, #96]\n"                                   \
            ld_ins "   q20, q21, [%1, #192]\n"                                 \
            ld_ins "   q22, q23, [%1, #224]\n"                                 \
            st_ins "    q16, q17, [%0, #128]\n"                                \
            st_ins "    q18, q19, [%0, #160]\n"                                \
            st_ins "    q20, q21, [%0, #192]\n"                                \
            st_ins "    q22, q23, [%0, #224]\n"                                \
            "sub    %2, %2, #4\n"                                              \
            "add    %1, %1, #512\n"                                            \
            "add    %0, %0, #512\n"                                            \
            "b      1b\n"                                                      \
            "2:\n"                                                             \
            "cmp    %2, #1\n"                                                  \
            "b.lt   3f\n"                                                      \
            ld_ins "   q0, q1, [%1]\n"                                         \
            ld_ins "   q2, q3, [%1, #32]\n"                                    \
            ld_ins "   q4, q5, [%1, #64]\n"                                    \
            ld_ins "   q6, q7, [%1, #96]\n"                                    \
            st_ins "    q0, q1, [%0]\n"                                        \
            st_ins "    q2, q3, [%0, #32]\n"                                   \
            st_ins "    q4, q5, [%0, #64]\n"                                   \
            st_ins "    q6, q7, [%0, #96]\n"                                   \
            "sub    %2, %2, #1\n"                                              \
            "add    %1, %1, #128\n"                                            \
            "add    %0, %0, #128\n"                                            \
            "b      2b\n"                                                      \
            "3:\n"                                                             \
            "cbz    %3, 5f\n"                                                  \
            "4:\n"                                                             \
            "cmp    %3, #32\n"                                                 \
            "b.lt   5f\n"                                                      \
            ld_ins "   q0, q1, [%1]\n"                                         \
            st_ins "    q0, q1, [%0]\n"                                        \
            "add    %1, %1, #32\n"                                             \
            "add    %0, %0, #32\n"                                             \
            "subs   %3, %3, #32\n"                                             \
            "b.ge   4b\n"                                                      \
            "5:\n"                                                             \
            : "+r"(dst), "+r"(src), "+r"(len), "+r"(rem)                       \
            :                                                                  \
            : "memory", "q0", "q1", "q2", "q3", "q4", "q5", "q6", "q7",       \
              "q16", "q17", "q18", "q19", "q20", "q21", "q22", "q23");         \
    } while (0)

#define memcpy_1536B_neon(dst, src, len, rem, ld_ins, st_ins)                  \
    do                                                                         \
    {                                                                          \
        asm volatile(                                                          \
            "0:\n"                                                             \
            "cmp    %2, #12\n"                                                 \
            "b.lt   1f\n"                                                      \
            ld_ins "   q0, q1, [%1]\n"                                         \
            ld_ins "   q2, q3, [%1, #32]\n"                                    \
            ld_ins "   q4, q5, [%1, #64]\n"                                    \
            ld_ins "   q6, q7, [%1, #96]\n"                                    \
            st_ins "    q0, q1, [%0]\n"                                        \
            st_ins "    q2, q3, [%0, #32]\n"                                   \
            ld_ins "   q16, q17, [%1, #128]\n"                                 \
            ld_ins "   q18, q19, [%1, #160]\n"                                 \
            st_ins "    q4, q5, [%0, #64]\n"                                   \
            st_ins "    q6, q7, [%0, #96]\n"                                   \
            ld_ins "   q20, q21, [%1, #192]\n"                                 \
            ld_ins "   q22, q23, [%1, #224]\n"                                 \
            st_ins "    q16, q17, [%0, #128]\n"                                \
            st_ins "    q18, q19, [%0, #160]\n"                                \
            ld_ins "   q0, q1, [%1, #256]\n"                                   \
            ld_ins "   q2, q3, [%1, #288]\n"                                   \
            st_ins "    q20, q21, [%0, #192]\n"                                \
            st_ins "    q22, q23, [%0, #224]\n"                                \
            ld_ins "   q4, q5, [%1, #320]\n"                                   \
            ld_ins "   q6, q7, [%1, #352]\n"                                   \
            st_ins "    q0, q1, [%0, #256]\n"                                  \
            st_ins "    q2, q3, [%0, #288]\n"                                  \
            ld_ins "   q16, q17, [%1, #384]\n"                                 \
            ld_ins "   q18, q19, [%1, #416]\n"                                 \
            st_ins "    q4, q5, [%0, #320]\n"                                  \
            st_ins "    q6, q7, [%0, #352]\n"                                  \
            ld_ins "   q20, q21, [%1, #448]\n"                                 \
            ld_ins "   q22, q23, [%1, #480]\n"                                 \
            st_ins "    q16, q17, [%0, #384]\n"                                \
            st_ins "    q18, q19, [%0, #416]\n"                                \
            ld_ins "   q0, q1, [%1, #512]\n"                                   \
            ld_ins "   q2, q3, [%1, #544]\n"                                   \
            st_ins "    q20, q21, [%0, #448]\n"                                \
            st_ins "    q22, q23, [%0, #480]\n"                                \
            ld_ins "   q4, q5, [%1, #576]\n"                                   \
            ld_ins "   q6, q7, [%1, #608]\n"                                   \
            st_ins "    q0, q1, [%0, #512]\n"                                  \
            st_ins "    q2, q3, [%0, #544]\n"                                  \
            ld_ins "   q16, q17, [%1, #640]\n"                                 \
            ld_ins "   q18, q19, [%1, #672]\n"                                 \
            st_ins "    q4, q5, [%0, #576]\n"                                  \
            st_ins "    q6, q7, [%0, #608]\n"                                  \
            ld_ins "   q20, q21, [%1, #704]\n"                                 \
            ld_ins "   q22, q23, [%1, #736]\n"                                 \
            st_ins "    q16, q17, [%0, #640]\n"                                \
            st_ins "    q18, q19, [%0, #672]\n"                                \
            st_ins "    q20, q21, [%0, #704]\n"                                \
            st_ins "    q22, q23, [%0, #736]\n"                                \
            "sub    %2, %2, #12\n"                                             \
            "add    %1, %1, #1536\n"                                           \
            "add    %0, %0, #1536\n"                                           \
            "b      0b\n"                                                      \
            "1:\n"                                                             \
            "cmp    %2, #8\n"                                                  \
            "b.lt   2f\n"                                                      \
            ld_ins "   q0, q1, [%1]\n"                                         \
            ld_ins "   q2, q3, [%1, #32]\n"                                    \
            ld_ins "   q4, q5, [%1, #64]\n"                                    \
            ld_ins "   q6, q7, [%1, #96]\n"                                    \
            st_ins "    q0, q1, [%0]\n"                                        \
            st_ins "    q2, q3, [%0, #32]\n"                                   \
            ld_ins "   q16, q17, [%1, #128]\n"                                 \
            ld_ins "   q18, q19, [%1, #160]\n"                                 \
            st_ins "    q4, q5, [%0, #64]\n"                                   \
            st_ins "    q6, q7, [%0, #96]\n"                                   \
            ld_ins "   q20, q21, [%1, #192]\n"                                 \
            ld_ins "   q22, q23, [%1, #224]\n"                                 \
            st_ins "    q16, q17, [%0, #128]\n"                                \
            st_ins "    q18, q19, [%0, #160]\n"                                \
            st_ins "    q20, q21, [%0, #192]\n"                                \
            st_ins "    q22, q23, [%0, #224]\n"                                \
            "sub    %2, %2, #8\n"                                              \
            "add    %1, %1, #1024\n"                                           \
            "add    %0, %0, #1024\n"                                           \
            "b      1b\n"                                                      \
            "2:\n"                                                             \
            "cmp    %2, #4\n"                                                  \
            "b.lt   3f\n"                                                      \
            ld_ins "   q0, q1, [%1]\n"                                         \
            ld_ins "   q2, q3, [%1, #32]\n"                                    \
            ld_ins "   q4, q5, [%1, #64]\n"                                    \
            ld_ins "   q6, q7, [%1, #96]\n"                                    \
            st_ins "    q0, q1, [%0]\n"                                        \
            st_ins "    q2, q3, [%0, #32]\n"                                   \
            ld_ins "   q16, q17, [%1, #128]\n"                                 \
            ld_ins "   q18, q19, [%1, #160]\n"                                 \
            st_ins "    q4, q5, [%0, #64]\n"                                   \
            st_ins "    q6, q7, [%0, #96]\n"                                   \
            st_ins "    q16, q17, [%0, #128]\n"                                \
            st_ins "    q18, q19, [%0, #160]\n"                                \
            "sub    %2, %2, #4\n"                                              \
            "add    %1, %1, #512\n"                                            \
            "add    %0, %0, #512\n"                                            \
            "b      2b\n"                                                      \
            "3:\n"                                                             \
            "cmp    %2, #1\n"                                                  \
            "b.lt   4f\n"                                                      \
            ld_ins "   q0, q1, [%1]\n"                                         \
            ld_ins "   q2, q3, [%1, #32]\n"                                    \
            ld_ins "   q4, q5, [%1, #64]\n"                                    \
            ld_ins "   q6, q7, [%1, #96]\n"                                    \
            st_ins "    q0, q1, [%0]\n"                                        \
            st_ins "    q2, q3, [%0, #32]\n"                                   \
            st_ins "    q4, q5, [%0, #64]\n"                                   \
            st_ins "    q6, q7, [%0, #96]\n"                                   \
            "sub    %2, %2, #1\n"                                              \
            "add    %1, %1, #128\n"                                            \
            "add    %0, %0, #128\n"                                            \
            "b      3b\n"                                                      \
            "4:\n"                                                             \
            "cbz    %3, 6f\n"                                                  \
            "5:\n"                                                             \
            "cmp    %3, #32\n"                                                 \
            "b.lt   6f\n"                                                      \
            ld_ins "   q0, q1, [%1]\n"                                         \
            st_ins "    q0, q1, [%0]\n"                                        \
            "add    %1, %1, #32\n"                                             \
            "add    %0, %0, #32\n"                                             \
            "subs   %3, %3, #32\n"                                             \
            "b.ge   5b\n"                                                      \
            "6:\n"                                                             \
            : "+r"(dst), "+r"(src), "+r"(len), "+r"(rem)                       \
            :                                                                  \
            : "memory", "q0", "q1", "q2", "q3", "q4", "q5", "q6", "q7",       \
              "q16", "q17", "q18", "q19", "q20", "q21", "q22", "q23");         \
    } while (0)

#ifdef __cplusplus
}

#endif

#endif
#endif
