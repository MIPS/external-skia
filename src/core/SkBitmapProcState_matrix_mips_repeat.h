
#define SCALE_NOFILTER_NAME     MAKENAME(_nofilter_scale)
#define SCALE_FILTER_NAME       MAKENAME(_filter_scale)
#define AFFINE_NOFILTER_NAME    MAKENAME(_nofilter_affine_mips)
#define AFFINE_FILTER_NAME      MAKENAME(_filter_affine_mips)
#define PERSP_NOFILTER_NAME     MAKENAME(_nofilter_persp_mips)
#define PERSP_FILTER_NAME       MAKENAME(_filter_persp)

#define PACK_FILTER_X_NAME  MAKENAME(_pack_filter_x)
#define PACK_FILTER_Y_NAME  MAKENAME(_pack_filter_y)

#ifndef PREAMBLE
    #define PREAMBLE(state)
    #define PREAMBLE_PARAM_X
    #define PREAMBLE_PARAM_Y
    #define PREAMBLE_ARG_X
    #define PREAMBLE_ARG_Y
#endif

void SCALE_NOFILTER_NAME(const SkBitmapProcState& s,
                                uint32_t xy[], int count, int x, int y) {
    SkASSERT((s.fInvType & ~(SkMatrix::kTranslate_Mask |
                             SkMatrix::kScale_Mask)) == 0);

    PREAMBLE(s);
    // we store y, x, x, x, x, x

    const unsigned maxX = s.fBitmap->width() - 1;
    SkFixed fx;
    {
        SkPoint pt;
        s.fInvProc(*s.fInvMatrix, SkIntToScalar(x) + SK_ScalarHalf,
                                  SkIntToScalar(y) + SK_ScalarHalf, &pt);
        fx = SkScalarToFixed(pt.fY);
        const unsigned maxY = s.fBitmap->height() - 1;
        *xy++ = TILEY_PROCF(fx, maxY);
        fx = SkScalarToFixed(pt.fX);
    }

    if (0 == maxX) {
        // all of the following X values must be 0
        memset(xy, 0, count * sizeof(uint16_t));
        return;
    }

    const SkFixed dx = s.fInvSx;

#ifdef CHECK_FOR_DECAL
    // test if we don't need to apply the tile proc
    if ((unsigned)(fx >> 16) <= maxX &&
        (unsigned)((fx + dx * (count - 1)) >> 16) <= maxX) {
        decal_nofilter_scale(xy, fx, dx, count);
    } else
#endif
    {
        int i;
        for (i = (count >> 2); i > 0; --i) {
            unsigned a, b;
            a = TILEX_PROCF(fx, maxX); fx += dx;
            b = TILEX_PROCF(fx, maxX); fx += dx;
#ifdef SK_CPU_BENDIAN
            *xy++ = (a << 16) | b;
#else
            *xy++ = (b << 16) | a;
#endif
            a = TILEX_PROCF(fx, maxX); fx += dx;
            b = TILEX_PROCF(fx, maxX); fx += dx;
#ifdef SK_CPU_BENDIAN
            *xy++ = (a << 16) | b;
#else
            *xy++ = (b << 16) | a;
#endif
        }
        uint16_t* xx = (uint16_t*)xy;
        for (i = (count & 3); i > 0; --i) {
            *xx++ = TILEX_PROCF(fx, maxX); fx += dx;
        }
    }
}

// note: we could special-case on a matrix which is skewed in X but not Y.
// this would require a more general setup thatn SCALE does, but could use
// SCALE's inner loop that only looks at dx

void AFFINE_NOFILTER_NAME(const SkBitmapProcState& s,
                                 uint32_t xy[], int count, int x, int y) {
    SkASSERT(s.fInvType & SkMatrix::kAffine_Mask);
    SkASSERT((s.fInvType & ~(SkMatrix::kTranslate_Mask |
                             SkMatrix::kScale_Mask |
                             SkMatrix::kAffine_Mask)) == 0);

    PREAMBLE(s);
    SkPoint srcPt;
    s.fInvProc(*s.fInvMatrix,
               SkIntToScalar(x) + SK_ScalarHalf,
               SkIntToScalar(y) + SK_ScalarHalf, &srcPt);

    SkFixed fx = SkScalarToFixed(srcPt.fX);
    SkFixed fy = SkScalarToFixed(srcPt.fY);
    SkFixed dx = s.fInvSx;
    SkFixed dy = s.fInvKy;
    int maxX = s.fBitmap->width() - 1;
    int maxY = s.fBitmap->height() - 1;

#if defined ( __mips_dsp)
    register int t1, t2, t3, t4, t5;
    __asm__ __volatile__ (
        "sll          %[t1], %[count], 2          \n\t"         // count*sizeof(int);
        "blez         %[count], 2f                \n\t"         // check for count <= 0
        "addu         %[t1], %[xy], %[t1]         \n\t"         // final address of xy
        "addu         %[t2], %[maxX], 1           \n\t"         //
        "addu         %[t3], %[maxY], 1           \n\t"         //
        "1:                                       \n\t"
        "and          %[t4], %[fx], 0xFFFF        \n\t"         // fx[15..0]
        "and          %[t5], %[fy], 0xFFFF        \n\t"         // fy[15..0]
        "mul          %[t4], %[t4], %[t2]         \n\t"         // (maxX + 1) * fx[15..0]
        "mul          %[t5], %[t5], %[t3]         \n\t"         // (maxY + 1) * fy[15..0]
        "addu         %[fx], %[fx], %[dx]         \n\t"         // fx += dx
        "addu         %[fy], %[fy], %[dy]         \n\t"         // fy += dy
        "addu         %[xy], %[xy], 4             \n\t"         // xy += 4
        "precrq.ph.w  %[t4], %[t5], %[t4]         \n\t"         //
        "sw           %[t4], -4(%[xy])            \n\t"         // store to xy[]
        "bne          %[xy], %[t1], 1b            \n\t"         //
        "2:                                       \n\t"
        : [fx] "+r" (fx), [fy] "+r" (fy), [xy] "+r" (xy), [t1] "=&r" (t1), [t2] "=&r" (t2), [t3] "=&r" (t3),
          [t4] "=&r" (t4), [t5] "=&r" (t5)
        : [count] "r" (count), [maxX] "r" (maxX), [maxY] "r" (maxY), [dx] "r" (dx), [dy] "r" (dy)
        : "memory", "hi", "lo"
    );
#else
    for (int i = count; i > 0; --i) {
        *xy++ = (TILEY_PROCF(fy, maxY) << 16) | TILEX_PROCF(fx, maxX);
        fx += dx; fy += dy;
    }
#endif
}

void PERSP_NOFILTER_NAME(const SkBitmapProcState& s,
                                uint32_t* SK_RESTRICT xy,
                                int count, int x, int y) {
    SkASSERT(s.fInvType & SkMatrix::kPerspective_Mask);

    PREAMBLE(s);

    int maxX = s.fBitmap->width() - 1;
    int maxY = s.fBitmap->height() - 1;
 #if defined ( __mips_dsp)
    register unsigned t0, t1, t2, t3, t4;
    register unsigned  max_tY, max_tX;
#endif
    SkPerspIter   iter(*s.fInvMatrix,
                       SkIntToScalar(x) + SK_ScalarHalf,
                       SkIntToScalar(y) + SK_ScalarHalf, count);

    while ((count = iter.next()) != 0) {
        const SkFixed* SK_RESTRICT srcXY = iter.getXY();
#if defined ( __mips_dsp)
        __asm__ __volatile__ (
            "addiu   %[max_tY], %[maxY], 1        \n\t"
            "addiu   %[max_tX], %[maxX], 1        \n\t"
            "1:                                   \n\t"
            "addiu   %[count], %[count], -1       \n\t"
            "bltz    %[count], 2f                 \n\t"
            "lw      %[t1], 4(%[srcXY])           \n\t"
            "lw      %[t0], 0(%[srcXY])           \n\t"
            "andi    %[t1], %[t1], 0xffff         \n\t"
            "mul     %[t2], %[t1], %[max_tY]      \n\t"
            "andi    %[t0], %[t0], 0xffff         \n\t"
            "mul     %[t3], %[t0], %[max_tX]      \n\t"
            "addiu   %[srcXY], %[srcXY], 8        \n\t"
            "precrq.ph.w %[t4], %[t2], %[t3]      \n\t"
            "sw      %[t4], 0(%[xy])              \n\t"
            "addiu   %[xy], %[xy], 4              \n\t"
            "b       1b                           \n\t"
            "2:                                   \n\t"
            : [t1] "=&r" (t1), [t0] "=&r" (t0), [t2] "=&r" (t2), [t3] "=&r" (t3), [t4] "=&r" (t4),
              [count] "+r" (count), [srcXY] "+r" (srcXY), [max_tY] "=&r" (max_tY), [max_tX] "=&r" (max_tX)
            : [xy] "r" (xy), [maxY] "r" (maxY), [maxX] "r" (maxX)
            : "memory", "hi", "lo"
            );
#else
        while (--count >= 0) {
            *xy++ = (TILEY_PROCF(srcXY[1], maxY) << 16) |
                     TILEX_PROCF(srcXY[0], maxX);
            srcXY += 2;
        }
#endif
    }
}

//////////////////////////////////////////////////////////////////////////////

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

void SCALE_FILTER_NAME(const SkBitmapProcState& s,
                              uint32_t xy[], int count, int x, int y) {
    SkASSERT((s.fInvType & ~(SkMatrix::kTranslate_Mask |
                             SkMatrix::kScale_Mask)) == 0);
    SkASSERT(s.fInvKy == 0);

    PREAMBLE(s);

    const unsigned maxX = s.fBitmap->width() - 1;
    const SkFixed one = s.fFilterOneX;
    const SkFixed dx = s.fInvSx;
    SkFixed fx;

    {
        SkPoint pt;
        s.fInvProc(*s.fInvMatrix, SkIntToScalar(x) + SK_ScalarHalf,
                                  SkIntToScalar(y) + SK_ScalarHalf, &pt);
        const SkFixed fy = SkScalarToFixed(pt.fY) - (s.fFilterOneY >> 1);
        const unsigned maxY = s.fBitmap->height() - 1;
        // compute our two Y values up front
        *xy++ = PACK_FILTER_Y_NAME(fy, maxY, s.fFilterOneY PREAMBLE_ARG_Y);
        // now initialize fx
        fx = SkScalarToFixed(pt.fX) - (one >> 1);
    }

#ifdef CHECK_FOR_DECAL
    // test if we don't need to apply the tile proc
    if (dx > 0 &&
            (unsigned)(fx >> 16) <= maxX &&
            (unsigned)((fx + dx * (count - 1)) >> 16) < maxX) {
        decal_filter_scale(xy, fx, dx, count);
    } else
#endif
    {
        do {
            *xy++ = PACK_FILTER_X_NAME(fx, maxX, one PREAMBLE_ARG_X);
            fx += dx;
        } while (--count != 0);
    }
}

void AFFINE_FILTER_NAME(const SkBitmapProcState& s,
                               uint32_t xy[], int count, int x, int y) {
    SkASSERT(s.fInvType & SkMatrix::kAffine_Mask);
    SkASSERT((s.fInvType & ~(SkMatrix::kTranslate_Mask |
                             SkMatrix::kScale_Mask |
                             SkMatrix::kAffine_Mask)) == 0);

    PREAMBLE(s);
    SkPoint srcPt;
    s.fInvProc(*s.fInvMatrix,
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

#if defined ( __mips__)
    register int t1, t2, t3, t4, t5, t6;
    __asm__ __volatile__ (
        "sll    %[t1], %[count], 3                \n\t"         // count*2*sizeof(int)
        "addu   %[t1], %[xy], %[t1]               \n\t"         // final address of xy
        "addu   %[t2], %[maxX], 1                 \n\t"         //
        "addu   %[t3], %[maxY], 1                 \n\t"         //
        "1:                                       \n\t"
        "and    %[t4], %[fy], 0xFFFF              \n\t"         // fy[15..0]
        "mul    %[t5], %[t4], %[t3]               \n\t"         // fy[15..0] * (maxY + 1)
        "addu   %[t6], %[fy], %[oneY]             \n\t"         // fy + oneY
        "and    %[t6], %[t6], 0xFFFF              \n\t"         // (fy + oneY)[15..0]
        "mul    %[t6], %[t6], %[t3]               \n\t"         //
        "addu   %[fy], %[fy], %[dy]               \n\t"         // fy += dy
        "sra    %[t5], %[t5], 12                  \n\t"         //
        "sll    %[t5], %[t5], 14                  \n\t"         //
        "sra    %[t6], %[t6], 16                  \n\t"         //
        "or     %[t5], %[t5], %[t6]               \n\t"         //
        "sw     %[t5], 0(%[xy])                   \n\t"         // store to xy[]

        "and    %[t4], %[fx], 0xFFFF              \n\t"         // fx[15..0]
        "mul    %[t5], %[t4], %[t2]               \n\t"         // fx[15..0] * (maxX + 1)
        "addu   %[t6], %[fx], %[oneX]             \n\t"         // fx + oneX
        "and    %[t6], %[t6], 0xFFFF              \n\t"         // (fx + oneX)[15..0]
        "mul    %[t6], %[t6], %[t2]               \n\t"         //
        "addu   %[fx], %[fx], %[dx]               \n\t"         // fx += dx
        "sra    %[t5], %[t5], 12                  \n\t"         //
        "sll    %[t5], %[t5], 14                  \n\t"         //
        "sra    %[t6], %[t6], 16                  \n\t"         //
        "or     %[t5], %[t5], %[t6]               \n\t"         //
        "sw     %[t5], 4(%[xy])                   \n\t"         // store to xy[]

        "addu   %[xy], %[xy], 8                   \n\t"         // xy += 8
        "bne    %[xy], %[t1], 1b                  \n\t"         //
        "2:                                       \n\t"
        : [fx] "+r" (fx), [fy] "+r" (fy), [xy] "+r" (xy), [t1] "=&r" (t1), [t2] "=&r" (t2),
          [t3] "=&r" (t3), [t4] "=&r" (t4), [t5] "=&r" (t5), [t6] "=&r" (t6)
        : [count] "r" (count), [dx] "r" (dx), [dy] "r" (dy), [maxX] "r" (maxX), [maxY] "r" (maxY),
          [oneX] "r" (oneX), [oneY] "r" (oneY)
        : "memory", "hi", "lo"
    );
#else
    do {
        *xy++ = PACK_FILTER_Y_NAME(fy, maxY, oneY PREAMBLE_ARG_Y);
        fy += dy;
        *xy++ = PACK_FILTER_X_NAME(fx, maxX, oneX PREAMBLE_ARG_X);
        fx += dx;
    } while (--count != 0);
#endif
}

void PERSP_FILTER_NAME(const SkBitmapProcState& s,
                              uint32_t* SK_RESTRICT xy, int count,
                              int x, int y) {
    SkASSERT(s.fInvType & SkMatrix::kPerspective_Mask);

    PREAMBLE(s);
    unsigned maxX = s.fBitmap->width() - 1;
    unsigned maxY = s.fBitmap->height() - 1;
    SkFixed oneX = s.fFilterOneX;
    SkFixed oneY = s.fFilterOneY;

    SkPerspIter   iter(*s.fInvMatrix,
                       SkIntToScalar(x) + SK_ScalarHalf,
                       SkIntToScalar(y) + SK_ScalarHalf, count);

    while ((count = iter.next()) != 0) {
        const SkFixed* SK_RESTRICT srcXY = iter.getXY();
        do {
            *xy++ = PACK_FILTER_Y_NAME(srcXY[1] - (oneY >> 1), maxY,
                                       oneY PREAMBLE_ARG_Y);
            *xy++ = PACK_FILTER_X_NAME(srcXY[0] - (oneX >> 1), maxX,
                                       oneX PREAMBLE_ARG_X);
            srcXY += 2;
        } while (--count != 0);
    }
}

static SkBitmapProcState::MatrixProc MAKENAME(_Procs)[] = {
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
#endif

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
