#pragma once

#include <citro2d.h>
#include <3ds.h>
#include <stdbool.h>
#include "ui_theme.h"

/**
 * @file ui_components.h
 * @brief Reusable UI components (buttons, sliders, list items, tabs).
 */

/**
 * @brief Set the text buffer used by UI components.
 * @param textBuf Text buffer instance created by the application.
 */
void uiSetTextBuf(C2D_TextBuf textBuf);

/** @brief Button size presets. */
typedef enum {
    BTN_SIZE_LARGE = 64,
    BTN_SIZE_MEDIUM = 40,
    BTN_SIZE_SMALL = 24
} ButtonSize;

/** @brief Square button configuration. */
typedef struct {
    float x, y;
    ButtonSize size;
    C2D_Sprite* icon;
    const char* label;
    bool isActive;
    bool isToggle;
    bool isSkeleton;
    bool useCustomColors;
    u32 bgColor;
    u32 iconColor;
    u32 labelColor;
    float iconScale;
} ButtonConfig;

/** @brief Rectangular button configuration. */
typedef struct {
    float x, y;
    float width, height;
    bool drawBackground;
    u32 bgColor;
    bool drawTopBorder;
    bool drawBottomBorder;
    u32 borderTopColor;
    u32 borderBottomColor;
    C2D_Sprite* icon;
    float iconScale;
    u32 iconColor;
    const char* text;
    float textScale;
    u32 textColor;
} RectButtonConfig;

/** @brief List item configuration with optional right icon. */
typedef struct {
    float x, y, width, height;
    u32 bgColor;
    bool drawBorder;
    u32 borderColor;
    const char* text;
    float textX, textY;
    float textScale;
    u32 textColor;
    C2D_Sprite* rightIcon;
    float rightIconX, rightIconY;
    float rightIconScale;
    u32 rightIconColor;
} ListItemConfig;

/** @brief Slider configuration. */
typedef struct {
    float x, y;           /**< Position (top-left of track) */
    float width;          /**< Track width */
    float height;         /**< Track height (default 8) */
    float knobRadius;     /**< Knob radius (default 10) */
    float value;          /**< Current value (0.0 - 1.0) */
    const char* label;    /**< Optional label above slider */
    bool showPercent;     /**< Show percentage text below */
} SliderConfig;

/** @brief Draw a square button. */
void drawButton(ButtonConfig* cfg);

/** @brief Draw a rectangular button. */
void drawRectButton(RectButtonConfig* cfg);

/** @brief Draw a tab icon with active/inactive color. */
void drawTabIcon(float centerX, float centerY, C2D_Sprite* icon, bool isActive);

/** @brief Draw a tab color swatch. */
void drawTabColorSwatch(float x, float y, float size, bool isActive, u32 color);

/** @brief Draw a tab number tile. */
void drawTabNumber(float x, float y, float size, bool isActive, int number);

/** @brief Draw a list item with optional right icon. */
void drawListItem(ListItemConfig* cfg);

/** @brief Draw a rectangle clipped between vertical bounds. */
void drawClippedRect(float x, float y, float w, float h, float clipTop, float clipBottom, u32 color);

/** @brief Hit test a square button. */
bool isButtonTouched(ButtonConfig* cfg, int touchX, int touchY);

/** @brief Draw a slider component. */
void drawSlider(SliderConfig* cfg);
