#include "blend.h"

u32 blendPixel(u32 dst, u32 src, BlendMode mode, u8 opacity) {
    u8 srcR = (src >> 24) & 0xFF;
    u8 srcG = (src >> 16) & 0xFF;
    u8 srcB = (src >> 8) & 0xFF;
    u8 srcA = src & 0xFF;

    u8 dstR = (dst >> 24) & 0xFF;
    u8 dstG = (dst >> 16) & 0xFF;
    u8 dstB = (dst >> 8) & 0xFF;
    u8 dstA = dst & 0xFF;

    srcA = (srcA * opacity) / 255;
    if (srcA == 0) return dst;

    u8 outR, outG, outB, outA;

    switch (mode) {
        case BLEND_ADD:
            outR = (dstR + (srcR * srcA / 255) > 255) ? 255 : dstR + (srcR * srcA / 255);
            outG = (dstG + (srcG * srcA / 255) > 255) ? 255 : dstG + (srcG * srcA / 255);
            outB = (dstB + (srcB * srcA / 255) > 255) ? 255 : dstB + (srcB * srcA / 255);
            outA = (dstA > srcA) ? dstA : srcA;
            break;
        case BLEND_MULTIPLY:
            {
                u8 mulR = (dstR * srcR) / 255;
                u8 mulG = (dstG * srcG) / 255;
                u8 mulB = (dstB * srcB) / 255;
                outR = dstR + ((mulR - dstR) * srcA) / 255;
                outG = dstG + ((mulG - dstG) * srcA) / 255;
                outB = dstB + ((mulB - dstB) * srcA) / 255;
                outA = (dstA > srcA) ? dstA : srcA;
            }
            break;
        case BLEND_NORMAL:
        default:
            if (srcA == 255) {
                outR = srcR;
                outG = srcG;
                outB = srcB;
                outA = 255;
            } else {
                u16 invSrcA = 255 - srcA;
                outR = (srcR * srcA + dstR * invSrcA) / 255;
                outG = (srcG * srcA + dstG * invSrcA) / 255;
                outB = (srcB * srcA + dstB * invSrcA) / 255;
                outA = srcA + (dstA * invSrcA) / 255;
            }
            break;
    }

    return (outR << 24) | (outG << 16) | (outB << 8) | outA;
}
