/*
 * Copyright 2012 The Android Open Source Project
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "SkBlitMask.h"
#include "SkBlitRow.h"
#include "SkColorPriv.h"
#include "SkDither.h"
#include "SkMathPriv.h"
#include "SkColor_opts_mips_msa.h"
#include "SkBitmapProcState_opts_mips_msa.h"
#include "SkUtils.h"
#include "SkOpts.h"

#include <msa.h>

#define UNROLL


static void S32_D565_Opaque_mips_msa(uint16_t* SK_RESTRICT dst,
                            const SkPMColor* SK_RESTRICT src, int count,
                            U8CPU alpha, int /*x*/, int /*y*/) {
    SkASSERT(255 == alpha);

    const v4u32* s = reinterpret_cast<const v4u32*>(src);
    v8u16* d = reinterpret_cast<v8u16*>(dst);
    const v16u8 u8_mask = {0, 2, 3, 8, 0, 2, 3, 8, 0, 2, 3, 8, 0, 2, 3, 8};
    const v8u16 u16_mask = {2, 0, 2, 0, 2, 0, 2, 0};

    while (count >= 8) {
        // Load 8 pixels of src (argb).
        v4u32 src_pixel1 = __builtin_msa_ld_w((void *)s++, 0);
        v4u32 src_pixel2 = __builtin_msa_ld_w((void *)s++, 0);

        src_pixel1 = __builtin_msa_srl_b(src_pixel1, u8_mask);
        src_pixel2 = __builtin_msa_srl_b(src_pixel2, u8_mask);

        src_pixel1 = __builtin_msa_sll_h(src_pixel1, u16_mask);
        src_pixel2 = __builtin_msa_sll_h(src_pixel2, u16_mask);

        src_pixel1 = __builtin_msa_srli_w(src_pixel1, 5);
        src_pixel2 = __builtin_msa_srli_w(src_pixel2, 5);

        v8u16 d_pixel = __builtin_msa_pckev_h(src_pixel2, src_pixel1);

        // Store 8 16-bit colors in dst.
        __builtin_msa_st_h(d_pixel, (void*)d++, 0);

        count -= 8;
    }

    src = reinterpret_cast<const SkPMColor*>(s);
    dst = reinterpret_cast<uint16_t*>(d);

    if (count > 0) {
        do {
            SkPMColor c = *src++;
            SkPMColorAssert(c);
            *dst++ = SkPixel32ToPixel16_ToU16(c);
        } while (--count != 0);
    }
}

static void S32_D565_Blend_mips_msa(uint16_t* SK_RESTRICT dst,
                                    const SkPMColor* SK_RESTRICT src, int count,
                                    U8CPU alpha, int /*x*/, int /*y*/) {
    SkASSERT(255 > alpha);

    if (count > 0) {
        int scale = SkAlpha255To256(alpha);
        const v8i16 v_scale = __builtin_msa_fill_h(scale);
        const v16i8 v_zero = __builtin_msa_ldi_b(0);

        while (count >= 8) {
            v4i32 v_temp0, v_temp1, v_temp2, v_temp3;
            v4i32 v_temp4, v_temp5, v_temp6, v_temp7;

            v_temp2 = __builtin_msa_ld_b((void*)src, 0);
            v_temp3 = __builtin_msa_ld_b((void*)src, 16);
            v_temp7 = __builtin_msa_ld_b((void*)dst, 0);
            v_temp4 = __builtin_msa_pckev_b(v_temp3, v_temp2);
            v_temp5 = __builtin_msa_pckod_b(v_temp3, v_temp2);
            v_temp0 = __builtin_msa_srli_h(v_temp4, 8);
            v_temp1 = __builtin_msa_ilvev_b(v_zero, v_temp5);
            v_temp2 = __builtin_msa_ilvev_b(v_zero, v_temp4);
            v_temp5 = __builtin_msa_slli_h(v_temp7, 5);
            v_temp6 = __builtin_msa_slli_h(v_temp7, 11);
            v_temp4 = __builtin_msa_srli_h(v_temp7, 11);
            v_temp5 = __builtin_msa_srli_h(v_temp5, 10);
            v_temp6 = __builtin_msa_srli_h(v_temp6, 11);
            v_temp0 = __builtin_msa_srli_h(v_temp0, 3);
            v_temp1 = __builtin_msa_srli_h(v_temp1, 2);
            v_temp2 = __builtin_msa_srli_h(v_temp2, 3);
            v_temp0 = __builtin_msa_subv_h(v_temp0, v_temp4);
            v_temp1 = __builtin_msa_subv_h(v_temp1, v_temp5);
            v_temp2 = __builtin_msa_subv_h(v_temp2, v_temp6);
            v_temp0 = __builtin_msa_mulv_h(v_temp0, v_scale);
            v_temp1 = __builtin_msa_mulv_h(v_temp1, v_scale);
            v_temp2 = __builtin_msa_mulv_h(v_temp2, v_scale);
            v_temp0 = __builtin_msa_srli_h(v_temp0, 8);
            v_temp1 = __builtin_msa_srli_h(v_temp1, 8);
            v_temp2 = __builtin_msa_srli_h(v_temp2, 8);
            v_temp0 = __builtin_msa_addv_h(v_temp0, v_temp4);
            v_temp1 = __builtin_msa_addv_h(v_temp1, v_temp5);
            v_temp2 = __builtin_msa_addv_h(v_temp2, v_temp6);
            v_temp1 = __builtin_msa_ilvev_b(v_zero, v_temp1);
            v_temp2 = __builtin_msa_ilvev_b(v_zero, v_temp2);
            v_temp0 = __builtin_msa_slli_h(v_temp0, 11);
            v_temp1 = __builtin_msa_slli_h(v_temp1, 5);
            v_temp0 = __builtin_msa_or_v(v_temp0, v_temp2);
            v_temp0 = __builtin_msa_or_v(v_temp0, v_temp1);

            __builtin_msa_st_b(v_temp0, (void*)dst, 0);

            src += 8;
            dst += 8;
            count -= 8;
        }

        while (count--) {
            SkPMColor c = *src++;
            SkPMColorAssert(c);
            uint16_t d = *dst;
            *dst++ = SkPackRGB16(
                    SkAlphaBlend(SkPacked32ToR16(c), SkGetPackedR16(d), scale),
                    SkAlphaBlend(SkPacked32ToG16(c), SkGetPackedG16(d), scale),
                    SkAlphaBlend(SkPacked32ToB16(c), SkGetPackedB16(d), scale));

        }
    }
}

static void S32A_D565_Opaque_mips_msa(uint16_t* SK_RESTRICT dst,
                                      const SkPMColor* SK_RESTRICT src, int count,
                                      U8CPU alpha, int /*x*/, int /*y*/) {

    SkASSERT(255 == alpha);

    if (count <= 0) {
        return;
    }

    if (count >= 8) {
        // Make dst 16 bytes alignment
        while (((size_t)dst & 0x0F) != 0) {
            SkPMColor c = *src++;
            if (c) {
              *dst = SkSrcOver32To16(c, *dst);
            }
            dst += 1;
            count--;
        }

        const v4u32* s = reinterpret_cast<const v4u32*>(src);
        v8u16* d = reinterpret_cast<v8u16*>(dst);
        v8u16 var255 = __builtin_msa_ldi_h(255);
        v8u16 r16_mask = __builtin_msa_ldi_h(SK_R16_MASK);
        v8u16 g16_mask = __builtin_msa_ldi_h(SK_G16_MASK);
        v8u16 b16_mask = __builtin_msa_ldi_h(SK_B16_MASK);
        v16u8 zero = __builtin_msa_ldi_h(0);

        while (count >= 8) {
            // Load 8 pixels of src.
            v4u32 src_pixel1 = __builtin_msa_ld_w((void *)s++, 0);
            v4u32 src_pixel2 = __builtin_msa_ld_w((void *)s++, 0);

            // Check whether src pixels are equal to 0.
            // If src pixels are all zero, src_cmp1 and
            // src_cmp2 will be all ones.
            v16u8 src_cmp1 = __builtin_msa_ceq_b(src_pixel1, zero);
            v16u8 src_cmp2 = __builtin_msa_ceq_b(src_pixel2, zero);
            src_cmp1 = __builtin_msa_and_v(src_cmp1, src_cmp2);

            if (__builtin_msa_bnz_b(src_cmp1)) {
                d++;
                count -= 8;
                continue;
            }

            // Load 8 pixels of dst.
            v8u16 dst_pixel = __builtin_msa_ld_h(d, 0);

            // Extract A from src.
            v8u16 sag = __builtin_msa_pckod_b(src_pixel2, src_pixel1);
            v8u16 sa = __builtin_msa_srli_h(sag, 8);

            // Extract R from src.
            v8u16 srb = __builtin_msa_pckev_b(src_pixel2, src_pixel1);
            v8u16 sr = __builtin_msa_srli_h(srb, 8);

            // Extract G from src.
            v8u16 sg = __builtin_msa_ilvev_b(zero, sag);

            // Extract B from src.
            v8u16 sb = __builtin_msa_ilvev_b(zero, srb);

            // Extract R G B from dst.
            v8u16 dr = __builtin_msa_srli_h(dst_pixel, SK_R16_SHIFT);
            dr = __builtin_msa_and_v(dr, r16_mask);
            v8u16 dg = __builtin_msa_srli_h(dst_pixel, SK_G16_SHIFT);
            dg = __builtin_msa_and_v(dg, g16_mask);
            v8u16 db = __builtin_msa_srli_h(dst_pixel, SK_B16_SHIFT);
            db = __builtin_msa_and_v(db, b16_mask);

            v8u16 isa = __builtin_msa_subs_u_h(var255, sa); // 255 -sa
            // Calculate R G B of result.
            // Original algorithm is in SkSrcOver32To16().
            dr = __builtin_msa_addv_h(sr, SkMul16ShiftRound_mips_msa(dr, isa, SK_R16_BITS));
            dr = __builtin_msa_srli_h(dr, 8 - SK_R16_BITS);
            dg = __builtin_msa_addv_h(sg, SkMul16ShiftRound_mips_msa(dg, isa, SK_G16_BITS));
            dg = __builtin_msa_srli_h(dg, 8 - SK_G16_BITS);
            db = __builtin_msa_addv_h(sb, SkMul16ShiftRound_mips_msa(db, isa, SK_B16_BITS));
            db = __builtin_msa_srli_h(db, 8 - SK_B16_BITS);

            // Pack R G B into 16-bit color.
            v8u16 d_pixel = SkPackRGB16_mips_msa(dr, dg, db);

            // Store 8 16-bit colors in dst.
            __builtin_msa_st_h(d_pixel, (void*)d++, 0);
            count -= 8;
        }

        src = reinterpret_cast<const SkPMColor*>(s);
        dst = reinterpret_cast<uint16_t*>(d);
    }

    if (count > 0) {
        do {
            SkPMColor c = *src++;
            SkPMColorAssert(c);
            if (c) {
                *dst = SkSrcOver32To16(c, *dst);
            }
            dst += 1;
        } while (--count != 0);
    }
}

static void S32A_D565_Blend_mips_msa(uint16_t* SK_RESTRICT dst,
                                     const SkPMColor* SK_RESTRICT src, int count,
                                     U8CPU alpha, int /*x*/, int /*y*/) {
    SkASSERT(255 > alpha);

    v8u16 zero = __builtin_msa_ldi_h(0);
    v8u16 mask_alpha = __builtin_msa_ldi_h(256);

    if (count >= 8) {
        do {
            v8u16 v_dc, v_src_scale, v_dst_scale;
            v8u16 v_dst_scale_0, v_dst_scale_1, v_dst_scale_2, v_dst_scale_3;
            v8u16 temp0, temp1, temp2, temp3, temp4, temp5, temp6, temp7;
            v8u16 temp8, temp9, temp10, temp11, temp12, temp13, temp14, temp15, temp16, temp17;
            v4u32 v_sc_0, v_sc_1;
            // [16] d7 d6 d5 d4 d3 d2 d1 d0
            v_dc = __builtin_msa_ld_h((void*)dst, 0);
            // [32] d3 d2 d1 d0
            v_sc_0 = __builtin_msa_ld_w((void*)src, 0);
            v_sc_1 = __builtin_msa_ld_w((void*)src, 16);

            // Convert from 565 to 8888
            // [32] d3 d2 d1 d0
            // xxxxxxxx xxxxxxxx r5r4r3r2r1g6g5g4 g3g2g1b5b4b3b2b1
            temp0 = __builtin_msa_ilvr_h(zero, v_dc);
            // [32] d7 d6 d5 d4
            temp1 = __builtin_msa_ilvl_h(zero, v_dc);
            // xxxxxxxx r5r4r3r2r1g6g5g4 g3g2g1b5b4b3b2b1 xxxxxxxx
            temp2 = __builtin_msa_slli_w(temp0, 8);
            temp3 = __builtin_msa_slli_w(temp1, 8);
            // xxxxxxxx xxxxxr5r4r3 r2r1g6g5g4g3g2g1 b5b4b3b2b1xxx
            temp4 = __builtin_msa_slli_w(temp0, 3);
            temp5 = __builtin_msa_slli_w(temp1, 3);
            // xxxxxxxx r5r4r3r2r1r5r4r3 r2r1g6g5g4g3g2g1 b5b4b3b2b1xxx
            temp2 = __builtin_msa_binsri_w(temp2, temp4, 17);
            temp3 = __builtin_msa_binsri_w(temp3, temp5, 17);
            // xxxxxxxx xxxr5r4r3r2r1 g6g5g4g3g2g1b5b4 b3b2b1xxxxx
            temp4 = __builtin_msa_slli_w(temp0, 5);
            temp5 = __builtin_msa_slli_w(temp1, 5);
            // xxxxxxxx r5r4r3r2r1r5r4r3 g6g5g4g3g2g1b5b4 b3b2b1xxxxx
            temp2 = __builtin_msa_binsri_w(temp2, temp4, 15);
            temp3 = __builtin_msa_binsri_w(temp3, temp5, 15);
            // xxxxxxxx xxxxxxxx xr5r4r3r2r1g6g5 g4g3g2g1b5b4b3b2
            temp4 = __builtin_msa_srli_w(temp0, 1);
            temp5 = __builtin_msa_srli_w(temp1, 1);
            // xxxxxxxx r5r4r3r2r1r5r4r3 g6g5g4g3g2g1g6g5 g4g3g2g1b5b4b3b2
            temp2 = __builtin_msa_binsri_w(temp2, temp4, 9);
            temp3 = __builtin_msa_binsri_w(temp3, temp5, 9);
            // xxxxxxxx xxxxxr5r4r3 r2r1g6g5g4g3g2g1 b5b4b3b2b1xxx
            temp4 = __builtin_msa_slli_w(temp0, 3);
            temp5 = __builtin_msa_slli_w(temp1, 3);
            // xxxxxxxx r5r4r3r2r1r5r4r3 g6g5g4g3g2g1g6g5 b5b4b3b2b1xxx
            temp2 = __builtin_msa_binsri_w(temp2, temp4, 7);
            temp3 = __builtin_msa_binsri_w(temp3, temp5, 7);
            // xxxxxxxx xxxxxxxx xxr5r4r3r2r1g6 g5g4g3g2g1b5b4b3
            temp4 = __builtin_msa_srli_w(temp0, 2);
            temp5 = __builtin_msa_srli_w(temp1, 2);
            // xxxxxxxx r5r4r3r2r1r5r4r3 g6g5g4g3g2g1g6g5 b5b4b3b2b1b5b4b3
            temp2 = __builtin_msa_binsri_w(temp2, temp4, 2);
            temp3 = __builtin_msa_binsri_w(temp3, temp5, 2);

            v_src_scale = __builtin_msa_fill_h(SkAlpha255To256(alpha));
            v_dst_scale = __builtin_msa_pckod_b(v_sc_1, v_sc_0);
            v_dst_scale = __builtin_msa_srli_h(v_dst_scale, 8);
            v_dst_scale = __builtin_msa_mulv_h(v_dst_scale, v_src_scale);
            v_dst_scale = __builtin_msa_srli_h(v_dst_scale, 8);
            v_dst_scale = __builtin_msa_subv_h(mask_alpha, v_dst_scale);

            temp10 = __builtin_msa_splati_h(v_dst_scale, 0);
            temp11 = __builtin_msa_splati_h(v_dst_scale, 1);
            temp12 = __builtin_msa_splati_h(v_dst_scale, 2);
            temp13 = __builtin_msa_splati_h(v_dst_scale, 3);
            temp14 = __builtin_msa_splati_h(v_dst_scale, 4);
            temp15 = __builtin_msa_splati_h(v_dst_scale, 5);
            temp16 = __builtin_msa_splati_h(v_dst_scale, 6);
            temp17 = __builtin_msa_splati_h(v_dst_scale, 7);

            v_dst_scale_0 = __builtin_msa_sldi_b(temp11, temp10, 8);
            v_dst_scale_1 = __builtin_msa_sldi_b(temp13, temp12, 8);
            v_dst_scale_2 = __builtin_msa_sldi_b(temp15, temp14, 8);
            v_dst_scale_3 = __builtin_msa_sldi_b(temp17, temp16, 8);

            // [16] a1 r1 g1 b1 a0 r0 g0 b0
            temp0 = __builtin_msa_ilvr_b(zero, v_sc_0);
            // [16] a3 r3 g3 b3 a2 r2 g2 b2
            temp1 = __builtin_msa_ilvl_b(zero, v_sc_0);

            // [16] a5 r5 g5 b5 a4 r4 g4 b4
            temp4 = __builtin_msa_ilvr_b(zero, v_sc_1);
            // [16] a7 r7 g7 b7 a6 r6 g6 b6
            temp5 = __builtin_msa_ilvl_b(zero, v_sc_1);

            temp6 = __builtin_msa_ilvr_b(zero, temp2);
            temp7 = __builtin_msa_ilvl_b(zero, temp2);
            temp8 = __builtin_msa_ilvr_b(zero, temp3);
            temp9 = __builtin_msa_ilvl_b(zero, temp3);

            temp0 = __builtin_msa_mulv_h(temp0, v_src_scale);
            temp1 = __builtin_msa_mulv_h(temp1, v_src_scale);
            temp4 = __builtin_msa_mulv_h(temp4, v_src_scale);
            temp5 = __builtin_msa_mulv_h(temp5, v_src_scale);

            temp6 = __builtin_msa_mulv_h(temp6, v_dst_scale_0);
            temp7 = __builtin_msa_mulv_h(temp7, v_dst_scale_1);
            temp8 = __builtin_msa_mulv_h(temp8, v_dst_scale_2);
            temp9 = __builtin_msa_mulv_h(temp9, v_dst_scale_3);

            temp0 = __builtin_msa_addv_b(temp0, temp6);
            temp1 = __builtin_msa_addv_b(temp1, temp7);
            temp4 = __builtin_msa_addv_b(temp4, temp8);
            temp5 = __builtin_msa_addv_b(temp5, temp9);

            temp0 = __builtin_msa_pckod_b(temp1, temp0);
            temp4 = __builtin_msa_pckod_b(temp5, temp4);

            // Convert from 8888 to 565
            // [32] d3 d2 d1 d0
            // xxxxxxxx r8r7r6r5r4r3r2r1 g8g7g6g5g4g3g2g1 b8b7b6b5b4b3b2b1
            // 00000000 00000000 00000000 b8b7b6b5b4b3b2b1
            temp10 = __builtin_msa_binsri_w(zero, temp0, 7);
            // 00000000 00000000 00000000 000b8b7b6b5b4
            temp10 = __builtin_msa_srli_w(temp10, 3);
            // 00000xxx xxxxxr8r7r6 r5r4r3r2r1g8g7g6 g5g4g3g2g1b8b7b6
            temp0 = __builtin_msa_srli_w(temp0, 5);
            // 00000xxx xxxxxr8r7r6 r5r4r3r2r1g8g7g6 g5g4g3b8b7b6b5b4
            temp10 = __builtin_msa_binsli_w(temp10, temp0, 26);
            // 00000000 xxxxxxxx r8r7r6r5r4r3r2r1 g8g7g6g5g4g3g2g1
            temp0 = __builtin_msa_srli_w(temp0, 3);
            // [16] 0 d3 0 d2 0 d1 0 d0
            // 00000000 xxxxxxxx r8r7r6r5r4g8g7g6 g5g4g3b8b7b6b5b4
            temp10 = __builtin_msa_binsli_w(temp10, temp0, 20);

            temp11 = __builtin_msa_binsri_w(zero, temp4, 7);
            temp11 = __builtin_msa_srli_w(temp11, 3);
            temp4 = __builtin_msa_srli_w(temp4, 5);
            temp11 = __builtin_msa_binsli_w(temp11, temp4, 26);
            temp4 = __builtin_msa_srli_w(temp4, 3);
            temp11 = __builtin_msa_binsli_w(temp11, temp4, 20);

            temp0 = __builtin_msa_pckev_h(temp11, temp10);

            __builtin_msa_st_h(temp0, (void*)dst, 0);

            dst += 8;
            src += 8;
            count -= 8;
        } while (count >= 8);
    }

    // leftovers
    while (count-- > 0) {
        SkPMColor sc = *src++;
        SkPMColorAssert(sc);
        if (sc) {
            uint16_t dc = *dst;
            SkPMColor res = SkBlendARGB32(sc, SkPixel16ToPixel32(dc), alpha);
            *dst = SkPixel32ToPixel16(res);
        }
        dst += 1;
    }
}

static void S32_D565_Opaque_Dither_mips_msa(uint16_t* SK_RESTRICT dst,
                                            const SkPMColor* SK_RESTRICT src,
                                            int count, U8CPU alpha, int x, int y) {
    SkASSERT(255 == alpha);

    if (count <= 0)
        return;

    if (count >= 8) {
        while (((size_t)dst & 0x0F) != 0) {
            DITHER_565_SCAN(y);
            SkPMColor c = *src++;
            SkPMColorAssert(c);

            unsigned dither = DITHER_VALUE(x);
            *dst++ = SkDitherRGB32To565(c, dither);
            DITHER_INC_X(x);
            count--;
        }
        unsigned short dither_value[8];
        v8u16 dither;
#ifdef ENABLE_DITHER_MATRIX_4X4
        const uint8_t* dither_scan = gDitherMatrix_3Bit_4X4[(y) & 3];
        dither_value[0] = dither_value[4] = dither_scan[(x) & 3];
        dither_value[1] = dither_value[5] = dither_scan[(x + 1) & 3];
        dither_value[2] = dither_value[6] = dither_scan[(x + 2) & 3];
        dither_value[3] = dither_value[7] = dither_scan[(x + 3) & 3];
#else
        const uint16_t dither_scan = gDitherMatrix_3Bit_16[(y) & 3];
        dither_value[0] = dither_value[4] = (dither_scan
                                             >> (((x) & 3) << 2)) & 0xF;
        dither_value[1] = dither_value[5] = (dither_scan
                                             >> (((x + 1) & 3) << 2)) & 0xF;
        dither_value[2] = dither_value[6] = (dither_scan
                                             >> (((x + 2) & 3) << 2)) & 0xF;
        dither_value[3] = dither_value[7] = (dither_scan
                                             >> (((x + 3) & 3) << 2)) & 0xF;
#endif
        dither = __builtin_msa_ld_h((void*) dither_value, 0);

        const v4u32* s = reinterpret_cast<const v4u32*>(src);
        v8u16* d = reinterpret_cast<v8u16*>(dst);
        v16u8 zero = __builtin_msa_ldi_h(0);

        while (count >= 8) {
            // Load 8 pixels of src.
            v4u32 src_pixel1 = __builtin_msa_ld_w((void*)s, 0);
            v4u32 src_pixel2 = __builtin_msa_ld_w((void*)s, 16);
            s += 2;

            // Extract R from src.
            v8u16 srb = __builtin_msa_pckev_b(src_pixel2, src_pixel1);
            v8u16 sr = __builtin_msa_srli_h(srb, 8);

            // SkDITHER_R32To565(sr, dither)
            v8u16 sr_offset = __builtin_msa_srli_h(sr, 5);
            sr = __builtin_msa_addv_h(sr, dither);
            sr = __builtin_msa_subv_h(sr, sr_offset);
            sr = __builtin_msa_srli_h(sr, SK_R32_BITS - SK_R16_BITS);

            // Extract G from src.
            v8u16 sag = __builtin_msa_pckod_b(src_pixel2, src_pixel1);
            v8u16 sg = __builtin_msa_ilvev_b(zero, sag);

            // SkDITHER_G32To565(sg, dither)
            v8u16 sg_offset = __builtin_msa_srli_h(sg, 6);
            sg = __builtin_msa_addv_h(sg, __builtin_msa_srli_h(dither, 1));
            sg = __builtin_msa_subv_h(sg, sg_offset);
            sg = __builtin_msa_srli_h(sg, SK_G32_BITS - SK_G16_BITS);

            // Extract B from src.
            v8u16 sb = __builtin_msa_ilvev_b(zero, srb);

            // SkDITHER_R32To565(sb, dither)
            v8u16 sb_offset = __builtin_msa_srli_h(sb, 5);
            sb = __builtin_msa_addv_h(sb, dither);
            sb = __builtin_msa_subv_h(sb, sb_offset);
            sb = __builtin_msa_srli_h(sb, SK_B32_BITS - SK_B16_BITS);

            // Pack and store 16-bit dst pixel.
            v8u16 d_pixel = SkPackRGB16_mips_msa(sr, sg, sb);
            __builtin_msa_st_h(d_pixel, (void*)d++, 0);

            count -= 8;
            x += 8;
        }

        src = reinterpret_cast<const SkPMColor*>(s);
        dst = reinterpret_cast<uint16_t*>(d);
    }

    if (count > 0) {
        DITHER_565_SCAN(y);
        do {
            SkPMColor c = *src++;
            SkPMColorAssert(c);

            unsigned dither = DITHER_VALUE(x);
            *dst++ = SkDitherRGB32To565(c, dither);
            DITHER_INC_X(x);
        } while (--count != 0);
    }
}

static void S32_D565_Blend_Dither_mips_msa(uint16_t* SK_RESTRICT dst,
                                           const SkPMColor* SK_RESTRICT src,
                                           int count, U8CPU alpha,
                                           int x, int y) {
    SkASSERT(255 > alpha);

    if (count > 0) {
        int scale = SkAlpha255To256(alpha);
        DITHER_565_SCAN(y);
        const v8i16 v_scale = __builtin_msa_fill_h(scale);
        const uint16_t dithers[8] = {(uint16_t)(DITHER_VALUE(x + 0)),
                                     (uint16_t)(DITHER_VALUE(x + 1)),
                                     (uint16_t)(DITHER_VALUE(x + 2)),
                                     (uint16_t)(DITHER_VALUE(x + 3)),
                                     (uint16_t)(DITHER_VALUE(x + 0)),
                                     (uint16_t)(DITHER_VALUE(x + 1)),
                                     (uint16_t)(DITHER_VALUE(x + 2)),
                                     (uint16_t)(DITHER_VALUE(x + 3))};
        const v8i16 v_dither = __builtin_msa_ld_b((void*)dithers, 0);
        const v8i16 v_dither_g = __builtin_msa_srli_h(v_dither, 1);
        const v16i8 v_zero = __builtin_msa_ldi_b(0);

        while (count >= 8) {
            v4i32 v_temp0, v_temp1, v_temp2, v_temp3;
            v4i32 v_temp4, v_temp5, v_temp6, v_temp7;

            v_temp2 = __builtin_msa_ld_b((void*)src, 0);
            v_temp3 = __builtin_msa_ld_b((void*)src, 16);
            v_temp7 = __builtin_msa_ld_b((void*)dst, 0);
            v_temp4 = __builtin_msa_pckev_b(v_temp3, v_temp2);
            v_temp5 = __builtin_msa_pckod_b(v_temp3, v_temp2);
            v_temp0 = __builtin_msa_srli_h(v_temp4, 8);
            v_temp1 = __builtin_msa_ilvev_b(v_zero, v_temp5);
            v_temp2 = __builtin_msa_ilvev_b(v_zero, v_temp4);
            v_temp3 = __builtin_msa_srli_h(v_temp0, 5);
            v_temp4 = __builtin_msa_srli_h(v_temp1, 6);
            v_temp5 = __builtin_msa_srli_h(v_temp2, 5);
            v_temp0 = __builtin_msa_addv_h(v_temp0, v_dither);
            v_temp1 = __builtin_msa_addv_h(v_temp1, v_dither_g);
            v_temp2 = __builtin_msa_addv_h(v_temp2, v_dither);
            v_temp0 = __builtin_msa_subv_h(v_temp0, v_temp3);
            v_temp1 = __builtin_msa_subv_h(v_temp1, v_temp4);
            v_temp2 = __builtin_msa_subv_h(v_temp2, v_temp5);
            v_temp0 = __builtin_msa_srli_h(v_temp0, 3);
            v_temp1 = __builtin_msa_srli_h(v_temp1, 2);
            v_temp2 = __builtin_msa_srli_h(v_temp2, 3);
            v_temp5 = __builtin_msa_slli_h(v_temp7, 5);
            v_temp6 = __builtin_msa_slli_h(v_temp7, 11);
            v_temp4 = __builtin_msa_srli_h(v_temp7, 11);
            v_temp5 = __builtin_msa_srli_h(v_temp5, 10);
            v_temp6 = __builtin_msa_srli_h(v_temp6, 11);
            v_temp0 = __builtin_msa_subv_h(v_temp0, v_temp4);
            v_temp1 = __builtin_msa_subv_h(v_temp1, v_temp5);
            v_temp2 = __builtin_msa_subv_h(v_temp2, v_temp6);
            v_temp0 = __builtin_msa_mulv_h(v_temp0, v_scale);
            v_temp1 = __builtin_msa_mulv_h(v_temp1, v_scale);
            v_temp2 = __builtin_msa_mulv_h(v_temp2, v_scale);
            v_temp0 = __builtin_msa_srli_h(v_temp0, 8);
            v_temp1 = __builtin_msa_srli_h(v_temp1, 8);
            v_temp2 = __builtin_msa_srli_h(v_temp2, 8);
            v_temp0 = __builtin_msa_addv_h(v_temp0, v_temp4);
            v_temp1 = __builtin_msa_addv_h(v_temp1, v_temp5);
            v_temp2 = __builtin_msa_addv_h(v_temp2, v_temp6);
            v_temp1 = __builtin_msa_ilvev_b(v_zero, v_temp1);
            v_temp2 = __builtin_msa_ilvev_b(v_zero, v_temp2);
            v_temp0 = __builtin_msa_slli_h(v_temp0, 11);
            v_temp1 = __builtin_msa_slli_h(v_temp1, 5);
            v_temp0 = __builtin_msa_or_v(v_temp0, v_temp2);
            v_temp0 = __builtin_msa_or_v(v_temp0, v_temp1);

            __builtin_msa_st_b(v_temp0, (void*)dst, 0);

            src += 8;
            dst += 8;
            x += 8;
            count -= 8;
        }
        while (count) {
            SkPMColor c = *src++;
            SkPMColorAssert(c);

            int dither = DITHER_VALUE(x);
            int sr = SkGetPackedR32(c);
            int sg = SkGetPackedG32(c);
            int sb = SkGetPackedB32(c);
            sr = SkDITHER_R32To565(sr, dither);
            sg = SkDITHER_G32To565(sg, dither);
            sb = SkDITHER_B32To565(sb, dither);

            uint16_t d = *dst;
            *dst++ = SkPackRGB16(SkAlphaBlend(sr, SkGetPackedR16(d), scale),
                                 SkAlphaBlend(sg, SkGetPackedG16(d), scale),
                                 SkAlphaBlend(sb, SkGetPackedB16(d), scale));
            DITHER_INC_X(x);
            --count;
        }
    }
}

static void S32A_D565_Opaque_Dither_mips_msa(uint16_t* SK_RESTRICT dst,
                                      const SkPMColor* SK_RESTRICT src,
                                      int count, U8CPU alpha, int x, int y) {
    SkASSERT(255 == alpha);
    if (count >= 8) {
        DITHER_565_SCAN(y);
        int a1 = 0x01000100;
        int a4 = 0x001f001f;
        v8u16 const_4_1 = __builtin_msa_fill_w(a1);
        v8u16 mask_5    = __builtin_msa_fill_w(a4);
        v8u16 zero      = __builtin_msa_ldi_w(0);  //inicijalizacija zero registra
        //const uint16_t dither_scan = gDitherMatrix_3Bit_16[(y) & 3];
        uint16_t dither_value[8];
        dither_value[0] = dither_value[4] = ((dither_scan >> (((x) & 3) << 2)) & 0xF);
        dither_value[1] = dither_value[5] = ((dither_scan >> (((x + 1) & 3) << 2)) & 0xF);
        dither_value[2] = dither_value[6] = ((dither_scan >> (((x + 2) & 3) << 2)) & 0xF);
        dither_value[3] = dither_value[7] = ((dither_scan >> (((x + 3) & 3) << 2)) & 0xF);
        v8u16 dither_value_vec  = __builtin_msa_ld_w((void*)dither_value, 0);
        while(count >= 8) {
            //  a | r | g | b
            v8u16 temp_src_0 = __builtin_msa_ld_w((void*)src, 0);
            v8u16 temp_src_1 = __builtin_msa_ld_w((void*)src, 16);
            v4i32 mask_1     = __builtin_msa_ceq_w(temp_src_0,zero);
            v4i32 mask_2     = __builtin_msa_ceq_w(temp_src_1,zero);
            v4i32 mask_3     = __builtin_msa_pckev_h(mask_2, mask_1);
            //r | b | r | b
            v8u16 temp_0     = __builtin_msa_pckev_b(temp_src_1, temp_src_0);
            //a | g | a | g
            v8u16 temp_1     = __builtin_msa_pckod_b(temp_src_1, temp_src_0);
            //b | b | b | b
            v8u16 temp_2     = __builtin_msa_ilvev_b(zero,temp_0);
            //r | r | r | r
            v8u16 temp_3     = __builtin_msa_ilvod_b(zero,temp_0);
            //g | g | g | g
            v8u16 temp_4     = __builtin_msa_ilvev_b(zero,temp_1);
            //a | a | a | a
            v8u16 temp_5 = __builtin_msa_ilvod_b(zero,temp_1);
            //a + 1 | a + 1 | a + 1 | a + 1
            v8u16 temp_6     = __builtin_msa_addvi_h(temp_5, 1);
            //int d = SkAlphaMul(((dither_scan >> (((x) & 3) << 2)) & 0xF), (a+1));
            temp_src_0       = __builtin_msa_mulv_h(dither_value_vec, temp_6);
            temp_src_0       = __builtin_msa_srli_h(temp_src_0, 8);
            //b>>5 g>>6 r>>5
            temp_src_1       = __builtin_msa_srli_h(temp_2, 5);
            temp_0           = __builtin_msa_srli_h(temp_4, 6);
            temp_1           = __builtin_msa_srli_h(temp_3, 5);

            temp_src_1       = __builtin_msa_subv_h(temp_2, temp_src_1);
            temp_0           = __builtin_msa_subv_h(temp_4, temp_0);
            temp_1           = __builtin_msa_subv_h(temp_3, temp_1);
            temp_src_1       = __builtin_msa_addv_h(temp_src_0, temp_src_1);
            temp_1           = __builtin_msa_addv_h(temp_src_0, temp_1);
            //d>>1
            temp_src_0       = __builtin_msa_srli_h(temp_src_0, 1);
            temp_0           = __builtin_msa_addv_h(temp_src_0, temp_0);
            temp_6           = __builtin_msa_subv_h(const_4_1,temp_5);
            temp_6           = __builtin_msa_srli_h(temp_6,3);
            v8u16 temp_dst   = __builtin_msa_ld_w((void*)dst, 0);
            //(*dst & SK_G16_MASK_IN_PLACE)
            temp_2           = __builtin_msa_and_v(temp_dst, mask_5);
            temp_3           = __builtin_msa_slli_h(temp_dst, 5);
            temp_3           = __builtin_msa_srli_h(temp_3,10);
            temp_4           = __builtin_msa_srli_h(temp_dst, 11);

            temp_2           = __builtin_msa_mulv_h(temp_2, temp_6);
            temp_3           = __builtin_msa_mulv_h(temp_3, temp_6);
            //r dst_expanded = dst_expanded * ((255 - a + 1) >> 3);
            temp_4           = __builtin_msa_mulv_h(temp_4, temp_6);
            // r g b expanded  in g:11 r:10   b:10
            temp_src_1       = __builtin_msa_slli_h(temp_src_1, 2);
            temp_1           = __builtin_msa_slli_h(temp_1, 2);
            temp_0           = __builtin_msa_slli_h(temp_0, 3);

            temp_src_1       = __builtin_msa_addv_h(temp_src_1, temp_2);
            temp_1           = __builtin_msa_addv_h(temp_1, temp_4);
            temp_0           = __builtin_msa_addv_h(temp_0, temp_3);
            temp_src_1       = __builtin_msa_srli_h(temp_src_1, 5);
            temp_1           = __builtin_msa_srli_h(temp_1, 5);
            temp_0           = __builtin_msa_srli_h(temp_0, 5);
            temp_src_1       = __builtin_msa_and_v(temp_src_1, mask_5);
            temp_0           = __builtin_msa_slli_h(temp_0, 10);
            temp_0           = __builtin_msa_srli_h(temp_0, 5);
            temp_1           = __builtin_msa_slli_h(temp_1, 11);
            temp_src_1       = __builtin_msa_or_v(temp_src_1, temp_1);
            temp_src_1       = __builtin_msa_or_v(temp_src_1, temp_0);
            mask_3           = __builtin_msa_bsel_v(mask_3,temp_src_1,temp_dst);
            __builtin_msa_st_b(mask_3,(void*)dst,0);
            dst+=8;
            x+=8;
            src+=8;
            count -= 8;
        }
    }
    if (count > 0) {
        DITHER_565_SCAN(y);
        while(count > 0) {
            SkPMColor c = *src++;
            SkPMColorAssert(c);
            if (c) {
                unsigned a = SkGetPackedA32(c);
                int d = SkAlphaMul(DITHER_VALUE(x), SkAlpha255To256(a));
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
            --count;
        }
    }
}

static void S32_Opaque_BlitRow32_mips_msa(SkPMColor* SK_RESTRICT dst,
                                 const SkPMColor* SK_RESTRICT src,
                                 int count, U8CPU alpha) {
    SkASSERT(255 == alpha);
    memcpy(dst, src, count * 4); // sizeof SkPMColor?
}

static void S32_Blend_BlitRow32_mips_msa(SkPMColor* SK_RESTRICT dst,
                                         const SkPMColor* SK_RESTRICT src,
                                         int count, U8CPU alpha) {
    SkASSERT(alpha <= 255);

    if (count > 0) {
        unsigned src_scale = SkAlpha255To256(alpha);
        unsigned dst_scale = 256 - src_scale;

        if (count >= 8) {
            const v16i8 v_zero = __builtin_msa_ldi_b(0);
            const v8i16 v_src_scale = __builtin_msa_fill_h(src_scale);
            const v8i16 v_dst_scale = __builtin_msa_fill_h(dst_scale);

            while (count >= 4) {
                v8i16 v_temp0, v_temp1, v_temp2, v_temp3, v_temp4, v_temp5;

                v_temp0 = __builtin_msa_ld_b((void*)src, 0);
                v_temp1 = __builtin_msa_ld_b((void*)dst, 0);
                v_temp2 = __builtin_msa_ilvr_b(v_zero, v_temp0);
                v_temp3 = __builtin_msa_ilvl_b(v_zero, v_temp0);
                v_temp5 = __builtin_msa_ilvr_b(v_zero, v_temp1);
                v_temp4 = __builtin_msa_ilvl_b(v_zero, v_temp1);
                v_temp2 = __builtin_msa_mulv_h(v_temp2, v_src_scale);
                v_temp5 = __builtin_msa_mulv_h(v_temp5, v_dst_scale);
                v_temp3 = __builtin_msa_mulv_h(v_temp3, v_src_scale);
                v_temp4 = __builtin_msa_mulv_h(v_temp4, v_dst_scale);
                v_temp2 = __builtin_msa_addv_b(v_temp2, v_temp5);
                v_temp3 = __builtin_msa_addv_b(v_temp3, v_temp4);
                v_temp2 = __builtin_msa_pckod_b(v_temp3, v_temp2);

                __builtin_msa_st_b(v_temp2, (void*)dst, 0);

                src += 4;
                dst += 4;
                count -= 4;
            }
        }

        while (count > 0) {
            *dst = SkAlphaMulQ(*src, src_scale) + SkAlphaMulQ(*dst, dst_scale);
            src++;
            dst++;
            count--;
        }
    }
}

static void S32A_Opaque_BlitRow32_mips_msa(SkPMColor* SK_RESTRICT dst,
                                  const SkPMColor* SK_RESTRICT src,
                                  int count, U8CPU alpha) {
    SkASSERT(255 == alpha);
    v4i32 v0 = __builtin_msa_fill_w(0);
    v4i32 v_256 = __builtin_msa_fill_w(256);
    while (count > 3) {
        v4i32 v1 = __builtin_msa_ld_w((void*)src, 0);
        v4i32 v2 = __builtin_msa_ld_w(dst, 0);
        v4i32 v3 = __builtin_msa_srli_w(v1, 24);
        v3 = __builtin_msa_subv_w(v_256, v3);
        v3 = __builtin_msa_ilvev_h(v3, v3);
        v4i32 v4 = __builtin_msa_ilvod_b(v0, v2);
        v2 = __builtin_msa_ilvev_b(v0, v2);
        v4 = __builtin_msa_mulv_h(v4, v3);
        v2 = __builtin_msa_mulv_h(v2, v3);
        src += 4;
        v2 = __builtin_msa_ilvod_b(v4, v2);
        count -= 4;
        v1 = __builtin_msa_addv_w(v1, v2);
        __builtin_msa_st_w(v1, dst, 0);
        dst += 4;
    }
    while (count > 0) {
        *dst = SkPMSrcOver(*(src++), *dst);
        dst += 1;
        count -= 1;
    }
}

// Test: DontOptimizeSaveLayerDrawDrawRestore
static void S32A_Blend_BlitRow32_mips_msa(SkPMColor* SK_RESTRICT dst,
                                 const SkPMColor* SK_RESTRICT src,
                                 int count, U8CPU alpha) {
    SkASSERT(alpha <= 255);
    if (count > 0) {
        v4i32 mask_256 = __builtin_msa_fill_w(256);
        v4i32 mask_1   = __builtin_msa_fill_w(0x00FF00FF);
        v4i32 alpha_1  = __builtin_msa_fill_w(alpha + 1);
        for(int i = 0; i < (count >> 2); i++)
        {
            v4i32 temp1, temp2, temp3, temp4, temp5, temp6, temp7;
            temp1 = __builtin_msa_ld_w((void*)(src), 0);
            temp2 = __builtin_msa_ld_w((void*)(dst), 0);
            temp3 = __builtin_msa_srli_w(temp1, 24);
            temp4 = __builtin_msa_mulv_w(temp3, alpha_1);
            temp4 = __builtin_msa_srai_w(temp4, 8);
            temp4 = __builtin_msa_subv_w(mask_256, temp4);
            temp6 = __builtin_msa_and_v(temp1, mask_1);
            temp6 = __builtin_msa_mulv_w(temp6, alpha_1);
            temp5 = __builtin_msa_srai_w(temp1, 8);
            temp5 = __builtin_msa_and_v(temp5, mask_1);
            temp5 = __builtin_msa_mulv_w(alpha_1, temp5);
            temp6 = __builtin_msa_ilvod_b(temp5, temp6);
            temp7 = __builtin_msa_and_v(temp2, mask_1);
            temp7 = __builtin_msa_mulv_w(temp7, temp4);
            temp5 = __builtin_msa_srai_w(temp2, 8);
            temp5 = __builtin_msa_and_v(temp5, mask_1);
            temp5 = __builtin_msa_mulv_w(temp4, temp5);
            temp7 = __builtin_msa_ilvod_b(temp5, temp7);
            temp6 = __builtin_msa_addv_w(temp6, temp7);
            __builtin_msa_st_b(temp6, dst, 0);
            src += 4;
            dst += 4;
        }
        for(int i = 0; i < (count & 3); i++)
        {
            *dst = SkBlendARGB32(*src, *dst, alpha);
            src += 1;
            dst += 1;
        }
    }
}

static const SkBlitRow::Proc16 platform_16_procs_mips_msa[] = {
    S32_D565_Opaque_mips_msa,              // S32_D565_Opaque
    S32_D565_Blend_mips_msa,               // S32_D565_Blend
    S32A_D565_Opaque_mips_msa,             // S32A_D565_Opaque
    S32A_D565_Blend_mips_msa,              // S32A_D565_Blend
    S32_D565_Opaque_Dither_mips_msa,       // S32_D565_Opaque_Dither
    S32_D565_Blend_Dither_mips_msa,        // S32_D565_Blend_Dither
    S32A_D565_Opaque_Dither_mips_msa,      // S32A_D565_Opaque_Dither
    nullptr,                               // S32A_D565_Blend_Dither
};

static const SkBlitRow::Proc32 platform_32_procs_mips_msa[] = {
    S32_Opaque_BlitRow32_mips_msa,
    S32_Blend_BlitRow32_mips_msa,
    S32A_Opaque_BlitRow32_mips_msa,
    S32A_Blend_BlitRow32_mips_msa
};


SkBlitRow::Proc16 SkBlitRow::PlatformFactory565(unsigned flags) {
    return platform_16_procs_mips_msa[flags];
}

SkBlitRow::ColorProc16 SkBlitRow::PlatformColorFactory565(unsigned flags) {
    return nullptr;
}

SkBlitRow::Proc32 SkBlitRow::PlatformProcs32(unsigned flags) {
    return platform_32_procs_mips_msa[flags];
}
