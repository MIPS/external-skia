/*
 * Copyright 2015 The Android Open Source Project
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef SkColor_opts_mips_msa_DEFINED
#define SkColor_opts_mips_msa_DEFINED

#include <msa.h>

static inline v8i16 SkMul16ShiftRound_mips_msa(const v8i16& a,
                                               const v8u16& b, int shift) {
    /* dotp sees a and b as v16u8 */
    v8i16 prod = __builtin_msa_dotp_u_h(a, b);
    v8i16 vectorShift = __builtin_msa_fill_h(shift);
    prod = __builtin_msa_addv_h(prod, __builtin_msa_fill_h(1 << (shift - 1)));
    prod = __builtin_msa_addv_h(prod, __builtin_msa_srl_h(prod, vectorShift));
    prod = __builtin_msa_srl_h(prod, vectorShift);

    return prod;
}

static inline v8i16 SkPackRGB16_mips_msa(const v8i16& r,
                                         const v8i16& g, const v8i16& b) {

    v8u16 dr = __builtin_msa_slli_h(r, SK_R16_SHIFT);
    v8u16 dg = __builtin_msa_slli_h(g, SK_G16_SHIFT);
    v8u16 c = __builtin_msa_or_v(dr, dg);

    return __builtin_msa_or_v(c, b);
}

static inline v4i32 SkPixel16ToPixel32_mips_msa(const v4i32& src) {

    // Convert from 565 to 8888
    v4i32 temp0, temp1;
    v4i32 v_ff = __builtin_msa_ldi_w(0xFF);
    v_ff = __builtin_msa_slli_w(v_ff, 24);

    // xxxxxxxx xxxxxxxx r5r4r3r2r1g6g5g4 g3g2g1b5b4b3b2b1
    // xxxxxxxx r5r4r3r2r1g6g5g4 g3g2g1b5b4b3b2b1 xxxxxxxx
    temp0 = __builtin_msa_slli_w(src, 8);
    // xxxxxxxx xxxxxr5r4r3 r2r1g6g5g4g3g2g1 b5b4b3b2b1xxx
    temp1 = __builtin_msa_slli_w(src, 3);
    // xxxxxxxx r5r4r3r2r1r5r4r3 r2r1g6g5g4g3g2g1 b5b4b3b2b1xxx
    temp0 = __builtin_msa_binsri_w(temp0, temp1, 18);
    // xxxxxxxx xxxr5r4r3r2r1 g6g5g4g3g2g1b5b4 b3b2b1xxxxx
    temp1 = __builtin_msa_slli_w(src, 5);
    // xxxxxxxx r5r4r3r2r1r5r4r3 g6g5g4g3g2g1b5b4 b3b2b1xxxxx
    temp0 = __builtin_msa_binsri_w(temp0, temp1, 15);
    // xxxxxxxx xxxxxxxx xr5r4r3r2r1g6g5 g4g3g2g1b5b4b3b2
    temp1 = __builtin_msa_srli_w(src, 1);
    // xxxxxxxx r5r4r3r2r1r5r4r3 g6g5g4g3g2g1g6g5 g4g3g2g1b5b4b3b2
    temp0 = __builtin_msa_binsri_w(temp0, temp1, 9);
    // xxxxxxxx xxxxxr5r4r3 r2r1g6g5g4g3g2g1 b5b4b3b2b1xxx
    temp1 = __builtin_msa_slli_w(src, 3);
    // xxxxxxxx r5r4r3r2r1r5r4r3 g6g5g4g3g2g1g6g5 b5b4b3b2b1xxx
    temp0 = __builtin_msa_binsri_w(temp0, temp1, 7);
    // xxxxxxxx xxxxxxxx xxr5r4r3r2r1g6 g5g4g3g2g1b5b4b3
    temp1 = __builtin_msa_srli_w(src, 2);
    // xxxxxxxx r5r4r3r2r1r5r4r3 g6g5g4g3g2g1g6g5 b5b4b3b2b1b5b4b3
    temp0 = __builtin_msa_binsri_w(temp0, temp1, 2);

    // ffffffff r5r4r3r2r1r5r4r3 g6g5g4g3g2g1g6g5 b5b4b3b2b1b5b4b3
    return __builtin_msa_or_v(temp0, v_ff);
}

static inline v4i32 SkPixel32ToPixel16_ToU16_mips_msa(const v4i32& src_pixel1,
                                                      const v4i32& src_pixel2) {
    // Convert from 8888 to 565
    v4i32 zero = __builtin_msa_ldi_w(0);
    v4i32 temp0, temp1, temp2, temp3;
    // xxxxxxxx r8r7r6r5r4r3r2r1 g8g7g6g5g4g3g2g1 b8b7b6b5b4b3b2b1
    // 00000000 00000000 00000000 b8b7b6b5b4b3b2b1
    temp1 = __builtin_msa_binsri_w(zero, src_pixel1, 7);
    temp2 = __builtin_msa_binsri_w(zero, src_pixel2, 7);
    // 00000000 00000000 00000000 000b8b7b6b5b4
    temp1 = __builtin_msa_srli_w(temp1, 3);
    temp2 = __builtin_msa_srli_w(temp2, 3);
    // 00000xxx xxxxxr8r7r6 r5r4r3r2r1g8g7g6 g5g4g3g2g1b8b7b6
    temp0 = __builtin_msa_srli_w(src_pixel1, 5);
    temp3 = __builtin_msa_srli_w(src_pixel2, 5);
    // 00000xxx xxxxxr8r7r6 r5r4r3r2r1g8g7g6 g5g4g3b8b7b6b5b4
    temp1 = __builtin_msa_binsli_w(temp1, temp0, 26);
    temp2 = __builtin_msa_binsli_w(temp2, temp3, 26);
    // 00000000 xxxxxxxx r8r7r6r5r4r3r2r1 g8g7g6g5g4g3g2g1
    temp0 = __builtin_msa_srli_w(temp0, 3);
    temp3 = __builtin_msa_srli_w(temp3, 3);
    // [16] 0 d3 0 d2 0 d1 0 d0
    // 00000000 xxxxxxxx r8r7r6r5r4g8g7g6 g5g4g3b8b7b6b5b4
    temp1 = __builtin_msa_binsli_w(temp1, temp0, 20);
    temp2 = __builtin_msa_binsli_w(temp2, temp3, 20);

    return __builtin_msa_pckev_h(temp2, temp1);
}

#endif // SkColor_opts_mips_msa_DEFINED
