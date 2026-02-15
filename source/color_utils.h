#pragma once

#include <3ds.h>

/**
 * @file color_utils.h
 * @brief RGB/HSV conversion helpers.
 */

/** @brief Convert HSV to RGBA (0xRRGGBBAA). */
u32 hsvToRgb(float h, float s, float v);

/** @brief Convert RGBA (0xRRGGBBAA) to HSV. */
void rgbToHsv(u32 color, float* h, float* s, float* v);
