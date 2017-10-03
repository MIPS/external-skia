/*
 * Copyright 2017 The Android Open Source Project
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

namespace SK_OPTS_NS {
    void blit_row_color32_mips_msa(SkPMColor* SK_RESTRICT dst,
                                   const SkPMColor* SK_RESTRICT src,
                                   int count, SkPMColor color) {
        if (count > 0) {
            if (0 == color) {
                if (src != dst) {
                    memcpy(dst, src, count * sizeof(SkPMColor));
                }
                return;
            }
            unsigned colorA = SkGetPackedA32(color);
            if (255 == colorA) {
                sk_memset32(dst, color, count);
            } else {
                unsigned scale = 255 - colorA;
                v4i32 mask_255 = __builtin_msa_fill_w(255);
                v4i32 colorA_1 = __builtin_msa_fill_w(colorA);
                v4i32 scale_1  = __builtin_msa_subv_w(mask_255, colorA_1);
                v4i32 mask_1   = __builtin_msa_fill_w(0x00FF00FF);
                v4i32 color_1  = __builtin_msa_fill_w(color);
                for(int i = 0; i < (count >> 2); i++)
                {
                    v4i32 temp1, temp2, temp3;
                    temp1 = __builtin_msa_ld_w((void*)(src), 0);
                    temp2 = __builtin_msa_and_v(temp1, mask_1);
                    temp2 = __builtin_msa_mulv_w(temp2, scale_1);
                    temp3 = __builtin_msa_srai_w(temp1, 8);
                    temp3 = __builtin_msa_and_v(temp3, mask_1);
                    temp3 = __builtin_msa_mulv_w(scale_1, temp3);
                    temp2 = __builtin_msa_ilvod_b(temp3, temp2);
                    temp2 = __builtin_msa_addv_w(temp2, color_1);
                    __builtin_msa_st_b(temp2,dst, 0);
                    dst += 4;
                    src += 4;
                }

                for(int i = 0; i < (count & 3); i++)
                {
                    *dst = color + SkAlphaMulQ(*src, scale);
                    src += 1;
                    dst += 1;
                }
            }
        }
    }
}
