/*
 * Copyright 2013 The Android Open Source Project
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef SkUtilsMips_DEFINED
#define SkUtilsMips_DEFINED

#include "SkUtils.h"

// Define SK_MIPS_DSP_MODE to one of the following values
// corresponding respectively to:
// - No MIPS DSP support at all  (don't have DSP)
// - Full MIPS DSP support (i.e. assume the CPU always supports it)
//
#define SK_MIPS_DSP_MODE_NONE       0
#define SK_MIPS_DSP_MODE_ALWAYS     1

#if (__mips_mdsp) || (__mips_dspr2)
#  define SK_MIPS_DSP_MODE  SK_MIPS_DSP_MODE_ALWAYS
#else
#  define SK_MIPS_DSP_MODE  SK_MIPS_DSP_MODE_NONE
#endif

// Convenience test macros, always defined as 0 or 1
#define SK_MIPS_DSP_IS_NONE      (SK_MIPS_DSP_MODE == SK_MIPS_DSP_MODE_NONE)
#define SK_MIPS_DSP_IS_ALWAYS    (SK_MIPS_DSP_MODE == SK_MIPS_DSP_MODE_ALWAYS)

#if SK_MIPS_DSP_IS_NONE
#  define SK_MIPS_DSP_WRAP(x)   (x)
#elif SK_MIPS_DSP_IS_ALWAYS
#  define SK_MIPS_DSP_WRAP(x)   (x ## _mips_dsp)
#endif

#endif // SkUtilsMips_DEFINED