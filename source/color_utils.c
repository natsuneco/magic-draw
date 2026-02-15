#include "color_utils.h"

u32 hsvToRgb(float h, float s, float v) {
    float r, g, b;

    int i = (int)(h / 60.0f) % 6;
    float f = (h / 60.0f) - i;
    float p = v * (1.0f - s);
    float q = v * (1.0f - f * s);
    float t = v * (1.0f - (1.0f - f) * s);

    switch (i) {
        case 0: r = v; g = t; b = p; break;
        case 1: r = q; g = v; b = p; break;
        case 2: r = p; g = v; b = t; break;
        case 3: r = p; g = q; b = v; break;
        case 4: r = t; g = p; b = v; break;
        case 5: r = v; g = p; b = q; break;
        default: r = v; g = t; b = p; break;
    }

    u8 rb = (u8)(r * 255);
    u8 gb = (u8)(g * 255);
    u8 bb = (u8)(b * 255);

    return (rb << 24) | (gb << 16) | (bb << 8) | 0xFF;
}

void rgbToHsv(u32 color, float* h, float* s, float* v) {
    float r = ((color >> 24) & 0xFF) / 255.0f;
    float g = ((color >> 16) & 0xFF) / 255.0f;
    float b = ((color >> 8) & 0xFF) / 255.0f;

    float maxC = r > g ? (r > b ? r : b) : (g > b ? g : b);
    float minC = r < g ? (r < b ? r : b) : (g < b ? g : b);
    float delta = maxC - minC;

    *v = maxC;

    if (maxC == 0) {
        *s = 0;
        *h = 0;
        return;
    }

    *s = delta / maxC;

    if (delta == 0) {
        *h = 0;
        return;
    }

    if (maxC == r) {
        *h = 60.0f * ((g - b) / delta);
    } else if (maxC == g) {
        *h = 60.0f * (2.0f + (b - r) / delta);
    } else {
        *h = 60.0f * (4.0f + (r - g) / delta);
    }

    if (*h < 0) *h += 360.0f;
}
