/*
 * Copyright 2015 The Android Open Source Project
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "SkBitmapFilter_opts_mips_msa.h"
#include "SkBitmapProcState_opts_mips_msa.h"
#include "SkBitmapProcState_utils.h"
#include "SkBitmapScaler.h"
#include "SkColorPriv.h"
#include "SkPaint.h"
#include "SkTypes.h"
#include "SkUtils.h"
#include "SkPerspIter.h"

#include "SkConvolver.h"

#include "SkMath.h"
#include "SkMathPriv.h"

#include <msa.h>

int array_const[4] = {0, 1, 2, 3};

void SkBitmapProcState::platformProcs()
{
    /* Check fMatrixProc */
    if (fMatrixProc == ClampX_ClampY_filter_scale) {
        fMatrixProc = ClampX_ClampY_filter_scale_mips_msa;
    } else if (fMatrixProc == ClampX_ClampY_nofilter_scale) {
        fMatrixProc = ClampX_ClampY_nofilter_scale_mips_msa;
    } else if (fMatrixProc == ClampX_ClampY_filter_affine) {
        fMatrixProc = ClampX_ClampY_filter_affine_mips_msa;
    } else if (fMatrixProc == ClampX_ClampY_nofilter_affine) {
        fMatrixProc = ClampX_ClampY_nofilter_affine_mips_msa;
    }
}

void SkBitmapScaler::PlatformConvolutionProcs(SkConvolutionProcs* procs) {
    procs->fExtraHorizontalReads = 3;
    procs->fConvolveHorizontally = &convolveHorizontally_mips_msa;
    procs->fConvolve4RowsHorizontally = &convolve4RowsHorizontally_mips_msa;
    procs->fConvolveVertically = &ConvolveVertically_mips_msa;
}

static inline uint32_t ClampX_ClampY_pack_filter(SkFixed f, unsigned max,
                                                 SkFixed one) {
    unsigned i = SkClampMax(f >> 16, max);
    i = (i << 4) | ((f >> 12) & 0xF);
    return (i << 14) | SkClampMax((f + one) >> 16, max);
}

static inline uint32_t RepeatX_RepeatY_pack_filter(SkFixed f, unsigned max,
                                                   SkFixed one) {
    unsigned i = SK_USHIFT16((f & 0xFFFF) * (max + 1));
    i = (i << 4) | (((f & 0xFFFF) * (max + 1) >> 12) & 0xF);
    return (i << 14) | SK_USHIFT16(((f + one) & 0xFFFF) * (max + 1));
}

void ClampX_ClampY_nofilter_affine_mips_msa(const SkBitmapProcState& s,
                                            uint32_t xy[], int count, int x, int y)
{
    SkASSERT(s.fInvType & SkMatrix::kAffine_Mask);
    SkASSERT((s.fInvType & ~(SkMatrix::kTranslate_Mask |
                             SkMatrix::kScale_Mask |
                             SkMatrix::kAffine_Mask)) == 0);

    const SkBitmapProcStateAutoMapper mapper(s, x, y);

    SkFixed fx = mapper.fixedX();
    SkFixed fy = mapper.fixedY();
    SkFixed dx = s.fInvSx;
    SkFixed dy = s.fInvKy;
    int maxX = s.fPixmap.width() - 1;
    int maxY = s.fPixmap.height() - 1;

    int maxXY = (maxY << 16) | maxX;

    SkFractionalInt dx4 = SkFractionalIntToFixed(dx) * 4;
    SkFractionalInt dy4 = SkFractionalIntToFixed(dy) * 4;

    v4i32 zero = __builtin_msa_ldi_w(0);
    v4i32 wide_i, temp0, temp1, wide_fy, wide_fx;
    v4i32 wide_maxXY, wide_dx4, wide_dy4;

    v4i32 c_array = __builtin_msa_ld_w(array_const, 0);

    temp0 = __builtin_msa_fill_w(SkFractionalIntToFixed(fx));
    temp1 = __builtin_msa_fill_w(SkFractionalIntToFixed(dx));
    temp1 = __builtin_msa_mulv_w(temp1, c_array);
    wide_fx = __builtin_msa_addv_w(temp0, temp1);

    temp0 = __builtin_msa_fill_w(SkFractionalIntToFixed(fy));
    temp1 = __builtin_msa_fill_w(SkFractionalIntToFixed(dy));
    temp1 = __builtin_msa_mulv_w(temp1, c_array);
    wide_fy = __builtin_msa_addv_w(temp0, temp1);

    wide_dx4  = __builtin_msa_fill_w(dx4);
    wide_dy4  = __builtin_msa_fill_w(dy4);

    wide_maxXY = __builtin_msa_fill_w(maxXY);

    for (int i = (count >> 2); i > 0; --i) {
        wide_i = __builtin_msa_ilvod_h(wide_fy, wide_fx);
        wide_i =  __builtin_msa_max_s_h(wide_i, zero);
        wide_i =  __builtin_msa_min_s_h(wide_i, wide_maxXY);

        __builtin_msa_st_w(wide_i, (void*)xy, 0);

        wide_fx = __builtin_msa_addv_w(wide_fx, wide_dx4);
        wide_fy = __builtin_msa_addv_w(wide_fy, wide_dy4);
        fx += dx4;
        fy += dy4;
        xy += 4;
    }
    for (int i = (count & 3); i > 0; --i) {
        *xy = (SkClampMax(SkFractionalIntToFixed(fy) >> 16, maxY) << 16) |
               SkClampMax(SkFractionalIntToFixed(fx) >> 16, maxX);
        fx += dx; fy += dy;
        xy++;
    }
}

void ClampX_ClampY_nofilter_scale_mips_msa(const SkBitmapProcState& s,
                                           uint32_t xy[], int count, int x, int y)
{
    SkASSERT((s.fInvType & ~(SkMatrix::kTranslate_Mask |
                             SkMatrix::kScale_Mask)) == 0);

    // we store y, x, x, x, x, x

    const unsigned maxX = s.fPixmap.width() - 1;
    const SkBitmapProcStateAutoMapper mapper(s, x, y);
    const unsigned maxY = s.fPixmap.height() - 1;
    *xy++ = SkClampMax(mapper.intY(), maxY);
    SkFixed fx = mapper.fixedX();

    if (0 == maxX) {
        // all of the following X values must be 0
        memset(xy, 0, count * sizeof(uint16_t));
        return;
    }

    const SkFixed dx = s.fInvSx;

    // test if we don't need to apply the tile proc
    if ((unsigned)(fx >> 16) <= maxX &&
        (unsigned)((fx + dx * (count - 1)) >> 16) <= maxX) {
        if (count >= 8) {
            v4i32 wide_dx4 = __builtin_msa_fill_w(dx * 4);
            v4i32 wide_low = {(int32_t)fx,
                              (int32_t)fx + (int32_t)dx,
                              (int32_t)fx + 2 * (int32_t)dx,
                              (int32_t)fx + 3 * (int32_t)dx
                             };
            v4i32 wide_dx8 = __builtin_msa_addv_w(wide_dx4, wide_dx4);
            v4i32 wide_high = __builtin_msa_addv_w(wide_low, wide_dx4);

            while (count >= 8) {
                v8i16 wide_result = __builtin_msa_pckod_h(wide_high, wide_low);
                __builtin_msa_st_h(wide_result, (void*)xy, 0);

                wide_low = __builtin_msa_addv_w(wide_low, wide_dx8);
                wide_high = __builtin_msa_addv_w(wide_high, wide_dx8);

                xy += 4;
                fx += 8 * dx;
                count -=8;
            }
        } // if count >= 8
        uint16_t* xx = (uint16_t*)xy;
        while (count > 0) {
            *xx++ = (uint16_t)(fx >> 16); fx += dx;
            count -=1;
        }
    } else {
        if ((count >= 8) && (maxX <= 0xFFFF)) {
            v8i16 zero_h = __builtin_msa_ldi_h(0);
            v8i16 max_h = __builtin_msa_fill_h(maxX);
            v4i32 wide_dx4 = __builtin_msa_fill_w(dx * 4);
            v4i32 wide_low = {(int32_t)fx,
                              (int32_t)fx + (int32_t)dx,
                              (int32_t)fx + 2 * (int32_t)dx,
                              (int32_t)fx + 3 * (int32_t)dx
                             };
            v4i32 wide_dx8 = __builtin_msa_addv_w(wide_dx4, wide_dx4);
            v4i32 wide_high = __builtin_msa_addv_w(wide_low, wide_dx4);

            while (count >= 8) {
                v8i16 wide_result = __builtin_msa_pckod_h(wide_high, wide_low);

                wide_result = __builtin_msa_max_s_h(wide_result, zero_h);
                wide_result = __builtin_msa_min_u_h(wide_result, max_h);

                __builtin_msa_st_h(wide_result, (void*)xy, 0);
                wide_low = __builtin_msa_addv_w(wide_low, wide_dx8);
                wide_high = __builtin_msa_addv_w(wide_high, wide_dx8);
                xy += 4;
                fx += 8 * dx;
                count -=8;
            }
        } // if count >= 8
        uint16_t* xx = (uint16_t*)xy;
        while (count-- > 0) {
            *xx++ = SkClampMax((fx >> 16), maxX); fx += dx;
        }
    }
}

void ClampX_ClampY_filter_scale_mips_msa(const SkBitmapProcState& s, uint32_t xy[],
                                         int count, int x, int y)
{
    SkASSERT((s.fInvType & ~(SkMatrix::kTranslate_Mask |
                             SkMatrix::kScale_Mask)) == 0);
    SkASSERT(s.fInvKy == 0);

    const unsigned maxX = s.fPixmap.width() - 1;
    const SkFixed one = s.fFilterOneX;
    const SkFixed dx = s.fInvSx;

    const SkBitmapProcStateAutoMapper mapper(s, x, y);
    const SkFixed fy = mapper.fixedY();
    const unsigned maxY = s.fPixmap.height() - 1;
    // compute our two Y values up front
    *xy++ = ClampX_ClampY_pack_filter(fy, maxY, s.fFilterOneY);
    // now initialize fx
    SkFixed fx = mapper.fixedX();

    if (dx > 0 && (unsigned)(fx >> 16) <= maxX &&
        (unsigned)((fx + dx * (count - 1)) >> 16) < maxX) {
        // SkFractionalInt fx = SkFractionalInt (fx >>16);
        if (count >= 4) {
            // SkFractionalInt dx = SkFractionalInt (dx >>16);
            // MSA version of decal_filter_scale
            while ((size_t(xy) & 0x0F) != 0) {
                SkASSERT((fx >> (16 + 14)) == 0);
                *xy++ = (fx >> 12 << 14) | ((fx >> 16) + 1);
                fx += dx;
                count--;
            }

            v4i32 wide_dx4  = __builtin_msa_fill_w(dx * 4);
            v4i32 wide_fx4 = {(int32_t)fx, (int32_t)fx + (int32_t)dx,
                              (int32_t)fx + 2*(int32_t)dx,
                              (int32_t)fx + 3*(int32_t)dx};

            while (count >= 4) {
                v4i32 wide_xy;

                wide_xy = __builtin_msa_slli_w(
                                __builtin_msa_srai_w(wide_fx4, 12), 14);
                wide_xy = __builtin_msa_or_v(wide_xy, __builtin_msa_addvi_w(
                                __builtin_msa_srai_w(wide_fx4, 16), 1));

                __builtin_msa_st_w(wide_xy, (void*)(xy), 0);

                xy += 4;
                fx += dx * 4;
                wide_fx4  = __builtin_msa_addv_w(wide_fx4, wide_dx4);
                count -= 4;
            } // while count >= 4
        } // if count >= 4

        while (count-- > 0) {
            SkASSERT((fx >> (16 + 14)) == 0);
            *xy++ = (fx >> 12 << 14) | ((fx >> 16) + 1);
            fx += dx;
        }
    } else {
        if(maxX <= 0xFFFF) {

            v4i32 wide_one = __builtin_msa_fill_w(one);
            v4i32 zero_w = __builtin_msa_ldi_w(0);
            v4i32 max_w = __builtin_msa_fill_w(maxX);
            v4i32 wide_dx4  = __builtin_msa_fill_w(dx * 4);
            v4i32 wide_fx4 = {(int32_t)fx, (int32_t)fx + (int32_t)dx,
                              (int32_t)fx + 2*(int32_t)dx,
                              (int32_t)fx + 3*(int32_t)dx};

            while(count >= 4) {
                v4i32 wide_fx_one;
                // ((fx >> 12)) << 14
                v4i32 wide_xy = __builtin_msa_slli_w(__builtin_msa_srai_w(wide_fx4, 12), 14);
                // i = SkClampMax(f>>16,maxX) << 18
                v4u32 wide_i = __builtin_msa_srai_w(wide_fx4, 16);
                wide_i = __builtin_msa_max_s_w(wide_i, zero_w);
                wide_i = __builtin_msa_min_u_w(wide_i, max_w);
                wide_i = __builtin_msa_slli_w(wide_i, 18);

                // SkClampMax((fx + one) >> 16, maxX)
                wide_fx_one = __builtin_msa_addv_w(wide_fx4, wide_one);
                wide_fx_one = __builtin_msa_srai_w(wide_fx_one, 16);
                wide_fx_one = __builtin_msa_max_s_w(wide_fx_one, zero_w);
                wide_fx_one = __builtin_msa_min_u_w(wide_fx_one, max_w);

                // *xy
                wide_xy = __builtin_msa_or_v(wide_xy, wide_fx_one);
                wide_xy = __builtin_msa_binsli_w(wide_xy, wide_i, 13);

                __builtin_msa_st_w(wide_xy, (void*)(xy), 0);

                wide_fx4  = __builtin_msa_addv_w(wide_fx4, wide_dx4);

                fx += dx * 4;
                xy += 4;
                count -= 4;
            }
        }
        while (count-- > 0) {
            unsigned i = SkClampMax(fx >> 16, maxX);
            i = (i << 4) | ((fx >> 12) & 0xF);
            *xy++ = (i << 14) | SkClampMax((fx + one) >> 16, maxX);
            fx += dx;
        }
    }
}

void ClampX_ClampY_filter_affine_mips_msa(const SkBitmapProcState& s,
                                          uint32_t xy[], int count, int x, int y)
{
    const SkBitmapProcStateAutoMapper mapper(s, x, y);

    SkFixed oneX = s.fFilterOneX;
    SkFixed oneY = s.fFilterOneY;
    SkFixed fx = mapper.fixedX();
    SkFixed fy = mapper.fixedY();
    SkFixed dx = s.fInvSx;
    SkFixed dy = s.fInvKy;
    unsigned maxX = s.fPixmap.width() - 1;
    unsigned maxY = s.fPixmap.height() - 1;
    unsigned i;

    v4u32 temp0, temp1;
    v4u32 wide_maxX, wide_maxY, wide_d4X, wide_d4Y, wide_oneX, wide_oneY;
    v4u32 wide_fx, wide_fy, wide_ix, wide_iy, wide_f1x, wide_f1y;

    v4u32 c_array = __builtin_msa_ld_w(array_const, 0);
    v4u32 zero    = __builtin_msa_ldi_w(0);

    SkFixed dx4 = dx * 4;
    SkFixed dy4 = dy * 4;

    wide_maxX = __builtin_msa_fill_w(maxX);
    wide_maxY = __builtin_msa_fill_w(maxY);
    wide_d4X = __builtin_msa_fill_w(dx4);
    wide_d4Y = __builtin_msa_fill_w(dy4);
    wide_oneX = __builtin_msa_fill_w(oneX);
    wide_oneY = __builtin_msa_fill_w(oneY);

    temp0 = __builtin_msa_fill_w(fx);
    temp1 = __builtin_msa_fill_w(dx);
    temp1 = __builtin_msa_mulv_w(temp1, c_array);
    wide_fx = __builtin_msa_addv_w(temp0, temp1);

    temp0 = __builtin_msa_fill_w(fy);
    temp1 = __builtin_msa_fill_w(dy);
    temp1 = __builtin_msa_mulv_w(temp1, c_array);
    wide_fy = __builtin_msa_addv_w(temp0, temp1);

    for (i = (count >> 2); i > 0; --i) {
        temp0 = __builtin_msa_srai_w(wide_fy, 16);
        temp1 = __builtin_msa_srai_w(wide_fx, 16);
        wide_iy = __builtin_msa_max_s_w(temp0, zero);
        wide_ix = __builtin_msa_max_s_w(temp1, zero);
        wide_iy = __builtin_msa_min_s_w(wide_iy, wide_maxY);
        wide_ix = __builtin_msa_min_s_w(wide_ix, wide_maxX);

        temp0 = __builtin_msa_srai_w(wide_fy, 12);
        temp1 = __builtin_msa_srai_w(wide_fx, 12);
        temp0 = __builtin_msa_binsri_w(zero, temp0, 3);
        temp1 = __builtin_msa_binsri_w(zero, temp1, 3);
        wide_iy = __builtin_msa_slli_w(wide_iy, 4);
        wide_ix = __builtin_msa_slli_w(wide_ix, 4);
        wide_iy = __builtin_msa_or_v(wide_iy, temp0);
        wide_ix = __builtin_msa_or_v(wide_ix, temp1);

        wide_iy = __builtin_msa_slli_w(wide_iy, 14);
        wide_ix = __builtin_msa_slli_w(wide_ix, 14);

        wide_f1y = __builtin_msa_addv_w(wide_fy, wide_oneY);
        wide_f1x = __builtin_msa_addv_w(wide_fx, wide_oneX);

        temp0 = __builtin_msa_srai_w(wide_f1y, 16);
        temp1 = __builtin_msa_srai_w(wide_f1x, 16);
        wide_f1y = __builtin_msa_max_s_w(temp0, zero);
        wide_f1x = __builtin_msa_max_s_w(temp1, zero);
        wide_f1y = __builtin_msa_min_s_w(wide_f1y, wide_maxY);
        wide_f1x = __builtin_msa_min_s_w(wide_f1x, wide_maxX);

        wide_iy = __builtin_msa_or_v(wide_iy, wide_f1y);
        wide_ix = __builtin_msa_or_v(wide_ix, wide_f1x);

        temp0 = __builtin_msa_ilvr_w(wide_ix, wide_iy);
        temp1 = __builtin_msa_ilvl_w(wide_ix, wide_iy);

        __builtin_msa_st_w(temp0, (void*)xy, 0);
        __builtin_msa_st_w(temp1, (void*)xy, 16);

        wide_fy = __builtin_msa_addv_w(wide_fy, wide_d4Y);
        wide_fx = __builtin_msa_addv_w(wide_fx, wide_d4X);

        fx += dx4;
        fy += dy4;
        xy += 8;
    }
    for (i = (count & 3); i > 0; --i) {
        *xy++ = ClampX_ClampY_pack_filter(fy, maxY, oneY);
        fy += dy;
        *xy++ = ClampX_ClampY_pack_filter(fx, maxX, oneX);
        fx += dx;
    }
}
