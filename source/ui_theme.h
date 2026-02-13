#pragma once

#include <citro2d.h>

/**
 * @file ui_theme.h
 * @brief UI color palette definitions.
 */

/** @brief Active elements color (0, 128, 255). */
#define UI_COLOR_ACTIVE      C2D_Color32(0x00, 0x80, 0xFF, 0xFF)
/** @brief Active dark color (0, 64, 128). */
#define UI_COLOR_ACTIVE_DARK C2D_Color32(0x00, 0x40, 0x80, 0xFF)
/** @brief Darkest gray (42). */
#define UI_COLOR_GRAY_1      C2D_Color32(0x2A, 0x2A, 0x2A, 0xFF)
/** @brief Dark gray (58). */
#define UI_COLOR_GRAY_2      C2D_Color32(0x3A, 0x3A, 0x3A, 0xFF)
/** @brief Medium gray (64). */
#define UI_COLOR_GRAY_3      C2D_Color32(0x40, 0x40, 0x40, 0xFF)
/** @brief Light gray (80). */
#define UI_COLOR_GRAY_4      C2D_Color32(0x50, 0x50, 0x50, 0xFF)
/** @brief White color. */
#define UI_COLOR_WHITE       C2D_Color32(0xFF, 0xFF, 0xFF, 0xFF)
/** @brief Standard text color. */
#define UI_COLOR_TEXT        C2D_Color32(0xDD, 0xDD, 0xDD, 0xFF)
/** @brief Disabled icon color (128). */
#define UI_COLOR_DISABLED    C2D_Color32(0x80, 0x80, 0x80, 0xFF)
/** @brief Dim text color. */
#define UI_COLOR_TEXT_DIM    C2D_Color32(0xAA, 0xAA, 0xAA, 0xFF)
