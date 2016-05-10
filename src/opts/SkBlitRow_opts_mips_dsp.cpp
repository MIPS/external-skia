/*
 * Copyright 2014 The Android Open Source Project
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "SkBlitRow.h"
#include "SkBlitMask.h"
#include "SkColorPriv.h"
#include "SkDither.h"
#include "SkMathPriv.h"

#define STR1(x) #x
#define STR(x) STR1(x)
static void S32_D565_Blend_mips_dsp(uint16_t* SK_RESTRICT dst,
                                    const SkPMColor* SK_RESTRICT src, int count,
                                    U8CPU alpha, int /*x*/, int /*y*/) {
#ifdef SK_MIPS_HAS_DSPR2
    int temp0, temp1, temp2, temp3, temp4;
    int temp5, temp6, temp7, temp8, temp9;
    uint16_t* loopEnd = dst + (count & ~1);
    alpha += 1;

    if (count >= 2) {
        __asm__ volatile (
           ".set       push                                                                                               \n\t"
           ".set       noreorder                                                                                          \n\t"
            "replv.ph  %[temp0],    %[alpha]                                                                              \n\t"
        "1:                                                                                                               \n\t"
            "lw        %[temp1],    0(%[src])                                                                             \n\t"
            "lw        %[temp2],    4(%[src])                                                                             \n\t"
            "ulw       %[temp3],    0(%[dst])                                                                             \n\t"
            "ext       %[temp4],    %[temp1],    " STR(SK_R32_SHIFT + SK_R32_BITS - SK_R16_BITS)",  " STR(SK_R16_BITS)"   \n\t"
            "ext       %[temp5],    %[temp1],    " STR(SK_G32_SHIFT + SK_G32_BITS - SK_G16_BITS)",  " STR(SK_G16_BITS)"   \n\t"
            "ext       %[temp6],    %[temp1],    " STR(SK_B32_SHIFT + SK_B32_BITS - SK_B16_BITS)",  " STR(SK_B16_BITS)"   \n\t"
            "ext       %[temp7],    %[temp2],    " STR(SK_R32_SHIFT + SK_R32_BITS - SK_R16_BITS)",  " STR(SK_R16_BITS)"   \n\t"
            "ext       %[temp8],    %[temp2],    " STR(SK_G32_SHIFT + SK_G32_BITS - SK_G16_BITS)",  " STR(SK_G16_BITS)"   \n\t"
            "ext       %[temp9],    %[temp2],    " STR(SK_B32_SHIFT + SK_B32_BITS - SK_B16_BITS)",  " STR(SK_B16_BITS)"   \n\t"
            "append    %[temp7],    %[temp4],    16                                                                       \n\t"
            "append    %[temp8],    %[temp5],    16                                                                       \n\t"
            "append    %[temp9],    %[temp6],    16                                                                       \n\t"
            "ext       %[temp4],    %[temp3],    " STR(SK_R16_SHIFT)",                              " STR(SK_R16_BITS)"   \n\t"
            "ext       %[temp5],    %[temp3],    " STR(SK_G16_SHIFT)",                              " STR(SK_G16_BITS)"   \n\t"
            "ext       %[temp6],    %[temp3],    " STR(SK_B16_SHIFT)",                              " STR(SK_B16_BITS)"   \n\t"
            "ext       %[temp1],    %[temp3],    " STR(SK_R16_SHIFT + 16)",                         " STR(SK_R16_BITS)"   \n\t"
            "ext       %[temp2],    %[temp3],    " STR(SK_G16_SHIFT + 16)",                         " STR(SK_G16_BITS)"   \n\t"
            "ext       %[temp3],    %[temp3],    " STR(SK_B16_SHIFT + 16)",                         " STR(SK_B16_BITS)"   \n\t"
            "append    %[temp1],    %[temp4],    16                                                                       \n\t"
            "append    %[temp2],    %[temp5],    16                                                                       \n\t"
            "append    %[temp3],    %[temp6],    16                                                                       \n\t"
            "subq.ph   %[temp7],    %[temp7],    %[temp1]                                                                 \n\t"
            "subq.ph   %[temp8],    %[temp8],    %[temp2]                                                                 \n\t"
            "subq.ph   %[temp9],    %[temp9],    %[temp3]                                                                 \n\t"
            "mul.ph    %[temp7],    %[temp7],    %[temp0]                                                                 \n\t"
            "mul.ph    %[temp8],    %[temp8],    %[temp0]                                                                 \n\t"
            "mul.ph    %[temp9],    %[temp9],    %[temp0]                                                                 \n\t"
            "shra.ph   %[temp7],    %[temp7],    8                                                                        \n\t"
            "shra.ph   %[temp8],    %[temp8],    8                                                                        \n\t"
            "shra.ph   %[temp9],    %[temp9],    8                                                                        \n\t"
            "addq.ph   %[temp7],    %[temp7],    %[temp1]                                                                 \n\t"
            "addq.ph   %[temp8],    %[temp8],    %[temp2]                                                                 \n\t"
            "addq.ph   %[temp9],    %[temp9],    %[temp3]                                                                 \n\t"
            "shll.ph   %[temp7],    %[temp7],    " STR(SK_R16_SHIFT)"                                                     \n\t"
            "shll.ph   %[temp8],    %[temp8],    " STR(SK_G16_SHIFT)"                                                     \n\t"
            "shll.ph   %[temp9],    %[temp9],    " STR(SK_B16_SHIFT)"                                                     \n\t"
            "or        %[temp7],    %[temp7],    %[temp8]                                                                 \n\t"
            "or        %[temp7],    %[temp7],    %[temp9]                                                                 \n\t"
            "usw       %[temp7],    0(%[dst])                                                                             \n\t"
            "addiu     %[dst],      %[dst],      4                                                                        \n\t"
            "bne       %[dst],      %[loopEnd], 1b                                                                        \n\t"
            " addiu    %[src],      %[src],      8                                                                        \n\t"
            ".set      pop                                                                                                \n\t"

            : [src]"+&r"(src), [dst]"+&r"(dst), [temp0]"=&r"(temp0), [temp1]"=&r"(temp1),
              [temp2]"=&r"(temp2), [temp3]"=&r"(temp3), [temp4]"=&r"(temp4),
              [temp5]"=&r"(temp5), [temp6]"=&r"(temp6), [temp7]"=&r"(temp7),
              [temp8]"=&r"(temp8), [temp9]"=&r"(temp9)
            : [loopEnd]"r"(loopEnd), [alpha]"r"(alpha)
            : "memory", "hi", "lo"
        );
    }

    if (count & 1) {
        SkPMColor c = *src++;
        SkPMColorAssert(c);
        SkASSERT(SkGetPackedA32(c) == 255);
        uint16_t d = *dst;
        *dst++ = SkPackRGB16(SkAlphaBlend(SkPacked32ToR16(c), SkGetPackedR16(d), alpha),
                             SkAlphaBlend(SkPacked32ToG16(c), SkGetPackedG16(d), alpha),
                             SkAlphaBlend(SkPacked32ToB16(c), SkGetPackedB16(d), alpha));
    }
#else
    if (count > 0) {
        int scale = SkAlpha255To256(alpha);
        register uint32_t t0, t1, t2, t3, t4, t5, t6;
        register uint32_t s0, s1, s2, s3, s4, s5, s6;

        do {
            __asm__ volatile (
               ".set             push                          \n\t"
               ".set             noreorder                     \n\t"
                "lw              %[s0],    0(%[src])           \n\t"
                "lh              %[t0],    0(%[dst])           \n\t"
                "srl             %[t2],    %[t0],    5         \n\t"
                "srl             %[t3],    %[t0],    11        \n\t"
                "andi            %[t1],    %[t0],    0x1f      \n\t"
                "andi            %[t2],    %[t2],    0x3f      \n\t"
                "andi            %[t3],    %[t3],    0x1f      \n\t"
                "srl             %[s3],    %[s0],    19        \n\t"
                "srl             %[s2],    %[s0],    10        \n\t"
                "srl             %[s1],    %[s0],    3         \n\t"
                "andi            %[s3],    %[s3],    0x1f      \n\t"
                "andi            %[s2],    %[s2],    0x3f      \n\t"
                "andi            %[s1],    %[s1],    0x1f      \n\t"
                "subu            %[t6],    %[s3],    %[t3]     \n\t"
                "subu            %[t5],    %[s2],    %[t2]     \n\t"
                "subu            %[t4],    %[s1],    %[t1]     \n\t"
                "mul             %[t6],    %[t6],    %[scale]  \n\t"
                "mul             %[t5],    %[t5],    %[scale]  \n\t"
                "mul             %[t4],    %[t4],    %[scale]  \n\t"
                "srl             %[t6],    %[t6],    8         \n\t"
                "srl             %[t5],    %[t5],    8         \n\t"
                "srl             %[t4],    %[t4],    8         \n\t"
                "addu            %[t6],    %[t6],    %[t3]     \n\t"
                "addu            %[t5],    %[t5],    %[t2]     \n\t"
                "addu            %[t4],    %[t4],    %[t1]     \n\t"
                "sll             %[t6],    %[t6],    11        \n\t"
                "sll             %[t5],    %[t5],    5         \n\t"
                "or              %[t6],    %[t6],    %[t5]     \n\t"
                "or              %[t6],    %[t6],    %[t4]     \n\t"
                "sh              %[t6],    0(%[dst])           \n\t"
                ".set            pop                           \n\t"
                : [t0]"=&r"(t0), [t1]"=&r"(t1), [t2]"=&r"(t2), [t3]"=&r"(t3),
                  [t4]"=&r"(t4), [t5]"=&r"(t5), [t6]"=&r"(t6), [s0]"=&r"(s0),
                  [s1]"=&r"(s1), [s2]"=&r"(s2), [s4]"=&r"(s4), [s3]"=&r"(s3),
                  [s5]"=&r"(s5), [s6]"=&r"(s6), [count]"+r"(count),
                  [dst]"+r"(dst), [src]"+r"(src)
                : [scale]"r"(scale)
                : "memory", "hi", "lo"
            );
            src++;
            dst++;
        } while (--count != 0);
    }
#endif // SK_MIPS_HAS_DSPR2
}

static void S32A_D565_Opaque_Dither_mips_dsp(uint16_t* __restrict__ dst,
                                             const SkPMColor* __restrict__ src,
                                             int count, U8CPU alpha, int x, int y) {
#ifdef SK_MIPS_HAS_DSPR2
    int temp0, temp1, temp2, temp3, temp4, temp5;
    int temp6, temp7, temp8, temp9, temp10;
    uint16_t* loopEnd = dst + (count & ~1);
    extern const uint16_t gDitherMatrix_3Bit_16[4];
    const uint16_t dither_scan = gDitherMatrix_3Bit_16[(y) & 3];
    uint32_t dithers[4];
    const uint32_t c1 = 0x10001;
    const uint32_t c256 = 256;

    __asm__ volatile (
        "ext   %[temp0],  %[dither_scan],  0,   4        \n\t"
        "ext   %[temp1],  %[dither_scan],  4,   4        \n\t"
        "ext   %[temp2],  %[dither_scan],  8,   4        \n\t"
        "ext   %[temp3],  %[dither_scan],  12,  4        \n\t"
        "ins   %[temp0],  %[temp1],        16,  4        \n\t"
        "ins   %[temp1],  %[temp2],        16,  4        \n\t"
        "ins   %[temp2],  %[temp3],        16,  4        \n\t"
        "ins   %[temp3],  %[temp0],        16,  4        \n\t"
        "sw    %[temp0],  0(%[dithers])                  \n\t"
        "sw    %[temp1],  4(%[dithers])                  \n\t"
        "sw    %[temp2],  8(%[dithers])                  \n\t"
        "sw    %[temp3],  12(%[dithers])                 \n\t"

        : [temp0]"=&r"(temp0), [temp1]"=&r"(temp1),
          [temp2]"=&r"(temp2), [temp3]"=&r"(temp3)
        : [dither_scan]"r"(dither_scan), [dithers]"r"(dithers)
        : "memory"
    );

    if (count >= 2) {
        __asm__ volatile (
            ".set      push                                                                       \n\t"
            ".set      noreorder                                                                  \n\t"
        "1:                                                                                       \n\t"
            "andi      %[temp0],  %[x],        3                                                  \n\t"
            "sll       %[temp0],  %[temp0],    2                                                  \n\t"
            "lw        %[temp1],  0(%[src])                                                       \n\t"
            "lw        %[temp2],  4(%[src])                                                       \n\t"
            "lwx       %[temp0],  %[temp0](%[dithers])                                            \n\t"
            "ext       %[temp9],  %[temp1],    " STR(SK_A32_SHIFT)",        8                     \n\t"
            "ext       %[temp4],  %[temp2],    " STR(SK_A32_SHIFT)",        8                     \n\t"
            "ext       %[temp10], %[temp2],    " STR(SK_A32_SHIFT)",        8                     \n\t"
            "addiu     %[x],      %[x],        2                                                  \n\t"
            "append    %[temp4],  %[temp9],    16                                                 \n\t"
            "addq.ph   %[temp3],  %[temp4],    %[c1]                                              \n\t"
            "mul.ph    %[temp0],  %[temp0],    %[temp3]                                           \n\t"
            "ext       %[temp3],  %[temp1],    " STR(SK_R32_SHIFT)",        8                     \n\t"
            "ext       %[temp4],  %[temp1],    " STR(SK_G32_SHIFT)",        8                     \n\t"
            "ext       %[temp5],  %[temp1],    " STR(SK_B32_SHIFT)",        8                     \n\t"
            "ext       %[temp6],  %[temp2],    " STR(SK_R32_SHIFT)",        8                     \n\t"
            "ext       %[temp7],  %[temp2],    " STR(SK_G32_SHIFT)",        8                     \n\t"
            "ext       %[temp8],  %[temp2],    " STR(SK_B32_SHIFT)",        8                     \n\t"
            "shra.ph   %[temp0],  %[temp0],    8                                                  \n\t"
            "append    %[temp6],  %[temp3],    16                                                 \n\t"
            "append    %[temp7],  %[temp4],    16                                                 \n\t"
            "append    %[temp8],  %[temp5],    16                                                 \n\t"
            "shrl.ph   %[temp1],  %[temp0],    1                                                  \n\t"
            "shrl.ph   %[temp3],  %[temp6],    5                                                  \n\t"
            "shrl.ph   %[temp4],  %[temp7],    6                                                  \n\t"
            "shrl.ph   %[temp5],  %[temp8],    5                                                  \n\t"
            "addq.ph   %[temp6],  %[temp6],    %[temp0]                                           \n\t"
            "addq.ph   %[temp7],  %[temp7],    %[temp1]                                           \n\t"
            "addq.ph   %[temp8],  %[temp8],    %[temp0]                                           \n\t"
            "subq.ph   %[temp6],  %[temp6],    %[temp3]                                           \n\t"
            "subq.ph   %[temp7],  %[temp7],    %[temp4]                                           \n\t"
            "subq.ph   %[temp8],  %[temp8],    %[temp5]                                           \n\t"
            "ext       %[temp0],  %[temp6],    0,                           16                    \n\t"
            "ext       %[temp1],  %[temp7],    0,                           16                    \n\t"
            "ext       %[temp2],  %[temp8],    0,                           16                    \n\t"
            "ext       %[temp6],  %[temp6],    16,                          16                    \n\t"
            "ext       %[temp7],  %[temp7],    16,                          16                    \n\t"
            "ext       %[temp8],  %[temp8],    16,                          16                    \n\t"
            "lhu       %[temp3],  0(%[dst])                                                       \n\t"
            "lhu       %[temp4],  2(%[dst])                                                       \n\t"
            "sll       %[temp0],  %[temp0],    13                                                 \n\t"
            "sll       %[temp6],  %[temp6],    13                                                 \n\t"
            "sll       %[temp1],  %[temp1],    24                                                 \n\t"
            "sll       %[temp7],  %[temp7],    24                                                 \n\t"
            "sll       %[temp2],  %[temp2],    2                                                  \n\t"
            "sll       %[temp8],  %[temp8],    2                                                  \n\t"
            "or        %[temp0],  %[temp0],    %[temp1]                                           \n\t"
            "or        %[temp6],  %[temp6],    %[temp7]                                           \n\t"
            "or        %[temp0],  %[temp0],    %[temp2]                                           \n\t"
            "or        %[temp6],  %[temp6],    %[temp8]                                           \n\t"
            "ext       %[temp2],  %[temp3],    " STR(SK_G16_SHIFT)",        " STR(SK_G16_BITS)"   \n\t"
            "ext       %[temp7],  %[temp4],    " STR(SK_G16_SHIFT)",        " STR(SK_G16_BITS)"   \n\t"
            "andi      %[temp3],  %[temp3],    0xf81f                                             \n\t"
            "andi      %[temp4],  %[temp4],    0xf81f                                             \n\t"
            "ins       %[temp3],  %[temp2],    " STR(SK_G16_SHIFT + 16)",   " STR(SK_G16_BITS)"   \n\t"
            "ins       %[temp4],  %[temp7],    " STR(SK_G16_SHIFT + 16)",   " STR(SK_G16_BITS)"   \n\t"
            "subu      %[temp5],  %[c256],     %[temp9]                                           \n\t"
            "subu      %[temp8],  %[c256],     %[temp10]                                          \n\t"
            "sra       %[temp5],  %[temp5],     3                                                 \n\t"
            "sra       %[temp8],  %[temp8],     3                                                 \n\t"
            "mul       %[temp5],  %[temp5],     %[temp3]                                          \n\t"
            "mul       %[temp8],  %[temp8],     %[temp4]                                          \n\t"
            "addu      %[temp5],  %[temp5],     %[temp0]                                          \n\t"
            "addu      %[temp8],  %[temp8],     %[temp6]                                          \n\t"
            "sra       %[temp5],  %[temp5],     5                                                 \n\t"
            "sra       %[temp8],  %[temp8],     5                                                 \n\t"
            "ext       %[temp2],  %[temp5],    " STR(SK_G16_SHIFT + 16)",   " STR(SK_G16_BITS)"   \n\t"
            "ext       %[temp3],  %[temp8],    " STR(SK_G16_SHIFT + 16)",   " STR(SK_G16_BITS)"   \n\t"
            "andi      %[temp5],  %[temp5],    0xf81f                                             \n\t"
            "andi      %[temp8],  %[temp8],    0xf81f                                             \n\t"
            "ins       %[temp5],  %[temp2],    " STR(SK_G16_SHIFT)",        " STR(SK_G16_BITS)"   \n\t"
            "ins       %[temp8],  %[temp3],    " STR(SK_G16_SHIFT)",        " STR(SK_G16_BITS)"   \n\t"
            "sh        %[temp5],  0(%[dst])                                                       \n\t"
            "sh        %[temp8],  2(%[dst])                                                       \n\t"
            "addiu     %[dst],    %[dst],      4                                                  \n\t"
            "bne       %[dst],    %[loopEnd], 1b                                                  \n\t"
            " addiu    %[src],    %[src],      8                                                  \n\t"
            ".set      pop                                                                        \n\t"

            : [src]"+&r"(src), [dst]"+&r"(dst), [temp0]"=&r"(temp0), [temp1]"=&r"(temp1),
              [temp2]"=&r"(temp2), [temp3]"=&r"(temp3), [temp4]"=&r"(temp4),
              [temp5]"=&r"(temp5), [temp6]"=&r"(temp6), [temp7]"=&r"(temp7),
              [temp8]"=&r"(temp8), [temp9]"=&r"(temp9), [temp10]"=&r"(temp10), [x]"+&r"(x)
            : [loopEnd]"r"(loopEnd), [c256]"r"(c256), [dithers]"r"(dithers), [c1]"r"(c1)
            : "memory", "hi", "lo"
        );
    }
#else
    register int32_t t0, t1, t2, t3, t4, t5, t6;
    register int32_t t7, t8, t9, s0, s1, s2, s3;
    const uint16_t dither_scan = gDitherMatrix_3Bit_16[(y) & 3];
    for (; count >=2; count -=2)
    {
        __asm__ volatile (
            ".set            push                                \n\t"
            ".set            noreorder                           \n\t"
            "li              %[s1],    0x01010101                \n\t"
            "li              %[s2],    -2017                     \n\t"
            "lw              %[t1],    0(%[src])                 \n\t"
            "lw              %[t2],    4(%[src])                 \n\t"
            "andi            %[t3],    %[x],     0x3             \n\t"
            "addiu           %[x],     %[x],     1               \n\t"
            "sll             %[t4],    %[t3],    2               \n\t"
            "srav            %[t5],    %[dither_scan], %[t4]     \n\t"
            "andi            %[t3],    %[t5],    0xf             \n\t"
            "andi            %[t4],    %[x],     0x3             \n\t"
            "sll             %[t5],    %[t4],    2               \n\t"
            "srav            %[t6],    %[dither_scan], %[t5]     \n\t"
            "addiu           %[x],     %[x],     1               \n\t"
            "ins             %[t3],    %[t6],    8,    4         \n\t"
            "srl             %[t4],    %[t1],    24              \n\t"
            "addiu           %[t0],    %[t4],    1               \n\t"
            "srl             %[t4],    %[t2],    24              \n\t"
            "addiu           %[t5],    %[t4],    1               \n\t"
            "ins             %[t0],    %[t5],    16,   16        \n\t"
            "muleu_s.ph.qbr  %[t4],    %[t3],    %[t0]           \n\t"
            "preceu.ph.qbla  %[t3],    %[t4]                     \n\t"
            "andi            %[t4],    %[t1],    0xff            \n\t"
            "ins             %[t4],    %[t2],    16,   8         \n\t"
            "shrl.qb         %[t5],    %[t4],    5               \n\t"
            "subu.qb         %[t6],    %[t3],    %[t5]           \n\t"
            "addq.ph         %[t5],    %[t6],    %[t4]           \n\t"
            "ext             %[t4],    %[t1],    8,    8         \n\t"
            "srl             %[t6],    %[t2],    8               \n\t"
            "ins             %[t4],    %[t6],    16,   8         \n\t"
            "shrl.qb         %[t6],    %[t4],    6               \n\t"
            "shrl.qb         %[t7],    %[t3],    1               \n\t"
            "subu.qb         %[t8],    %[t7],    %[t6]           \n\t"
            "addq.ph         %[t6],    %[t8],    %[t4]           \n\t"
            "ext             %[t4],    %[t1],    16,   8         \n\t"
            "srl             %[t7],    %[t2],    16              \n\t"
            "ins             %[t4],    %[t7],    16,   8         \n\t"
            "shrl.qb         %[t7],    %[t4],    5               \n\t"
            "subu.qb         %[t8],    %[t3],    %[t7]           \n\t"
            "addq.ph         %[t7],    %[t8],    %[t4]           \n\t"
            "andi            %[t9],    %[t5],    0xff            \n\t"
            "sll             %[t9],    %[t9],    2               \n\t"
            "srl             %[s0],    %[t5],    16              \n\t"
            "andi            %[s0],    %[s0],    0xff            \n\t"
            "sll             %[s0],    %[s0],    2               \n\t"
            "andi            %[t3],    %[t6],    0xff            \n\t"
            "srl             %[t4],    %[t6],    16              \n\t"
            "andi            %[t4],    %[t4],    0xff            \n\t"
            "andi            %[t6],    %[t7],    0xff            \n\t"
            "srl             %[t7],    %[t7],    16              \n\t"
            "andi            %[t7],    %[t7],    0xff            \n\t"
            "subq.ph         %[t5],    %[s1],    %[t0]           \n\t"
            "srl             %[t0],    %[t5],    3               \n\t"
            "lhu             %[t5],    0(%[dst])                 \n\t"
            "sll             %[t1],    %[t6],    13              \n\t"
            "or              %[t8],    %[t9],    %[t1]           \n\t"
            "sll             %[t1],    %[t3],    24              \n\t"
            "or              %[t9],    %[t1],    %[t8]           \n\t"
            "andi            %[t3],    %[t5],    0x7e0           \n\t"
            "sll             %[t6],    %[t3],    0x10            \n\t"
            "and             %[t8],    %[s2],    %[t5]           \n\t"
            "or              %[t5],    %[t6],    %[t8]           \n\t"
            "andi            %[t6],    %[t0],    0xff            \n\t"
            "mul             %[t1],    %[t6],    %[t5]           \n\t"
            "addu            %[t5],    %[t1],    %[t9]           \n\t"
            "srl             %[t6],    %[t5],    5               \n\t"
            "and             %[t5],    %[s2],    %[t6]           \n\t"
            "srl             %[t8],    %[t6],    16              \n\t"
            "andi            %[t6],    %[t8],    0x7e0           \n\t"
            "or              %[t1],    %[t5],    %[t6]           \n\t"
            "sh              %[t1],    0(%[dst])                 \n\t"
            "lhu             %[t5],    2(%[dst])                 \n\t"
            "sll             %[t1],    %[t7],    13              \n\t"
            "or              %[t8],    %[s0],    %[t1]           \n\t"
            "sll             %[t1],    %[t4],    24              \n\t"
            "or              %[t9],    %[t1],    %[t8]           \n\t"
            "andi            %[t3],    %[t5],    0x7e0           \n\t"
            "sll             %[t6],    %[t3],    0x10            \n\t"
            "and             %[t8],    %[s2],    %[t5]           \n\t"
            "or              %[t5],    %[t6],    %[t8]           \n\t"
            "srl             %[t6],    %[t0],    16              \n\t"
            "mul             %[t1],    %[t6],    %[t5]           \n\t"
            "addu            %[t5],    %[t1],    %[t9]           \n\t"
            "srl             %[t6],    %[t5],    5               \n\t"
            "and             %[t5],    %[s2],    %[t6]           \n\t"
            "srl             %[t8],    %[t6],    16              \n\t"
            "andi            %[t6],    %[t8],    0x7e0           \n\t"
            "or              %[t1],    %[t5],    %[t6]           \n\t"
            "sh              %[t1],    2(%[dst])                 \n\t"
            ".set            pop                                 \n\t"
            : [src]"+r"(src), [dst]"+r"(dst), [x]"+r"(x),
              [t0]"=&r"(t0), [t1]"=&r"(t1), [t2]"=&r"(t2),
              [t3]"=&r"(t3), [t4]"=&r"(t4), [t5]"=&r"(t5),
              [t6]"=&r"(t6), [t7]"=&r"(t7), [t8]"=&r"(t8),
              [t9]"=&r"(t9), [s0]"=&r"(s0), [s1]"=&r"(s1),
              [s2]"=&r"(s2), [s3]"=&r"(s3)
            : [dither_scan]"r"(dither_scan)
            : "memory", "hi", "lo"
        );
        dst += 2;
        src += 2;
    }
#endif
    if (count & 1) {
        SkPMColor c = *src++;
        SkPMColorAssert(c);
        if (c) {
            unsigned a = SkGetPackedA32(c);

            int d = SkAlphaMul(dither_scan >> ((x & 3) << 2) & 0xF, SkAlpha255To256(a));

            unsigned sr = SkGetPackedR32(c);
            unsigned sg = SkGetPackedG32(c);
            unsigned sb = SkGetPackedB32(c);
            sr = SkDITHER_R32_FOR_565(sr, d);
            sg = SkDITHER_G32_FOR_565(sg, d);
            sb = SkDITHER_B32_FOR_565(sb, d);

            uint32_t src_expanded = (sg << 24) | (sr << 13) | (sb << 2);
            uint32_t dst_expanded = SkExpand_rgb_16(*dst);
            dst_expanded = dst_expanded * (SkAlpha255To256(255 - a) >> 3);
            // now src and dst expanded are in g:11 r:10 x:1 b:10
            *dst = SkCompact_rgb_16((src_expanded + dst_expanded) >> 5);
        }
        dst += 1;
        DITHER_INC_X(x);
    }
}

#ifdef SK_MIPS_HAS_DSPR2
static void S32_D565_Opaque_Dither_mips_dsp(uint16_t* __restrict__ dst,
                                            const SkPMColor* __restrict__ src,
                                            int count, U8CPU alpha, int x, int y) {
    int temp0, temp1, temp2, temp3, temp4, temp5;
    extern const uint16_t gDitherMatrix_3Bit_16[4];
    const uint16_t dither_scan = gDitherMatrix_3Bit_16[(y) & 3];
    uint16_t* loopEnd = dst + (count & ~1);
    uint32_t dither[4];

    __asm__ volatile(
        "ext    %[temp0],   %[dither_scan],     0,      4   \n\t"
        "ext    %[temp1],   %[dither_scan],     4,      4   \n\t"
        "ext    %[temp2],   %[dither_scan],     8,      4   \n\t"
        "ext    %[temp3],   %[dither_scan],     12,     4   \n\t"
        "ins    %[temp0],   %[temp1],           16,     4   \n\t"
        "ins    %[temp1],   %[temp2],           16,     4   \n\t"
        "ins    %[temp2],   %[temp3],           16,     4   \n\t"
        "ins    %[temp3],   %[temp0],           16,     4   \n\t"
        "sw     %[temp0],   0(%[dither])                    \n\t"
        "sw     %[temp1],   4(%[dither])                    \n\t"
        "sw     %[temp2],   8(%[dither])                    \n\t"
        "sw     %[temp3],   12(%[dither])                   \n\t"
        : [temp0]"=&r"(temp0), [temp1]"=&r"(temp1),
          [temp2]"=&r"(temp2), [temp3]"=&r"(temp3)
        : [dither]"r"(dither), [dither_scan]"r"(dither_scan)
        : "memory"
    );

    if (count >= 2)
    {
        __asm__ volatile(
            ".set      push                                                    \n\t"
            ".set      noreorder                                               \n\t"
            "1:                                                                \n\t"
            "lw        %[temp0],  0(%[src])                                    \n\t"
            "lw        %[temp1],  4(%[src])                                    \n\t"
            "ext       %[temp2],  %[temp0],    " STR(SK_R32_SHIFT)",      8    \n\t"
            "ext       %[temp3],  %[temp0],    " STR(SK_G32_SHIFT)",      8    \n\t"
            "ext       %[temp4],  %[temp0],    " STR(SK_B32_SHIFT)",      8    \n\t"
            "ext       %[temp5],  %[temp1],    " STR(SK_R32_SHIFT)",      8    \n\t"
            "ext       %[temp0],  %[temp1],    " STR(SK_G32_SHIFT)",      8    \n\t"
            "ext       %[temp1],  %[temp1],    " STR(SK_B32_SHIFT)",      8    \n\t"
            "append    %[temp5],  %[temp2],    16                              \n\t"
            "append    %[temp0],  %[temp3],    16                              \n\t"
            "append    %[temp1],  %[temp4],    16                              \n\t"
            "andi      %[temp2],  %[x],        3                               \n\t"
            "sll       %[temp2],  %[temp2],    2                               \n\t"
            "lwx       %[temp4],  %[temp2](%[dither])                          \n\t"
            "srl       %[temp3],  %[temp4],    1                               \n\t"
            "srl       %[temp1],  %[temp5],    5                               \n\t"
            "srl       %[temp2],  %[temp0],    6                               \n\t"
            "srl       %[temp3],  %[temp1],    5                               \n\t"
            "addq.ph   %[temp5],  %[temp5],    %[temp4]                        \n\t"
            "addq.ph   %[temp0],  %[temp0],    %[temp3]                        \n\t"
            "addq.ph   %[temp1],  %[temp1],    %[temp4]                        \n\t"
            "subq.ph   %[temp5],  %[temp5],    %[temp1]                        \n\t"
            "subq.ph   %[temp0],  %[temp0],    %[temp2]                        \n\t"
            "subq.ph   %[temp1],  %[temp1],    %[temp3]                        \n\t"
            "shll.ph   %[temp5],  %[temp5],    " STR(SK_R16_SHIFT)"            \n\t"
            "shll.ph   %[temp0],  %[temp0],    " STR(SK_G16_SHIFT)"            \n\t"
            "shll.ph   %[temp1],  %[temp1],    " STR(SK_B16_SHIFT)"            \n\t"
            "or        %[temp5],  %[temp5],    %[temp0]                        \n\t"
            "or        %[temp5],  %[temp5],    %[temp1]                        \n\t"
            "usw       %[temp5],  0(%[dst])                                    \n\t"
            "addiu     %[dst],    %[dst],      4                               \n\t"
            "addiu     %[x],      %[x],        2                               \n\t"
            "bne       %[dst],    %[loopEnd],  1b                              \n\t"
            " addiu    %[src],    %[src],      8                               \n\t"
            ".set       pop                                                    \n\t"

            : [dst]"+&r"(dst), [src]"+&r"(src), [temp0]"=&r"(temp0),
              [temp1]"=&r"(temp1), [temp2]"=&r"(temp2), [temp3]"=&r"(temp3),
              [temp4]"=&r"(temp4), [temp5]"=&r"(temp5), [x]"+&r"(x)
            : [dither]"r"(dither), [loopEnd]"r"(loopEnd)
            : "memory"
        );
    }

    if (count & 1) {
        SkPMColor c = *src++;
        SkPMColorAssert(c); // only if DEBUG is turned on
        SkASSERT(SkGetPackedA32(c) == 255);
        unsigned dither = (dither_scan >> ((x & 3) << 2)) & 0xF;
        *dst++ = SkDitherRGB32To565(c, dither);
    }
}

static void S32_D565_Blend_Dither_mips_dsp(uint16_t* dst,
                                           const SkPMColor* src,
                                           int count, U8CPU alpha, int x, int y) {

    int temp0, temp1, temp2, temp3, temp4, temp5;
    int temp6, temp7, temp8, temp9;
    uint16_t* loopEnd = dst + (count & ~1);
    extern const uint16_t gDitherMatrix_3Bit_16[4];
    const uint16_t dither_scan = gDitherMatrix_3Bit_16[(y) & 3];
    uint32_t dithers[4];

    __asm__ volatile (
        "ext   %[temp0],  %[dither_scan],  0,   4        \n\t"
        "ext   %[temp1],  %[dither_scan],  4,   4        \n\t"
        "ext   %[temp2],  %[dither_scan],  8,   4        \n\t"
        "ext   %[temp3],  %[dither_scan],  12,  4        \n\t"
        "ins   %[temp0],  %[temp1],        16,  4        \n\t"
        "ins   %[temp1],  %[temp2],        16,  4        \n\t"
        "ins   %[temp2],  %[temp3],        16,  4        \n\t"
        "ins   %[temp3],  %[temp0],        16,  4        \n\t"
        "sw    %[temp0],  0(%[dithers])                  \n\t"
        "sw    %[temp1],  4(%[dithers])                  \n\t"
        "sw    %[temp2],  8(%[dithers])                  \n\t"
        "sw    %[temp3],  12(%[dithers])                 \n\t"

        : [temp0]"=&r"(temp0), [temp1]"=&r"(temp1),
          [temp2]"=&r"(temp2), [temp3]"=&r"(temp3)
        : [dither_scan]"r"(dither_scan), [dithers]"r"(dithers)
        : "memory"
    );

    alpha += 1;
    if (count >= 2) {
        __asm__ volatile (
            ".set      push                                                                       \n\t"
            ".set      noreorder                                                                  \n\t"
            "replv.ph  %[temp9],  %[alpha]                                                        \n\t"
        "1:                                                                                       \n\t"
            "andi      %[temp0],  %[x],        3                                                  \n\t"
            "lw        %[temp1],  0(%[src])                                                       \n\t"
            "lw        %[temp2],  4(%[src])                                                       \n\t"
            "sll       %[temp0],  %[temp0],    2                                                  \n\t"
            "ext       %[temp3],  %[temp1],    " STR(SK_R32_SHIFT)",        8                     \n\t"
            "ext       %[temp4],  %[temp1],    " STR(SK_G32_SHIFT)",        8                     \n\t"
            "ext       %[temp5],  %[temp1],    " STR(SK_B32_SHIFT)",        8                     \n\t"
            "ext       %[temp6],  %[temp2],    " STR(SK_R32_SHIFT)",        8                     \n\t"
            "ext       %[temp7],  %[temp2],    " STR(SK_G32_SHIFT)",        8                     \n\t"
            "ext       %[temp8],  %[temp2],    " STR(SK_B32_SHIFT)",        8                     \n\t"
            "lwx       %[temp0],  %[temp0](%[dithers])                                            \n\t"
            "addiu     %[x],      %[x],        2                                                  \n\t"
            "append    %[temp6],  %[temp3],    16                                                 \n\t"
            "append    %[temp7],  %[temp4],    16                                                 \n\t"
            "append    %[temp8],  %[temp5],    16                                                 \n\t"
            "shrl.ph   %[temp1],  %[temp0],    1                                                  \n\t"
            "shrl.ph   %[temp3],  %[temp6],    5                                                  \n\t"
            "shrl.ph   %[temp4],  %[temp7],    6                                                  \n\t"
            "shrl.ph   %[temp5],  %[temp8],    5                                                  \n\t"
            "addq.ph   %[temp6],  %[temp6],    %[temp0]                                           \n\t"
            "addq.ph   %[temp7],  %[temp7],    %[temp1]                                           \n\t"
            "addq.ph   %[temp8],  %[temp8],    %[temp0]                                           \n\t"
            "subq.ph   %[temp6],  %[temp6],    %[temp3]                                           \n\t"
            "subq.ph   %[temp7],  %[temp7],    %[temp4]                                           \n\t"
            "subq.ph   %[temp8],  %[temp8],    %[temp5]                                           \n\t"
            "ulw       %[temp0],  0(%[dst])                                                       \n\t"
            "shrl.ph   %[temp6],  %[temp6],    " STR(SK_R32_BITS - SK_R16_BITS)"                  \n\t"
            "shrl.ph   %[temp7],  %[temp7],    " STR(SK_G32_BITS - SK_G16_BITS)"                  \n\t"
            "shrl.ph   %[temp8],  %[temp8],    " STR(SK_B32_BITS - SK_B16_BITS)"                  \n\t"
            "ext       %[temp1],  %[temp0],    " STR(SK_R16_SHIFT)",        " STR(SK_R16_BITS)"   \n\t"
            "ext       %[temp2],  %[temp0],    " STR(SK_G16_SHIFT)",        " STR(SK_G16_BITS)"   \n\t"
            "ext       %[temp3],  %[temp0],    " STR(SK_B16_SHIFT)",        " STR(SK_B16_BITS)"   \n\t"
            "ext       %[temp4],  %[temp0],    " STR(SK_R16_SHIFT + 16)",   " STR(SK_R16_BITS)"   \n\t"
            "ext       %[temp5],  %[temp0],    " STR(SK_G16_SHIFT + 16)",   " STR(SK_G16_BITS)"   \n\t"
            "ext       %[temp0],  %[temp0],    " STR(SK_B16_SHIFT + 16)",   " STR(SK_B16_BITS)"   \n\t"
            "append    %[temp4],  %[temp1],    16                                                 \n\t"
            "append    %[temp5],  %[temp2],    16                                                 \n\t"
            "append    %[temp0],  %[temp3],    16                                                 \n\t"
            "subq.ph   %[temp6],  %[temp6],    %[temp4]                                           \n\t"
            "subq.ph   %[temp7],  %[temp7],    %[temp5]                                           \n\t"
            "subq.ph   %[temp8],  %[temp8],    %[temp0]                                           \n\t"
            "mul.ph    %[temp6],  %[temp6],    %[temp9]                                           \n\t"
            "mul.ph    %[temp7],  %[temp7],    %[temp9]                                           \n\t"
            "mul.ph    %[temp8],  %[temp8],    %[temp9]                                           \n\t"
            "shra.ph   %[temp6],  %[temp6],    8                                                  \n\t"
            "shra.ph   %[temp7],  %[temp7],    8                                                  \n\t"
            "shra.ph   %[temp8],  %[temp8],    8                                                  \n\t"
            "addq.ph   %[temp6],  %[temp6],    %[temp4]                                           \n\t"
            "addq.ph   %[temp7],  %[temp7],    %[temp5]                                           \n\t"
            "addq.ph   %[temp8],  %[temp8],    %[temp0]                                           \n\t"
            "shll.ph   %[temp6],  %[temp6],    " STR(SK_R16_SHIFT)"                               \n\t"
            "shll.ph   %[temp7],  %[temp7],    " STR(SK_G16_SHIFT)"                               \n\t"
            "shll.ph   %[temp8],  %[temp8],    " STR(SK_B16_SHIFT)"                               \n\t"
            "or        %[temp6],  %[temp6],    %[temp7]                                           \n\t"
            "or        %[temp6],  %[temp6],    %[temp8]                                           \n\t"
            "usw       %[temp6],  0(%[dst])                                                       \n\t"
            "addiu     %[dst],    %[dst],      4                                                  \n\t"
            "bne       %[dst],    %[loopEnd],  1b                                                 \n\t"
            " addiu    %[src],    %[src],      8                                                  \n\t"
            ".set      pop                                                                        \n\t"

            : [src]"+&r"(src), [dst]"+&r"(dst), [temp0]"=&r"(temp0), [temp1]"=&r"(temp1),
              [temp2]"=&r"(temp2), [temp3]"=&r"(temp3), [temp4]"=&r"(temp4),
              [temp5]"=&r"(temp5), [temp6]"=&r"(temp6), [temp7]"=&r"(temp7),
              [temp8]"=&r"(temp8), [temp9]"=&r"(temp9), [x]"+&r"(x)
            : [loopEnd]"r"(loopEnd), [alpha]"r"(alpha), [dithers]"r"(dithers)
            : "memory", "hi", "lo"
        );
    }

    if (count & 1) {
        SkPMColor c = *src++;

        int dither = (dither_scan >> ((x & 3) << 2)) & 0xF;
        int sr = SkGetPackedR32(c);
        int sg = SkGetPackedG32(c);
        int sb = SkGetPackedB32(c);
        sr = SkDITHER_R32To565(sr, dither);
        sg = SkDITHER_G32To565(sg, dither);
        sb = SkDITHER_B32To565(sb, dither);

        uint16_t d = *dst;
        *dst++ = SkPackRGB16(SkAlphaBlend(sr, SkGetPackedR16(d), alpha),
                             SkAlphaBlend(sg, SkGetPackedG16(d), alpha),
                             SkAlphaBlend(sb, SkGetPackedB16(d), alpha));
        DITHER_INC_X(x);
    }
}
#endif

static void S32A_D565_Opaque_mips_dsp(uint16_t* __restrict__ dst,
                                      const SkPMColor* __restrict__ src,
                                      int count, U8CPU alpha, int x, int y) {
#ifdef SK_MIPS_HAS_DSPR2
    int temp0, temp1, temp2, temp3, temp4, temp5;
    int temp6, temp7, temp8, temp9, temp10;
    uint16_t* loopEnd = dst + (count & ~1);
    const uint32_t c255 = 0xff00ff;

    if (count >= 2) {
        __asm__ volatile (
            ".set      push                                                                       \n\t"
            ".set      noreorder                                                                  \n\t"
        "1:                                                                                       \n\t"
            "lw        %[temp0],  0(%[src])                                                       \n\t"
            "lw        %[temp1],  4(%[src])                                                       \n\t"
            "ulw       %[temp2],  0(%[dst])                                                       \n\t"
            "ext       %[temp7],  %[temp0],   " STR(SK_A32_SHIFT)",        8                      \n\t"
            "ext       %[temp8],  %[temp0],   " STR(SK_B32_SHIFT)",        8                      \n\t"
            "ext       %[temp9],  %[temp0],   " STR(SK_G32_SHIFT)",        8                      \n\t"
            "ext       %[temp10], %[temp0],   " STR(SK_R32_SHIFT)",        8                      \n\t"
            "ext       %[temp3],  %[temp1],   " STR(SK_A32_SHIFT)",        8                      \n\t"
            "ext       %[temp4],  %[temp1],   " STR(SK_B32_SHIFT)",        8                      \n\t"
            "ext       %[temp5],  %[temp1],   " STR(SK_G32_SHIFT)",        8                      \n\t"
            "ext       %[temp6],  %[temp1],   " STR(SK_R32_SHIFT)",        8                      \n\t"
            "append    %[temp3],  %[temp7],    16                                                 \n\t"
            "append    %[temp4],  %[temp8],    16                                                 \n\t"
            "append    %[temp5],  %[temp9],    16                                                 \n\t"
            "append    %[temp6],  %[temp10],   16                                                 \n\t"
            "ext       %[temp7],  %[temp2],    " STR(SK_R16_SHIFT)",       " STR(SK_R16_BITS)"    \n\t"
            "ext       %[temp8],  %[temp2],    " STR(SK_G16_SHIFT)",       " STR(SK_G16_BITS)"    \n\t"
            "ext       %[temp9],  %[temp2],    " STR(SK_B16_SHIFT)",       " STR(SK_B16_BITS)"    \n\t"
            "ext       %[temp0],  %[temp2],    " STR(SK_R16_SHIFT + 16)",  " STR(SK_R16_BITS)"    \n\t"
            "ext       %[temp1],  %[temp2],    " STR(SK_G16_SHIFT + 16)",  " STR(SK_G16_BITS)"    \n\t"
            "ext       %[temp2],  %[temp2],    " STR(SK_B16_SHIFT + 16)",  " STR(SK_B16_BITS)"    \n\t"
            "subu.ph   %[temp3],  %[c255],     %[temp3]                                           \n\t"
            "append    %[temp0],  %[temp7],    16                                                 \n\t"
            "append    %[temp1],  %[temp8],    16                                                 \n\t"
            "append    %[temp2],  %[temp9],    16                                                 \n\t"
            "mul.ph    %[temp7],  %[temp0],    %[temp3]                                           \n\t"
            "mul.ph    %[temp8],  %[temp1],    %[temp3]                                           \n\t"
            "mul.ph    %[temp9],  %[temp2],    %[temp3]                                           \n\t"
            "shra_r.ph %[temp0],  %[temp7],    " STR(SK_R16_BITS)"                                \n\t"
            "shra_r.ph %[temp1],  %[temp8],    " STR(SK_G16_BITS)"                                \n\t"
            "shra_r.ph %[temp2],  %[temp9],    " STR(SK_B16_BITS)"                                \n\t"
            "addu.ph   %[temp7],  %[temp7],    %[temp0]                                           \n\t"
            "addu.ph   %[temp8],  %[temp8],    %[temp1]                                           \n\t"
            "addu.ph   %[temp9],  %[temp9],    %[temp2]                                           \n\t"
            "shra_r.ph %[temp7],  %[temp7],    " STR(SK_R16_BITS)"                                \n\t"
            "shra_r.ph %[temp8],  %[temp8],    " STR(SK_G16_BITS)"                                \n\t"
            "shra_r.ph %[temp9],  %[temp9],    " STR(SK_B16_BITS)"                                \n\t"
            "addu.ph   %[temp6],  %[temp6],    %[temp7]                                           \n\t"
            "addu.ph   %[temp5],  %[temp5],    %[temp8]                                           \n\t"
            "addu.ph   %[temp4],  %[temp4],    %[temp9]                                           \n\t"
            "shra.ph   %[temp6],  %[temp6],    " STR(8 - SK_R16_BITS)"                            \n\t"
            "shra.ph   %[temp5],  %[temp5],    " STR(8 - SK_G16_BITS)"                            \n\t"
            "shra.ph   %[temp4],  %[temp4],    " STR(8 - SK_B16_BITS)"                            \n\t"
            "shll.ph   %[temp6],  %[temp6],    " STR(SK_R16_SHIFT)"                               \n\t"
            "shll.ph   %[temp5],  %[temp5],    " STR(SK_G16_SHIFT)"                               \n\t"
            "shll.ph   %[temp4],  %[temp4],    " STR(SK_B16_SHIFT)"                               \n\t"
            "or        %[temp6],  %[temp6],    %[temp5]                                           \n\t"
            "or        %[temp6],  %[temp6],    %[temp4]                                           \n\t"
            "usw       %[temp6],  0(%[dst])                                                       \n\t"
            "addiu     %[dst],    %[dst],      4                                                  \n\t"
            "bne       %[dst],    %[loopEnd],  1b                                                 \n\t"
            " addiu    %[src],    %[src],      8                                                  \n\t"
            ".set      pop                                                                        \n\t"

            : [src]"+&r"(src), [dst]"+&r"(dst), [temp0]"=&r"(temp0), [temp1]"=&r"(temp1),
              [temp2]"=&r"(temp2), [temp3]"=&r"(temp3), [temp4]"=&r"(temp4),
              [temp5]"=&r"(temp5), [temp6]"=&r"(temp6), [temp7]"=&r"(temp7),
              [temp8]"=&r"(temp8), [temp9]"=&r"(temp9), [temp10]"=&r"(temp10)
            : [loopEnd]"r"(loopEnd), [c255]"r"(c255)
            : "memory", "hi", "lo"
        );
    }
#else
    __asm__ volatile (
        "pref  0,  0(%[src])     \n\t"
        "pref  1,  0(%[dst])     \n\t"
        "pref  0,  32(%[src])    \n\t"
        "pref  1,  32(%[dst])    \n\t"
        :
        : [src]"r"(src), [dst]"r"(dst)
        : "memory"
    );

    register int t0, t1, t2, t3, t4, t5, t6, t7, t8;
    register int add_x10 = 0x100010;
    register int add_x20 = 0x200020;
    register int sa = 0xff00ff;

    SkASSERT(255 == alpha);

    for (int i = (count >> 1); i > 0; i--)
    {
        __asm__ volatile (
            ".set           push                            \n\t"
            ".set           noreorder                       \n\t"
            "lw             %[t1],    0(%[src])             \n\t"
            "lw             %[t0],    4(%[src])             \n\t"
            "precrq.ph.w    %[t2],    %[t1],    %[t0]       \n\t"
            "preceu.ph.qbra %[t8],    %[t2]                 \n\t"
            "ins            %[t0],    %[t1],    16,    16   \n\t"
            "preceu.ph.qbra %[t3],    %[t0]                 \n\t"
            "preceu.ph.qbla %[t4],    %[t0]                 \n\t"
            "preceu.ph.qbla %[t0],    %[t2]                 \n\t"
            "subq.ph        %[t1],    %[sa],    %[t0]       \n\t"
            "sra            %[t2],    %[t1],    8           \n\t"
            "or             %[t5],    %[t2],    %[t1]       \n\t"
            "replv.ph       %[t2],    %[t5]                 \n\t"
            "lh             %[t1],    0(%[dst])             \n\t"
            "lh             %[t0],    2(%[dst])             \n\t"
            "ins            %[t0],    %[t1],    16,    16   \n\t"
            "and            %[t1],    %[t0],    0x1f001f    \n\t"
            "shra.ph        %[t6],    %[t0],    11          \n\t"
            "and            %[t6],    %[t6],    0x1f001f    \n\t"
            "and            %[t7],    %[t0],    0x7e007e0   \n\t"
            "shra.ph        %[t5],    %[t7],    5           \n\t"
            "muleu_s.ph.qbl %[t0],    %[t2],    %[t6]       \n\t"
            "addq.ph        %[t7],    %[t0],    %[add_x10]  \n\t"
            "shra.ph        %[t6],    %[t7],    5           \n\t"
            "addq.ph        %[t6],    %[t7],    %[t6]       \n\t"
            "shra.ph        %[t0],    %[t6],    5           \n\t"
            "addq.ph        %[t7],    %[t0],    %[t8]       \n\t"
            "shra.ph        %[t6],    %[t7],    3           \n\t"
            "muleu_s.ph.qbl %[t0],    %[t2],    %[t1]       \n\t"
            "addq.ph        %[t7],    %[t0],    %[add_x10]  \n\t"
            "shra.ph        %[t0],    %[t7],    5           \n\t"
            "addq.ph        %[t7],    %[t7],    %[t0]       \n\t"
            "shra.ph        %[t0],    %[t7],    5           \n\t"
            "addq.ph        %[t7],    %[t0],    %[t3]       \n\t"
            "shra.ph        %[t3],    %[t7],    3           \n\t"
            "muleu_s.ph.qbl %[t0],    %[t2],    %[t5]       \n\t"
            "addq.ph        %[t7],    %[t0],    %[add_x20]  \n\t"
            "shra.ph        %[t0],    %[t7],    6           \n\t"
            "addq.ph        %[t8],    %[t7],    %[t0]       \n\t"
            "shra.ph        %[t0],    %[t8],    6           \n\t"
            "addq.ph        %[t7],    %[t0],    %[t4]       \n\t"
            "shra.ph        %[t8],    %[t7],    2           \n\t"
            "shll.ph        %[t0],    %[t8],    5           \n\t"
            "shll.ph        %[t1],    %[t6],    11          \n\t"
            "or             %[t2],    %[t0],    %[t1]       \n\t"
            "or             %[t3],    %[t2],    %[t3]       \n\t"
            "sra            %[t4],    %[t3],    16          \n\t"
            "sh             %[t4],    0(%[dst])             \n\t"
            "sh             %[t3],    2(%[dst])             \n\t"
            "addiu          %[src],   %[src],   8           \n\t"
            "addiu          %[dst],   %[dst],   4           \n\t"
            ".set           pop                             \n\t"
            : [dst]"+r"(dst), [src]"+r"(src), [count]"+r"(count),
              [t0]"=&r"(t0), [t1]"=&r"(t1), [t2]"=&r"(t2),
              [t3]"=&r"(t3), [t4]"=&r"(t4), [t5]"=&r"(t5),
              [t6]"=&r"(t6), [t7]"=&r"(t7), [t8]"=&r"(t8)
            : [add_x10]"r"(add_x10), [add_x20]"r"(add_x20),
              [sa]"r"(sa)
            : "memory", "hi", "lo"
        );
    }
#endif
    if (count & 1) {
        SkPMColor c = *src++;
        SkPMColorAssert(c);
        if (c) {
            *dst = SkSrcOver32To16(c, *dst);
        }
        dst += 1;
    }
}

static void S32A_D565_Blend_mips_dsp(uint16_t* SK_RESTRICT dst,
                                     const SkPMColor* SK_RESTRICT src, int count,
                                     U8CPU alpha, int /*x*/, int /*y*/) {
    int temp0, temp1, temp2, temp3, temp4, temp5, temp6;
    uint16_t* loopEnd = dst + count;

    alpha += 1;
    const int c255 = 255;
    const int c256 = 256;
    if (count)
    {
        __asm__ volatile (
                ".set            push                                                        \n\t"
                ".set            noreorder                                                   \n\t"
                "replv.ph        %[temp0],    %[alpha]                                       \n\t"
            "1:                                                                              \n\t"
                "lw              %[temp1],    0(%[src])                                      \n\t"
                "ext             %[temp2],    %[temp1],    " STR(SK_A32_SHIFT)",     8       \n\t"
                "mul             %[temp2],    %[temp2],    %[alpha]                          \n\t"
                "srl             %[temp2],    %[temp2],    8                                 \n\t"
                "subu            %[temp2],    %[c256],     %[temp2]                          \n\t"
                "replv.ph        %[temp2],    %[temp2]                                       \n\t"
                "muleu_s.ph.qbl  %[temp3],    %[temp1],    %[temp0]                          \n\t"
                "muleu_s.ph.qbr  %[temp4],    %[temp1],    %[temp0]                          \n\t"
                "precrq.qb.ph    %[temp5],    %[temp3],    %[temp4]                          \n\t"
                "lhu             %[temp1],    0(%[dst])                                      \n\t"
                "ins             %[temp3],    %[temp1],    16,                       16      \n\t"
                "ins             %[temp3],    %[temp1],    11,                       16      \n\t"
                "ins             %[temp3],    %[temp1],    13,                       11      \n\t"
                "ins             %[temp3],    %[temp1],    7,                        11      \n\t"
                "ins             %[temp3],    %[temp1],    11,                       5       \n\t"
                "ins             %[temp3],    %[temp1],    6,                        5       \n\t"
                "srl             %[temp3],    %[temp3],    8                                 \n\t"
                "ins             %[temp3],    %[c255],     24,                       8       \n\t"
                "muleu_s.ph.qbl  %[temp1],    %[temp3],    %[temp2]                          \n\t"
                "muleu_s.ph.qbr  %[temp2],    %[temp3],    %[temp2]                          \n\t"
                "precrq.qb.ph    %[temp6],    %[temp1],    %[temp2]                          \n\t"
                "addu            %[temp6],    %[temp6],    %[temp5]                          \n\t"
                "sll             %[temp5],    %[temp6],    8                                 \n\t"
                "ins             %[temp5],    %[temp6],    11,                       16      \n\t"
                "ins             %[temp5],    %[temp6],    13,                       8       \n\t"
                "srl             %[temp5],    %[temp5],    16                                \n\t"
                "sh              %[temp5],    0(%[dst])                                      \n\t"
                "addiu           %[dst],      %[dst],      2                                 \n\t"
                "bne             %[dst],      %[loopEnd],  1b                                \n\t"
                " addiu          %[src],      %[src],      4                                 \n\t"
                ".set            pop                                                         \n\t"

                : [src]"+&r"(src), [dst]"+&r"(dst), [temp0]"=&r"(temp0), [temp1]"=&r"(temp1),
                  [temp2]"=&r"(temp2), [temp3]"=&r"(temp3), [temp4]"=&r"(temp4),
                  [temp5]"=&r"(temp5), [temp6]"=&r"(temp6)
                : [loopEnd]"r"(loopEnd), [alpha]"r"(alpha), [c255]"r"(c255), [c256]"r"(c256)
                : "memory", "hi", "lo"
        );
    }
}

static void S32_Blend_BlitRow32_mips_dsp(SkPMColor* SK_RESTRICT dst,
                                         const SkPMColor* SK_RESTRICT src,
                                         int count, U8CPU alpha) {
    register int32_t t0, t1, t2, t3, t4, t5, t6, t7;

    __asm__ volatile (
        ".set            push                         \n\t"
        ".set            noreorder                    \n\t"
        "li              %[t2],    0x100              \n\t"
        "addiu           %[t0],    %[alpha], 1        \n\t"
        "subu            %[t1],    %[t2],    %[t0]    \n\t"
        "replv.qb        %[t7],    %[t0]              \n\t"
        "replv.qb        %[t6],    %[t1]              \n\t"
    "1:                                               \n\t"
        "blez            %[count], 2f                 \n\t"
        "lw              %[t0],    0(%[src])          \n\t"
        "lw              %[t1],    0(%[dst])          \n\t"
        "preceu.ph.qbr   %[t2],    %[t0]              \n\t"
        "preceu.ph.qbl   %[t3],    %[t0]              \n\t"
        "preceu.ph.qbr   %[t4],    %[t1]              \n\t"
        "preceu.ph.qbl   %[t5],    %[t1]              \n\t"
        "muleu_s.ph.qbr  %[t2],    %[t7],    %[t2]    \n\t"
        "muleu_s.ph.qbr  %[t3],    %[t7],    %[t3]    \n\t"
        "muleu_s.ph.qbr  %[t4],    %[t6],    %[t4]    \n\t"
        "muleu_s.ph.qbr  %[t5],    %[t6],    %[t5]    \n\t"
        "addiu           %[src],   %[src],   4        \n\t"
        "addiu           %[count], %[count], -1       \n\t"
        "precrq.qb.ph    %[t0],    %[t3],    %[t2]    \n\t"
        "precrq.qb.ph    %[t2],    %[t5],    %[t4]    \n\t"
        "addu            %[t1],    %[t0],    %[t2]    \n\t"
        "sw              %[t1],    0(%[dst])          \n\t"
        "b               1b                           \n\t"
        " addi           %[dst],   %[dst],   4        \n\t"
    "2:                                               \n\t"
        ".set            pop                          \n\t"
        : [src]"+r"(src), [dst]"+r"(dst), [count]"+r"(count),
          [t0]"=&r"(t0), [t1]"=&r"(t1), [t2]"=&r"(t2), [t3]"=&r"(t3),
          [t4]"=&r"(t4), [t5]"=&r"(t5), [t6]"=&r"(t6), [t7]"=&r"(t7)
        : [alpha]"r"(alpha)
        : "memory", "hi", "lo"
    );
}

static void S32_D565_Opaque_mips_dsp(uint16_t* SK_RESTRICT dst,
                                     const SkPMColor* SK_RESTRICT src, int count,
                                     U8CPU alpha, int /*x*/, int /*y*/) {
    SkASSERT(255 == alpha);
    if (count > 0) {
        int temp0, temp1, temp2, temp3, temp4;
        SkPMColor* const loopEnd = (SkPMColor*)src + count;

        __asm__ volatile (
            ".set    push                                                                     \n\t"
            ".set    noreorder                                                                \n\t"
            "andi    %[temp4],  %[count],    1                                                \n\t"
            "beq     %[temp4],  $zero,       1f                                               \n\t"
            " srl    %[count],  %[count],    1                                                \n\t"
            "lw      %[temp0],  0(%[src])                                                     \n\t"
            "addiu   %[src],    %[src],      4                                                \n\t"
            "ext     %[temp1],  %[temp0],    " STR((SK_B32_SHIFT + (8 - SK_B16_BITS)))",  5   \n\t"
            "ext     %[temp2],  %[temp0],    " STR((SK_G32_SHIFT + (8 - SK_G16_BITS)))",  6   \n\t"
            "ext     %[temp3],  %[temp0],    " STR((SK_R32_SHIFT + (8 - SK_R16_BITS)))",  5   \n\t"
            "ins     %[temp4],  %[temp1],    " STR(SK_B16_SHIFT)",                        5   \n\t"
            "ins     %[temp4],  %[temp2],    " STR(SK_G16_SHIFT)",                        6   \n\t"
            "ins     %[temp4],  %[temp3],    " STR(SK_R16_SHIFT)",                        5   \n\t"
            "sh      %[temp4],  0(%[dst])                                                     \n\t"
            "beq     %[src],    %[loopEnd],  2f                                               \n\t"
            " addiu  %[dst],    %[dst],      2                                                \n\t"
        "1:                                                                                   \n\t"
            "lw      %[temp0],  0(%[src])                                                     \n\t"
            "ext     %[temp1],  %[temp0],    " STR((SK_B32_SHIFT + (8 - SK_B16_BITS)))",  5   \n\t"
            "ext     %[temp2],  %[temp0],    " STR((SK_G32_SHIFT + (8 - SK_G16_BITS)))",  6   \n\t"
            "ext     %[temp3],  %[temp0],    " STR((SK_R32_SHIFT + (8 - SK_R16_BITS)))",  5   \n\t"
            "lw      %[temp0],  4(%[src])                                                     \n\t"
            "ins     %[temp4],  %[temp1],    " STR(SK_B16_SHIFT)",                        5   \n\t"
            "ins     %[temp4],  %[temp2],    " STR(SK_G16_SHIFT)",                        6   \n\t"
            "ins     %[temp4],  %[temp3],    " STR(SK_R16_SHIFT)",                        5   \n\t"
            "ext     %[temp1],  %[temp0],    " STR((SK_B32_SHIFT + (8 - SK_B16_BITS)))",  5   \n\t"
            "ext     %[temp2],  %[temp0],    " STR((SK_G32_SHIFT + (8 - SK_G16_BITS)))",  6   \n\t"
            "ext     %[temp3],  %[temp0],    " STR((SK_R32_SHIFT + (8 - SK_R16_BITS)))",  5   \n\t"
            "ins     %[temp4],  %[temp1],    " STR(SK_B16_SHIFT + 16)",                   5   \n\t"
            "ins     %[temp4],  %[temp2],    " STR(SK_G16_SHIFT + 16)",                   6   \n\t"
            "ins     %[temp4],  %[temp3],    " STR(SK_R16_SHIFT + 16)",                   5   \n\t"
            "addiu   %[src],    %[src],      8                                                \n\t"
            "usw     %[temp4],  0(%[dst])                                                     \n\t"
            "bne     %[src],    %[loopEnd],  1b                                               \n\t"
            " addiu  %[dst],    %[dst],      4                                                \n\t"
        "2:                                                                                   \n\t"
            ".set   pop                                                                       \n\t"
            : [src]"+r"(src), [dst]"+r"(dst), [count]"+r"(count), [temp0]"=&r"(temp0),
              [temp1]"=&r"(temp1), [temp2]"=&r"(temp2), [temp3]"=&r"(temp3), [temp4]"=&r"(temp4)
            : [loopEnd]"r"(loopEnd)
            : "memory"
        );
    }
}
#undef STR
#undef STR1

static void S32A_Opaque_BlitRow32_mips_dsp(SkPMColor* SK_RESTRICT dst,
                                           const SkPMColor* SK_RESTRICT src,
                                           int count, U8CPU alpha) {

    SkASSERT(255 == alpha);

    if (count > 0) {
        register int t0 = 256;
        register int t1, t2, t3, t4, t5, t6, t7, t8;

        __asm__ volatile (
            ".set            push                       \n\t"
            ".set            noreorder                  \n\t"
            "sll             %[t1],  %[count], 2        \n\t"
            "andi            %[t2],  %[count], 1        \n\t"
            "beqz            %[t2],  1f                 \n\t"
            " addu           %[t8],  %[src],   %[t1]    \n\t"
        "0:                                             \n\t"
            "lw              %[t6],  0(%[src])          \n\t"
            "lw              %[t5],  0(%[dst])          \n\t"
            "addiu           %[src], 4                  \n\t"
            "ext             %[t1],  %[t6],    24,    8 \n\t"
            "subu            %[t1],  %[t0],    %[t1]    \n\t"
            "replv.ph        %[t1],  %[t1]              \n\t"
            "muleu_s.ph.qbl  %[t2],  %[t5],    %[t1]    \n\t"
            "muleu_s.ph.qbr  %[t3],  %[t5],    %[t1]    \n\t"
            "precrq.qb.ph    %[t2],  %[t2],    %[t3]    \n\t"
            "addu            %[t2],  %[t2],    %[t6]    \n\t"
            "sw              %[t2],  0(%[dst])          \n\t"
            "beq             %[src], %[t8],    2f       \n\t"
            " addiu          %[dst], 4                  \n\t"
        "1:                                             \n\t"
            "lw              %[t6],  0(%[src])          \n\t"
            "lw              %[t4],  4(%[src])          \n\t"
            "lw              %[t5],  0(%[dst])          \n\t"
            "lw              %[t3],  4(%[dst])          \n\t"
            "ext             %[t1],  %[t6],    24,   8  \n\t"
            "ext             %[t2],  %[t4],    24,   8  \n\t"
            "subu            %[t1],  %[t0],    %[t1]    \n\t"
            "subu            %[t2],  %[t0],    %[t2]    \n\t"
            "replv.ph        %[t1],  %[t1]              \n\t"
            "replv.ph        %[t2],  %[t2]              \n\t"
            "muleu_s.ph.qbl  %[t7],  %[t5],    %[t1]    \n\t"
            "muleu_s.ph.qbr  %[t5],  %[t5],    %[t1]    \n\t"
            "muleu_s.ph.qbl  %[t1],  %[t3],    %[t2]    \n\t"
            "muleu_s.ph.qbr  %[t3],  %[t3],    %[t2]    \n\t"
            "addiu           %[src], 8                  \n\t"
            "precrq.qb.ph    %[t2],  %[t7],    %[t5]    \n\t"
            "precrq.qb.ph    %[t1],  %[t1],    %[t3]    \n\t"
            "addu            %[t2],  %[t2],    %[t6]    \n\t"
            "addu            %[t1],  %[t1],    %[t4]    \n\t"
            "sw              %[t2],  0(%[dst])          \n\t"
            "sw              %[t1],  4(%[dst])          \n\t"
            "bne             %[src], %[t8],    1b       \n\t"
            " addiu          %[dst], 8                  \n\t"
        "2:                                             \n\t"
            ".set            pop                        \n\t"
            : [src]"+r"(src), [dst]"+r"(dst), [t1]"=&r"(t1),
              [t2]"=&r"(t2), [t3]"=&r"(t3), [t4]"=&r"(t4),
              [t5]"=&r"(t5), [t6]"=&r"(t6), [t7]"=&r"(t7),
              [t8]"=&r"(t8)
            : [count]"r"(count), [t0]"r"(t0)
            : "memory", "hi", "lo"
        );
    }
}

void blitmask_d565_opaque_mips(int width, int height, uint16_t* device,
                               unsigned deviceRB, const uint8_t* alpha,
                               uint32_t expanded32, unsigned maskRB) {
    register uint32_t s0, s1, s2, s3;

    __asm__ volatile (
        ".set            push                                    \n\t"
        ".set            noreorder                               \n\t"
        ".set            noat                                    \n\t"
        "li              $t9,       0x7E0F81F                    \n\t"
    "1:                                                          \n\t"
        "move            $t8,       %[width]                     \n\t"
        "addiu           %[height], %[height],     -1            \n\t"
    "2:                                                          \n\t"
        "beqz            $t8,       4f                           \n\t"
        " addiu          $t0,       $t8,           -4            \n\t"
        "bltz            $t0,       3f                           \n\t"
        " nop                                                    \n\t"
        "addiu           $t8,       $t8,           -4            \n\t"
        "lhu             $t0,       0(%[device])                 \n\t"
        "lhu             $t1,       2(%[device])                 \n\t"
        "lhu             $t2,       4(%[device])                 \n\t"
        "lhu             $t3,       6(%[device])                 \n\t"
        "lbu             $t4,       0(%[alpha])                  \n\t"
        "lbu             $t5,       1(%[alpha])                  \n\t"
        "lbu             $t6,       2(%[alpha])                  \n\t"
        "lbu             $t7,       3(%[alpha])                  \n\t"
        "replv.ph        $t0,       $t0                          \n\t"
        "replv.ph        $t1,       $t1                          \n\t"
        "replv.ph        $t2,       $t2                          \n\t"
        "replv.ph        $t3,       $t3                          \n\t"
        "addiu           %[s0],     $t4,           1             \n\t"
        "addiu           %[s1],     $t5,           1             \n\t"
        "addiu           %[s2],     $t6,           1             \n\t"
        "addiu           %[s3],     $t7,           1             \n\t"
        "srl             %[s0],     %[s0],         3             \n\t"
        "srl             %[s1],     %[s1],         3             \n\t"
        "srl             %[s2],     %[s2],         3             \n\t"
        "srl             %[s3],     %[s3],         3             \n\t"
        "and             $t0,       $t0,           $t9           \n\t"
        "and             $t1,       $t1,           $t9           \n\t"
        "and             $t2,       $t2,           $t9           \n\t"
        "and             $t3,       $t3,           $t9           \n\t"
        "subu            $t4,       %[expanded32], $t0           \n\t"
        "subu            $t5,       %[expanded32], $t1           \n\t"
        "subu            $t6,       %[expanded32], $t2           \n\t"
        "subu            $t7,       %[expanded32], $t3           \n\t"
        "mul             $t4,       $t4,           %[s0]         \n\t"
        "mul             $t5,       $t5,           %[s1]         \n\t"
        "mul             $t6,       $t6,           %[s2]         \n\t"
        "mul             $t7,       $t7,           %[s3]         \n\t"
        "addiu           %[alpha],  %[alpha],      4             \n\t"
        "srl             $t4,       $t4,           5             \n\t"
        "srl             $t5,       $t5,           5             \n\t"
        "srl             $t6,       $t6,           5             \n\t"
        "srl             $t7,       $t7,           5             \n\t"
        "addu            $t4,       $t0,           $t4           \n\t"
        "addu            $t5,       $t1,           $t5           \n\t"
        "addu            $t6,       $t2,           $t6           \n\t"
        "addu            $t7,       $t3,           $t7           \n\t"
        "and             $t4,       $t4,           $t9           \n\t"
        "and             $t5,       $t5,           $t9           \n\t"
        "and             $t6,       $t6,           $t9           \n\t"
        "and             $t7,       $t7,           $t9           \n\t"
        "srl             $t0,       $t4,           16            \n\t"
        "srl             $t1,       $t5,           16            \n\t"
        "srl             $t2,       $t6,           16            \n\t"
        "srl             $t3,       $t7,           16            \n\t"
        "or              %[s0],     $t0,           $t4           \n\t"
        "or              %[s1],     $t1,           $t5           \n\t"
        "or              %[s2],     $t2,           $t6           \n\t"
        "or              %[s3],     $t3,           $t7           \n\t"
        "sh              %[s0],     0(%[device])                 \n\t"
        "sh              %[s1],     2(%[device])                 \n\t"
        "sh              %[s2],     4(%[device])                 \n\t"
        "sh              %[s3],     6(%[device])                 \n\t"
        "b               2b                                      \n\t"
        " addiu          %[device], %[device],     8             \n\t"
    "3:                                                          \n\t"
        "lhu             $t0,       0(%[device])                 \n\t"
        "lbu             $t1,       0(%[alpha])                  \n\t"
        "addiu           $t8,       $t8,           -1            \n\t"
        "replv.ph        $t2,       $t0                          \n\t"
        "and             $t2,       $t2,           $t9           \n\t"
        "addiu           $t0,       $t1,           1             \n\t"
        "srl             $t0,       $t0,           3             \n\t"
        "subu            $t3,       %[expanded32], $t2           \n\t"
        "mul             $t3,       $t3,           $t0           \n\t"
        "addiu           %[alpha],  %[alpha],      1             \n\t"
        "srl             $t3,       $t3,           5             \n\t"
        "addu            $t3,       $t2,           $t3           \n\t"
        "and             $t3,       $t3,           $t9           \n\t"
        "srl             $t4,       $t3,           16            \n\t"
        "or              %[s0],     $t4,           $t3           \n\t"
        "sh              %[s0],     0(%[device])                 \n\t"
        "bnez            $t8,       3b                           \n\t"
         "addiu          %[device], %[device],     2             \n\t"
    "4:                                                          \n\t"
        "addu            %[device], %[device],     %[deviceRB]   \n\t"
        "bgtz            %[height], 1b                           \n\t"
        " addu           %[alpha],  %[alpha],      %[maskRB]     \n\t"
        ".set            pop                                     \n\t"
        : [height]"+r"(height), [alpha]"+r"(alpha), [device]"+r"(device),
          [deviceRB]"+r"(deviceRB), [maskRB]"+r"(maskRB), [s0]"=&r"(s0),
          [s1]"=&r"(s1), [s2]"=&r"(s2), [s3]"=&r"(s3)
        : [expanded32] "r" (expanded32), [width] "r" (width)
        : "memory", "hi", "lo", "t0", "t1", "t2", "t3",
          "t4", "t5", "t6", "t7", "t8", "t9"
    );
}

///////////////////////////////////////////////////////////////////////////////

const SkBlitRow::Proc16 platform_565_procs_mips_dsp[] = {
    // no dither
    S32_D565_Opaque_mips_dsp,
    S32_D565_Blend_mips_dsp,
    S32A_D565_Opaque_mips_dsp,
    S32A_D565_Blend_mips_dsp,

    // dither
#ifdef SK_MIPS_HAS_DSPR2
    S32_D565_Opaque_Dither_mips_dsp,
    S32_D565_Blend_Dither_mips_dsp,
#else
    NULL,
    NULL,
#endif
    S32A_D565_Opaque_Dither_mips_dsp,
    NULL,
};

static const SkBlitRow::Proc32 platform_32_procs_mips_dsp[] = {
    NULL,   // S32_Opaque,
    S32_Blend_BlitRow32_mips_dsp,
    S32A_Opaque_BlitRow32_mips_dsp,
    NULL,   // S32A_Blend,
};

SkBlitRow::Proc16 SkBlitRow::PlatformFactory565(unsigned flags) {
    return platform_565_procs_mips_dsp[flags];
}

SkBlitRow::ColorProc16 SkBlitRow::PlatformColorFactory565(unsigned flags) {
    return NULL;
}

SkBlitRow::Proc32 SkBlitRow::PlatformProcs32(unsigned flags) {
    return platform_32_procs_mips_dsp[flags];
}
