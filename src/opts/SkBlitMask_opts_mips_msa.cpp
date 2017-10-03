/*
 * Copyright 2017 Google Inc.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "SkBlitMask.h"
#include "SkColor.h"
#include "SkColorPriv.h"
#include "SkBlitMask_opts_mips_msa.h"
#include "SkCpu.h"
#include <msa.h>

void SkBlitLCD16OpaqueRow_mips_msa(SkPMColor dst[], const uint16_t mask[],
                                   SkColor src, int width, SkPMColor opaqueDst){
    int srcR = SkColorGetR(src);
    int srcG = SkColorGetG(src);
    int srcB = SkColorGetB(src);
    v8i16 alpha8 = __builtin_msa_ldi_h(0xff);
    v8i16 mask8 = __builtin_msa_ldi_h(0x1f);
    v8i16 srcR8 = __builtin_msa_fill_h(srcR);
    v8i16 srcG8 = __builtin_msa_fill_h(srcG);
    v8i16 srcB8 = __builtin_msa_fill_h(srcB);

    while (width >= 8) {
        v8i16 mask8_msa = __builtin_msa_ld_h((void*)mask, 0);
        v4i32 dst4_msa = __builtin_msa_ld_w(dst, 0);
        v4i32 dst8_msa = __builtin_msa_ld_w(dst, 16);
        v8i16 mask8_msa_R = __builtin_msa_srli_h(mask8_msa, 11);
        v8i16 mask8_msa_G = __builtin_msa_slli_h(mask8_msa, 5);
        mask8_msa_G = __builtin_msa_srli_h(mask8_msa_G, 11);
        v8i16 mask8_msa_B = __builtin_msa_and_v(mask8_msa, mask8);
        v8i16 mask8_msa_R32 = __builtin_msa_srli_h(mask8_msa_R, 4);
        v8i16 mask8_msa_G32 = __builtin_msa_srli_h(mask8_msa_G, 4);
        v8i16 mask8_msa_B32 = __builtin_msa_srli_h(mask8_msa_B, 4);
        mask8_msa_R = __builtin_msa_addv_h(mask8_msa_R, mask8_msa_R32);
        mask8_msa_G = __builtin_msa_addv_h(mask8_msa_G, mask8_msa_G32);
        mask8_msa_B = __builtin_msa_addv_h(mask8_msa_B, mask8_msa_B32);
        v8i16 dst8GB = __builtin_msa_pckev_h(dst8_msa, dst4_msa);
        v8i16 dst8B = __builtin_msa_and_v(dst8GB, alpha8);
        v8i16 dst8G = __builtin_msa_srli_h(dst8GB, 8);
        v8i16 dst8AR = __builtin_msa_pckod_h(dst8_msa, dst4_msa);
        v8i16 dst8R =  __builtin_msa_slli_h(dst8AR, 8);
        dst8R = __builtin_msa_srli_h(dst8R, 8);
        v8i16 sub8R = __builtin_msa_subv_h(srcR8, dst8R);
        v8i16 sub8G = __builtin_msa_subv_h(srcG8, dst8G);
        v8i16 sub8B = __builtin_msa_subv_h(srcB8, dst8B);
        sub8R = __builtin_msa_mulv_h(sub8R, mask8_msa_R);
        sub8G = __builtin_msa_mulv_h(sub8G, mask8_msa_G);
        sub8B = __builtin_msa_mulv_h(sub8B, mask8_msa_B);
        sub8R = __builtin_msa_srli_h(sub8R, 5);
        sub8G = __builtin_msa_srli_h(sub8G, 5);
        sub8B = __builtin_msa_srli_h(sub8B, 5);
        sub8R = __builtin_msa_addv_h(sub8R, dst8R);
        sub8G = __builtin_msa_addv_h(sub8G, dst8G);
        sub8B = __builtin_msa_addv_h(sub8B, dst8B);
        sub8R = __builtin_msa_ilvev_b(alpha8, sub8R);
        v8i16 sub8G8B = __builtin_msa_ilvev_b(sub8G, sub8B);
        dst8_msa = __builtin_msa_ilvl_h(sub8R, sub8G8B);
        dst4_msa = __builtin_msa_ilvr_h(sub8R, sub8G8B);
        __builtin_msa_st_h(dst4_msa, (void*)dst, 0);
        __builtin_msa_st_h(dst8_msa, (void*)dst, 16);
        dst += 8;
        mask += 8;
        width -= 8;
    }

    while (width > 0) {
        *dst = SkBlendLCD16Opaque(srcR, srcG, srcB, *dst, *mask, opaqueDst);
        mask++;
        dst++;
        width--;
    }
}

void SkBlitLCD16Row_mips_msa(SkPMColor dst[], const uint16_t mask[],
                             SkColor src, int width, SkPMColor) {
    int srcA = SkColorGetA(src);
    int srcR = SkColorGetR(src);
    int srcG = SkColorGetG(src);
    int srcB = SkColorGetB(src);

    srcA = SkAlpha255To256(srcA);

    const v8i16 v_srcA = __builtin_msa_fill_h(srcA);
    const v8i16 v_srcR = __builtin_msa_fill_h(srcR);
    const v8i16 v_srcG = __builtin_msa_fill_h(srcG);
    const v8i16 v_srcB = __builtin_msa_fill_h(srcB);
    const v8i16 v_msb = __builtin_msa_ldi_h(0xff);
    const v8i16 v_maskB = __builtin_msa_ldi_h(0x1f);
    while (width >= 8) {
        v4i32 v_temp0, v_temp1, v_temp2, v_temp3, v_temp4;
        v4i32 v_temp5, v_temp6, v_temp7, v_temp8;
        v_temp0 = __builtin_msa_ld_b((void*)dst, 0);
        v_temp1 = __builtin_msa_ld_b((void*)dst, 16);
        v_temp2 = __builtin_msa_ld_h((void*)mask, 0);
        v_temp3 = __builtin_msa_srli_h(v_temp2, 11);
        v_temp4 = __builtin_msa_slli_h(v_temp2, 5);
        v_temp5 = __builtin_msa_and_v(v_temp2, v_maskB);
        v_temp4 = __builtin_msa_srli_h(v_temp4, 11);
        v_temp2 = __builtin_msa_srli_h(v_temp3, 4);
        v_temp6 = __builtin_msa_srli_h(v_temp4, 4);
        v_temp7 = __builtin_msa_srli_h(v_temp5, 4);
        v_temp3 = __builtin_msa_addv_h(v_temp3, v_temp2);
        v_temp4 = __builtin_msa_addv_h(v_temp4, v_temp6);
        v_temp5 = __builtin_msa_addv_h(v_temp5, v_temp7);
        v_temp3 = __builtin_msa_mulv_h(v_temp3, v_srcA);
        v_temp4 = __builtin_msa_mulv_h(v_temp4, v_srcA);
        v_temp5 = __builtin_msa_mulv_h(v_temp5, v_srcA);
        v_temp3 = __builtin_msa_srli_h(v_temp3, 8);
        v_temp4 = __builtin_msa_srli_h(v_temp4, 8);
        v_temp5 = __builtin_msa_srli_h(v_temp5, 8);
        v_temp2 = __builtin_msa_pckod_h(v_temp1, v_temp0);
        v_temp1 = __builtin_msa_pckev_h(v_temp1, v_temp0);
        v_temp2 = __builtin_msa_and_v(v_temp2, v_msb);
        v_temp0 = __builtin_msa_and_v(v_temp1, v_msb);
        v_temp1 = __builtin_msa_srli_h(v_temp1, 8);
        v_temp6 = __builtin_msa_subv_h(v_srcR, v_temp2);
        v_temp7 = __builtin_msa_subv_h(v_srcG, v_temp1);
        v_temp8 = __builtin_msa_subv_h(v_srcB, v_temp0);
        v_temp6 = __builtin_msa_mulv_h(v_temp6, v_temp3);
        v_temp7 = __builtin_msa_mulv_h(v_temp7, v_temp4);
        v_temp8 = __builtin_msa_mulv_h(v_temp8, v_temp5);
        v_temp6 = __builtin_msa_srli_h(v_temp6, 5);
        v_temp7 = __builtin_msa_srli_h(v_temp7, 5);
        v_temp8 = __builtin_msa_srli_h(v_temp8, 5);
        v_temp2 = __builtin_msa_addv_h(v_temp2, v_temp6);
        v_temp1 = __builtin_msa_addv_h(v_temp1, v_temp7);
        v_temp0 = __builtin_msa_addv_h(v_temp0, v_temp8);
        v_temp2 = __builtin_msa_ilvev_b(v_msb, v_temp2);
        v_temp3 = __builtin_msa_ilvev_b(v_temp1, v_temp0);
        v_temp0 = __builtin_msa_ilvr_h(v_temp2, v_temp3);
        v_temp1 = __builtin_msa_ilvl_h(v_temp2, v_temp3);
        __builtin_msa_st_b(v_temp0, (void*)dst, 0);
        __builtin_msa_st_b(v_temp1, (void*)dst, 16);
        dst += 8;
        mask +=8;
        width -= 8;
    }
    while (width--) {
        *dst = SkBlendLCD16(srcA, srcR, srcG, srcB, *dst, *mask);
        mask++;
        dst++;
    }
}

SkBlitMask::BlitLCD16RowProc SkBlitMask::PlatformBlitRowProcs16(bool isOpaque)
{
    if (SkCpu::Supports(SkCpu::MSA)) {
        if (isOpaque) {
            return SkBlitLCD16OpaqueRow_mips_msa;
        } else {
            return SkBlitLCD16Row_mips_msa;
        }
    } else {
        return nullptr;
    }
}

SkBlitMask::RowProc SkBlitMask::PlatformRowProcs(SkColorType dstCT,
                                                 SkMask::Format maskFormat,
                                                 RowFlags flags) {
    return NULL;
}
