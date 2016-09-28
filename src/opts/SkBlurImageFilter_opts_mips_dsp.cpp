/*
 * Copyright 2013 The Android Open Source Project
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "SkBitmap.h"
#include "SkBlurImageFilter_opts_mips_dsp.h"
#include "SkColorPriv.h"
#include "SkRect.h"

#define STR1( x ) #x
#define STR( x ) STR1( x )

namespace {
enum BlurDirection {
    kX, kY
};

template<BlurDirection srcDirection, BlurDirection dstDirection>
static void SkBoxBlur_mips_dsp(const SkPMColor* src, int srcStride, SkPMColor* dst, int kernelSize,
                               int leftOffset, int rightOffset, int width, int height)
{
    int rightBorder = SkMin32(rightOffset + 1, width);
    int srcStrideX = srcDirection == kX ? 1 : srcStride;
    int dstStrideX = dstDirection == kX ? 1 : height;
    int srcStrideY = srcDirection == kX ? srcStride : 1;
    int dstStrideY = dstDirection == kX ? width : 1;
    uint32_t scale = (1 << 24) / kernelSize;
    int temp0, temp1, temp2, temp3, temp4, temp5;
    SkPMColor* loopEnd;
    const int border = rightBorder * srcStrideX;

    for (int y = 0; y < height; ++y) {
        int sumA, sumR, sumG, sumB;
        const SkPMColor* p = src;
        loopEnd = (SkPMColor*)p + border;
        __asm__ volatile(
            ".set   push                                                   \n\t"
            ".set   noreorder                                              \n\t"
            "beq    %[loopEnd], %[p],          1f                          \n\t"
            " sll   %[temp5],   %[srcStrideX], 2                           \n\t"
            "xor    %[sumA],    %[sumA],       %[sumA]                     \n\t"
            "xor    %[sumR],    %[sumR],       %[sumR]                     \n\t"
            "xor    %[sumG],    %[sumG],       %[sumG]                     \n\t"
            "xor    %[sumB],    %[sumB],       %[sumB]                     \n\t"
        "2:                                                                \n\t"
            "lw     %[temp0],   0(%[p])                                    \n\t"
            "ext    %[temp1],   %[temp0],      " STR(SK_A32_SHIFT)", 8      \n\t"
            "ext    %[temp2],   %[temp0],      " STR(SK_R32_SHIFT)", 8      \n\t"
            "ext    %[temp3],   %[temp0],      " STR(SK_G32_SHIFT)", 8      \n\t"
            "ext    %[temp4],   %[temp0],      " STR(SK_B32_SHIFT)", 8      \n\t"
            "addu   %[p],       %[p],          %[temp5]                    \n\t"
            "addu   %[sumA],    %[sumA],       %[temp1]                    \n\t"
            "addu   %[sumR],    %[sumR],       %[temp2]                    \n\t"
            "addu   %[sumG],    %[sumG],       %[temp3]                    \n\t"
            "bne    %[loopEnd], %[p],          2b                          \n\t"
            " addu  %[sumB],    %[sumB],       %[temp4]                    \n\t"
        "1:                                                                \n\t"
            ".set   pop                                                    \n\t"
            : [temp0]"=&r"(temp0), [temp1]"=&r"(temp1), [temp2]"=&r"(temp2),
              [temp3]"=&r"(temp3), [temp4]"=&r"(temp4), [temp5]"=&r"(temp5),
              [sumA]"=&r"(sumA), [sumR]"=&r"(sumR), [sumG]"=&r"(sumG),
              [sumB]"=&r"(sumB), [p]"+r"(p)
            : [srcStrideX]"r"(srcStrideX), [loopEnd]"r"(loopEnd)
            : "memory"
        );

        const SkPMColor* sptr = src;
        SkColor* dptr = dst;
        for (int x = 0; x < width; ++x) {
            __asm__ volatile(
                "mul        %[temp0], %[sumA],  %[scale]                    \n\t"
                "mul        %[temp1], %[sumR],  %[scale]                    \n\t"
                "mul        %[temp2], %[sumG],  %[scale]                    \n\t"
                "mul        %[temp3], %[sumB],  %[scale]                    \n\t"
                "shra_r.w   %[temp0], %[temp0], 24                          \n\t"
                "shra_r.w   %[temp1], %[temp1], 24                          \n\t"
                "shra_r.w   %[temp2], %[temp2], 24                          \n\t"
                "shra_r.w   %[temp3], %[temp3], 24                          \n\t"
                "ins        %[temp4], %[temp0], " STR(SK_A32_SHIFT)", 8      \n\t"
                "ins        %[temp4], %[temp1], " STR(SK_R32_SHIFT)", 8      \n\t"
                "ins        %[temp4], %[temp2], " STR(SK_G32_SHIFT)", 8      \n\t"
                "ins        %[temp4], %[temp3], " STR(SK_B32_SHIFT)", 8      \n\t"
                : [temp0]"=&r"(temp0), [temp1]"=&r"(temp1), [temp2]"=&r"(temp2),
                  [temp3]"=&r"(temp3), [temp4]"=&r"(temp4), [sumA]"+r"(sumA),
                  [sumR]"+r"(sumR), [sumG]"+r"(sumG), [sumB]"+r"(sumB)
                : [scale]"r"(scale)
                : "hi", "lo"
            );
            *dptr = temp4;
            if (x >= leftOffset) {
                SkColor l = *(sptr - leftOffset * srcStrideX);
                __asm__ volatile(
                    "ext    %[temp1],   %[l],      " STR(SK_A32_SHIFT)", 8      \n\t"
                    "ext    %[temp2],   %[l],      " STR(SK_R32_SHIFT)", 8      \n\t"
                    "ext    %[temp3],   %[l],      " STR(SK_G32_SHIFT)", 8      \n\t"
                    "ext    %[temp4],   %[l],      " STR(SK_B32_SHIFT)", 8      \n\t"
                    "subu   %[sumA],    %[sumA],   %[temp1]                    \n\t"
                    "subu   %[sumR],    %[sumR],   %[temp2]                    \n\t"
                    "subu   %[sumG],    %[sumG],   %[temp3]                    \n\t"
                    "subu   %[sumB],    %[sumB],   %[temp4]                    \n\t"
                    : [temp1]"=&r"(temp1), [temp2]"=&r"(temp2), [temp3]"=&r"(temp3),
                      [temp4]"=&r"(temp4), [sumA]"+r"(sumA), [sumR]"+r"(sumR),
                      [sumG]"+r"(sumG), [sumB]"+r"(sumB)
                    : [l]"r"(l)
                );
            }
            if (x + rightOffset + 1 < width) {
                SkColor r = *(sptr + (rightOffset + 1) * srcStrideX);
                __asm__ volatile(
                    "ext    %[temp1],   %[r],      " STR(SK_A32_SHIFT)", 8      \n\t"
                    "ext    %[temp2],   %[r],      " STR(SK_R32_SHIFT)", 8      \n\t"
                    "ext    %[temp3],   %[r],      " STR(SK_G32_SHIFT)", 8      \n\t"
                    "ext    %[temp4],   %[r],      " STR(SK_B32_SHIFT)", 8      \n\t"
                    "addu   %[sumA],    %[sumA],   %[temp1]                    \n\t"
                    "addu   %[sumR],    %[sumR],   %[temp2]                    \n\t"
                    "addu   %[sumG],    %[sumG],   %[temp3]                    \n\t"
                    "addu   %[sumB],    %[sumB],   %[temp4]                    \n\t"
                    : [temp1]"=&r"(temp1), [temp2]"=&r"(temp2), [temp3]"=&r"(temp3),
                      [temp4]"=&r"(temp4), [sumA]"+r"(sumA), [sumR]"+r"(sumR),
                      [sumG]"+r"(sumG), [sumB]"+r"(sumB)
                    : [r]"r"(r)
                );
            }
            sptr += srcStrideX;
            if (srcDirection == kY) {
                SK_PREFETCH(sptr + (rightOffset + 1) * srcStrideX);
            }
            dptr += dstStrideX;
        }
        src += srcStrideY;
        dst += dstStrideY;
    }
}
} // namespace

#undef STR
#undef STR1

bool SkBoxBlurGetPlatformProcs_mips_dsp(SkBoxBlurProc* boxBlurX,
                                        SkBoxBlurProc* boxBlurY,
                                        SkBoxBlurProc* boxBlurXY) {
    *boxBlurX = SkBoxBlur_mips_dsp<kX, kX>;
    *boxBlurY = SkBoxBlur_mips_dsp<kY, kY>;
    *boxBlurXY = SkBoxBlur_mips_dsp<kX, kY>;
    // *boxBlurYX = SkBoxBlur_mips_dsp<kY, kX>;
    return true;
}

///////////////////////////////////////////////////////////////////////////////

bool SkBoxBlurGetPlatformProcs(SkBoxBlurProc* boxBlurX,
                               SkBoxBlurProc* boxBlurY,
                               SkBoxBlurProc* boxBlurXY) {
#ifdef SK_DISABLE_BLUR_DIVISION_OPTIMIZATION
    return false;
#else
    return SkBoxBlurGetPlatformProcs_mips_dsp(boxBlurX, boxBlurY, boxBlurXY);
#endif
}
