/*
 * Copyright 2013 The Android Open Source Project
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef SkBlurImage_opts_MIPS_DSP_DEFINED
#define SkBlurImage_opts_MIPS_DSP_DEFINED

#include "SkBlurImage_opts.h"

bool SkBoxBlurGetPlatformProcs_mips_dsp(SkBoxBlurProc* boxBlurX,
                                        SkBoxBlurProc* boxBlurY,
                                        SkBoxBlurProc* boxBlurXY);

#endif
