/*
 * Copyright 2013 The Android Open Source Project
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef SkMorphology_opts_MIPS_DSP_DEFINED
#define SkMorphology_opts_MIPS_DSP_DEFINED

#include "SkColor.h"

void SkDilateX_mips_dsp(const SkPMColor* src, SkPMColor* dst, int radius,
                        int width, int height, int srcStride, int dstStride);
void SkDilateY_mips_dsp(const SkPMColor* src, SkPMColor* dst, int radius,
                        int width, int height, int srcStride, int dstStride);
void SkErodeX_mips_dsp(const SkPMColor* src, SkPMColor* dst, int radius,
                       int width, int height, int srcStride, int dstStride);
void SkErodeY_mips_dsp(const SkPMColor* src, SkPMColor* dst, int radius,
                       int width, int height, int srcStride, int dstStride);

#endif