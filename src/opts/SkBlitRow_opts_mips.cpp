#include "SkBlitRow.h"
#include "SkColorPriv.h"
#include "SkDither.h"

#if defined ( __mips_dsp) && defined(SK_CPU_LENDIAN)
static void S32_D565_Blend_mips(uint16_t* SK_RESTRICT dst,
				const SkPMColor* SK_RESTRICT src, int count,
				U8CPU alpha, int /*x*/, int /*y*/) {

	asm volatile (
		"addiu	$sp, $sp, -16		\n\t"
		"sw	$s0, 4($sp)		\n\t"
		"sw	$s1, 8($sp)		\n\t"
		"sw	$s2, 12($sp)		\n\t"
		"sw	$s3, 16($sp)		\n\t"
		"addiu	$a3, $a3, 1		\n\t"
		"blt	$a2, 2, loopend		\n\t"
		"sll	$s4, $a3, 8		\n\t"/*mult vector is used later*/
		"or	$s4, $s4, $a3		\n\t"
		"loopB:				\n\t"
		"lwr	$s0, 0($a0)		\n\t"
		"lwl	$s0, 3($a0)		\n\t"
		"lw	$s2, 0($a1)		\n\t"
		"lw	$s3, 4($a1)		\n\t"
		"and	$t1, $s0, 0x001f001f	\n\t"/*Get two blues from two Dst pixels*/
		"shra.ph $t0, $s0, 5		\n\t"/*Get two greens from two Dst pixels*/
		"and	$t2, $t0, 0x003f003f	\n\t"
		"shra.ph $t0, $s0, 11		\n\t"/*Get two reds from two Dst pixels*/
		"and	$t3, $t0, 0x001f001f	\n\t"
		"precrq.ph.w $t0, $s2, $s3	\n\t"
		"shrl.qb $t5, $t0, 3		\n\t"
		"and	$t4, $t5, 0x001f001f	\n\t"/*t4 has two blues from src*/
		"ins	$s2, $s3, 16, 16	\n\t"
		"and	$t0, $s2, 0x00ff00ff	\n\t"
		"shrl.qb $t6, $t0, 3		\n\t"/*t6 has got two reds from src*/
		"shra.ph $t0, $s2, 0xa		\n\t"
		"and	$t5, $t0, 0x003f003f	\n\t"/*t5 has got two greens from src*/
		"subu.qb	$t7, $t4, $t1	\n\t"
		"subu.qb	$t8, $t5, $t2	\n\t"
		"subu.qb	$t9, $t6, $t3	\n\t"
		"muleu_s.ph.qbr	$t4, $s4, $t7	\n\t"
		"muleu_s.ph.qbr	$t5, $s4, $t8	\n\t"
		"muleu_s.ph.qbr	$t6, $s4, $t9	\n\t"
		"shra.ph	$t7, $t4, 8	\n\t"
		"shra.ph	$t8, $t5, 8	\n\t"
		"shra.ph	$t9, $t6, 8	\n\t"
		"addu.qb	$t4, $t7, $t1	\n\t"
		"addu.qb	$t5, $t8, $t2	\n\t"
		"addu.qb	$t6, $t9, $t3	\n\t"
		"ext	$s0, $t4, 3, 5		\n\t"/* set the blue into Dst */
		"ext	$t0, $t5, 2, 6		\n\t"/* set the Green into Dst*/
		"ins	$s0, $t0, 5, 6 		\n\t"
		"ext	$t0, $t6, 3, 5		\n\t"/* set the Red into Dst */
		"ins	$s0, $t0, 11, 5		\n\t"
		"sh	$s0, 0($a0)		\n\t"/* Store s0 */
		"ext	$s1, $t4, 19, 5		\n\t"
		"ext	$t0, $t5, 18, 6		\n\t"
		"ins	$s1, $t0, 5, 6		\n\t"
		"ext	$t0, $t6, 19, 5		\n\t"
		"ins	$s1, $t0, 11, 5		\n\t"
		"sh	$s0, 2($a0)		\n\t"
		"addiu	$a2, $a2, -2		\n\t"
		"addiu	$a1, $a1, 8		\n\t"
		"addiu	$a0, $a0, 4		\n\t"
		"bge	$a2, 2, loopB		\n\t"
		"loopend:			\n\t"
		"blez	$a2, end		\n\t"
		"lhu	$v1, 0($a0)		\n\t"
		"lw	$v0, 0($a1)		\n\t"
		"addiu  $a2, $a2, -2		\n\t"
		"srl	$t1, $v1, 0xb		\n\t"
		"ext	$t0, $v1, 0x5, 0x6	\n\t"
		"ext	$t9, $v0, 0x3, 0x5	\n\t"
		"ext	$t8, $v0, 0xa, 0x6	\n\t"
		"subu	$t7, $t9, $t1		\n\t"
		"subu	$t6, $t8, $t0		\n\t"
		"mul	$t5, $t7, $a3		\n\t"
		"mul	$t4, $t6, $a3		\n\t"
		"sra	$t3, $t5, 0x8		\n\t"
		"sra	$t2, $t4, 0x8		\n\t"
		"addu	$t8, $t3, $t1		\n\t"
		"addu	$t6, $t2, $t0		\n\t"
		"ext	$t9, $v0, 0x13, 0x5	\n\t"
		"andi	$t2, $v1, 0x1f		\n\t"
		"subu	$t7, $t9, $t2		\n\t"
		"sll	$t4, $t8, 0xb		\n\t"
		"mul	$t5, $t7, $a3		\n\t"
		"sll	$t3, $t6, 0x5		\n\t"
		"sra	$v0, $t5, 0x8		\n\t"
		"or	$t0, $t4, $t3		\n\t"
		"addu	$t1, $v0, $t2		\n\t"
		"or	$v1, $t0, $t1		\n\t"
		"sh	$v1, 0($a0)		\n\t"
		"end:				\n\t"
		"lw	$s0, 4($sp) 		\n\t"
		"lw	$s1, 8($sp) 		\n\t"
		"lw	$s2, 12($sp) 		\n\t"
		"lw	$s3, 16($sp) 		\n\t"
		"jr	$ra			\n\t"
		"addiu	$sp, $sp, 16		\n\t"
		:
		:
		:"t0","t1","t2","t3","t4","t5","t6","t7","t8","t9",
                  "s0","s1","s2","s3","s4","a0","a1","a2","a3","v0","v1"
		);
}

static void S32A_D565_Opaque_Dither_mips(uint16_t* __restrict__ dst,
                                      const SkPMColor* __restrict__ src,
                                      int count, U8CPU alpha, int x, int y) {

	register int32_t t0, t1, t2, t3, t4, t5, t6;
	register int32_t t7, t8, t9, t10, t11, t12, t13, t14;
	const uint16_t dither_scan = gDitherMatrix_3Bit_16[(y) & 3];
	if (count >= 2) {
	asm volatile (
		"li     %[t12], 0x01010101		\n"/*257 in 16 bits for SIMD*/
		"li	%[t13], -2017			\n"
		"li	%[t14], 1			\n"
		"loopStart:				\n"
		"lw	%[t1], 0(%[src])			\n" /*Load src1 to t1 */
#if 1 
		"andi	%[t3], %[x], 0x3		\n"
		"addiu	%[x], %[x], 1			\n"/*increment x*/
		"sll	%[t4], %[t3], 2			\n"
		"srav	%[t5], %[dither_scan], %[t4] 	\n"
		"andi	%[t3], %[t5], 0xf		\n" /*t3 has mul op1*/
		"lw	%[t2], 4(%[src])		\n" /*Load src2 to t2*/
		"andi   %[t4], %[x], 0x3                \n"
		"sll    %[t5], %[t4], 2                 \n"
		"srav   %[t6], %[dither_scan], %[t5]    \n"
		"addiu	%[x], %[x], 1			\n"/*increment x*/
		"ins	%[t3], %[t6], 8, 4		\n" /*t3 has 2 ops*/
		"srl	%[t4], %[t1], 24		\n" /*a0*/
		"addiu	%[t0], %[t4], 1			\n" /*SkAlpha255To256(a)*/
		"srl	%[t4], %[t2], 24		\n" /*a1*/
		"addiu	%[t5], %[t4], 1			\n" /*SkAlpha255To256(a)*/
		"ins	%[t0], %[t5], 16, 16		\n" /*t0 has 2 d*/
		"muleu_s.ph.qbr %[t4], %[t3], %[t0]	\n" 
		"preceu.ph.qbla %[t3], %[t4]		\n" /*t3 = t4>>8*/
		"andi	%[t4], %[t1], 0xff		\n"/*Calculating sr from src1*/
		"ins	%[t4], %[t2], 16, 8		\n"/*multiple sr's in reg t4*/
		"shrl.qb %[t5], %[t4], 5		\n"
		"subu.qb %[t6], %[t3], %[t5]		\n"
		"addq.ph %[t5], %[t6], %[t4]		\n" /*new sr*/

		"ext	%[t4], %[t1], 8, 8		\n"/*Calculating sg from src1*/	
		"srl	%[t6], %[t2], 8			\n"
		"ins	%[t4], %[t6], 16, 8		\n"
		"shrl.qb %[t6], %[t4], 6		\n"
		"shrl.qb %[t7], %[t3], 1		\n"
		"subu.qb %[t8], %[t7], %[t6]		\n"
		"addq.ph %[t6], %[t8], %[t4]		\n"/*new sg*/

		"ext	%[t4], %[t1], 16, 8		\n"/*Calculating sb from src1*/
		"srl	%[t7], %[t2], 16		\n"
		"ins	%[t4], %[t7], 16, 8		\n"
		"shrl.qb %[t7], %[t4], 5		\n"
		"subu.qb %[t8], %[t3], %[t7]            \n"
		"addq.ph %[t7], %[t8], %[t4]		\n" /*new sg*/
		
		"shll.ph %[t4], %[t7], 2		\n"/*src expanded*/
		"andi	%[t9], %[t4], 0xffff		\n"/*sb1*/
		"srl	%[t10], %[t4], 16		\n"/*sb2*/
		"andi	%[t3], %[t6], 0xffff		\n"/*sg1*/
		"srl	%[t4], %[t6], 16		\n"/*sg2*/
		"andi	%[t6], %[t5], 0xffff		\n"/*sr1*/
		"srl	%[t7], %[t5], 16		\n"/*sr2*/

		"beqz	%[t1], src1null			\n"
		"lhu	%[t5], 0(%[dst])		\n"
		"sll	%[t11], %[t6], 13		\n"/*sr << 13*/
		"or	%[t8], %[t9], %[t11]		\n"/*sr1|sb1*/
		"sll	%[t11], %[t3], 24		\n"/*sg1 << 24*/
		"or	%[t9], %[t11], %[t8]		\n"/*t9 = sr1|srb1|sg1*/
		
		"andi	%[t3], %[t5], 0x7e0		\n"
		"sll	%[t6], %[t3], 0x10		\n"
		"and	%[t8], %[t13], %[t5]		\n"
		"or	%[t5], %[t6], %[t8]		\n"/*dst_expanded*/
		"subq.ph %[t11], %[t12], %[t0]		\n"/*257 - (a+1)*/
		"shra.ph %[t8], %[t11], 3		\n"/*(256-a)>>3*/
		"andi	%[t0], %[t8], 0xff		\n"
		"mul	%[t11], %[t0], %[t5]		\n"/*dst_expanded*/	
		"srl	%[t1], %[t8], 0x10		\n"/**/
		"addu	%[t5], %[t11], %[t9]		\n"
		"srl	%[t6], %[t5], 5			\n"
		"and	%[t5], %[t13], %[t6]		\n"/*~greenmask*/
		"srl	%[t8], %[t6], 16		\n"
		"andi	%[t10], %[t8], 0x7e0		\n"
		"or	%[t11], %[t5], %[t10]		\n"
		"sh	%[t11], 0(%[dst])		\n"
		"src1null:				\n"
		"beqz	%[t2], src2null			\n"
		"lhu	%[t5], 2(%[dst])		\n"
		"sll	%[t11], %[t7], 13		\n"/*sr << 13*/
		"or	%[t8], %[t10], %[t11]		\n"/*sr1|sb1*/
		"sll	%[t11], %[t4], 24		\n"/*sg1 << 24*/
		"or	%[t9], %[t11], %[t8]		\n"/*t9 = sr1|srb1|sg1*/
		
		"andi	%[t3], %[t5], 0x7e0		\n"
		"sll	%[t6], %[t3], 0x10		\n"
		"and	%[t8], %[t13], %[t5]		\n"
		"or	%[t5], %[t6], %[t8]		\n"/*dst_expanded*/
		"subq.ph %[t11], %[t12], %[t0]		\n"/*257 - (a+1)*/
		"shra.ph %[t8], %[t11], 3		\n"/*(256-a)>>3*/
		"andi	%[t0], %[t8], 0xff		\n"
		"mul	%[t11], %[t0], %[t5]		\n"/*dst_expanded*/	
		"srl	%[t1], %[t8], 0x10		\n"/**/
		"addu	%[t5], %[t11], %[t9]		\n"
		"srl	%[t6], %[t5], 5			\n"
		"and	%[t5], %[t13], %[t6]		\n"/*~greenmask*/
		"srl	%[t8], %[t6], 16		\n"
		"andi	%[t10], %[t8], 0x7e0		\n"
		"or	%[t11], %[t5], %[t10]		\n"
		"sh	%[t11], 2(%[dst])		\n"
		"src2null:				\n"
#endif
		"addiu	%[count], %[count], -2		\n"
		"addiu	%[src], %[src], 8		\n"
		"addiu	%[dst], %[dst], 4		\n"
		"bgt	%[count], %[t14], loopStart	\n"
		: [t0]"=r"(t0), [t1]"=&r"(t1), [src]"=&r"(src), [count]"=&r"(count), [dst]"=&r"(dst), 
		  [x]"=&r"(x), [t2]"=&r"(t2), [t3]"=&r"(t3), [t4]"=&r"(t4),  [t5]"=&r"(t5),
                  [t6]"=&r"(t6), [t7]"=&r"(t7),  [t8]"=&r"(t8),  [t9]"=&r"(t9),  [t10]"=&r"(t10),  
                  [t11]"=&r"(t11), [t12]"=&r"(t12), [t13]"=&r"(t13), [t14]"=&r"(t14)
		: [dither_scan]"r"(dither_scan)
		: "hi", "lo", "memory" 
	);
	}
	if (count == 1) {
            SkPMColor c = *src++;
            SkPMColorAssert(c);
            if (c) {
                unsigned a = SkGetPackedA32(c);
                
                int d = SkAlphaMul(DITHER_VALUE(x), SkAlpha255To256(a));
                
                unsigned sr = SkGetPackedR32(c);
                unsigned sg = SkGetPackedG32(c);
                unsigned sb = SkGetPackedB32(c);
                sr = SkDITHER_R32_FOR_565(sr, d);
                sg = SkDITHER_G32_FOR_565(sg, d);
                sb = SkDITHER_B32_FOR_565(sb, d);
                
                uint32_t src_expanded = (sg << 24) | (sr << 13) | (sb << 2);
                uint32_t dst_expanded = SkExpand_rgb_16(*dst);
                dst_expanded = dst_expanded * (SkAlpha255To256(255 - a) >> 3);
                // now src and dst expanded are in g:11 r:10 x:1 b:10
                *dst = SkCompact_rgb_16((src_expanded + dst_expanded) >> 5);
            }
            dst += 1;
            DITHER_INC_X(x);
	}
}

#define	S32A_D565_Opaque_Dither_PROC S32A_D565_Opaque_Dither_mips

#define S32_D565_Blend_PROC	S32_D565_Blend_mips
//#define S32_D565_Blend_PROC	S32A_D565_Blend_PROC
#else
#define	S32A_D565_Opaque_Dither_PROC NULL
#define S32_D565_Blend_PROC NULL
#define S32A_D565_Blend_PROC NULL
#endif

static const SkBlitRow::Proc platform_565_procs[] = {
    // no dither
    NULL,
    S32_D565_Blend_PROC,
    NULL,
    NULL,
    
    // dither
    NULL,
    NULL,
    S32A_D565_Opaque_Dither_PROC,
    NULL,
};
static const SkBlitRow::Proc platform_4444_procs[] = {
    // no dither
    NULL,   // S32_D4444_Opaque,
    NULL,   // S32_D4444_Blend,
    NULL,   // S32A_D4444_Opaque,
    NULL,   // S32A_D4444_Blend,
    
    // dither
    NULL,   // S32_D4444_Opaque_Dither,
    NULL,   // S32_D4444_Blend_Dither,
    NULL,   // S32A_D4444_Opaque_Dither,
    NULL,   // S32A_D4444_Blend_Dither
};

static const SkBlitRow::Proc32 platform_32_procs[] = {
    NULL,   // S32_Opaque,
    NULL,   // S32_Blend,
    NULL,   // S32A_Opaque,
    NULL,   // S32A_Blend,
};

SkBlitRow::Proc SkBlitRow::PlatformProcs4444(unsigned flags) {
    return platform_4444_procs[flags];
}

SkBlitRow::Proc SkBlitRow::PlatformProcs565(unsigned flags) {
    return platform_565_procs[flags];
}

SkBlitRow::Proc32 SkBlitRow::PlatformProcs32(unsigned flags) {
    return platform_32_procs[flags];
}
