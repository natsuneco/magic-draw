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

/**
 * @brief Button size presets in pixels.
 */
typedef enum {
    BTN_SIZE_LARGE = 64,
    BTN_SIZE_MEDIUM = 40,
    BTN_SIZE_SMALL = 24
} ButtonSize;

/**
 * @brief Square button configuration.
 */
typedef struct {
    float x, y;             /**< Top-left position in screen space. */
    ButtonSize size;        /**< Button size preset. */
    C2D_Sprite* icon;       /**< Optional icon sprite. */
    const char* label;      /**< Optional label text. */
    bool isActive;          /**< Active state for highlight. */
    bool isToggle;          /**< Toggle state for alternate background. */
    bool isSkeleton;        /**< Render placeholder when true. */
    bool useCustomColors;   /**< Use custom colors instead of defaults. */
    u32 bgColor;            /**< Custom background color. */
    u32 iconColor;          /**< Custom icon tint color. */
    u32 labelColor;         /**< Custom label color. */
    float iconScale;        /**< Icon scale factor. */
} ButtonConfig;

/**
 * @brief Rectangular button configuration.
 */
typedef struct {
    float x, y;             /**< Top-left position in screen space. */
    float width, height;    /**< Dimensions in pixels. */
    bool drawBackground;    /**< Draw background rectangle. */
    u32 bgColor;            /**< Background color. */
    bool drawTopBorder;     /**< Draw top border line. */
    bool drawBottomBorder;  /**< Draw bottom border line. */
    u32 borderTopColor;     /**< Top border color. */
    u32 borderBottomColor;  /**< Bottom border color. */
    C2D_Sprite* icon;       /**< Optional icon sprite. */
    float iconScale;        /**< Icon scale factor. */
    u32 iconColor;          /**< Icon tint color. */
    const char* text;       /**< Optional label text. */
    float textScale;        /**< Text scale factor. */
    u32 textColor;          /**< Text color. */
} RectButtonConfig;

/**
 * @brief List item configuration with optional right icon.
 */
typedef struct {
    float x, y, width, height; /**< Item bounds in screen space. */
    u32 bgColor;               /**< Background color. */
    bool drawBorder;           /**< Draw border around item. */
    u32 borderColor;           /**< Border color. */
    const char* text;          /**< Item label text. */
    float textX, textY;        /**< Text position. */
    float textScale;           /**< Text scale factor. */
    u32 textColor;             /**< Text color. */
    C2D_Sprite* rightIcon;     /**< Optional right-side icon. */
    float rightIconX, rightIconY; /**< Right icon position. */
    float rightIconScale;      /**< Right icon scale factor. */
    u32 rightIconColor;        /**< Right icon tint color. */
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

/**
 * @brief Draw a square button.
 * @param cfg Button configuration.
 */
void drawButton(ButtonConfig* cfg);

/**
 * @brief Draw a rectangular button.
 * @param cfg Button configuration.
 */
void drawRectButton(RectButtonConfig* cfg);

/**
 * @brief Draw a tab icon with active/inactive color.
 * @param centerX Icon center X position.
 * @param centerY Icon center Y position.
 * @param icon Icon sprite to draw.
 * @param isActive Active state for tint selection.
 */
void drawTabIcon(float centerX, float centerY, C2D_Sprite* icon, bool isActive);

/**
 * @brief Draw a tab color swatch.
 * @param x Top-left X position.
 * @param y Top-left Y position.
 * @param size Size of the swatch in pixels.
 * @param isActive Active state for border color.
 * @param color Swatch fill color.
 */
void drawTabColorSwatch(float x, float y, float size, bool isActive, u32 color);

/**
 * @brief Draw a tab number tile.
 * @param x Top-left X position.
 * @param y Top-left Y position.
 * @param size Tile size in pixels.
 * @param isActive Active state for border color.
 * @param number Number to display.
 */
void drawTabNumber(float x, float y, float size, bool isActive, int number);

/**
 * @brief Draw a list item with optional right icon.
 * @param cfg List item configuration.
 */
void drawListItem(ListItemConfig* cfg);

/**
 * @brief Draw a rectangle clipped between vertical bounds.
 * @param x Rectangle X position.
 * @param y Rectangle Y position.
 * @param w Rectangle width.
 * @param h Rectangle height.
 * @param clipTop Upper clip boundary.
 * @param clipBottom Lower clip boundary.
 * @param color Fill color.
 */
void drawClippedRect(float x, float y, float w, float h, float clipTop, float clipBottom, u32 color);

/**
 * @brief Hit test a square button.
 * @param cfg Button configuration.
 * @param touchX Touch X coordinate.
 * @param touchY Touch Y coordinate.
 * @return true if the touch is within the button bounds.
 */
bool isButtonTouched(ButtonConfig* cfg, int touchX, int touchY);

/**
 * @brief Draw a slider component.
 * @param cfg Slider configuration.
 */
void drawSlider(SliderConfig* cfg);

/**
 * @brief Show a dialog with title and message (blocking, waits for touch).
 * @param topScreen Top screen render target.
 * @param bottomScreen Bottom screen render target.
 * @param title Dialog title text.
 * @param message Dialog message text (supports \n line breaks).
 */
void showDialog(C3D_RenderTarget* topScreen, C3D_RenderTarget* bottomScreen, const char* title, const char* message);

/**
 * @brief Show a confirmation dialog with Yes/No buttons.
 * @param topScreen Top screen render target.
 * @param bottomScreen Bottom screen render target.
 * @param title Dialog title text.
 * @param message Dialog message text (supports \n line breaks).
 * @return true if Yes was pressed, false otherwise.
 */
bool showConfirmDialog(C3D_RenderTarget* topScreen, C3D_RenderTarget* bottomScreen, const char* title, const char* message);
