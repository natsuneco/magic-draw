#include "ui_components.h"

#include <stdio.h>

/** @brief Internal text buffer for UI drawing. */
static C2D_TextBuf g_uiTextBuf = NULL;

void uiSetTextBuf(C2D_TextBuf textBuf) {
    g_uiTextBuf = textBuf;
}

void drawButton(ButtonConfig* cfg) {
    float btnSize = (float)cfg->size;

    if (cfg->isSkeleton) {
        C2D_DrawRectSolid(cfg->x, cfg->y, 0, btnSize, btnSize, UI_COLOR_GRAY_2);
        return;
    }

    u32 bgColor;
    if (cfg->useCustomColors) {
        bgColor = cfg->bgColor;
    } else if (cfg->isActive) {
        bgColor = UI_COLOR_ACTIVE;
    } else if (cfg->isToggle) {
        bgColor = UI_COLOR_ACTIVE_DARK;
    } else {
        bgColor = UI_COLOR_ACTIVE_DARK;
    }

    C2D_DrawRectSolid(cfg->x, cfg->y, 0, btnSize, btnSize, bgColor);

    if (cfg->icon) {
        float iconScale = (cfg->iconScale > 0) ? cfg->iconScale : (btnSize / 64.0f) * 0.75f;
        u32 iconColor = cfg->useCustomColors ? cfg->iconColor : UI_COLOR_WHITE;
        C2D_ImageTint tint;
        C2D_PlainImageTint(&tint, iconColor, 1.0f);
        C2D_SpriteSetPos(cfg->icon, cfg->x + btnSize / 2, cfg->y + btnSize / 2);
        C2D_SpriteSetScale(cfg->icon, iconScale, iconScale);
        C2D_DrawSpriteTinted(cfg->icon, &tint);
    }

    if (cfg->label && g_uiTextBuf) {
        u32 labelColor = cfg->useCustomColors ? cfg->labelColor : UI_COLOR_TEXT;
        C2D_TextBufClear(g_uiTextBuf);
        C2D_Text labelText;
        C2D_TextParse(&labelText, g_uiTextBuf, cfg->label);
        C2D_TextOptimize(&labelText);

        float textWidth, textHeight;
        C2D_TextGetDimensions(&labelText, 0.4f, 0.4f, &textWidth, &textHeight);
        float textX = cfg->x + (btnSize - textWidth) / 2;
        float textY = cfg->y + btnSize + 2;

        C2D_DrawText(&labelText, C2D_WithColor, textX, textY, 0, 0.4f, 0.4f, labelColor);
    }
}

void drawRectButton(RectButtonConfig* cfg) {
    if (cfg->drawBackground) {
        C2D_DrawRectSolid(cfg->x, cfg->y, 0, cfg->width, cfg->height, cfg->bgColor);
    }
    if (cfg->drawTopBorder) {
        C2D_DrawRectSolid(cfg->x, cfg->y, 0, cfg->width, 1, cfg->borderTopColor);
    }
    if (cfg->drawBottomBorder) {
        C2D_DrawRectSolid(cfg->x, cfg->y + cfg->height - 1, 0, cfg->width, 1, cfg->borderBottomColor);
    }
    if (cfg->icon) {
        float iconScale = (cfg->iconScale > 0) ? cfg->iconScale : 0.5f;
        C2D_ImageTint tint;
        C2D_PlainImageTint(&tint, cfg->iconColor, 1.0f);
        C2D_SpriteSetPos(cfg->icon, cfg->x + cfg->width / 2, cfg->y + cfg->height / 2);
        C2D_SpriteSetScale(cfg->icon, iconScale, iconScale);
        C2D_DrawSpriteTinted(cfg->icon, &tint);
    }
    if (cfg->text && g_uiTextBuf) {
        float textScale = (cfg->textScale > 0) ? cfg->textScale : 0.5f;
        C2D_TextBufClear(g_uiTextBuf);
        C2D_Text text;
        C2D_TextParse(&text, g_uiTextBuf, cfg->text);
        C2D_TextOptimize(&text);
        float textWidth, textHeight;
        C2D_TextGetDimensions(&text, textScale, textScale, &textWidth, &textHeight);
        float textX = cfg->x + cfg->width / 2 - textWidth / 2;
        float textY = cfg->y + cfg->height / 2 - textHeight / 2;
        C2D_DrawText(&text, C2D_WithColor, textX, textY, 0, textScale, textScale, cfg->textColor);
    }
}

void drawTabIcon(float centerX, float centerY, C2D_Sprite* icon, bool isActive) {
    u32 tabIconActiveColor = UI_COLOR_WHITE;
    u32 tabIconColor = UI_COLOR_TEXT_DIM;
    C2D_ImageTint tint;
    C2D_PlainImageTint(&tint, isActive ? tabIconActiveColor : tabIconColor, 1.0f);
    C2D_SpriteSetPos(icon, centerX, centerY);
    C2D_SpriteSetScale(icon, 0.4f, 0.4f);
    C2D_DrawSpriteTinted(icon, &tint);
}

void drawTabColorSwatch(float x, float y, float size, bool isActive, u32 color) {
    u32 borderCol = isActive ? C2D_Color32(0xFF, 0xFF, 0xFF, 0xFF) : C2D_Color32(0x80, 0x80, 0x80, 0xFF);
    C2D_DrawRectSolid(x - 2, y - 2, 0, size + 4, size + 4, borderCol);
    C2D_DrawRectSolid(x, y, 0, size, size, color);
}

void drawTabNumber(float x, float y, float size, bool isActive, int number) {
    u32 borderCol = isActive ? C2D_Color32(0xFF, 0xFF, 0xFF, 0xFF) : C2D_Color32(0x80, 0x80, 0x80, 0xFF);
    C2D_DrawRectSolid(x, y, 0, size, size, borderCol);
    C2D_DrawRectSolid(x + 2, y + 2, 0, size - 4, size - 4, C2D_Color32(0x40, 0x40, 0x40, 0xFF));
    if (g_uiTextBuf) {
        C2D_TextBufClear(g_uiTextBuf);
        char numStr[16];
        snprintf(numStr, sizeof(numStr), "%d", number);
        C2D_Text numText;
        C2D_TextParse(&numText, g_uiTextBuf, numStr);
        C2D_TextOptimize(&numText);
        float textWidth, textHeight;
        C2D_TextGetDimensions(&numText, 0.6f, 0.6f, &textWidth, &textHeight);
        float textX = x + size / 2 - textWidth / 2;
        float textY = y + size / 2 - textHeight / 2;
        C2D_DrawText(&numText, C2D_WithColor, textX, textY, 0, 0.6f, 0.6f, C2D_Color32(0xFF, 0xFF, 0xFF, 0xFF));
    }
}

void drawListItem(ListItemConfig* cfg) {
    C2D_DrawRectSolid(cfg->x, cfg->y, 0, cfg->width, cfg->height, cfg->bgColor);
    if (cfg->drawBorder) {
        C2D_DrawRectSolid(cfg->x, cfg->y, 0, cfg->width, 1, cfg->borderColor);
        C2D_DrawRectSolid(cfg->x, cfg->y + cfg->height - 1, 0, cfg->width, 1, cfg->borderColor);
        C2D_DrawRectSolid(cfg->x, cfg->y, 0, 1, cfg->height, cfg->borderColor);
        C2D_DrawRectSolid(cfg->x + cfg->width - 1, cfg->y, 0, 1, cfg->height, cfg->borderColor);
    }
    if (cfg->text && g_uiTextBuf) {
        C2D_TextBufClear(g_uiTextBuf);
        C2D_Text text;
        C2D_TextParse(&text, g_uiTextBuf, cfg->text);
        C2D_TextOptimize(&text);
        C2D_DrawText(&text, C2D_WithColor, cfg->textX, cfg->textY, 0, cfg->textScale, cfg->textScale, cfg->textColor);
    }
    if (cfg->rightIcon) {
        C2D_ImageTint tint;
        C2D_PlainImageTint(&tint, cfg->rightIconColor, 1.0f);
        C2D_SpriteSetPos(cfg->rightIcon, cfg->rightIconX, cfg->rightIconY);
        C2D_SpriteSetScale(cfg->rightIcon, cfg->rightIconScale, cfg->rightIconScale);
        C2D_DrawSpriteTinted(cfg->rightIcon, &tint);
    }
}

void drawClippedRect(float x, float y, float w, float h, float clipTop, float clipBottom, u32 color) {
    float top = (y < clipTop) ? clipTop : y;
    float bottom = (y + h > clipBottom) ? clipBottom : (y + h);
    float height = bottom - top;
    if (height > 0) {
        C2D_DrawRectSolid(x, top, 0, w, height, color);
    }
}

bool isButtonTouched(ButtonConfig* cfg, int touchX, int touchY) {
    if (cfg->isSkeleton) return false;
    float btnSize = (float)cfg->size;
    return (touchX >= cfg->x && touchX < cfg->x + btnSize &&
            touchY >= cfg->y && touchY < cfg->y + btnSize);
}

void drawSlider(SliderConfig* cfg) {
    float trackHeight = (cfg->height > 0) ? cfg->height : 8;
    float knobRadius = (cfg->knobRadius > 0) ? cfg->knobRadius : 10;
    float labelHeight = (cfg->label) ? 14 : 0;
    float trackY = cfg->y + labelHeight + knobRadius;

    float value = cfg->value;
    if (value < 0) value = 0;
    if (value > 1) value = 1;

    if (cfg->label && g_uiTextBuf) {
        C2D_TextBufClear(g_uiTextBuf);
        C2D_Text labelText;
        C2D_TextParse(&labelText, g_uiTextBuf, cfg->label);
        C2D_TextOptimize(&labelText);
        C2D_DrawText(&labelText, C2D_WithColor, cfg->x, cfg->y, 0, 0.4f, 0.4f, UI_COLOR_TEXT);
    }

    if (cfg->showPercent && g_uiTextBuf) {
        C2D_TextBufClear(g_uiTextBuf);
        char percentStr[16];
        snprintf(percentStr, sizeof(percentStr), "%d%%", (int)(value * 100 + 0.5f));
        C2D_Text percentText;
        C2D_TextParse(&percentText, g_uiTextBuf, percentStr);
        C2D_TextOptimize(&percentText);
        float textWidth, textHeight;
        C2D_TextGetDimensions(&percentText, 0.4f, 0.4f, &textWidth, &textHeight);
        C2D_DrawText(&percentText, C2D_WithColor, cfg->x + cfg->width - textWidth,
                     cfg->y, 0, 0.4f, 0.4f, UI_COLOR_WHITE);
    }

    C2D_DrawRectSolid(cfg->x, trackY - trackHeight / 2, 0, cfg->width, trackHeight, UI_COLOR_GRAY_2);
    C2D_DrawRectSolid(cfg->x, trackY - trackHeight / 2, 0, cfg->width * value, trackHeight, UI_COLOR_ACTIVE);

    float knobX = cfg->x + cfg->width * value;
    C2D_DrawCircleSolid(knobX, trackY, 0, knobRadius, UI_COLOR_WHITE);
    C2D_DrawCircleSolid(knobX, trackY, 0, knobRadius - 2, UI_COLOR_ACTIVE);
}
