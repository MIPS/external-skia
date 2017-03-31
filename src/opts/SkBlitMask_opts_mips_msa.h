#ifndef SkBlitMask_opts_mips_msa_DEFINED
#define SkBlitMask_opts_mips_msa_DEFINED

#include "SkColor.h"
#include "SkBlitMask.h"

#include "SkBlitMask.h"
#include "SkColor.h"
#include "SkColorPriv.h"
#include <msa.h>

extern void SkBlitLCD16OpaqueRow_mips_msa(SkPMColor dst[], const uint16_t mask[],
                                        SkColor src, int width,
                                        SkPMColor opaqueDst);

extern void SkBlitLCD16Row_mips_msa(SkPMColor dst[], const uint16_t mask[],
                                    SkColor src, int width, SkPMColor);



namespace SK_OPTS_NS {

    static void D32_A8_Color_mips_msa(void* SK_RESTRICT dst, size_t dstRB,
                             const void* SK_RESTRICT maskPtr, size_t maskRB,
                             SkColor color, int width, int height) {
        SkPMColor pmc = SkPreMultiplyColor(color);
        size_t dstOffset = dstRB - (width << 2);
        size_t maskOffset = maskRB - width;
        SkPMColor* SK_RESTRICT device = (SkPMColor *)dst;
        const uint8_t* SK_RESTRICT mask = (const uint8_t*)maskPtr;
        v4i32 mask_256 = __builtin_msa_fill_w(256);
        v4i32 mask_1   = __builtin_msa_fill_h(0x00FF);
        v4i32 tmp_pmc  = __builtin_msa_fill_w(pmc);
        v4i32 zero     = __builtin_msa_fill_w(0);
        do {
            for(int i =0; i< (width >> 2); i++) {
                v4i32  temp1, temp2, temp3, temp4, temp5, temp6, temp7;
                temp1 = __builtin_msa_ld_w((void*)(mask), 0);
                temp3 = __builtin_msa_ilvr_b(zero, temp1);
                temp1 = __builtin_msa_ilvr_h(zero, temp3);
                temp1 = __builtin_msa_addvi_w(temp1, 1);
                temp2 = __builtin_msa_ld_w((void*)(device), 0);
                temp3 = __builtin_msa_srli_w(tmp_pmc, 24);
                temp4 = __builtin_msa_mulv_w(temp3, temp1);
                temp4 = __builtin_msa_srai_w(temp4, 8);
                temp4 = __builtin_msa_subv_w(mask_256, temp4);
                temp6 = __builtin_msa_and_v(tmp_pmc, mask_1);
                temp6 = __builtin_msa_mulv_w(temp6, temp1);
                temp5 = __builtin_msa_srai_w(tmp_pmc, 8);
                temp5 = __builtin_msa_and_v(temp5, mask_1);
                temp5 = __builtin_msa_mulv_w(temp1, temp5);
                temp6 = __builtin_msa_ilvod_b(temp5, temp6);
                temp7 = __builtin_msa_and_v(temp2, mask_1);
                temp7 = __builtin_msa_mulv_w(temp7, temp4);
                temp5 = __builtin_msa_srai_w(temp2, 8);
                temp5 = __builtin_msa_and_v(temp5, mask_1);
                temp5 = __builtin_msa_mulv_w(temp4, temp5);
                temp7 = __builtin_msa_ilvod_b(temp5, temp7);
                temp6 = __builtin_msa_addv_w(temp6, temp7);
                __builtin_msa_st_b(temp6,device, 0);
                device += 4;
                mask +=4;
            }

            for(int i =0; i< (width & 3); i++)
            {
                unsigned aa = *mask++;
                *device = SkBlendARGB32(pmc, *device, aa);
                device += 1;
            }

            device = (uint32_t*)((char*)device + dstOffset);
            mask += maskOffset;
        } while (--height != 0);
    }

    static void D32_A8_Opaque_mips_msa(void* SK_RESTRICT dst, size_t dstRB,
                              const void* SK_RESTRICT maskPtr, size_t maskRB,
                              SkColor color, int width, int height) {
        SkPMColor pmc = SkPreMultiplyColor(color);
        SkPMColor* SK_RESTRICT device = (SkPMColor*)dst;
        const uint8_t* SK_RESTRICT mask = (const uint8_t*)maskPtr;
        maskRB -= width;
        dstRB -= (width << 2);
        v4i32 zero     = __builtin_msa_fill_w(0);
        v4i32 mask_1   = __builtin_msa_fill_h(0x00FF);
        v4i32 mask_256 = __builtin_msa_fill_w(256);
        v4i32 temp_pcm = __builtin_msa_fill_w(pmc);
        do {
            for (int i =0; i< (width >> 2); i++) {
                v4i32 temp1, temp2, temp3, temp4;
                v4i32 temp5, temp6, temp7, temp8;
                temp5 = __builtin_msa_ld_w((void*)(mask), 0);
                temp4 = __builtin_msa_ilvr_b(zero, temp5);
                temp4 = __builtin_msa_ilvr_h(zero, temp4);
                temp6 = __builtin_msa_ld_w((void*)(device), 0);
                temp7 = __builtin_msa_addvi_w(temp4, 1);
                temp8 = __builtin_msa_subv_w(mask_256, temp4);
                temp5 = __builtin_msa_and_v(temp_pcm, mask_1);
                temp2 = __builtin_msa_and_v(temp6, mask_1);
                temp5 = __builtin_msa_mulv_w(temp5, temp7);
                temp2 = __builtin_msa_mulv_w(temp2, temp8);
                temp4 = __builtin_msa_srai_w(temp_pcm, 8);
                temp3 = __builtin_msa_srai_w(temp6, 8);
                temp4 = __builtin_msa_and_v(temp4, mask_1);
                temp3 = __builtin_msa_and_v(temp3, mask_1);
                temp4 = __builtin_msa_mulv_w(temp7, temp4);
                temp3 = __builtin_msa_mulv_w(temp8, temp3);
                temp1 = __builtin_msa_ilvod_b(temp4, temp5);
                temp2 = __builtin_msa_ilvod_b(temp3, temp2);
                temp1 = __builtin_msa_addv_w(temp1, temp2);
                __builtin_msa_st_b(temp1,device, 0);
                device += 4;
                mask+=4;
            }
            for (int i =0; i< (width & 3); i++) {
                unsigned aa = *mask++;
                *device = SkAlphaMulQ(pmc, (aa + 1)) + SkAlphaMulQ(*device, (256 - aa));
                device += 1;
            }
            device = (uint32_t*)((char*)device + dstRB);
            mask += maskRB;
        } while (--height != 0);
    }

    static void D32_A8_Black_mips_msa(void* SK_RESTRICT dst, size_t dstRB,
                             const void* SK_RESTRICT maskPtr, size_t maskRB,
                             int width, int height) {
        SkPMColor* SK_RESTRICT device = (SkPMColor*)dst;
        const uint8_t* SK_RESTRICT mask = (const uint8_t*)maskPtr;

        maskRB -= width;
        dstRB -= (width << 2);
        v4i32 zero     = __builtin_msa_ldi_w(0);
        v4i32 mask_256 = __builtin_msa_fill_w(256);
        v4i32 mask_1   = __builtin_msa_fill_h(0x00FF);
        do {
            for(int i =0; i < (width >> 2); i++){
                v4i32 temp1, temp2, temp3, temp4, temp5, temp6;
                temp1 = __builtin_msa_ld_w((void*)(device), 0);
                temp2 = __builtin_msa_ld_w((void*)(mask), 0);
                temp3 = __builtin_msa_ilvr_b(zero, temp2);
                temp4 = __builtin_msa_ilvr_h(zero, temp3);
                temp3 = __builtin_msa_slli_w(temp4, 24);
                temp5 = __builtin_msa_subv_w(mask_256, temp4);
                temp6 = __builtin_msa_and_v(temp1, mask_1);
                temp6 = __builtin_msa_mulv_w(temp6, temp5);
                temp2 = __builtin_msa_srai_w(temp1, 8);
                temp2 = __builtin_msa_and_v(temp2, mask_1);
                temp2 = __builtin_msa_mulv_w(temp5, temp2);
                temp6 = __builtin_msa_ilvod_b(temp2, temp6);
                temp5 = __builtin_msa_addv_w(temp3, temp6);
                __builtin_msa_st_b(temp5,device, 0);
                device += 4;
                mask+=4;
            }

            for(int i =0; i < (width & 3); i++){
                unsigned aa = *mask++;
                *device = (aa << SK_A32_SHIFT) + SkAlphaMulQ(*device, SkAlpha255To256(255 - aa));
                device += 1;
            }
            device = (uint32_t*)((char*)device + dstRB);
            mask += maskRB;
        } while (--height != 0);
    }

    void blit_mask_d32_a8(SkPMColor* dst, size_t dstRB,
                             const SkAlpha* mask, size_t maskRB,
                             SkColor color, int w, int h) {
        if (color == SK_ColorBLACK) {
            D32_A8_Black_mips_msa(dst, dstRB, mask, maskRB, w, h);
        } else if (SkColorGetA(color) == 0xFF) {
            D32_A8_Opaque_mips_msa(dst, dstRB, mask, maskRB, color, w, h);
        } else {
            D32_A8_Color_mips_msa(dst, dstRB, mask, maskRB, color, w, h);
        }
    }

} // SK_OPTS_NS
#endif // #ifndef SkBlitMask_opts_mips_msa_DEFINED