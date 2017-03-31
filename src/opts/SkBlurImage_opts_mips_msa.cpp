/*
 * Copyright 2014 ARM Ltd.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "SkBlurImage_opts_mips_msa.h"
#include <msa.h>

namespace {
enum BlurDirection {
    kX, kY
};

template<BlurDirection srcDirection, BlurDirection dstDirection>
static void boxBlur_mips_msa(const SkPMColor* src, int srcStride, SkPMColor* dst, int kernelSize,
                    int leftOffset, int rightOffset, int width, int height)
{
    int rightBorder = SkMin32(rightOffset + 1, width);
    int srcStrideX = srcDirection == kX ? 1 : srcStride;
    int dstStrideX = dstDirection == kX ? 1 : height;
    int srcStrideY = srcDirection == kX ? srcStride : 1;
    int dstStrideY = dstDirection == kX ? width : 1;
    uint32_t scale = (1 << 24) / kernelSize;
    uint32_t half = 1 << 23;
    v4u32 zero       = __builtin_msa_fill_w(0);
    v4u32 temp_scale = __builtin_msa_fill_w(scale);
    v4u32 temp_half  = __builtin_msa_fill_w(half);
    for (int y = 0; y < height; ++y) {
        v4u32 temp_sum = __builtin_msa_fill_w(0);
        const SkPMColor* p = src;
        for (int i = 0; i < rightBorder; ++i) {
            v4u32 temp1, temp2;
            int a1 = *p;
            temp1 = __builtin_msa_fill_w(a1);
            temp2 = __builtin_msa_ilvr_b(zero, temp1);
            temp2 = __builtin_msa_ilvr_h(zero, temp2);
            temp_sum = __builtin_msa_addv_w(temp_sum, temp2);
            p += srcStrideX;
        }

        const SkPMColor* sptr = src;
        SkColor* dptr = dst;
        for (int x = 0; x < width; ++x) {
             v4u32 temp1, temp2, temp3;
             temp1 = __builtin_msa_mulv_w(temp_sum, temp_scale);
             temp2 = __builtin_msa_addv_w(temp1, temp_half);
             temp1 = __builtin_msa_pckod_h(zero, temp2);
             temp3 = __builtin_msa_pckod_b(zero, temp1);
            *dptr = temp3[0];
            if (x >= leftOffset) {
                SkColor l = *(sptr - leftOffset * srcStrideX);
                v4u32 temp3, temp4, temp5;
                temp3 = __builtin_msa_fill_w(l);
                temp4 = __builtin_msa_ilvr_b(zero, temp3);
                temp5 = __builtin_msa_ilvr_h(zero, temp4);
                temp_sum = __builtin_msa_subv_w(temp_sum, temp5);
            }
            if (x + rightOffset + 1 < width) {
                SkColor r = *(sptr + (rightOffset + 1) * srcStrideX);
                v4u32 temp1, temp2;
                temp1 = __builtin_msa_fill_w(r);
                temp2 = __builtin_msa_ilvr_b(zero, temp1);
                temp2 = __builtin_msa_ilvr_h(zero, temp2);
                temp_sum = __builtin_msa_addv_w(temp_sum, temp2);
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

bool SkBoxBlurGetPlatformProcs_mips_msa(SkBoxBlurProc* boxBlurX,
                                    SkBoxBlurProc* boxBlurY,
                                    SkBoxBlurProc* boxBlurXY,
                                    SkBoxBlurProc* boxBlurYX) {
    *boxBlurX = boxBlur_mips_msa<kX, kX>;
    *boxBlurY = boxBlur_mips_msa<kY, kY>;
    *boxBlurXY = boxBlur_mips_msa<kX, kY>;
    *boxBlurYX = boxBlur_mips_msa<kY, kX>;
    return true;
}

bool SkBoxBlurGetPlatformProcs(SkBoxBlurProc* boxBlurX,
                               SkBoxBlurProc* boxBlurY,
                               SkBoxBlurProc* boxBlurXY,
                               SkBoxBlurProc* boxBlurYX) {

    return SkBoxBlurGetPlatformProcs_mips_msa(boxBlurX, boxBlurY, boxBlurXY, boxBlurYX);
}
