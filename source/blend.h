#pragma once

#include "app_state.h"

/**
 * @file blend.h
 * @brief Pixel blending helpers.
 */

/** @brief Blend two pixels with specified blend mode and opacity. */
u32 blendPixel(u32 dst, u32 src, BlendMode mode, u8 opacity);
