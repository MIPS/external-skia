/*
 * Copyright 2013 Google Inc.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef SkBitmapFilter_opts_mips_msa_DEFINED
#define SkBitmapFilter_opts_mips_msa_DEFINED

#include "SkBitmapProcState.h"
#include "SkConvolver.h"

void convolveHorizontally_mips_msa(const unsigned char* src_data,
                                   const SkConvolutionFilter1D& filter,
                                   unsigned char* out_row,
                                   bool has_alpha);
void convolve4RowsHorizontally_mips_msa(const unsigned char* src_data[4],
                                        const SkConvolutionFilter1D& filter,
                                        unsigned char* out_row[4],
                                        size_t outRowBytes);
void ConvolveVertically_mips_msa(const SkConvolutionFilter1D::ConvolutionFixed* filterValues,
                                 int filterLength,
                                 unsigned char* const* sourceDataRows,
                                 int pixelWidth,
                                 unsigned char* outRow,
                                 bool sourceHasAlpha);
#endif
