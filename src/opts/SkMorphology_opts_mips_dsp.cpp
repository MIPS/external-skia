/*
 * Copyright 2013 The Android Open Source Project
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "SkColorPriv.h"
#include "SkMorphology_opts.h"
#include "SkMorphology_opts_mips_dsp.h"

enum MorphDirection {
    kX, kY
};

template<MorphDirection direction>
static void erode_mips_dsp(const SkPMColor* src, SkPMColor* dst,
                           int radius, int width, int height,
                           int srcStride, int dstStride)
{
    const int srcStrideX = direction == kX ? 1 : srcStride;
    const int dstStrideX = direction == kX ? 1 : dstStride;
    const int srcStrideY = direction == kX ? srcStride : 1;
    const int dstStrideY = direction == kX ? dstStride : 1;
    radius = SkMin32(radius, width - 1);
    const SkPMColor* upperSrc = src + radius * srcStrideX;
    for (int x = 0; x < width; ++x) {
        const SkPMColor* lp = src;
        const SkPMColor* up = upperSrc;
        SkPMColor* dptr = dst;

         for (int y = 0; y < height; ++y) {
            int tmp_const = 0xFFFFFFFF;
            int b_1 = 0;
            for (const SkPMColor* p = lp; p <= up; p += srcStrideX) {
                __asm__ volatile (
                    "lw          %[b_1],       0(%[p])                    \n\t"
                    "cmpu.lt.qb  %[b_1],       %[tmp_const]               \n\t"
                    "pick.qb     %[tmp_const], %[b_1],      %[tmp_const]  \n\t"
                    : [b_1]"=&r"(b_1), [tmp_const]"+r"(tmp_const)
                    : [p]"r"(p)
                    : "memory"
                );
            }
            *dptr =  tmp_const;
            dptr += dstStrideY;
            lp += srcStrideY;
            up += srcStrideY;
        }
        if (x >= radius) src += srcStrideX;
        if (x + radius < width - 1) upperSrc += srcStrideX;
        dst += dstStrideX;
    }
}

template<MorphDirection direction>
static void dilate_mips_dsp(const SkPMColor* src, SkPMColor* dst,
                            int radius, int width, int height,
                            int srcStride, int dstStride)
{
    const int srcStrideX = direction == kX ? 1 : srcStride;
    const int dstStrideX = direction == kX ? 1 : dstStride;
    const int srcStrideY = direction == kX ? srcStride : 1;
    const int dstStrideY = direction == kX ? dstStride : 1;
    radius = SkMin32(radius, width - 1);
    const SkPMColor* upperSrc = src + radius * srcStrideX;
    for (int x = 0; x < width; ++x) {
        const SkPMColor* lp = src;
        const SkPMColor* up = upperSrc;
        SkPMColor* dptr = dst;

        for (int y = 0; y < height; ++y) {
            int tmp_const = 0;
            int b_1 = 0;
            for (const SkPMColor* p = lp; p <= up; p += srcStrideX) {
                __asm__ volatile (
                    "lw          %[b_1],       0(%[p])                    \n\t"
                    "cmpu.lt.qb  %[tmp_const], %[b_1]                     \n\t"
                    "pick.qb     %[tmp_const], %[b_1],   %[tmp_const]     \n\t"
                    : [b_1]"=&r"(b_1), [tmp_const]"+r"(tmp_const)
                    : [p]"r"(p)
                    : "memory"
                );
            }
            *dptr =  tmp_const;
            dptr += dstStrideY;
            lp += srcStrideY;
            up += srcStrideY;
        }

        if (x >= radius) src += srcStrideX;
        if (x + radius < width - 1) upperSrc += srcStrideX;
        dst += dstStrideX;
    }
}

void SkDilateX_mips_dsp(const SkPMColor* src, SkPMColor* dst, int radius,
                    int width, int height, int srcStride, int dstStride)
{
    dilate_mips_dsp<kX>(src, dst, radius, width, height, srcStride, dstStride);
}

void SkErodeX_mips_dsp(const SkPMColor* src, SkPMColor* dst, int radius,
                   int width, int height, int srcStride, int dstStride)
{
    erode_mips_dsp<kX>(src, dst, radius, width, height, srcStride, dstStride);
}

void SkDilateY_mips_dsp(const SkPMColor* src, SkPMColor* dst, int radius,
                    int width, int height, int srcStride, int dstStride)
{
    dilate_mips_dsp<kY>(src, dst, radius, width, height, srcStride, dstStride);
}

void SkErodeY_mips_dsp(const SkPMColor* src, SkPMColor* dst, int radius,
                   int width, int height, int srcStride, int dstStride)
{
    erode_mips_dsp<kY>(src, dst, radius, width, height, srcStride, dstStride);
}
/////////////////////////////////////////////////////////////////////////////////////////

SkMorphologyImageFilter::Proc SkMorphologyGetPlatformProc(SkMorphologyProcType type) {
    switch (type) {
        case kDilateX_SkMorphologyProcType:
            return SkDilateX_mips_dsp;
        case kDilateY_SkMorphologyProcType:
            return SkDilateY_mips_dsp;
        case kErodeX_SkMorphologyProcType:
            return SkErodeX_mips_dsp;
        case kErodeY_SkMorphologyProcType:
            return SkErodeY_mips_dsp;
        default:
            return NULL;
    }
}