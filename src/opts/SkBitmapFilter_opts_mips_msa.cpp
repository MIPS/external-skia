/*
 * Copyright 2013 Google Inc.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "SkBitmap.h"
#include "SkBitmapFilter_opts_mips_msa.h"
#include "SkBitmapProcState.h"
#include "SkColor.h"
#include "SkColorPriv.h"
#include "SkConvolver.h"
#include "SkShader.h"
#include "SkUnPreMultiply.h"

#include <msa.h>

short mask_array[24] = {-1, 0, 0, 0, 0, 0, 0, 0,
                        -1, -1, 0, 0, 0, 0, 0, 0,
                        -1, -1, -1, 0, 0, 0, 0, 0};

inline unsigned char ClampTo8_mips_msa(int a) {
    if (static_cast<unsigned>(a) < 256) {
        return a;  // Avoid the extra check in the common case.
    }
    if (a < 0) {
        return 0;
    }
    return 255;
}

// Convolves horizontally along a single row. The row data is given in
// |src_data| and continues for the num_values() of the filter.
void convolveHorizontally_mips_msa(const unsigned char* src_data,
                                   const SkConvolutionFilter1D& filter,
                                   unsigned char* out_row,
                                   bool /*has_alpha*/) {
    int num_values = filter.numValues();
    int filter_offset, filter_length;

    v4i32 zero = __builtin_msa_ldi_w(0);
    v4i32 mask_255 = __builtin_msa_ldi_w(255);

    v4i32 mask[4];
    // |mask| will be used to decimate all extra filter coefficients that are
    // loaded by SIMD when |filter_length| is not divisible by 4.
    // mask[0] is not used in following algorithm.
    mask[1] = __builtin_msa_ld_h(mask_array, 0);
    mask[2] = __builtin_msa_ld_h(mask_array, 16);
    mask[3] = __builtin_msa_ld_h(mask_array, 32);

    // Output one pixel each iteration, calculating all channels (RGBA) together.
    for (int out_x = 0; out_x < num_values; out_x++) {
        const SkConvolutionFilter1D::ConvolutionFixed* filter_values =
            filter.FilterForValue(out_x, &filter_offset, &filter_length);

        v4i32 accum = __builtin_msa_ldi_w(0);
        // Compute the first pixel in this row that the filter affects. It will
        // touch |filter_length| pixels (4 bytes each) after this.

        const v4i32* row_to_filter =
            reinterpret_cast<const v4i32*>(&src_data[filter_offset << 2]);
        // We will load and accumulate with four coefficients per iteration.
        for (int filter_x = 0; filter_x < filter_length >> 2; filter_x++) {

            // Load 4 coefficients => duplicate 1st and 2nd of them for all channels.
            v4i32 coeff, sign, coeff_0, coeff_1, coeff_2, coeff_3;
            // [16] xx xx xx xx c3 c2 c1 c0
            coeff = __builtin_msa_ld_h((void*)(filter_values), 0);

            sign = __builtin_msa_srai_h(coeff, 15);
            // [32] c3 c2 c1 c0
            coeff = __builtin_msa_ilvr_h(sign, coeff);
            // [32] c0 c0 c0 c0
            coeff_0 = __builtin_msa_splati_w(coeff, 0);
            // [32] c1 c1 c1 c1
            coeff_1 = __builtin_msa_splati_w(coeff, 1);
            // [32] c2 c2 c2 c2
            coeff_2 = __builtin_msa_splati_w(coeff, 2);
            // [32] c3 c3 c3 c3
            coeff_3 = __builtin_msa_splati_w(coeff, 3);

            // Load four pixels => unpack the first two pixels to 16 bits =>
            // multiply with coefficients => accumulate the convolution result.
            // [8] a3 b3 g3 r3 a2 b2 g2 r2 a1 b1 g1 r1 a0 b0 g0 r0
            v4i32 src8 = __builtin_msa_ld_b((void*)row_to_filter, 0);
            // [16] a1 b1 g1 r1 a0 b0 g0 r0
            v4i32 src16_0 = __builtin_msa_ilvr_b(zero, src8);
            // [32] a0 b0 g0 r0
            v4i32 src_0 = __builtin_msa_ilvr_h(zero, src16_0);
            // [32] a1 b1 g1 r1
            v4i32 src_1 = __builtin_msa_ilvl_h(zero, src16_0);
            // [16] a3 b3 g3 r3 a2 b2 g2 r2
            v4i32 src16_1 = __builtin_msa_ilvl_b(zero, src8);
            // [32] a2 b2 g2 r2
            v4i32 src_2 = __builtin_msa_ilvr_h(zero, src16_1);
            // [32] a3 b3 g3 r3
            v4i32 src_3 = __builtin_msa_ilvl_h(zero, src16_1);

            // [32] accum += a0*c0 b0*c0 g0*c0 r0*c0
            accum = __builtin_msa_maddv_w(accum, src_0, coeff_0);
            // [32] accum += a1*c1 b1*c1 g1*c1 r1*c1
            accum = __builtin_msa_maddv_w(accum, src_1, coeff_1);
            // [32] accum += a2*c2 b2*c2 g2*c2 r2*c2
            accum = __builtin_msa_maddv_w(accum, src_2, coeff_2);
            // [32] accum += a3*c3 b3*c3 g3*c3 r3*c3
            accum = __builtin_msa_maddv_w(accum, src_3, coeff_3);

            // Advance the pixel and coefficients pointers.
            row_to_filter += 1;
            filter_values += 4;
        }

        // When |filter_length| is not divisible by 4, we need to decimate some of
        // the filter coefficient that was loaded incorrectly to zero; Other than
        // that the algorithm is same with above, exceot that the 4th pixel will be
        // always absent.
        int r = filter_length&3;
        if (r) {
            // Note: filter_values must be padded to align_up(filter_offset, 8).
            v8i16 coeff, sign, coeff_0, coeff_1, coeff_2;
            // [16] xx xx xx xx c3 c2 c1 c0
            coeff = __builtin_msa_ld_h((void*)filter_values, 0);
            // Mask out extra filter taps.
            coeff = __builtin_msa_and_v(coeff, mask[r]);
            sign = __builtin_msa_srai_h(coeff, 15);
            // [32] c3 c2 c1 c0
            coeff = __builtin_msa_ilvr_h(sign, coeff);
            // [32] c0 c0 c0 c0
            coeff_0 = __builtin_msa_splati_w(coeff, 0);
            // [32] c1 c1 c1 c1
            coeff_1 = __builtin_msa_splati_w(coeff, 1);
            // [32] c2 c2 c2 c2
            coeff_2 = __builtin_msa_splati_w(coeff, 2);

            // Load four pixels => unpack the first two pixels to 16 bits =>
            // multiply with coefficients => accumulate the convolution result.
            // [8] a3 b3 g3 r3 a2 b2 g2 r2 a1 b1 g1 r1 a0 b0 g0 r0
            v16u8 src8 = __builtin_msa_ld_b((void*)row_to_filter, 0);
            // [16] a1 b1 g1 r1 a0 b0 g0 r0
            v8i16 src16_0 = __builtin_msa_ilvr_b(zero, src8);
            // [32] a0 b0 g0 r0
            v8i16 src_0 = __builtin_msa_ilvr_h(zero, src16_0);
            // [32] a1 b1 g1 r1
            v8i16 src_1 = __builtin_msa_ilvl_h(zero, src16_0);
            // [16] a3 b3 g3 r3 a2 b2 g2 r2
            v8i16 src16_1 = __builtin_msa_ilvl_b(zero, src8);
            // [32] a2 b2 g2 r2
            v8i16 src_2 = __builtin_msa_ilvr_h(zero, src16_1);

            // [32] accum += a0*c0 b0*c0 g0*c0 r0*c0
            accum = __builtin_msa_maddv_w(accum, src_0, coeff_0);
            // [32] accum += a1*c1 b1*c1 g1*c1 r1*c1
            accum = __builtin_msa_maddv_w(accum, src_1, coeff_1);
            // [32] accum += a2*c2 b2*c2 g2*c2 r2*c2
            accum = __builtin_msa_maddv_w(accum, src_2, coeff_2);
        }

        // Shift right for fixed point implementation.
        accum = __builtin_msa_srai_w(accum, SkConvolutionFilter1D::kShiftBits);

        accum = __builtin_msa_max_s_w(accum, zero);
        accum = __builtin_msa_min_s_w(accum, mask_255);

        // Packing 32 bits |accum| to 16 bits per channel (signed saturation).
        accum = __builtin_msa_pckev_h(zero, accum);
        // Packing 16 bits |accum| to 8 bits per channel (unsigned saturation).
        accum = __builtin_msa_pckev_b(zero, accum);

        // Store the pixel value of 32 bits.
        *(reinterpret_cast<int*>(out_row)) = __builtin_msa_copy_s_w(accum, 0);
        out_row += 4;
    }
}

// Convolves horizontally along four rows. The row data is given in
// |src_data| and continues for the num_values() of the filter.
// The algorithm is almost same as |ConvolveHorizontally_mips_msa|.
// Please refer to that function for detailed comments.
void convolve4RowsHorizontally_mips_msa(const unsigned char* src_data[4],
                                        const SkConvolutionFilter1D& filter,
                                        unsigned char* out_row[4],
                                        size_t outRowBytes) {
    int num_values = filter.numValues();
    int filter_offset, filter_length;
    v4i32 zero = __builtin_msa_ldi_w(0);
    v4i32 mask_255 = __builtin_msa_ldi_w(255);
    v4i32 mask[4];
    // |mask| will be used to decimate all extra filter coefficients that are
    // loaded by SIMD when |filter_length| is not divisible by 4.
    // mask[0] is not used in following algorithm.
    mask[1] = __builtin_msa_ld_h(mask_array, 0);
    mask[2] = __builtin_msa_ld_h(mask_array, 16);
    mask[3] = __builtin_msa_ld_h(mask_array, 32);

    // Output one pixel each iteration, calculating all channels (RGBA) together.
    for (int out_x = 0; out_x < num_values; out_x++) {
        const SkConvolutionFilter1D::ConvolutionFixed* filter_values =
            filter.FilterForValue(out_x, &filter_offset, &filter_length);

        // four pixels in a column per iteration.
        v4i32 accum0 = __builtin_msa_ldi_w(0);
        v4i32 accum1 = __builtin_msa_ldi_w(0);
        v4i32 accum2 = __builtin_msa_ldi_w(0);
        v4i32 accum3 = __builtin_msa_ldi_w(0);
        int start = (filter_offset<<2);
        // We will load and accumulate with four coefficients per iteration.
        for (int filter_x = 0; filter_x < (filter_length >> 2); filter_x++) {
            v4i32 coeff, sign, coeff_0, coeff_1, coeff_2, coeff_3;
            v4i32 src8, src16_0, src_0, src_1, src16_1, src_2, src_3;
            // [16] xx xx xx xx c3 c2 c1 c0
            coeff = __builtin_msa_ld_h((void*)(filter_values), 0);

            sign = __builtin_msa_srai_h(coeff, 15);
            coeff = __builtin_msa_ilvr_h(sign, coeff);
            coeff_0 = __builtin_msa_splati_w(coeff, 0);
            coeff_1 = __builtin_msa_splati_w(coeff, 1);
            coeff_2 = __builtin_msa_splati_w(coeff, 2);
            coeff_3 = __builtin_msa_splati_w(coeff, 3);

            src8 = __builtin_msa_ld_b((void*)(src_data[0] + start), 0);
            src16_0 = __builtin_msa_ilvr_b(zero, src8);
            src_0 = __builtin_msa_ilvr_h(zero, src16_0);
            src_1 = __builtin_msa_ilvl_h(zero, src16_0);
            src16_1 = __builtin_msa_ilvl_b(zero, src8);
            src_2 = __builtin_msa_ilvr_h(zero, src16_1);
            src_3 = __builtin_msa_ilvl_h(zero, src16_1);

            accum0 = __builtin_msa_maddv_w(accum0, src_0, coeff_0);
            accum0 = __builtin_msa_maddv_w(accum0, src_1, coeff_1);
            accum0 = __builtin_msa_maddv_w(accum0, src_2, coeff_2);
            accum0 = __builtin_msa_maddv_w(accum0, src_3, coeff_3);

            src8 = __builtin_msa_ld_b((void*)(src_data[1] + start), 0);
            src16_0 = __builtin_msa_ilvr_b(zero, src8);
            src_0 = __builtin_msa_ilvr_h(zero, src16_0);
            src_1 = __builtin_msa_ilvl_h(zero, src16_0);
            src16_1 = __builtin_msa_ilvl_b(zero, src8);
            src_2 = __builtin_msa_ilvr_h(zero, src16_1);
            src_3 = __builtin_msa_ilvl_h(zero, src16_1);

            accum1 = __builtin_msa_maddv_w(accum1, src_0, coeff_0);
            accum1 = __builtin_msa_maddv_w(accum1, src_1, coeff_1);
            accum1 = __builtin_msa_maddv_w(accum1, src_2, coeff_2);
            accum1 = __builtin_msa_maddv_w(accum1, src_3, coeff_3);

            src8 = __builtin_msa_ld_b((void*)(src_data[2] + start), 0);
            src16_0 = __builtin_msa_ilvr_b(zero, src8);
            src_0 = __builtin_msa_ilvr_h(zero, src16_0);
            src_1 = __builtin_msa_ilvl_h(zero, src16_0);
            src16_1 = __builtin_msa_ilvl_b(zero, src8);
            src_2 = __builtin_msa_ilvr_h(zero, src16_1);
            src_3 = __builtin_msa_ilvl_h(zero, src16_1);

            accum2 = __builtin_msa_maddv_w(accum2, src_0, coeff_0);
            accum2 = __builtin_msa_maddv_w(accum2, src_1, coeff_1);
            accum2 = __builtin_msa_maddv_w(accum2, src_2, coeff_2);
            accum2 = __builtin_msa_maddv_w(accum2, src_3, coeff_3);

            src8 = __builtin_msa_ld_b((void*)(src_data[3] + start), 0);
            src16_0 = __builtin_msa_ilvr_b(zero, src8);
            src_0 = __builtin_msa_ilvr_h(zero, src16_0);
            src_1 = __builtin_msa_ilvl_h(zero, src16_0);
            src16_1 = __builtin_msa_ilvl_b(zero, src8);
            src_2 = __builtin_msa_ilvr_h(zero, src16_1);
            src_3 = __builtin_msa_ilvl_h(zero, src16_1);

            accum3 = __builtin_msa_maddv_w(accum3, src_0, coeff_0);
            accum3 = __builtin_msa_maddv_w(accum3, src_1, coeff_1);
            accum3 = __builtin_msa_maddv_w(accum3, src_2, coeff_2);
            accum3 = __builtin_msa_maddv_w(accum3, src_3, coeff_3);

            start += 16;
            filter_values += 4;
        }

        int r = filter_length & 3;
        if (r) {
            // Note: filter_values must be padded to align_up(filter_offset, 8);
            v8i16 coeff, sign, coeff_0, coeff_1, coeff_2, coeff_3;
            v4i32 src8, src16_0, src_0, src_1, src16_1, src_2, src_3;
            coeff = __builtin_msa_ld_h((void*)filter_values, 0);
            coeff = __builtin_msa_and_v(coeff, mask[r]);
            sign = __builtin_msa_srai_h(coeff, 15);
            coeff = __builtin_msa_ilvr_h(sign, coeff);
            coeff_0 = __builtin_msa_splati_w(coeff, 0);
            coeff_1 = __builtin_msa_splati_w(coeff, 1);
            coeff_2 = __builtin_msa_splati_w(coeff, 2);
            coeff_3 = __builtin_msa_splati_w(coeff, 3);

            src8 = __builtin_msa_ld_b((void*)(src_data[0] + start), 0);
            src16_0 = __builtin_msa_ilvr_b(zero, src8);
            src_0 = __builtin_msa_ilvr_h(zero, src16_0);
            src_1 = __builtin_msa_ilvl_h(zero, src16_0);
            src16_1 = __builtin_msa_ilvl_b(zero, src8);
            src_2 = __builtin_msa_ilvr_h(zero, src16_1);
            src_3 = __builtin_msa_ilvl_h(zero, src16_1);

            accum0 = __builtin_msa_maddv_w(accum0, src_0, coeff_0);
            accum0 = __builtin_msa_maddv_w(accum0, src_1, coeff_1);
            accum0 = __builtin_msa_maddv_w(accum0, src_2, coeff_2);
            accum0 = __builtin_msa_maddv_w(accum0, src_3, coeff_3);

            src8 = __builtin_msa_ld_b((void*)(src_data[1] + start), 0);
            src16_0 = __builtin_msa_ilvr_b(zero, src8);
            src_0 = __builtin_msa_ilvr_h(zero, src16_0);
            src_1 = __builtin_msa_ilvl_h(zero, src16_0);
            src16_1 = __builtin_msa_ilvl_b(zero, src8);
            src_2 = __builtin_msa_ilvr_h(zero, src16_1);
            src_3 = __builtin_msa_ilvl_h(zero, src16_1);

            accum1 = __builtin_msa_maddv_w(accum1, src_0, coeff_0);
            accum1 = __builtin_msa_maddv_w(accum1, src_1, coeff_1);
            accum1 = __builtin_msa_maddv_w(accum1, src_2, coeff_2);
            accum1 = __builtin_msa_maddv_w(accum1, src_3, coeff_3);

            src8 = __builtin_msa_ld_b((void*)(src_data[2] + start), 0);
            src16_0 = __builtin_msa_ilvr_b(zero, src8);
            src_0 = __builtin_msa_ilvr_h(zero, src16_0);
            src_1 = __builtin_msa_ilvl_h(zero, src16_0);
            src16_1 = __builtin_msa_ilvl_b(zero, src8);
            src_2 = __builtin_msa_ilvr_h(zero, src16_1);
            src_3 = __builtin_msa_ilvl_h(zero, src16_1);

            accum2 = __builtin_msa_maddv_w(accum2, src_0, coeff_0);
            accum2 = __builtin_msa_maddv_w(accum2, src_1, coeff_1);
            accum2 = __builtin_msa_maddv_w(accum2, src_2, coeff_2);
            accum2 = __builtin_msa_maddv_w(accum2, src_3, coeff_3);

            src8 = __builtin_msa_ld_b((void*)(src_data[3] + start), 0);
            src16_0 = __builtin_msa_ilvr_b(zero, src8);
            src_0 = __builtin_msa_ilvr_h(zero, src16_0);
            src_1 = __builtin_msa_ilvl_h(zero, src16_0);
            src16_1 = __builtin_msa_ilvl_b(zero, src8);
            src_2 = __builtin_msa_ilvr_h(zero, src16_1);
            src_3 = __builtin_msa_ilvl_h(zero, src16_1);

            accum3 = __builtin_msa_maddv_w(accum3, src_0, coeff_0);
            accum3 = __builtin_msa_maddv_w(accum3, src_1, coeff_1);
            accum3 = __builtin_msa_maddv_w(accum3, src_2, coeff_2);
            accum3 = __builtin_msa_maddv_w(accum3, src_3, coeff_3);
        }

        accum0 = __builtin_msa_srai_w(accum0, SkConvolutionFilter1D::kShiftBits);
        accum0 = __builtin_msa_max_s_w(accum0, zero);
        accum0 = __builtin_msa_min_s_w(accum0, mask_255);
        accum0 = __builtin_msa_pckev_h(zero, accum0);
        accum0 = __builtin_msa_pckev_b(zero, accum0);

        accum1 = __builtin_msa_srai_w(accum1, SkConvolutionFilter1D::kShiftBits);
        accum1 = __builtin_msa_max_s_w(accum1, zero);
        accum1 = __builtin_msa_min_s_w(accum1, mask_255);
        accum1 = __builtin_msa_pckev_h(zero, accum1);
        accum1 = __builtin_msa_pckev_b(zero, accum1);

        accum2 = __builtin_msa_srai_w(accum2, SkConvolutionFilter1D::kShiftBits);
        accum2 = __builtin_msa_max_s_w(accum2, zero);
        accum2 = __builtin_msa_min_s_w(accum2, mask_255);
        accum2 = __builtin_msa_pckev_h(zero, accum2);
        accum2 = __builtin_msa_pckev_b(zero, accum2);

        accum3 = __builtin_msa_srai_w(accum3, SkConvolutionFilter1D::kShiftBits);
        accum3 = __builtin_msa_max_s_w(accum3, zero);
        accum3 = __builtin_msa_min_s_w(accum3, mask_255);
        accum3 = __builtin_msa_pckev_h(zero, accum3);
        accum3 = __builtin_msa_pckev_b(zero, accum3);

        *(reinterpret_cast<int*>(out_row[0])) = __builtin_msa_copy_s_w(accum0, 0);
        *(reinterpret_cast<int*>(out_row[1])) = __builtin_msa_copy_s_w(accum1, 0);
        *(reinterpret_cast<int*>(out_row[2])) = __builtin_msa_copy_s_w(accum2, 0);
        *(reinterpret_cast<int*>(out_row[3])) = __builtin_msa_copy_s_w(accum3, 0);

        out_row[0] += 4;
        out_row[1] += 4;
        out_row[2] += 4;
        out_row[3] += 4;
    }
}

template<bool hasAlpha>
    void ConvolveVertically_mips_msa(const SkConvolutionFilter1D::ConvolutionFixed* filterValues,
                            int filterLength,
                            unsigned char* const* sourceDataRows,
                            int pixelWidth,
                            unsigned char* outRow) {
        // We go through each column in the output and do a vertical convolution,
        // generating one output pixel each time.
        int outX =0;
        int aaa = 255;
        v4i32 const_4 = __builtin_msa_fill_w(aaa);
        v4i32 zero    = __builtin_msa_ldi_w(0);  //inicijalizacija zero registra
        for (int i = 0; i < (pixelWidth>>2); i++) {
            // Compute the number of bytes over in each row that the current column
            // we're convolving starts at. The pixel will cover the next 4 bytes.
            // Compute the number of bytes over in each row that the current column
            // we're convolving starts at. The pixel will cover the next 4 bytes.
            int byteOffset1 = outX * 4;
            outX+=4;
            // Apply the filter to one column of pixels.

            v4i32 accum_4   = __builtin_msa_ldi_w(0);
            v4i32 accum_4_1 = __builtin_msa_ldi_w(0);
            v4i32 accum_4_2 = __builtin_msa_ldi_w(0);
            v4i32 accum_4_3 = __builtin_msa_ldi_w(0);
            v4i32 sourceDataRows_4;
            v4i32 sourceDataRows_4_1;
            v4i32 sourceDataRows_4_2;
            v4i32 sourceDataRows_4_3;
            v4i32 temp_4;
            v4i32 temp_4_1;
            v4i32 temp_4_2;
            v4i32 temp_4_3;
            for (int filterY = 0; filterY < filterLength; filterY++) {
                SkConvolutionFilter1D::ConvolutionFixed curFilter = filterValues[filterY];
                v4i32 curFilter_4 =  __builtin_msa_fill_w(curFilter);
                sourceDataRows_4   = __builtin_msa_ld_w(&sourceDataRows[filterY][byteOffset1 + 0], 0);
                sourceDataRows_4_1 = __builtin_msa_splat_w(sourceDataRows_4, 1);
                sourceDataRows_4_2 = __builtin_msa_splat_w(sourceDataRows_4, 2);
                sourceDataRows_4_3 = __builtin_msa_splat_w(sourceDataRows_4, 3);
                sourceDataRows_4   = __builtin_msa_ilvr_b(zero, sourceDataRows_4);
                sourceDataRows_4   = __builtin_msa_ilvr_h(zero, sourceDataRows_4);
                sourceDataRows_4_1 = __builtin_msa_ilvr_b(zero, sourceDataRows_4_1);
                sourceDataRows_4_1 = __builtin_msa_ilvr_h(zero, sourceDataRows_4_1);
                sourceDataRows_4_2 = __builtin_msa_ilvr_b(zero, sourceDataRows_4_2);
                sourceDataRows_4_2 = __builtin_msa_ilvr_h(zero, sourceDataRows_4_2);
                sourceDataRows_4_3 = __builtin_msa_ilvr_b(zero, sourceDataRows_4_3);
                sourceDataRows_4_3 = __builtin_msa_ilvr_h(zero, sourceDataRows_4_3);
                accum_4            = __builtin_msa_maddv_w(accum_4,   sourceDataRows_4,   curFilter_4);
                accum_4_1          = __builtin_msa_maddv_w(accum_4_1, sourceDataRows_4_1, curFilter_4);
                accum_4_2          = __builtin_msa_maddv_w(accum_4_2, sourceDataRows_4_2, curFilter_4);
                accum_4_3          = __builtin_msa_maddv_w(accum_4_3, sourceDataRows_4_3, curFilter_4);
            }
            // Bring this value back in range. All of the filter scaling factors
            // are in fixed point with kShiftBits(14) bits of precision.
            accum_4   = __builtin_msa_srai_w(accum_4, 14);
            accum_4   = __builtin_msa_max_s_w(accum_4, zero);
            accum_4   = __builtin_msa_min_s_w(accum_4,const_4);
            accum_4_1 = __builtin_msa_srai_w(accum_4_1, 14);
            accum_4_1 = __builtin_msa_max_s_w(accum_4_1, zero);
            accum_4_1 = __builtin_msa_min_s_w(accum_4_1,const_4);
            accum_4_2 = __builtin_msa_srai_w(accum_4_2, 14);
            accum_4_2 = __builtin_msa_max_s_w(accum_4_2, zero);
            accum_4_2 = __builtin_msa_min_s_w(accum_4_2,const_4);
            accum_4_3 = __builtin_msa_srai_w(accum_4_3, 14);
            accum_4_3 = __builtin_msa_max_s_w(accum_4_3, zero);
            accum_4_3 = __builtin_msa_min_s_w(accum_4_3,const_4);

            if (hasAlpha) {
                unsigned char* outRow_tmp = outRow + byteOffset1;
                /// trazenje max elementa u vektoru
                sourceDataRows_4 = __builtin_msa_splat_d(accum_4, 1);
                sourceDataRows_4 = __builtin_msa_max_s_w(sourceDataRows_4,accum_4);
                temp_4           = __builtin_msa_splat_w(sourceDataRows_4, 2);
                sourceDataRows_4 = __builtin_msa_max_s_w(sourceDataRows_4,temp_4);
                temp_4           = __builtin_msa_splat_w(sourceDataRows_4, 3);
                sourceDataRows_4 = __builtin_msa_max_s_w(sourceDataRows_4,temp_4);
                accum_4          = __builtin_msa_insve_w(accum_4, 3, sourceDataRows_4);

                sourceDataRows_4_1 = __builtin_msa_splat_d(accum_4_1, 1);
                sourceDataRows_4_1 = __builtin_msa_max_u_w(sourceDataRows_4_1,accum_4_1);
                temp_4_1           = __builtin_msa_splat_w(sourceDataRows_4_1, 2);
                sourceDataRows_4_1 = __builtin_msa_max_u_w(sourceDataRows_4_1,temp_4_1);
                temp_4_1           = __builtin_msa_splat_w(sourceDataRows_4_1, 3);
                sourceDataRows_4_1 = __builtin_msa_max_u_w(sourceDataRows_4_1,temp_4_1);
                accum_4_1          = __builtin_msa_insve_w(accum_4_1, 3, sourceDataRows_4_1);

                sourceDataRows_4_2 = __builtin_msa_splat_d(accum_4_2, 1);
                sourceDataRows_4_2 = __builtin_msa_max_u_w(sourceDataRows_4_2,accum_4_2);
                temp_4_2           = __builtin_msa_splat_w(sourceDataRows_4_2, 2);
                sourceDataRows_4_2 = __builtin_msa_max_u_w(sourceDataRows_4_2,temp_4_2);
                temp_4_2           = __builtin_msa_splat_w(sourceDataRows_4_2, 3);
                sourceDataRows_4_2 = __builtin_msa_max_u_w(sourceDataRows_4_2,temp_4_2);
                accum_4_2          = __builtin_msa_insve_w(accum_4_2, 3, sourceDataRows_4_2);

                sourceDataRows_4_3 = __builtin_msa_splat_d(accum_4_3, 1);
                sourceDataRows_4_3 = __builtin_msa_max_u_w(sourceDataRows_4_3,accum_4_3);
                temp_4_3           = __builtin_msa_splat_w(sourceDataRows_4_3, 2);
                sourceDataRows_4_3 = __builtin_msa_max_u_w(sourceDataRows_4_3,temp_4_3);
                temp_4_3           = __builtin_msa_splat_w(sourceDataRows_4_3, 3);
                sourceDataRows_4_3 = __builtin_msa_max_u_w(sourceDataRows_4_3,temp_4_3);
                accum_4_3          = __builtin_msa_insve_w(accum_4_3, 3, sourceDataRows_4_3);

                temp_4   = __builtin_msa_pckev_b(accum_4_1, accum_4);
                temp_4_1 = __builtin_msa_pckev_b(accum_4_3, accum_4_2);
                temp_4   = __builtin_msa_pckev_b(temp_4, temp_4);
                temp_4_1 = __builtin_msa_pckev_b(temp_4_1, temp_4_1);
                temp_4_2 = __builtin_msa_ilvev_d(temp_4_1, temp_4);
                __builtin_msa_st_b(temp_4_2,outRow_tmp, 0);
            } else {
                // No alpha channel, the image is opaque.
                unsigned char* outRow_tmp = outRow + byteOffset1;

                accum_4   = __builtin_msa_insve_w(accum_4, 3, const_4);
                accum_4_1 = __builtin_msa_insve_w(accum_4_1, 3, const_4);
                accum_4_2 = __builtin_msa_insve_w(accum_4_2, 3, const_4);
                accum_4_3 = __builtin_msa_insve_w(accum_4_3, 3, const_4);
                temp_4    = __builtin_msa_pckev_b(accum_4_1, accum_4);
                temp_4_1  = __builtin_msa_pckev_b(accum_4_3, accum_4_2);
                temp_4    = __builtin_msa_pckev_b(temp_4, temp_4);
                temp_4_1  = __builtin_msa_pckev_b(temp_4_1, temp_4_1);
                temp_4_2  =  __builtin_msa_ilvev_d(temp_4_1, temp_4);
                __builtin_msa_st_b(temp_4_2,outRow_tmp, 0);
            }
        }

        for (int i = 0; i < (pixelWidth&3); i++) {
            // Compute the number of bytes over in each row that the current column
            // we're convolving starts at. The pixel will cover the next 4 bytes.
            int byteOffset = outX * 4;
            outX+=1;
            // Apply the filter to one column of pixels.
            int accum[4] = {0};
            for (int filterY = 0; filterY < filterLength; filterY++) {
                SkConvolutionFilter1D::ConvolutionFixed curFilter = filterValues[filterY];
                accum[0] += curFilter * sourceDataRows[filterY][byteOffset + 0];
                accum[1] += curFilter * sourceDataRows[filterY][byteOffset + 1];
                accum[2] += curFilter * sourceDataRows[filterY][byteOffset + 2];
                if (hasAlpha) {
                    accum[3] += curFilter * sourceDataRows[filterY][byteOffset + 3];
                }
            }
            // Bring this value back in range. All of the filter scaling factors
            // are in fixed point with kShiftBits bits of precision.
            accum[0] >>= SkConvolutionFilter1D::kShiftBits;
            accum[1] >>= SkConvolutionFilter1D::kShiftBits;
            accum[2] >>= SkConvolutionFilter1D::kShiftBits;
            if (hasAlpha) {
                accum[3] >>= SkConvolutionFilter1D::kShiftBits;
            }

            // Store the new pixel.
            outRow[byteOffset + 0] = ClampTo8_mips_msa(accum[0]);
            outRow[byteOffset + 1] = ClampTo8_mips_msa(accum[1]);
            outRow[byteOffset + 2] = ClampTo8_mips_msa(accum[2]);

            if (hasAlpha) {
                unsigned char alpha = ClampTo8_mips_msa(accum[3]);

                // Make sure the alpha channel doesn't come out smaller than any of the
                // color channels. We use premultipled alpha channels, so this should
                // never happen, but rounding errors will cause this from time to time.
                // These "impossible" colors will cause overflows (and hence random pixel
                // values) when the resulting bitmap is drawn to the screen.
                //
                // We only need to do this when generating the final output row (here).
                int maxColorChannel = SkTMax(outRow[byteOffset + 0],
                                               SkTMax(outRow[byteOffset + 1],
                                                      outRow[byteOffset + 2]));
                if (alpha < maxColorChannel) {
                    outRow[byteOffset + 3] = maxColorChannel;
                } else {
                    outRow[byteOffset + 3] = alpha;
                }
            } else {
                // No alpha channel, the image is opaque.
                outRow[byteOffset + 3] = 0xff;
            }
        }
    }

    void ConvolveVertically_mips_msa(const SkConvolutionFilter1D::ConvolutionFixed* filterValues,
                            int filterLength,
                            unsigned char* const* sourceDataRows,
                            int pixelWidth,
                            unsigned char* outRow,
                            bool sourceHasAlpha) {
        if (sourceHasAlpha) {
            ConvolveVertically_mips_msa<true>(filterValues, filterLength,
                                     sourceDataRows, pixelWidth,
                                     outRow);
        } else {
            ConvolveVertically_mips_msa<false>(filterValues, filterLength,
                                      sourceDataRows, pixelWidth,
                                      outRow);
        }
    }
