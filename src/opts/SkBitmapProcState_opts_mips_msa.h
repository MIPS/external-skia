/*
 * Copyright 2017 The Android Open Source Project
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef SkBitmapProcState_opts_MIPS_MSA_DEFINED
#define SkBitmapProcState_opts_MIPS_MSA_DEFINED

#include "SkBitmapProcState.h"

void ClampX_ClampY_nofilter_affine_mips_msa(const SkBitmapProcState& s,
                                            uint32_t xy[], int count, int x,
                                            int y);
void ClampX_ClampY_nofilter_scale_mips_msa(const SkBitmapProcState& s,
                                           uint32_t xy[], int count, int x,
                                           int y);
void ClampX_ClampY_filter_scale_mips_msa(const SkBitmapProcState& s,
                                         uint32_t xy[], int count, int x,
                                         int y);
void ClampX_ClampY_nofilter_persp_mips_msa(const SkBitmapProcState& s,
                                           uint32_t xy[], int count, int x,
                                           int y);
void ClampX_ClampY_filter_affine_mips_msa(const SkBitmapProcState& s,
                                          uint32_t xy[], int count, int x,
                                          int y);
void RepeatX_RepeatY_filter_scale_mips_msa(const SkBitmapProcState& s,
                                           uint32_t xy[], int count, int x,
                                           int y);
void RepeatX_RepeatY_filter_affine_mips_msa(const SkBitmapProcState& s,
                                            uint32_t xy[], int count, int x,
                                            int y);
#endif
