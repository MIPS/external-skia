/*
 * Copyright 2011 Google Inc.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "SkBlitRow.h"
#include "SkBlitMask.h"
#include "SkColorPriv.h"
#include "SkUtils.h"

#define UNROLL

SkBlitRow::ColorRectProc PlatformColorRectProcFactory();

static void S32_Opaque_BlitRow32(SkPMColor* SK_RESTRICT dst,
                                 const SkPMColor* SK_RESTRICT src,
                                 int count, U8CPU alpha) {
    SkASSERT(255 == alpha);
    sk_memcpy32(dst, src, count);
}

static void S32_Blend_BlitRow32(SkPMColor* SK_RESTRICT dst,
                                const SkPMColor* SK_RESTRICT src,
                                int count, U8CPU alpha) {
    SkASSERT(alpha <= 255);
    if (count > 0) {
        unsigned src_scale = SkAlpha255To256(alpha);
        unsigned dst_scale = 256 - src_scale;

#ifdef UNROLL
        if (count & 1) {
            *dst = SkAlphaMulQ(*(src++), src_scale) + SkAlphaMulQ(*dst, dst_scale);
            dst += 1;
            count -= 1;
        }

        const SkPMColor* SK_RESTRICT srcEnd = src + count;
        while (src != srcEnd) {
            *dst = SkAlphaMulQ(*(src++), src_scale) + SkAlphaMulQ(*dst, dst_scale);
            dst += 1;
            *dst = SkAlphaMulQ(*(src++), src_scale) + SkAlphaMulQ(*dst, dst_scale);
            dst += 1;
        }
#else
        do {
            *dst = SkAlphaMulQ(*src, src_scale) + SkAlphaMulQ(*dst, dst_scale);
            src += 1;
            dst += 1;
        } while (--count > 0);
#endif
    }
}

static void S32A_Opaque_BlitRow32(SkPMColor* SK_RESTRICT dst,
                                  const SkPMColor* SK_RESTRICT src,
                                  int count, U8CPU alpha) {
    SkASSERT(255 == alpha);
    if (count > 0) {
#ifdef UNROLL
        if (count & 1) {
            *dst = SkPMSrcOver(*(src++), *dst);
            dst += 1;
            count -= 1;
        }

        const SkPMColor* SK_RESTRICT srcEnd = src + count;
        while (src != srcEnd) {
            *dst = SkPMSrcOver(*(src++), *dst);
            dst += 1;
            *dst = SkPMSrcOver(*(src++), *dst);
            dst += 1;
        }
#else
        do {
            *dst = SkPMSrcOver(*src, *dst);
            src += 1;
            dst += 1;
        } while (--count > 0);
#endif
    }
}

static void S32A_Blend_BlitRow32(SkPMColor* SK_RESTRICT dst,
                                 const SkPMColor* SK_RESTRICT src,
                                 int count, U8CPU alpha) {
    SkASSERT(alpha <= 255);
    if (count > 0) {
#ifdef UNROLL
        if (count & 1) {
            *dst = SkBlendARGB32(*(src++), *dst, alpha);
            dst += 1;
            count -= 1;
        }

        const SkPMColor* SK_RESTRICT srcEnd = src + count;
        while (src != srcEnd) {
            *dst = SkBlendARGB32(*(src++), *dst, alpha);
            dst += 1;
            *dst = SkBlendARGB32(*(src++), *dst, alpha);
            dst += 1;
        }
#else
        do {
            *dst = SkBlendARGB32(*src, *dst, alpha);
            src += 1;
            dst += 1;
        } while (--count > 0);
#endif
    }
}

///////////////////////////////////////////////////////////////////////////////

static const SkBlitRow::Proc32 gDefault_Procs32[] = {
    S32_Opaque_BlitRow32,
    S32_Blend_BlitRow32,
    S32A_Opaque_BlitRow32,
    S32A_Blend_BlitRow32
};

SkBlitRow::Proc32 SkBlitRow::Factory32(unsigned flags) {
    SkASSERT(flags < SK_ARRAY_COUNT(gDefault_Procs32));
    // just so we don't crash
    flags &= kFlags32_Mask;

    SkBlitRow::Proc32 proc = PlatformProcs32(flags);
    if (NULL == proc) {
        proc = gDefault_Procs32[flags];
    }
    SkASSERT(proc);
    return proc;
}

SkBlitRow::Proc32 SkBlitRow::ColorProcFactory() {
    SkBlitRow::ColorProc proc = PlatformColorProc();
    if (NULL == proc) {
        proc = Color32;
    }
    SkASSERT(proc);
    return proc;
}

#ifndef SK_CPU_MIPS
void SkBlitRow::Color32(SkPMColor* SK_RESTRICT dst,
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
            unsigned scale = 256 - SkAlpha255To256(colorA);
            do {
                *dst = color + SkAlphaMulQ(*src, scale);
                src += 1;
                dst += 1;
            } while (--count);
        }
    }
}
#else
void SkBlitRow::Color32(SkPMColor* SK_RESTRICT dst,
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
            unsigned scale = 256 - SkAlpha255To256(colorA);
            register int tmp_a;
            register int tmp1, tmp2, tmp3, tmp4, tmp5, tmp6, tmp7, tmp8;

            __asm__ volatile (
                ".set             push                                   \n\t"
                ".set             noreorder                              \n\t"
                "replv.ph         %[scale],      %[scale]                \n\t"
                "andi             %[tmp_a],      %[count],     3         \n\t"
                "srl              %[count],      %[count],     2         \n\t"
                "blez             %[count],      2f                      \n\t"
            "1:                                                          \n\t"
                "lw               %[tmp1],       0(%[src])               \n\t"
                "lw               %[tmp2],       4(%[src])               \n\t"
                "lw               %[tmp3],       8(%[src])               \n\t"
                "lw               %[tmp4],       12(%[src])              \n\t"
                "muleu_s.ph.qbl   %[tmp5],       %[tmp1],      %[scale]  \n\t"
                "muleu_s.ph.qbr   %[tmp1],       %[tmp1],      %[scale]  \n\t"
                "muleu_s.ph.qbl   %[tmp6],       %[tmp2],      %[scale]  \n\t"
                "muleu_s.ph.qbr   %[tmp2],       %[tmp2],      %[scale]  \n\t"
                "muleu_s.ph.qbl   %[tmp7],       %[tmp3],      %[scale]  \n\t"
                "muleu_s.ph.qbr   %[tmp3],       %[tmp3],      %[scale]  \n\t"
                "muleu_s.ph.qbl   %[tmp8],       %[tmp4],      %[scale]  \n\t"
                "muleu_s.ph.qbr   %[tmp4],       %[tmp4],      %[scale]  \n\t"
                "precrq.qb.ph     %[tmp1],       %[tmp5],      %[tmp1]   \n\t"
                "precrq.qb.ph     %[tmp2],       %[tmp6],      %[tmp2]   \n\t"
                "precrq.qb.ph     %[tmp3],       %[tmp7],      %[tmp3]   \n\t"
                "precrq.qb.ph     %[tmp4],       %[tmp8],      %[tmp4]   \n\t"
                "addu             %[tmp1],       %[tmp1],      %[color]  \n\t"
                "addu             %[tmp2],       %[tmp2],      %[color]  \n\t"
                "addu             %[tmp3],       %[tmp3],      %[color]  \n\t"
                "addu             %[tmp4],       %[tmp4],      %[color]  \n\t"
                "sw               %[tmp1],       0(%[dst])               \n\t"
                "sw               %[tmp2],       4(%[dst])               \n\t"
                "sw               %[tmp3],       8(%[dst])               \n\t"
                "sw               %[tmp4],       12(%[dst])              \n\t"
                "addiu            %[count],      %[count],     -1        \n\t"
                "addiu            %[src],        %[src],       16        \n\t"
                "bnez             %[count],      1b                      \n\t"
                " addiu           %[dst],        %[dst],       16        \n\t"
            "2:                                                          \n\t"
                "blez             %[tmp_a],      4f                      \n\t"
                " nop                                                    \n\t"
            "3:                                                          \n\t"
                "lw               %[tmp1],       0(%[src])               \n\t"
                "muleu_s.ph.qbl   %[tmp5],       %[tmp1],      %[scale]  \n\t"
                "muleu_s.ph.qbr   %[tmp1],       %[tmp1],      %[scale]  \n\t"
                "precrq.qb.ph     %[tmp1],       %[tmp5],      %[tmp1]   \n\t"
                "addu             %[tmp1],       %[tmp1],      %[color]  \n\t"
                "sw               %[tmp1],       0(%[dst])               \n\t"
                "addiu            %[tmp_a],      %[tmp_a],     -1        \n\t"
                "addiu            %[src],        %[src],       4         \n\t"
                "bnez             %[tmp_a],      3b                      \n\t"
                " addiu           %[dst],        %[dst],       4         \n\t"
            "4:                                                          \n\t"
                ".set             pop                                    \n\t"
                : [tmp1]"=&r"(tmp1), [tmp2]"=&r"(tmp2), [tmp3]"=&r"(tmp3),
                  [tmp4]"=&r"(tmp4), [tmp5]"=&r"(tmp5), [tmp6]"=&r"(tmp6),
                  [tmp7]"=&r"(tmp7), [tmp8]"=&r"(tmp8), [src]"+r"(src),
                  [dst]"+r"(dst), [scale]"+r"(scale), [count]"+r"(count),
                  [tmp_a]"=&r"(tmp_a)
                : [color]"r"(color)
                : "hi", "lo", "memory"
            );
        }
    }
}
#endif // SK_CPU_MIPS

template <size_t N> void assignLoop(SkPMColor* dst, SkPMColor color) {
    for (size_t i = 0; i < N; ++i) {
        *dst++ = color;
    }
}

static inline void assignLoop(SkPMColor dst[], SkPMColor color, int count) {
    while (count >= 4) {
        *dst++ = color;
        *dst++ = color;
        *dst++ = color;
        *dst++ = color;
        count -= 4;
    }
    if (count >= 2) {
        *dst++ = color;
        *dst++ = color;
        count -= 2;
    }
    if (count > 0) {
        *dst++ = color;
    }
}

void SkBlitRow::ColorRect32(SkPMColor* dst, int width, int height,
                            size_t rowBytes, SkPMColor color) {
    if (width <= 0 || height <= 0 || 0 == color) {
        return;
    }

    // Just made up this value, since I saw it once in a SSE2 file.
    // We should consider writing some tests to find the optimimal break-point
    // (or query the Platform proc?)
    static const int MIN_WIDTH_FOR_SCANLINE_PROC = 32;

    bool isOpaque = (0xFF == SkGetPackedA32(color));

    if (!isOpaque || width >= MIN_WIDTH_FOR_SCANLINE_PROC) {
        SkBlitRow::ColorProc proc = SkBlitRow::ColorProcFactory();
        while (--height >= 0) {
            (*proc)(dst, dst, width, color);
            dst = (SkPMColor*) ((char*)dst + rowBytes);
        }
    } else {
        switch (width) {
            case 1:
                while (--height >= 0) {
                    assignLoop<1>(dst, color);
                    dst = (SkPMColor*) ((char*)dst + rowBytes);
                }
                break;
            case 2:
                while (--height >= 0) {
                    assignLoop<2>(dst, color);
                    dst = (SkPMColor*) ((char*)dst + rowBytes);
                }
                break;
            case 3:
                while (--height >= 0) {
                    assignLoop<3>(dst, color);
                    dst = (SkPMColor*) ((char*)dst + rowBytes);
                }
                break;
            default:
                while (--height >= 0) {
                    assignLoop(dst, color, width);
                    dst = (SkPMColor*) ((char*)dst + rowBytes);
                }
                break;
        }
    }
}

SkBlitRow::ColorRectProc SkBlitRow::ColorRectProcFactory() {
    SkBlitRow::ColorRectProc proc = PlatformColorRectProcFactory();
    if (NULL == proc) {
        proc = ColorRect32;
    }
    SkASSERT(proc);
    return proc;
}
