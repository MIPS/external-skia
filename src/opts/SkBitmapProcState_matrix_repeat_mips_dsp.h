/* MIPS DSPR2 optimized code (C) COPYRIGHT 2013 MIPS Technologies
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */
/*
 * This has been modified on the knowledge that (at the time)
 * we had the following macro definitions in the parent file
 *
 * #define MAKENAME(suffix)        RepeatX_RepeatY ## suffix
 * #define TILEX_PROCF(fx, max)    (((fx) & 0xFFFF) * ((max) + 1) >> 16)
 * #define TILEY_PROCF(fy, max)    (((fy) & 0xFFFF) * ((max) + 1) >> 16)
 * #define TILEX_LOW_BITS(fx, max) ((((fx) & 0xFFFF) * ((max) + 1) >> 12) & 0xF)
 * #define TILEY_LOW_BITS(fy, max) ((((fy) & 0xFFFF) * ((max) + 1) >> 12) & 0xF)
 */

/* SkClampMax(val,max) -- bound to 0..max */

#define SCALE_NOFILTER_NAME     MAKENAME(_nofilter_scale)
#define SCALE_FILTER_NAME       MAKENAME(_filter_scale)
#define AFFINE_NOFILTER_NAME    MAKENAME(_nofilter_affine)
#define AFFINE_FILTER_NAME      MAKENAME(_filter_affine)
#define PERSP_NOFILTER_NAME     MAKENAME(_nofilter_persp)
#define PERSP_FILTER_NAME       MAKENAME(_filter_persp)

#define PACK_FILTER_X_NAME  MAKENAME(_pack_filter_x)
#define PACK_FILTER_Y_NAME  MAKENAME(_pack_filter_y)

#ifndef PREAMBLE
    #define PREAMBLE(state)
    #define PREAMBLE_PARAM_X
    #define PREAMBLE_PARAM_Y
    #define PREAMBLE_ARG_X
    #define PREAMBLE_ARG_Y
#endif // PREAMBLE

static void SCALE_NOFILTER_NAME(const SkBitmapProcState& s,
                                uint32_t xy[], int count, int x, int y) {
    SkASSERT((s.fInvType & ~(SkMatrix::kTranslate_Mask |
                             SkMatrix::kScale_Mask)) == 0);
    PREAMBLE(s);
    // we store y, x, x, x, x, x
    const unsigned maxX = s.fBitmap->width() - 1;
    SkFractionalInt fx;
    {
        SkPoint pt;
        s.fInvProc(s.fInvMatrix, SkIntToScalar(x) + SK_ScalarHalf,
                                  SkIntToScalar(y) + SK_ScalarHalf, &pt);
        fx = SkScalarToFractionalInt(pt.fY);
        const unsigned maxY = s.fBitmap->height() - 1;
        *xy++ = TILEY_PROCF(SkFractionalIntToFixed(fx), maxY);
        fx = SkScalarToFractionalInt(pt.fX);
    }

    if (0 == maxX) {
        // all of the following X values must be 0
        memset(xy, 0, count * sizeof(uint16_t));
        return;
    }

    const SkFractionalInt dx = s.fInvSxFractionalInt;

#ifdef CHECK_FOR_DECAL
    // test if we don't need to apply the tile proc
    SkFixed tmpFx = SkFractionalIntToFixed(fx);
    SkFixed tmpDx = SkFractionalIntToFixed(dx);
    if ((unsigned)(tmpFx >> 16) <= maxX &&
        (unsigned)((tmpFx + tmpDx * (count - 1)) >> 16) <= maxX) {
        decal_nofilter_scale_mips_dsp(xy, tmpFx, tmpDx, count);
    } else
#endif // CHECK_FOR_DECAL
    {
        register int t1, t2, t3, t4, t5, t6;

        __asm__ volatile (
            "addu         %[t1], %[fx],    %[dx]   \n\t"
            "addu         %[t2], %[dx],    %[dx]   \n\t"
            "addu         %[t3], %[maxX],  1       \n\t"
            "srl          %[t4], %[count], 1       \n\t"
            "blez         %[t4], 2f                \n\t"
        "1:                                        \n\t"
            "and          %[t5], %[fx],    0xFFFF  \n\t"
            "and          %[t6], %[t1],    0xFFFF  \n\t"
            "mul          %[t5], %[t5],    %[t3]   \n\t"
            "mul          %[t6], %[t6],    %[t3]   \n\t"
            "addu         %[fx], %[fx],    %[t2]   \n\t"
            "addu         %[t1], %[t1],    %[t2]   \n\t"
            "addu         %[xy], %[xy],    4       \n\t"
            "subu         %[t4], %[t4],    1       \n\t"
            "precrq.ph.w  %[t5], %[t6],    %[t5]   \n\t"
            "sw           %[t5], -4(%[xy])         \n\t"
            "bnez         %[t4], 1b                \n\t"
        "2:                                        \n\t"
            "and          %[t5], %[count], 1       \n\t"
            "beqz         %[t5], 3f                \n\t"
            "and          %[t6], %[fx],    0xFFFF  \n\t"
            "mul          %[t6], %[t6],    %[t3]   \n\t"
            "sra          %[t6], %[t6],    16      \n\t"
            "sh           %[t6], 0(%[xy])          \n\t"
        "3:                                        \n\t"
            : [t1]"=&r"(t1), [t2]"=&r"(t2), [t3]"=&r"(t3),
              [t4]"=&r"(t4), [t5]"=&r"(t5), [t6]"=&r"(t6),
              [fx]"+r"(fx), [xy]"+r"(xy)
            : [count]"r"(count), [dx]"r"(dx), [maxX]"r"(maxX)
            : "memory", "hi", "lo"
        );
    }
}

// note: we could special-case on a matrix which is skewed in X but not Y.
// this would require a more general setup thatn SCALE does, but could use
// SCALE's inner loop that only looks at dx


static void AFFINE_NOFILTER_NAME(const SkBitmapProcState& s,
                                 uint32_t xy[], int count, int x, int y) {
    SkASSERT(s.fInvType & SkMatrix::kAffine_Mask);
    SkASSERT((s.fInvType & ~(SkMatrix::kTranslate_Mask |
                             SkMatrix::kScale_Mask |
                             SkMatrix::kAffine_Mask)) == 0);
    PREAMBLE(s);
    SkPoint srcPt;
    s.fInvProc(s.fInvMatrix,
               SkIntToScalar(x) + SK_ScalarHalf,
               SkIntToScalar(y) + SK_ScalarHalf, &srcPt);

    SkFractionalInt fx = SkScalarToFractionalInt(srcPt.fX);
    SkFractionalInt fy = SkScalarToFractionalInt(srcPt.fY);
    SkFractionalInt dx = s.fInvSxFractionalInt;
    SkFractionalInt dy = s.fInvKyFractionalInt;
    int maxX = s.fBitmap->width() - 1;
    int maxY = s.fBitmap->height() - 1;

    register int t1, t2, t3, t4, t5;

    __asm__ volatile (
        "sll          %[t1],    %[count], 2       \n\t"
        "blez         %[count], 2f                \n\t"
        "addu         %[t1],    %[xy],    %[t1]   \n\t"
        "addu         %[t2],    %[maxX],  1       \n\t"
        "addu         %[t3],    %[maxY],  1       \n\t"
    "1:                                           \n\t"
        "and          %[t4],    %[fx],    0xFFFF  \n\t"
        "and          %[t5],    %[fy],    0xFFFF  \n\t"
        "mul          %[t4],    %[t4],    %[t2]   \n\t"
        "mul          %[t5],    %[t5],    %[t3]   \n\t"
        "addu         %[fx],    %[fx],    %[dx]   \n\t"
        "addu         %[fy],    %[fy],    %[dy]   \n\t"
        "addu         %[xy],    %[xy],    4       \n\t"
        "precrq.ph.w  %[t4],    %[t5],    %[t4]   \n\t"
        "sw           %[t4],    -4(%[xy])         \n\t"
        "bne          %[xy],    %[t1],    1b      \n\t"
    "2:                                           \n\t"
        : [fx]"+r"(fx), [fy]"+r"(fy), [xy]"+r"(xy),
          [t1]"=&r"(t1), [t2]"=&r"(t2), [t3]"=&r"(t3),
          [t4]"=&r"(t4), [t5]"=&r"(t5)
        : [count]"r"(count), [maxX]"r"(maxX),
          [maxY]"r"(maxY), [dx]"r"(dx), [dy]"r"(dy)
        : "memory", "hi", "lo"
    );
}

static void PERSP_NOFILTER_NAME(const SkBitmapProcState& s,
                                uint32_t* SK_RESTRICT xy,
                                int count, int x, int y) {
    SkASSERT(s.fInvType & SkMatrix::kPerspective_Mask);
    PREAMBLE(s);
    int maxX = s.fBitmap->width() - 1;
    int maxY = s.fBitmap->height() - 1;
    register unsigned t0, t1, t2, t3, t4;
    register unsigned  max_tY, max_tX;

    SkPerspIter   iter(s.fInvMatrix,
                       SkIntToScalar(x) + SK_ScalarHalf,
                       SkIntToScalar(y) + SK_ScalarHalf, count);

    while ((count = iter.next()) != 0) {
        const SkFixed* SK_RESTRICT srcXY = iter.getXY();

        __asm__ volatile (
            "addiu        %[max_tY], %[maxY],  1          \n\t"
            "addiu        %[max_tX], %[maxX],  1          \n\t"
        "1:                                               \n\t"
            "addiu        %[count],  %[count], -1         \n\t"
            "bltz         %[count],  2f                   \n\t"
            "lw           %[t1],     4(%[srcXY])          \n\t"
            "lw           %[t0],     0(%[srcXY])          \n\t"
            "andi         %[t1],     %[t1],    0xffff     \n\t"
            "mul          %[t2],     %[t1],    %[max_tY]  \n\t"
            "andi         %[t0],     %[t0],    0xffff     \n\t"
            "mul          %[t3],     %[t0],    %[max_tX]  \n\t"
            "addiu        %[srcXY],  %[srcXY], 8          \n\t"
            "precrq.ph.w  %[t4],     %[t2],    %[t3]      \n\t"
            "sw           %[t4],     0(%[xy])             \n\t"
            "addiu        %[xy],     %[xy],    4          \n\t"
            "b            1b                              \n\t"
        "2:                                               \n\t"
            : [t1]"=&r"(t1), [t0]"=&r"(t0), [t2]"=&r"(t2), [t3]"=&r"(t3),
              [t4]"=&r"(t4), [count]"+r"(count), [srcXY]"+r"(srcXY),
              [xy]"+r"(xy), [max_tY]"=&r"(max_tY), [max_tX]"=&r"(max_tX)
            : [maxY]"r"(maxY), [maxX]"r"(maxX)
            : "memory", "hi", "lo"
        );
    }
}

static inline uint32_t PACK_FILTER_Y_NAME(SkFixed f, unsigned max,
                                          SkFixed one PREAMBLE_PARAM_Y) {
    unsigned i = TILEY_PROCF(f, max);
    i = (i << 4) | TILEY_LOW_BITS(f, max);
    return (i << 14) | (TILEY_PROCF((f + one), max));
}

static inline uint32_t PACK_FILTER_X_NAME(SkFixed f, unsigned max,
                                          SkFixed one PREAMBLE_PARAM_X) {
    unsigned i = TILEX_PROCF(f, max);
    i = (i << 4) | TILEX_LOW_BITS(f, max);
    return (i << 14) | (TILEX_PROCF((f + one), max));
}

static void SCALE_FILTER_NAME(const SkBitmapProcState& s,
                              uint32_t xy[], int count, int x, int y) {
    __asm__ volatile (
        "pref 1, 0(%[xy])  \n\t"
        "pref 1, 32(%[xy]) \n\t"
        :
        : [xy]"r"(xy)
        : "memory"
    );

    SkASSERT((s.fInvType & ~(SkMatrix::kTranslate_Mask |
                             SkMatrix::kScale_Mask)) == 0);
    SkASSERT(s.fInvKy == 0);
    PREAMBLE(s);
    const unsigned maxX = s.fBitmap->width() - 1;
    const SkFixed one = s.fFilterOneX;
    const SkFractionalInt dx = s.fInvSxFractionalInt;
    SkFractionalInt fx;

    {
        SkPoint pt;
        s.fInvProc(s.fInvMatrix, SkIntToScalar(x) + SK_ScalarHalf,
                                  SkIntToScalar(y) + SK_ScalarHalf, &pt);
        const SkFixed fy = SkScalarToFixed(pt.fY) - (s.fFilterOneY >> 1);
        const unsigned maxY = s.fBitmap->height() - 1;
        // compute our two Y values up front
        *xy++ = PACK_FILTER_Y_NAME(fy, maxY, s.fFilterOneY PREAMBLE_ARG_Y);
        // now initialize fx
        fx = SkScalarToFractionalInt(pt.fX) - (SkFixedToFractionalInt(one) >> 1);
    }

#ifdef CHECK_FOR_DECAL
    // test if we don't need to apply the tile proc
    if (dx > 0 &&
            (unsigned)(fx >> 16) <= maxX &&
            (unsigned)((fx + dx * (count - 1)) >> 16) < maxX) {
        decal_filter_scale_mips_dsp(xy, (SkFixed) fx, (SkFixed) dx, count);
    } else
#endif // CHECK_FOR_DECAL
    {
        register int t1, t2, t3, t4, t5;

        __asm__ volatile (
            "sll   %[t1], %[count], 2       \n\t"
            "addu  %[t1], %[xy],    %[t1]   \n\t"
            "addu  %[t2], %[maxX],  1       \n\t"
        "1:                                 \n\t"
            "and   %[t3], %[fx],    0xFFFF  \n\t"
            "mul   %[t4], %[t3],    %[t2]   \n\t"
            "addu  %[t5], %[fx],    %[one]  \n\t"
            "and   %[t5], %[t5],    0xFFFF  \n\t"
            "mul   %[t5], %[t5],    %[t2]   \n\t"
            "addu  %[fx], %[fx],    %[dx]   \n\t"
            "sra   %[t4], %[t4],    12      \n\t"
            "sll   %[t4], %[t4],    14      \n\t"
            "sra   %[t5], %[t5],    16      \n\t"
            "or    %[t4], %[t4],    %[t5]   \n\t"
            "sw    %[t4], 0(%[xy])          \n\t"
            "addu  %[xy], %[xy],    4       \n\t"
            "bne   %[xy], %[t1],    1b      \n\t"
        "2:                                 \n\t"
            : [t1]"=&r"(t1), [t2]"=&r"(t2), [t3]"=&r"(t3),
              [t4]"=&r"(t4), [t5]"=&r"(t5), [fx]"+r"(fx),
              [xy]"+r"(xy)
            : [count]"r"(count), [dx]"r"(dx),
              [maxX]"r"(maxX), [one]"r"(one)
            : "memory", "hi", "lo"
        );
    }
}

static void AFFINE_FILTER_NAME(const SkBitmapProcState& s,
                               uint32_t xy[], int count, int x, int y) {
    SkASSERT(s.fInvType & SkMatrix::kAffine_Mask);
    SkASSERT((s.fInvType & ~(SkMatrix::kTranslate_Mask |
                             SkMatrix::kScale_Mask |
                             SkMatrix::kAffine_Mask)) == 0);
    PREAMBLE(s);
    SkPoint srcPt;
    s.fInvProc(s.fInvMatrix,
               SkIntToScalar(x) + SK_ScalarHalf,
               SkIntToScalar(y) + SK_ScalarHalf, &srcPt);

    SkFixed oneX = s.fFilterOneX;
    SkFixed oneY = s.fFilterOneY;
    SkFixed fx = SkScalarToFixed(srcPt.fX) - (oneX >> 1);
    SkFixed fy = SkScalarToFixed(srcPt.fY) - (oneY >> 1);
    SkFixed dx = s.fInvSx;
    SkFixed dy = s.fInvKy;
    unsigned maxX = s.fBitmap->width() - 1;
    unsigned maxY = s.fBitmap->height() - 1;

    register int t1, t2, t3, t4, t5, t6;

    __asm__ volatile (
        "sll   %[t1], %[count], 3        \n\t"
        "addu  %[t1], %[xy],    %[t1]    \n\t"
        "addu  %[t2], %[maxX],  1        \n\t"
        "addu  %[t3], %[maxY],  1        \n\t"
    "1:                                  \n\t"
        "and   %[t4], %[fy],    0xFFFF   \n\t"
        "mul   %[t5], %[t4],    %[t3]    \n\t"
        "addu  %[t6], %[fy],    %[oneY]  \n\t"
        "and   %[t6], %[t6],    0xFFFF   \n\t"
        "mul   %[t6], %[t6],    %[t3]    \n\t"
        "addu  %[fy], %[fy],    %[dy]    \n\t"
        "sra   %[t5], %[t5],    12       \n\t"
        "sll   %[t5], %[t5],    14       \n\t"
        "sra   %[t6], %[t6],    16       \n\t"
        "or    %[t5], %[t5],    %[t6]    \n\t"
        "sw    %[t5], 0(%[xy])           \n\t"
        "and   %[t4], %[fx],    0xFFFF   \n\t"
        "mul   %[t5], %[t4],    %[t2]    \n\t"
        "addu  %[t6], %[fx],    %[oneX]  \n\t"
        "and   %[t6], %[t6],    0xFFFF   \n\t"
        "mul   %[t6], %[t6],    %[t2]    \n\t"
        "addu  %[fx], %[fx],    %[dx]    \n\t"
        "sra   %[t5], %[t5],    12       \n\t"
        "sll   %[t5], %[t5],    14       \n\t"
        "sra   %[t6], %[t6],    16       \n\t"
        "or    %[t5], %[t5],    %[t6]    \n\t"
        "sw    %[t5], 4(%[xy])           \n\t"
        "addu  %[xy], %[xy],    8        \n\t"
        "bne   %[xy], %[t1],    1b       \n\t"
    "2:                                  \n\t"
        : [fx]"+r"(fx), [fy]"+r"(fy), [xy]"+r"(xy),
          [t1]"=&r"(t1), [t2]"=&r"(t2), [t3]"=&r"(t3),
          [t4]"=&r"(t4), [t5]"=&r"(t5), [t6]"=&r"(t6)
        : [count]"r"(count), [dx]"r"(dx), [dy]"r"(dy),
          [maxX]"r"(maxX), [maxY]"r"(maxY),
          [oneX]"r"(oneX), [oneY]"r"(oneY)
        : "memory", "hi", "lo"
    );
}

static void PERSP_FILTER_NAME(const SkBitmapProcState& s,
                              uint32_t* SK_RESTRICT xy, int count,
                              int x, int y) {
    SkASSERT(s.fInvType & SkMatrix::kPerspective_Mask);
    PREAMBLE(s);
    unsigned maxX = s.fBitmap->width() - 1;
    unsigned maxY = s.fBitmap->height() - 1;
    SkFixed oneX = s.fFilterOneX;
    SkFixed oneY = s.fFilterOneY;
    SkPerspIter   iter(s.fInvMatrix,
                       SkIntToScalar(x) + SK_ScalarHalf,
                       SkIntToScalar(y) + SK_ScalarHalf, count);

    while ((count = iter.next()) != 0) {
        const SkFixed* SK_RESTRICT srcXY = iter.getXY();
        register unsigned max_tY, max_tX;

        max_tY = maxY + 1;
        max_tX = maxX + 1;

        register int t0, t1, t2, t3, t4, t5, t6, t7, t9, t10;

        __asm__ volatile (
        "1:                                              \n\t"
            "addiu        %[count], %[count], -1         \n\t"
            "bltz         %[count], 2f                   \n\t"
            "lw           %[t1],    4(%[srcXY])          \n\t"
            "lw           %[t0],    0(%[srcXY])          \n\t"
            "sra          %[t3],    %[oneY],  1          \n\t"
            "sra          %[t2],    %[oneX],  1          \n\t"
            "subu         %[t5],    %[t1],    %[t3]      \n\t"
            "subu         %[t4],    %[t0],    %[t2]      \n\t"
            "andi         %[t5],    %[t5],    0xFFFF     \n\t"
            "andi         %[t4],    %[t4],    0xFFFF     \n\t"
            "mul          %[t1],    %[t5],    %[max_tY]  \n\t"
            "mul          %[t0],    %[t4],    %[max_tX]  \n\t"
            "addu         %[t3],    %[t5],    %[oneY]    \n\t"
            "addu         %[t2],    %[t4],    %[oneX]    \n\t"
            "andi         %[t3],    %[t3],    0xFFFF     \n\t"
            "andi         %[t2],    %[t2],    0xFFFF     \n\t"
            "sra          %[t5],    %[t1],    12         \n\t"
            "sra          %[t4],    %[t0],    12         \n\t"
            "andi         %[t5],    %[t5],    0xF        \n\t"
            "andi         %[t4],    %[t4],    0xF        \n\t"
            "mul          %[t7],    %[t3],    %[max_tY]  \n\t"
            "mul          %[t6],    %[t2],    %[max_tX]  \n\t"
            "addiu        %[srcXY], %[srcXY], 8          \n\t"
            "precrq.ph.w  %[t9],    %[t1],    %[t0]      \n\t"
            "precrq.ph.w  %[t10],   %[t7],    %[t6]      \n\t"
            "sra          %[t1],    %[t9],    16         \n\t"
            "sra          %[t0],    %[t10],   16         \n\t"
            "sll          %[t2],    %[t1],    4          \n\t"
            "or           %[t3],    %[t2],    %[t5]      \n\t"
            "sll          %[t6],    %[t3],    14         \n\t"
            "or           %[t1],    %[t6],    %[t0]      \n\t"
            "sw           %[t1],    0(%[xy])             \n\t"
            "addiu        %[xy],    %[xy],    4          \n\t"
            "andi         %[t1],    %[t9],    0xFFFF     \n\t"
            "andi         %[t0],    %[t10],   0xFFFF     \n\t"
            "sll          %[t2],    %[t1],    4          \n\t"
            "or           %[t3],    %[t2],    %[t4]      \n\t"
            "sll          %[t6],    %[t3],    14         \n\t"
            "or           %[t1],    %[t6],    %[t0]      \n\t"
            "sw           %[t1],    0(%[xy])             \n\t"
            "addiu        %[xy],    %[xy],    4          \n\t"
            "b            1b                             \n\t"
        "2:                                              \n\t"
            : [t0]"=&r"(t0), [t1]"=&r"(t1), [t2]"=&r"(t2), [t3]"=&r"(t3),
              [xy]"+r"(xy), [t4]"=&r"(t4), [t5]"=&r"(t5), [t6]"=&r"(t6),
              [t7]"=&r"(t7), [t9]"=&r"(t9), [t10]"=&r"(t10),
              [srcXY]"+r"(srcXY), [count]"+r"(count)
            : [max_tY]"r"(max_tY), [max_tX]"r"(max_tX),
              [oneY]"r"(oneY), [oneX]"r"(oneX)
            : "memory", "hi", "lo"
        );
    }
}

const SkBitmapProcState::MatrixProc MAKENAME(_Procs)[] = {
    SCALE_NOFILTER_NAME,
    SCALE_FILTER_NAME,
    AFFINE_NOFILTER_NAME,
    AFFINE_FILTER_NAME,
    PERSP_NOFILTER_NAME,
    PERSP_FILTER_NAME
};

#undef MAKENAME
#undef TILEX_PROCF
#undef TILEY_PROCF
#ifdef CHECK_FOR_DECAL
    #undef CHECK_FOR_DECAL
#endif // CHECK_FOR_DECAL

#undef SCALE_NOFILTER_NAME
#undef SCALE_FILTER_NAME
#undef AFFINE_NOFILTER_NAME
#undef AFFINE_FILTER_NAME
#undef PERSP_NOFILTER_NAME
#undef PERSP_FILTER_NAME

#undef PREAMBLE
#undef PREAMBLE_PARAM_X
#undef PREAMBLE_PARAM_Y
#undef PREAMBLE_ARG_X
#undef PREAMBLE_ARG_Y

#undef TILEX_LOW_BITS
#undef TILEY_LOW_BITS
