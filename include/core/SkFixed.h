/*
 * Copyright 2006 The Android Open Source Project
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef SkFixed_DEFINED
#define SkFixed_DEFINED

#include "SkTypes.h"

/** \file SkFixed.h

    Types and macros for 16.16 fixed point
*/

/** 32 bit signed integer used to represent fractions values with 16 bits to the right of the decimal point
*/
typedef int32_t             SkFixed;
#define SK_Fixed1           (1 << 16)
#define SK_FixedHalf        (1 << 15)
#define SK_FixedMax         (0x7FFFFFFF)
#define SK_FixedMin         (-SK_FixedMax)
#define SK_FixedNaN         ((int) 0x80000000)
#define SK_FixedPI          (0x3243F)
#define SK_FixedSqrt2       (92682)
#define SK_FixedTanPIOver8  (0x6A0A)
#define SK_FixedRoot2Over2  (0xB505)

#define SkFixedToFloat(x)   ((x) * 1.5258789e-5f)
#if 1
    #define SkFloatToFixed(x)   ((SkFixed)((x) * SK_Fixed1))
#else
    // pins over/under flows to max/min int32 (slower than just a cast)
    static inline SkFixed SkFloatToFixed(float x) {
        int64_t n = x * SK_Fixed1;
        return (SkFixed)n;
    }
#endif

#ifdef SK_DEBUG
    static inline SkFixed SkFloatToFixed_Check(float x) {
        int64_t n64 = (int64_t)(x * SK_Fixed1);
        SkFixed n32 = (SkFixed)n64;
        SkASSERT(n64 == n32);
        return n32;
    }
#else
    #define SkFloatToFixed_Check(x) SkFloatToFixed(x)
#endif

#define SkFixedToDouble(x)  ((x) * 1.5258789e-5)
#define SkDoubleToFixed(x)  ((SkFixed)((x) * SK_Fixed1))

/** Converts an integer to a SkFixed, asserting that the result does not overflow
    a 32 bit signed integer
*/
#ifdef SK_DEBUG
    inline SkFixed SkIntToFixed(int n)
    {
        SkASSERT(n >= -32768 && n <= 32767);
        return n << 16;
    }
#else
    //  force the cast to SkFixed to ensure that the answer is signed (like the debug version)
    #define SkIntToFixed(n)     (SkFixed)((n) << 16)
#endif

#define SkFixedRoundToInt(x)    (((x) + SK_FixedHalf) >> 16)
#define SkFixedCeilToInt(x)     (((x) + SK_Fixed1 - 1) >> 16)
#define SkFixedFloorToInt(x)    ((x) >> 16)

#define SkFixedRoundToFixed(x)  (((x) + SK_FixedHalf) & 0xFFFF0000)
#define SkFixedCeilToFixed(x)   (((x) + SK_Fixed1 - 1) & 0xFFFF0000)
#define SkFixedFloorToFixed(x)  ((x) & 0xFFFF0000)

#define SkFixedAbs(x)       SkAbs32(x)
#define SkFixedAve(a, b)    (((a) + (b)) >> 1)

SkFixed SkFixedMul_portable(SkFixed, SkFixed);

#define SkFixedDiv(numer, denom)    SkDivBits(numer, denom, 16)

///////////////////////////////////////////////////////////////////////////////
// TODO: move fixed sin/cos into SkCosineMapper, as that is the only caller
//       or rewrite SkCosineMapper to not use it at all

SkFixed SkFixedSinCos(SkFixed radians, SkFixed* cosValueOrNull);
#define SkFixedSin(radians)         SkFixedSinCos(radians, NULL)
static inline SkFixed SkFixedCos(SkFixed radians) {
    SkFixed cosValue;
    (void)SkFixedSinCos(radians, &cosValue);
    return cosValue;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////
// Now look for ASM overrides for our portable versions (should consider putting this in its own file)

#ifdef SkLONGLONG
    inline SkFixed SkFixedMul_longlong(SkFixed a, SkFixed b)
    {
        return (SkFixed)((int64_t)a * b >> 16);
    }
    #define SkFixedMul(a,b)     SkFixedMul_longlong(a,b)
#endif

#if defined(SK_CPU_ARM32)
    /* This guy does not handle NaN or other obscurities, but is faster than
       than (int)(x*65536).  When built on Android with -Os, needs forcing
       to inline or we lose the speed benefit.
    */
    SK_ALWAYS_INLINE SkFixed SkFloatToFixed_arm(float x)
    {
        int32_t y, z;
        asm("movs    %1, %3, lsl #1         \n"
            "mov     %2, #0x8E              \n"
            "sub     %1, %2, %1, lsr #24    \n"
            "mov     %2, %3, lsl #8         \n"
            "orr     %2, %2, #0x80000000    \n"
            "mov     %1, %2, lsr %1         \n"
            "it cs                          \n"
            "rsbcs   %1, %1, #0             \n"
            : "=r"(x), "=&r"(y), "=&r"(z)
            : "r"(x)
            : "cc"
            );
        return y;
    }
    inline SkFixed SkFixedMul_arm(SkFixed x, SkFixed y)
    {
        int32_t t;
        asm("smull  %0, %2, %1, %3          \n"
            "mov    %0, %0, lsr #16         \n"
            "orr    %0, %0, %2, lsl #16     \n"
            : "=r"(x), "=&r"(y), "=r"(t)
            : "r"(x), "1"(y)
            :
            );
        return x;
    }
    #undef SkFixedMul
    #define SkFixedMul(x, y)        SkFixedMul_arm(x, y)

    #undef SkFloatToFixed
    #define SkFloatToFixed(x)  SkFloatToFixed_arm(x)
#endif

#if defined (SK_CPU_MIPS)
    /* This guy does not handle NaN or other obscurities, but is faster than
       than (int)(x*65536) when we only have software floats
    */
    inline SkFixed SkFloatToFixed_mips(float x) {
        register int32_t t0,t1,t2,t3;
        SkFixed res;

        asm("srl    %[t0], %[x], 31       \n\t"  // t0 <- sign bit
#ifdef SK_MIPS_HAS_DSPR2
            "ext    %[t1], %[x], 23, 8    \n\t"  // get exponent
#else
            "srl    %[t1], %[x], 23       \n\t"
            "andi   %[t1], %[t1], 0xff    \n\t"
#endif
            "li     %[t2], 0x8e           \n\t"
            "subu   %[t1], %[t2], %[t1]   \n\t"  // t1=127+15-exponent
            "sll    %[t2], %[x], 8        \n\t"  // mantissa<<8
            "lui    %[t3], 0x8000         \n\t"
            "or     %[t2], %[t2], %[t3]   \n\t"  // add the missing 1
            "srl    %[res], %[t2], %[t1]  \n\t"  // scale to 16.16
            "subu   %[t2], $zero, %[res]  \n\t"
            "movn   %[res], %[t2], %[t0]  \n\t"  // if negative?
            "sltiu  %[t3], %[t1], 32      \n\t"  // if t1>=32 the float value is too small
            "movz   %[res], $zero, %[t3]  \n\t"  // so res=0
            : [res]"=&r"(res),[t0]"=&r"(t0),[t1]"=&r"(t1),[t2]"=&r"(t2),[t3]"=&r"(t3)
            : [x] "r" (x)
            );
        return res;
    }
    inline SkFixed SkFixedMul_mips(SkFixed x, SkFixed y) {
        SkFixed res;
        int32_t t0;
        asm("mult  %[x], %[y]             \n\t"
            "mflo  %[res]                 \n\t"
            "mfhi  %[t0]                  \n\t"
            "srl   %[res], %[res], 16     \n\t"
            "sll   %[t0], %[t0], 16       \n\t"
            "or    %[res], %[res], %[t0]  \n\t"
            : [res]"=&r"(res),[t0]"=&r"(t0)
            : [x] "r" (x), [y] "r" (y)
            : "hi", "lo"
            );
        return res;
    }
    inline SkFixed SkFixedMulAdd_mips(SkFixed x, SkFixed y, SkFixed a) {
        SkFixed res;
        int32_t t0;
        asm("mult  %[x], %[y]             \n\t"
            "mflo  %[res]                 \n\t"
            "mfhi  %[t0]                  \n\t"
            "srl   %[res], %[res], 16     \n\t"
            "sll   %[t0], %[t0], 16       \n\t"
            "or    %[res], %[res], %[t0]  \n\t"
            "add   %[res], %[res], %[a]   \n\t"
            : [res]"=&r"(res),[t0]"=&r"(t0)
            : [x] "r" (x), [y] "r" (y), [a] "r" (a)
            : "hi", "lo"
            );
        return res;
    }
    inline SkFixed SkFractMul_mips(SkFixed x, SkFixed y) {
        SkFixed res;
        int32_t t0;
        asm("mult  %[x], %[y]             \n\t"
            "mfhi  %[t0]                  \n\t"
            "mflo  %[res]                 \n\t"
            "srl   %[res], %[res], 30     \n\t"
            "sll   %[t0], %[t0], 2        \n\t"
            "or    %[res], %[res], %[t0]  \n\t"
            : [res]"=&r"(res),[t0]"=&r"(t0)
            : [x] "r" (x), [y] "r" (y)
            : "hi", "lo"
            );
        return res;
    }

    #undef SkFixedMul
    #undef SkFractMul
    #define SkFixedMul(x, y)        SkFixedMul_mips(x, y)
    #define SkFractMul(x, y)        SkFractMul_mips(x, y)
    #define SkFixedMulAdd(x, y, a)  SkFixedMulAdd_mips(x, y, a)

    #undef SkFloatToFixed
    #define SkFloatToFixed(x)  SkFloatToFixed_mips(x)

#endif // if defined (SK_CPU_MIPS)

/////////////////////// Now define our macros to the portable versions if they weren't overridden

#ifndef SkFixedSquare
    #define SkFixedSquare(x)    SkFixedSquare_portable(x)
#endif

#ifndef SkFixedMul
    #define SkFixedMul(x, y)    SkFixedMul_portable(x, y)
#endif

///////////////////////////////////////////////////////////////////////////////

typedef int64_t SkFixed48;

#define SkIntToFixed48(x)       ((SkFixed48)(x) << 48)
#define SkFixed48ToInt(x)       ((int)((x) >> 48))
#define SkFixedToFixed48(x)     ((SkFixed48)(x) << 32)
#define SkFixed48ToFixed(x)     ((SkFixed)((x) >> 32))
#define SkFloatToFixed48(x)     ((SkFixed48)((x) * (65536.0f * 65536.0f * 65536.0f)))

#define SkScalarToFixed48(x)    SkFloatToFixed48(x)

#endif
