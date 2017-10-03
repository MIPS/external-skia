/*
 * Copyright 2017 Google Inc.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "SkOpts.h"

#define SK_OPTS_NS msa
#include "SkBlitRow_opts.h"
#include "SkBlurImageFilter_opts.h"

namespace SkOpts {
    void Init_msa() {
        box_blur_xx                  = msa::box_blur_xx;
        box_blur_xy                  = msa::box_blur_xy;
        box_blur_yx                  = msa::box_blur_yx;

        blit_row_s32a_opaque         = msa::blit_row_s32a_opaque;
    }
}
