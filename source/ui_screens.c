#include "ui_screens.h"

#include <stdio.h>

#include "app_state.h"
#include "color_utils.h"
#include "ui_components.h"
#include "ui_theme.h"

void initIcons(void) {
    iconSpriteSheet = C2D_SpriteSheetLoad("romfs:/gfx/icons.t3x");
    bannerSpriteSheet = C2D_SpriteSheetLoad("romfs:/gfx/banner.t3x");
    menuButtonBgSpriteSheet = C2D_SpriteSheetLoad("romfs:/gfx/menu_button_bg.t3x");
    guideSpriteSheet = C2D_SpriteSheetLoad("romfs:/gfx/guide.t3x");
    creditSpriteSheet = C2D_SpriteSheetLoad("romfs:/gfx/credit.t3x");

    C2D_SpriteFromSheet(&bucketIconSprite, iconSpriteSheet, 0);
    C2D_SpriteSetCenter(&bucketIconSprite, 0.5f, 0.5f);

    C2D_SpriteFromSheet(&wrenchIconSprite, iconSpriteSheet, 1);
    C2D_SpriteSetCenter(&wrenchIconSprite, 0.5f, 0.5f);

    C2D_SpriteFromSheet(&brushIconSprite, iconSpriteSheet, 2);
    C2D_SpriteSetCenter(&brushIconSprite, 0.5f, 0.5f);

    C2D_SpriteFromSheet(&eraserIconSprite, iconSpriteSheet, 3);
    C2D_SpriteSetCenter(&eraserIconSprite, 0.5f, 0.5f);

    C2D_SpriteFromSheet(&closeIconSprite, iconSpriteSheet, 4);
    C2D_SpriteSetCenter(&closeIconSprite, 0.5f, 0.5f);

    C2D_SpriteFromSheet(&plusIconSprite, iconSpriteSheet, 7);
    C2D_SpriteSetCenter(&plusIconSprite, 0.5f, 0.5f);

    C2D_SpriteFromSheet(&minusIconSprite, iconSpriteSheet, 6);
    C2D_SpriteSetCenter(&minusIconSprite, 0.5f, 0.5f);

    C2D_SpriteFromSheet(&eyeIconSprite, iconSpriteSheet, 9);
    C2D_SpriteSetCenter(&eyeIconSprite, 0.5f, 0.5f);

    C2D_SpriteFromSheet(&clearIconSprite, iconSpriteSheet, 8);
    C2D_SpriteSetCenter(&clearIconSprite, 0.5f, 0.5f);

    C2D_SpriteFromSheet(&upArrowIconSprite, iconSpriteSheet, 10);
    C2D_SpriteSetCenter(&upArrowIconSprite, 0.5f, 0.5f);

    C2D_SpriteFromSheet(&downArrowIconSprite, iconSpriteSheet, 11);
    C2D_SpriteSetCenter(&downArrowIconSprite, 0.5f, 0.5f);

    C2D_SpriteFromSheet(&mergeIconSprite, iconSpriteSheet, 12);
    C2D_SpriteSetCenter(&mergeIconSprite, 0.5f, 0.5f);

    C2D_SpriteFromSheet(&zoomOutIconSprite, iconSpriteSheet, 13);
    C2D_SpriteSetCenter(&zoomOutIconSprite, 0.5f, 0.5f);

    C2D_SpriteFromSheet(&zoomInIconSprite, iconSpriteSheet, 14);
    C2D_SpriteSetCenter(&zoomInIconSprite, 0.5f, 0.5f);

    C2D_SpriteFromSheet(&undoIconSprite, iconSpriteSheet, 15);
    C2D_SpriteSetCenter(&undoIconSprite, 0.5f, 0.5f);

    C2D_SpriteFromSheet(&redoIconSprite, iconSpriteSheet, 16);
    C2D_SpriteSetCenter(&redoIconSprite, 0.5f, 0.5f);

    C2D_SpriteFromSheet(&saveIconSprite, iconSpriteSheet, 17);
    C2D_SpriteSetCenter(&saveIconSprite, 0.5f, 0.5f);

    C2D_SpriteFromSheet(&saveAsIconSprite, iconSpriteSheet, 18);
    C2D_SpriteSetCenter(&saveAsIconSprite, 0.5f, 0.5f);

    C2D_SpriteFromSheet(&exportIconSprite, iconSpriteSheet, 19);
    C2D_SpriteSetCenter(&exportIconSprite, 0.5f, 0.5f);

    C2D_SpriteFromSheet(&palettePlusIconSprite, iconSpriteSheet, 20);
    C2D_SpriteSetCenter(&palettePlusIconSprite, 0.5f, 0.5f);

    C2D_SpriteFromSheet(&paletteMinusIconSprite, iconSpriteSheet, 21);
    C2D_SpriteSetCenter(&paletteMinusIconSprite, 0.5f, 0.5f);

    C2D_SpriteFromSheet(&clippingIconSprite, iconSpriteSheet, 23);
    C2D_SpriteSetCenter(&clippingIconSprite, 0.5f, 0.5f);

    C2D_SpriteFromSheet(&alphaLockIconSprite, iconSpriteSheet, 24);
    C2D_SpriteSetCenter(&alphaLockIconSprite, 0.5f, 0.5f);

    C2D_SpriteFromSheet(&pencilIconSprite, iconSpriteSheet, 25);
    C2D_SpriteSetCenter(&pencilIconSprite, 0.5f, 0.5f);

    C2D_SpriteFromSheet(&crossArrowIconSprite, iconSpriteSheet, 26);
    C2D_SpriteSetCenter(&crossArrowIconSprite, 0.5f, 0.5f);

    C2D_SpriteFromSheet(&checkIconSprite, iconSpriteSheet, 27);
    C2D_SpriteSetCenter(&checkIconSprite, 0.5f, 0.5f);

    C2D_SpriteFromSheet(&folderIconSprite, iconSpriteSheet, 28);
    C2D_SpriteSetCenter(&folderIconSprite, 0.5f, 0.5f);

    C2D_SpriteFromSheet(&settingsIconSprite, iconSpriteSheet, 29);
    C2D_SpriteSetCenter(&settingsIconSprite, 0.5f, 0.5f);

    C2D_SpriteFromSheet(&newFileIconSprite, iconSpriteSheet, 30);
    C2D_SpriteSetCenter(&newFileIconSprite, 0.5f, 0.5f);

    C2D_SpriteFromSheet(&backArrowIconSprite, iconSpriteSheet, 31);
    C2D_SpriteSetCenter(&backArrowIconSprite, 0.5f, 0.5f);

    if (bannerSpriteSheet) {
        C2D_SpriteFromSheet(&bannerSprite, bannerSpriteSheet, 0);
        C2D_SpriteSetCenter(&bannerSprite, 0.0f, 0.0f);
    }
    if (menuButtonBgSpriteSheet) {
        C2D_SpriteFromSheet(&menuButtonBgSprite, menuButtonBgSpriteSheet, 0);
        C2D_SpriteSetCenter(&menuButtonBgSprite, 0.0f, 0.0f);
    }
    if (guideSpriteSheet) {
        C2D_SpriteFromSheet(&guideSprite, guideSpriteSheet, 0);
        C2D_SpriteSetCenter(&guideSprite, 0.0f, 0.0f);
    }
    if (creditSpriteSheet) {
        C2D_SpriteFromSheet(&creditSprite, creditSpriteSheet, 0);
        C2D_SpriteSetCenter(&creditSprite, 0.0f, 0.0f);
    }
}

void exitIcons(void) {
    C2D_SpriteSheetFree(iconSpriteSheet);
    if (bannerSpriteSheet) {
        C2D_SpriteSheetFree(bannerSpriteSheet);
        bannerSpriteSheet = NULL;
    }
    if (menuButtonBgSpriteSheet) {
        C2D_SpriteSheetFree(menuButtonBgSpriteSheet);
        menuButtonBgSpriteSheet = NULL;
    }
    if (guideSpriteSheet) {
        C2D_SpriteSheetFree(guideSpriteSheet);
        guideSpriteSheet = NULL;
    }
    if (creditSpriteSheet) {
        C2D_SpriteSheetFree(creditSpriteSheet);
        creditSpriteSheet = NULL;
    }
}

void renderUI(C3D_RenderTarget* target) {
    C2D_TargetClear(target, C2D_Color32(0x30, 0x30, 0x30, 0xFF));
    C2D_SceneBegin(target);

    float scaleX = (float)TOP_SCREEN_WIDTH / CANVAS_WIDTH;
    float scaleY = (float)TOP_SCREEN_HEIGHT / CANVAS_HEIGHT;
    float scale = (scaleX < scaleY) ? scaleX : scaleY;

    float previewWidth = CANVAS_WIDTH * scale;
    float previewHeight = CANVAS_HEIGHT * scale;
    float previewX = (TOP_SCREEN_WIDTH - previewWidth) / 2;
    float previewY = (TOP_SCREEN_HEIGHT - previewHeight) / 2;

    C2D_DrawImageAt(canvasImage, previewX, previewY, 0, NULL, scale, scale);

    float drawX = canvasPanX + (BOTTOM_SCREEN_WIDTH - CANVAS_WIDTH * canvasZoom) / 2;
    float drawY = canvasPanY + (BOTTOM_SCREEN_HEIGHT - CANVAS_HEIGHT * canvasZoom) / 2;

    float viewLeft = -drawX / canvasZoom;
    float viewTop = -drawY / canvasZoom;
    float viewWidth = BOTTOM_SCREEN_WIDTH / canvasZoom;
    float viewHeight = BOTTOM_SCREEN_HEIGHT / canvasZoom;

    if (viewLeft < 0) viewLeft = 0;
    if (viewTop < 0) viewTop = 0;
    if (viewLeft + viewWidth > CANVAS_WIDTH) viewWidth = CANVAS_WIDTH - viewLeft;
    if (viewTop + viewHeight > CANVAS_HEIGHT) viewHeight = CANVAS_HEIGHT - viewTop;

    float rectX = previewX + viewLeft * scale;
    float rectY = previewY + viewTop * scale;
    float rectW = viewWidth * scale;
    float rectH = viewHeight * scale;

    u32 borderColor = C2D_Color32(0x00, 0x80, 0xFF, 0xFF);
    float borderThickness = 2.0f;

    C2D_DrawRectSolid(rectX, rectY, 0, rectW, borderThickness, borderColor);
    C2D_DrawRectSolid(rectX, rectY + rectH - borderThickness, 0, rectW, borderThickness, borderColor);
    C2D_DrawRectSolid(rectX, rectY, 0, borderThickness, rectH, borderColor);
    C2D_DrawRectSolid(rectX + rectW - borderThickness, rectY, 0, borderThickness, rectH, borderColor);

    if (brushSizeSliderActive) {
        float previewCenterX = previewX + previewWidth / 2;
        float previewCenterY = previewY + previewHeight / 2;
        float previewRadius = getCurrentBrushSize() * scale;

        C2D_DrawCircleSolid(previewCenterX, previewCenterY, 0, previewRadius + 2, C2D_Color32(0xFF, 0xFF, 0xFF, 0xFF));
        u8 curR = (currentColor >> 24) & 0xFF;
        u8 curG = (currentColor >> 16) & 0xFF;
        u8 curB = (currentColor >> 8) & 0xFF;
        C2D_DrawCircleSolid(previewCenterX, previewCenterY, 0, previewRadius, C2D_Color32(curR, curG, curB, brushAlpha));
    }

    float textScale = 0.4f;
    float lineHeight = 12.0f;
    u32 textColor = C2D_Color32(0xAA, 0xAA, 0xAA, 0xFF);
    char textBuf[64];
    C2D_Text text;
    float textWidth, textHeight;
    float rightMargin = 4.0f;

    float infoY = 4;

    C2D_TextBufClear(g_textBuf);
    snprintf(textBuf, sizeof(textBuf), "Canvas: %dx%d", CANVAS_WIDTH, CANVAS_HEIGHT);
    C2D_TextParse(&text, g_textBuf, textBuf);
    C2D_TextOptimize(&text);
    C2D_TextGetDimensions(&text, textScale, textScale, &textWidth, &textHeight);
    C2D_DrawText(&text, C2D_WithColor, TOP_SCREEN_WIDTH - textWidth - rightMargin, infoY, 0, textScale, textScale, textColor);
    infoY += lineHeight;

    C2D_TextBufClear(g_textBuf);
    snprintf(textBuf, sizeof(textBuf), "FPS: %.1f", currentFPS);
    C2D_TextParse(&text, g_textBuf, textBuf);
    C2D_TextOptimize(&text);
    C2D_TextGetDimensions(&text, textScale, textScale, &textWidth, &textHeight);
    C2D_DrawText(&text, C2D_WithColor, TOP_SCREEN_WIDTH - textWidth - rightMargin, infoY, 0, textScale, textScale, textColor);
    infoY += lineHeight;

    u32 memUsed = osGetMemRegionUsed(MEMREGION_ALL);
    u32 memTotal = osGetMemRegionSize(MEMREGION_ALL);
    C2D_TextBufClear(g_textBuf);
    snprintf(textBuf, sizeof(textBuf), "Mem: %.1f/%.1fMB", memUsed / (1024.0f * 1024.0f), memTotal / (1024.0f * 1024.0f));
    C2D_TextParse(&text, g_textBuf, textBuf);
    C2D_TextOptimize(&text);
    C2D_TextGetDimensions(&text, textScale, textScale, &textWidth, &textHeight);
    C2D_DrawText(&text, C2D_WithColor, TOP_SCREEN_WIDTH - textWidth - rightMargin, infoY, 0, textScale, textScale, textColor);
    infoY += lineHeight;

    C2D_TextBufClear(g_textBuf);
    snprintf(textBuf, sizeof(textBuf), "Zoom: x%.1f", canvasZoom);
    C2D_TextParse(&text, g_textBuf, textBuf);
    C2D_TextOptimize(&text);
    C2D_TextGetDimensions(&text, textScale, textScale, &textWidth, &textHeight);
    C2D_DrawText(&text, C2D_WithColor, TOP_SCREEN_WIDTH - textWidth - rightMargin, infoY, 0, textScale, textScale, textColor);

    float toolY = TOP_SCREEN_HEIGHT - 26;

    const char* toolName;
    switch (currentTool) {
        case TOOL_ERASER: toolName = "Eraser"; break;
        case TOOL_FILL: toolName = "Fill"; break;
        default: toolName = "Brush"; break;
    }
    C2D_TextBufClear(g_textBuf);
    snprintf(textBuf, sizeof(textBuf), "Tool: %s", toolName);
    C2D_TextParse(&text, g_textBuf, textBuf);
    C2D_TextOptimize(&text);
    C2D_TextGetDimensions(&text, textScale, textScale, &textWidth, &textHeight);
    C2D_DrawText(&text, C2D_WithColor, TOP_SCREEN_WIDTH - textWidth - rightMargin, toolY, 0, textScale, textScale, textColor);
    toolY += lineHeight;

    C2D_TextBufClear(g_textBuf);
    snprintf(textBuf, sizeof(textBuf), "Size: %d", getCurrentBrushSize());
    C2D_TextParse(&text, g_textBuf, textBuf);
    C2D_TextOptimize(&text);
    C2D_TextGetDimensions(&text, textScale, textScale, &textWidth, &textHeight);

    u8 r = (currentColor >> 24) & 0xFF;
    u8 g = (currentColor >> 16) & 0xFF;
    u8 b = (currentColor >> 8) & 0xFF;
    float colorBoxSize = 10;
    float colorBoxX = TOP_SCREEN_WIDTH - colorBoxSize - rightMargin;
    float colorBoxY = toolY + 1;
    C2D_DrawRectSolid(colorBoxX - 1, colorBoxY - 1, 0, colorBoxSize + 2, colorBoxSize + 2, C2D_Color32(0x80, 0x80, 0x80, 0xFF));
    C2D_DrawRectSolid(colorBoxX, colorBoxY, 0, colorBoxSize, colorBoxSize, C2D_Color32(r, g, b, 0xFF));

    C2D_DrawText(&text, C2D_WithColor, colorBoxX - textWidth - 8, toolY, 0, textScale, textScale, textColor);

    float layerX = 4;
    float layerY = TOP_SCREEN_HEIGHT - 26;

    C2D_TextBufClear(g_textBuf);
    snprintf(textBuf, sizeof(textBuf), "Layer: %d/%d", currentLayerIndex + 1, MAX_LAYERS);
    C2D_TextParse(&text, g_textBuf, textBuf);
    C2D_TextOptimize(&text);
    C2D_DrawText(&text, C2D_WithColor, layerX, layerY, 0, textScale, textScale, textColor);
    layerY += lineHeight;

    int opacityPercent = (int)(layers[currentLayerIndex].opacity * 100 / 255);
    const char* blendName;
    switch (layers[currentLayerIndex].blendMode) {
        case BLEND_ADD: blendName = "Add"; break;
        case BLEND_MULTIPLY: blendName = "Multiply"; break;
        default: blendName = "Normal"; break;
    }
    C2D_TextBufClear(g_textBuf);
    snprintf(textBuf, sizeof(textBuf), "%d%% / %s", opacityPercent, blendName);
    C2D_TextParse(&text, g_textBuf, textBuf);
    C2D_TextOptimize(&text);
    C2D_DrawText(&text, C2D_WithColor, layerX, layerY, 0, textScale, textScale, textColor);
}

void renderCanvas(C3D_RenderTarget* target, bool showOverlay) {
    C2D_TargetClear(target, C2D_Color32(0x80, 0x80, 0x80, 0xFF));
    C2D_SceneBegin(target);

    float drawX = canvasPanX + (BOTTOM_SCREEN_WIDTH - CANVAS_WIDTH * canvasZoom) / 2;
    float drawY = canvasPanY + (BOTTOM_SCREEN_HEIGHT - CANVAS_HEIGHT * canvasZoom) / 2;

    C2D_DrawImageAt(canvasImage, drawX, drawY, 0, NULL, canvasZoom, canvasZoom);

    if (showDrawMenuButton) {
        u32 menuBtnColor;
        switch (currentTool) {
            case TOOL_ERASER: menuBtnColor = C2D_Color32(0x80, 0x30, 0x30, 0xFF); break;
            case TOOL_FILL: menuBtnColor = C2D_Color32(0x30, 0x80, 0x30, 0xFF); break;
            default: menuBtnColor = C2D_Color32(0x30, 0x30, 0x80, 0xFF); break;
        }
        if (menuButtonBgSpriteSheet) {
            float scale = (float)DRAW_MENU_BTN_SIZE / 64.0f;
            C2D_SpriteSetPos(&menuButtonBgSprite, DRAW_MENU_BTN_X, DRAW_MENU_BTN_Y);
            C2D_SpriteSetScale(&menuButtonBgSprite, scale, scale);
            C2D_DrawSprite(&menuButtonBgSprite);
        } else {
            C2D_DrawRectSolid(DRAW_MENU_BTN_X, DRAW_MENU_BTN_Y, 0, DRAW_MENU_BTN_SIZE, DRAW_MENU_BTN_SIZE, C2D_Color32(0x30, 0x30, 0x30, 0xFF));
            C2D_DrawRectSolid(DRAW_MENU_BTN_X + 2, DRAW_MENU_BTN_Y + 2, 0, DRAW_MENU_BTN_SIZE - 4, DRAW_MENU_BTN_SIZE - 4, menuBtnColor);
        }

        C2D_ImageTint tint;
        C2D_PlainImageTint(&tint, C2D_Color32(0xFF, 0xFF, 0xFF, 0xFF), 1.0f);

        C2D_Sprite* toolIcon;
        switch (currentTool) {
            case TOOL_ERASER: toolIcon = &eraserIconSprite; break;
            case TOOL_FILL: toolIcon = &bucketIconSprite; break;
            default: toolIcon = &brushIconSprite; break;
        }
        C2D_SpriteSetPos(toolIcon, DRAW_MENU_BTN_X + DRAW_MENU_BTN_SIZE / 2, DRAW_MENU_BTN_Y + DRAW_MENU_BTN_SIZE / 2);
        C2D_SpriteSetScale(toolIcon, 0.3f, 0.3f);
        C2D_DrawSpriteTinted(toolIcon, &tint);
    }

    if (showOverlay) {
        C2D_DrawRectSolid(0, BOTTOM_SCREEN_HEIGHT - OVERLAY_BTN_SIZE - OVERLAY_MARGIN * 2, 0,
                          BOTTOM_SCREEN_WIDTH, OVERLAY_BTN_SIZE + OVERLAY_MARGIN * 2,
                          C2D_Color32(0x00, 0x00, 0x00, 0x80));

        float zoomOutX = OVERLAY_MARGIN;
        float zoomInX = OVERLAY_MARGIN + OVERLAY_BTN_SIZE + OVERLAY_MARGIN;
        float btnY = BOTTOM_SCREEN_HEIGHT - OVERLAY_BTN_SIZE - OVERLAY_MARGIN;

        ButtonConfig zoomOutBtn = {
            .x = zoomOutX,
            .y = btnY,
            .size = BTN_SIZE_MEDIUM,
            .icon = &zoomOutIconSprite,
            .label = NULL,
            .isActive = false,
            .isToggle = false,
            .isSkeleton = false,
            .useCustomColors = true,
            .bgColor = UI_COLOR_GRAY_3,
            .iconColor = UI_COLOR_WHITE,
            .labelColor = UI_COLOR_TEXT,
            .iconScale = 0.6f
        };
        drawButton(&zoomOutBtn);

        ButtonConfig zoomInBtn = {
            .x = zoomInX,
            .y = btnY,
            .size = BTN_SIZE_MEDIUM,
            .icon = &zoomInIconSprite,
            .label = NULL,
            .isActive = false,
            .isToggle = false,
            .isSkeleton = false,
            .useCustomColors = true,
            .bgColor = UI_COLOR_GRAY_3,
            .iconColor = UI_COLOR_WHITE,
            .labelColor = UI_COLOR_TEXT,
            .iconScale = 0.6f
        };
        drawButton(&zoomInBtn);

        C2D_TextBufClear(g_textBuf);
        char zoomStr[16];
        snprintf(zoomStr, sizeof(zoomStr), "x%.1f", canvasZoom);
        C2D_Text zoomText;
        C2D_TextParse(&zoomText, g_textBuf, zoomStr);
        C2D_TextOptimize(&zoomText);
        C2D_DrawText(&zoomText, C2D_WithColor, zoomInX + OVERLAY_BTN_SIZE + 8, btnY + OVERLAY_BTN_SIZE / 2 - 6, 0, 0.5f, 0.5f, C2D_Color32(0xFF, 0xFF, 0xFF, 0xFF));

        float redoX = BOTTOM_SCREEN_WIDTH - OVERLAY_MARGIN - OVERLAY_BTN_SIZE;
        float undoX = redoX - OVERLAY_MARGIN - OVERLAY_BTN_SIZE;

        ButtonConfig undoBtn = {
            .x = undoX,
            .y = btnY,
            .size = BTN_SIZE_MEDIUM,
            .icon = &undoIconSprite,
            .label = NULL,
            .isActive = false,
            .isToggle = false,
            .isSkeleton = false,
            .useCustomColors = true,
            .bgColor = UI_COLOR_GRAY_3,
            .iconColor = UI_COLOR_WHITE,
            .labelColor = UI_COLOR_TEXT,
            .iconScale = 0.6f
        };
        drawButton(&undoBtn);

        ButtonConfig redoBtn = {
            .x = redoX,
            .y = btnY,
            .size = BTN_SIZE_MEDIUM,
            .icon = &redoIconSprite,
            .label = NULL,
            .isActive = false,
            .isToggle = false,
            .isSkeleton = false,
            .useCustomColors = true,
            .bgColor = UI_COLOR_GRAY_3,
            .iconColor = UI_COLOR_WHITE,
            .labelColor = UI_COLOR_TEXT,
            .iconScale = 0.6f
        };
        drawButton(&redoBtn);
    }
}

void renderMenu(C3D_RenderTarget* target) {
    C2D_TargetClear(target, UI_COLOR_GRAY_1);
    C2D_SceneBegin(target);

    u32 tabBgColor = UI_COLOR_GRAY_2;
    u32 tabActiveColor = UI_COLOR_GRAY_4;
    for (int i = 0; i < 4; i++) {
        u32 bgCol = (i == currentMenuTab) ? tabActiveColor : tabBgColor;
        C2D_DrawRectSolid(i * MENU_TAB_WIDTH, 0, 0, MENU_TAB_WIDTH, MENU_TAB_HEIGHT, bgCol);

        if (i > 0) {
            C2D_DrawRectSolid(i * MENU_TAB_WIDTH, 5, 0, 1, MENU_TAB_HEIGHT - 10, UI_COLOR_GRAY_1);
        }
    }

    float tabCenterY = MENU_TAB_HEIGHT / 2;

    C2D_Sprite* currentToolIcon;
    switch (currentTool) {
        case TOOL_ERASER: currentToolIcon = &eraserIconSprite; break;
        case TOOL_FILL: currentToolIcon = &bucketIconSprite; break;
        default: currentToolIcon = &brushIconSprite; break;
    }
    drawTabIcon(MENU_TAB_WIDTH * 0.5f, tabCenterY, currentToolIcon, currentMenuTab == TAB_TOOL);

    drawTabIcon(MENU_TAB_WIDTH * 1.5f, tabCenterY, &wrenchIconSprite, currentMenuTab == TAB_BRUSH);

    {
        float colorTabX = MENU_TAB_WIDTH * 2 + (MENU_TAB_WIDTH - 20) / 2;
        float colorTabY = (MENU_TAB_HEIGHT - 20) / 2;
        u8 cr = (currentColor >> 24) & 0xFF;
        u8 cg = (currentColor >> 16) & 0xFF;
        u8 cb = (currentColor >> 8) & 0xFF;
        drawTabColorSwatch(colorTabX, colorTabY, 20, currentMenuTab == TAB_COLOR, C2D_Color32(cr, cg, cb, 0xFF));
    }

    {
        float layerTabX = MENU_TAB_WIDTH * 3 + (MENU_TAB_WIDTH - 24) / 2;
        float layerTabY = (MENU_TAB_HEIGHT - 24) / 2;
        drawTabNumber(layerTabX, layerTabY, 24, currentMenuTab == TAB_LAYER, currentLayerIndex + 1);
    }

    C2D_DrawRectSolid(currentMenuTab * MENU_TAB_WIDTH, MENU_TAB_HEIGHT - 3, 0, MENU_TAB_WIDTH, 3, UI_COLOR_ACTIVE);

    if (currentMenuTab == TAB_TOOL) {
        const int GRID_COLS = 4;
        const int GRID_ROWS = 2;
        const float GRID_GAP = 8;
        float gridStartX = (BOTTOM_SCREEN_WIDTH - (BTN_SIZE_LARGE * GRID_COLS + GRID_GAP * (GRID_COLS - 1))) / 2;
        float gridStartY = MENU_CONTENT_Y + 8;

        C2D_Sprite* toolIcons[3] = { &brushIconSprite, &eraserIconSprite, &bucketIconSprite };
        const char* toolLabels[3] = { "Brush", "Eraser", "Fill" };

        for (int row = 0; row < GRID_ROWS; row++) {
            for (int col = 0; col < GRID_COLS; col++) {
                int idx = row * GRID_COLS + col;
                float btnX = gridStartX + col * (BTN_SIZE_LARGE + GRID_GAP);
                float btnY = gridStartY + row * (BTN_SIZE_LARGE + GRID_GAP + 8);

                ButtonConfig btn = {
                    .x = btnX,
                    .y = btnY,
                    .size = BTN_SIZE_LARGE,
                    .icon = (idx < 3) ? toolIcons[idx] : NULL,
                    .label = (idx < 3) ? toolLabels[idx] : NULL,
                    .isActive = (idx < 3 && idx == currentTool),
                    .isToggle = false,
                    .isSkeleton = (idx >= 3)
                };
                drawButton(&btn);
            }
        }
    } else if (currentMenuTab == TAB_BRUSH) {
        if (currentTool == TOOL_FILL) {
            float settingsX = MENU_CONTENT_PADDING;
            float settingsY = MENU_CONTENT_Y + MENU_CONTENT_PADDING;
            float settingsWidth = BOTTOM_SCREEN_WIDTH - MENU_CONTENT_PADDING * 2;

            float knobRadius = 10;
            float sliderX = settingsX + knobRadius;
            float sliderY = settingsY;
            float sliderWidth = settingsWidth - knobRadius * 2;

            // --- Tolerance slider ---
            C2D_TextBufClear(g_textBuf);
            C2D_Text toleranceLabel;
            C2D_TextParse(&toleranceLabel, g_textBuf, "Tolerance");
            C2D_TextOptimize(&toleranceLabel);
            C2D_DrawText(&toleranceLabel, C2D_WithColor, settingsX, sliderY, 0, 0.4f, 0.4f, UI_COLOR_TEXT);

            C2D_TextBufClear(g_textBuf);
            char toleranceValBuf[8];
            snprintf(toleranceValBuf, sizeof(toleranceValBuf), "%d%%", fillTolerance);
            C2D_Text toleranceVal;
            C2D_TextParse(&toleranceVal, g_textBuf, toleranceValBuf);
            C2D_TextOptimize(&toleranceVal);
            float toleranceValWidth, toleranceValHeight;
            C2D_TextGetDimensions(&toleranceVal, 0.4f, 0.4f, &toleranceValWidth, &toleranceValHeight);
            C2D_DrawText(&toleranceVal, C2D_WithColor, settingsX + settingsWidth - toleranceValWidth, sliderY, 0, 0.4f, 0.4f, UI_COLOR_WHITE);

            float toleranceRatio = (float)fillTolerance / FILL_TOLERANCE_MAX;
            SliderConfig toleranceSlider = {
                .x = sliderX,
                .y = sliderY + 14,
                .width = sliderWidth,
                .height = 8,
                .knobRadius = knobRadius,
                .value = toleranceRatio,
                .label = NULL,
                .showPercent = false
            };
            drawSlider(&toleranceSlider);

            // --- Expand slider (shifted down) ---
            float expandSliderY = sliderY + 14 + knobRadius * 2 + 12;

            C2D_TextBufClear(g_textBuf);
            C2D_Text expandLabel;
            C2D_TextParse(&expandLabel, g_textBuf, "Expand");
            C2D_TextOptimize(&expandLabel);
            C2D_DrawText(&expandLabel, C2D_WithColor, settingsX, expandSliderY, 0, 0.4f, 0.4f, UI_COLOR_TEXT);

            C2D_TextBufClear(g_textBuf);
            char expandValBuf[8];
            snprintf(expandValBuf, sizeof(expandValBuf), "%d", fillExpand);
            C2D_Text expandVal;
            C2D_TextParse(&expandVal, g_textBuf, expandValBuf);
            C2D_TextOptimize(&expandVal);
            float expandValWidth, expandValHeight;
            C2D_TextGetDimensions(&expandVal, 0.4f, 0.4f, &expandValWidth, &expandValHeight);
            C2D_DrawText(&expandVal, C2D_WithColor, settingsX + settingsWidth - expandValWidth, expandSliderY, 0, 0.4f, 0.4f, UI_COLOR_WHITE);

            float expandFillRatio = (float)fillExpand / FILL_EXPAND_MAX;
            SliderConfig expandSlider = {
                .x = sliderX,
                .y = expandSliderY + 14,
                .width = sliderWidth,
                .height = 8,
                .knobRadius = knobRadius,
                .value = expandFillRatio,
                .label = NULL,
                .showPercent = false
            };
            drawSlider(&expandSlider);
        } else {
            float listX = MENU_CONTENT_PADDING;
            float listY = MENU_CONTENT_Y + MENU_CONTENT_PADDING;
            float listWidth = 150;
            float listHeight = MENU_BTN_Y - listY - MENU_CONTENT_PADDING;
            float itemHeight = 40;

            float settingsX = listX + listWidth + MENU_CONTENT_PADDING;
            float settingsY = MENU_CONTENT_Y + MENU_CONTENT_PADDING;
            float settingsWidth = BOTTOM_SCREEN_WIDTH - settingsX - MENU_CONTENT_PADDING;

            C2D_DrawRectSolid(listX, listY, 0, listWidth, listHeight, UI_COLOR_GRAY_1);

            float totalContentHeight = NUM_BRUSHES * itemHeight;
            float maxScroll = totalContentHeight - listHeight;
            if (maxScroll < 0) maxScroll = 0;
            if (brushListScrollY < 0) brushListScrollY = 0;
            if (brushListScrollY > maxScroll) brushListScrollY = maxScroll;

            for (int i = 0; i < (int)NUM_BRUSHES; i++) {
                float itemY = listY + i * itemHeight - brushListScrollY;

                if (itemY + itemHeight <= listY || itemY >= listY + listHeight) continue;

                float clipTop = (itemY < listY) ? listY : itemY;
                float clipBottom = (itemY + itemHeight > listY + listHeight) ? listY + listHeight : itemY + itemHeight;
                float clipHeight = clipBottom - clipTop;

                if (clipHeight <= 0) continue;

                u32 itemBg = (i == currentBrushType) ? UI_COLOR_ACTIVE : UI_COLOR_GRAY_3;
                float textScale = 0.55f;
                float textX = listX + 10;
                float textY = itemY + itemHeight / 2 - 6;
                bool fullyVisible = (itemY >= listY && itemY + itemHeight <= listY + listHeight);

                if (fullyVisible) {
                    char itemLabel[64];
                    snprintf(itemLabel, sizeof(itemLabel), "%s", brushDefs[i].name);
                    ListItemConfig item = {
                        .x = listX + 2,
                        .y = itemY + 2,
                        .width = listWidth - 4,
                        .height = itemHeight - 4,
                        .bgColor = itemBg,
                        .drawBorder = false,
                        .borderColor = 0,
                        .text = itemLabel,
                        .textX = textX,
                        .textY = textY,
                        .textScale = textScale,
                        .textColor = UI_COLOR_WHITE,
                        .rightIcon = NULL,
                        .rightIconX = 0,
                        .rightIconY = 0,
                        .rightIconScale = 0.0f,
                        .rightIconColor = UI_COLOR_WHITE
                    };
                    drawListItem(&item);
                } else {
                    float bgTop = (itemY + 2 < listY) ? listY : itemY + 2;
                    float bgBottom = (itemY + itemHeight - 2 > listY + listHeight) ? listY + listHeight : itemY + itemHeight - 2;
                    if (bgBottom > bgTop) {
                        drawClippedRect(listX + 2, bgTop, listWidth - 4, bgBottom - bgTop, listY, listY + listHeight, itemBg);
                    }

                    char itemLabel[64];
                    snprintf(itemLabel, sizeof(itemLabel), "%s", brushDefs[i].name);
                    C2D_TextBufClear(g_textBuf);
                    C2D_Text nameText;
                    C2D_TextParse(&nameText, g_textBuf, itemLabel);
                    C2D_TextOptimize(&nameText);
                    C2D_DrawText(&nameText, C2D_WithColor, textX, textY, 0, textScale, textScale, UI_COLOR_WHITE);
                }
            }

            C2D_DrawRectSolid(listX, listY, 0, listWidth, 2, UI_COLOR_GRAY_3);
            C2D_DrawRectSolid(listX, listY + listHeight - 2, 0, listWidth, 2, UI_COLOR_GRAY_3);
            C2D_DrawRectSolid(listX, listY, 0, 2, listHeight, UI_COLOR_GRAY_3);
            C2D_DrawRectSolid(listX + listWidth - 2, listY, 0, 2, listHeight, UI_COLOR_GRAY_3);

            char brushNameBuf[64];
            snprintf(brushNameBuf, sizeof(brushNameBuf), "%s", brushDefs[currentBrushType].name);
            C2D_TextBufClear(g_textBuf);
            C2D_Text brushNameText;
            C2D_TextParse(&brushNameText, g_textBuf, brushNameBuf);
            C2D_TextOptimize(&brushNameText);
            C2D_DrawText(&brushNameText, C2D_WithColor, settingsX, settingsY, 0, 0.6f, 0.6f, C2D_Color32(0xFF, 0xFF, 0xFF, 0xFF));

            float sliderX = settingsX;
            float sliderY = settingsY + 25;
            float sliderWidth = settingsWidth - 10;

            int minSize = 1;
            int maxSize = 16;
            float fillRatio = (float)(getCurrentBrushSize() - minSize) / (maxSize - minSize);

            C2D_TextBufClear(g_textBuf);
            C2D_Text sizeLabel;
            C2D_TextParse(&sizeLabel, g_textBuf, "Size");
            C2D_TextOptimize(&sizeLabel);
            C2D_DrawText(&sizeLabel, C2D_WithColor, sliderX, sliderY, 0, 0.4f, 0.4f, UI_COLOR_TEXT);

            C2D_TextBufClear(g_textBuf);
            char sizeValBuf[8];
            snprintf(sizeValBuf, sizeof(sizeValBuf), "%d", getCurrentBrushSize());
            C2D_Text sizeVal;
            C2D_TextParse(&sizeVal, g_textBuf, sizeValBuf);
            C2D_TextOptimize(&sizeVal);
            float sizeValWidth, sizeValHeight;
            C2D_TextGetDimensions(&sizeVal, 0.4f, 0.4f, &sizeValWidth, &sizeValHeight);
            C2D_DrawText(&sizeVal, C2D_WithColor, sliderX + sliderWidth - sizeValWidth, sliderY, 0, 0.4f, 0.4f, UI_COLOR_WHITE);

            SliderConfig sizeSlider = {
                .x = sliderX,
                .y = sliderY + 14,
                .width = sliderWidth,
                .height = 8,
                .knobRadius = 10,
                .value = fillRatio,
                .label = NULL,
                .showPercent = false
            };
            drawSlider(&sizeSlider);
        }
    } else if (currentMenuTab == TAB_COLOR) {
        int svBlockSize = 5;
        for (int sy = 0; sy < MCP_SV_SIZE; sy += svBlockSize) {
            for (int sx = 0; sx < MCP_SV_SIZE; sx += svBlockSize) {
                float s = (float)sx / (MCP_SV_SIZE - 1);
                float v = 1.0f - (float)sy / (MCP_SV_SIZE - 1);
                u32 col = hsvToRgb(currentHue, s, v);
                u8 r = (col >> 24) & 0xFF;
                u8 g = (col >> 16) & 0xFF;
                u8 b = (col >> 8) & 0xFF;
                C2D_DrawRectSolid(MCP_SV_X + sx, MCP_SV_Y + sy, 0, svBlockSize, svBlockSize, C2D_Color32(r, g, b, 0xFF));
            }
        }

        int svCursorX = MCP_SV_X + (int)(currentSaturation * (MCP_SV_SIZE - 1));
        int svCursorY = MCP_SV_Y + (int)((1.0f - currentValue) * (MCP_SV_SIZE - 1));
        C2D_DrawCircleSolid(svCursorX, svCursorY, 0, 6, C2D_Color32(0x00, 0x00, 0x00, 0xFF));
        C2D_DrawCircleSolid(svCursorX, svCursorY, 0, 4, C2D_Color32(0xFF, 0xFF, 0xFF, 0xFF));

        int hueBlockSize = 5;
        for (int hy = 0; hy < MCP_HUE_HEIGHT; hy += hueBlockSize) {
            float h = (float)hy / (MCP_HUE_HEIGHT - 1) * 360.0f;
            u32 col = hsvToRgb(h, 1.0f, 1.0f);
            u8 r = (col >> 24) & 0xFF;
            u8 g = (col >> 16) & 0xFF;
            u8 b = (col >> 8) & 0xFF;
            C2D_DrawRectSolid(MCP_HUE_X, MCP_HUE_Y + hy, 0, MCP_HUE_WIDTH, hueBlockSize, C2D_Color32(r, g, b, 0xFF));
        }

        int hueCursorY = MCP_HUE_Y + (int)(currentHue / 360.0f * (MCP_HUE_HEIGHT - 1));
        C2D_DrawRectSolid(MCP_HUE_X - 3, hueCursorY - 2, 0, MCP_HUE_WIDTH + 6, 5, C2D_Color32(0xFF, 0xFF, 0xFF, 0xFF));
        C2D_DrawRectSolid(MCP_HUE_X - 2, hueCursorY - 1, 0, MCP_HUE_WIDTH + 4, 3, C2D_Color32(0x00, 0x00, 0x00, 0xFF));

        float alphaRatio = brushAlpha / 255.0f;
        SliderConfig alphaSlider = {
            .x = MCP_ALPHA_SLIDER_X,
            .y = MCP_ALPHA_SLIDER_Y,
            .width = MCP_ALPHA_SLIDER_WIDTH,
            .height = 8,
            .knobRadius = 10,
            .value = alphaRatio,
            .label = "Alpha",
            .showPercent = true
        };
        drawSlider(&alphaSlider);

        float paletteStartX = 148;
        float paletteStartY = MENU_CONTENT_Y + MENU_CONTENT_PADDING;

        for (int row = 0; row < PALETTE_ROWS; row++) {
            for (int col = 0; col < PALETTE_COLS; col++) {
                int idx = row * PALETTE_COLS + col;
                float cellX = paletteStartX + col * (PALETTE_CELL_SIZE + PALETTE_SPACING);
                float cellY = paletteStartY + row * (PALETTE_CELL_SIZE + PALETTE_SPACING);

                if (paletteUsed[idx]) {
                    u8 r = (paletteColors[idx] >> 24) & 0xFF;
                    u8 g = (paletteColors[idx] >> 16) & 0xFF;
                    u8 b = (paletteColors[idx] >> 8) & 0xFF;
                    C2D_DrawRectSolid(cellX, cellY, 0, PALETTE_CELL_SIZE, PALETTE_CELL_SIZE, C2D_Color32(r, g, b, 0xFF));

                    u32 borderColor = paletteDeleteMode ? C2D_Color32(0xFF, 0x50, 0x50, 0xFF) : UI_COLOR_GRAY_3;
                    C2D_DrawRectSolid(cellX, cellY, 0, PALETTE_CELL_SIZE, 1, borderColor);
                    C2D_DrawRectSolid(cellX, cellY + PALETTE_CELL_SIZE - 1, 0, PALETTE_CELL_SIZE, 1, borderColor);
                    C2D_DrawRectSolid(cellX, cellY, 0, 1, PALETTE_CELL_SIZE, borderColor);
                    C2D_DrawRectSolid(cellX + PALETTE_CELL_SIZE - 1, cellY, 0, 1, PALETTE_CELL_SIZE, borderColor);
                } else {
                    C2D_DrawRectSolid(cellX, cellY, 0, PALETTE_CELL_SIZE, PALETTE_CELL_SIZE, UI_COLOR_GRAY_2);
                    C2D_DrawRectSolid(cellX, cellY, 0, PALETTE_CELL_SIZE, 1, C2D_Color32(0x60, 0x60, 0x60, 0xFF));
                    C2D_DrawRectSolid(cellX, cellY + PALETTE_CELL_SIZE - 1, 0, PALETTE_CELL_SIZE, 1, C2D_Color32(0x60, 0x60, 0x60, 0xFF));
                    C2D_DrawRectSolid(cellX, cellY, 0, 1, PALETTE_CELL_SIZE, C2D_Color32(0x60, 0x60, 0x60, 0xFF));
                    C2D_DrawRectSolid(cellX + PALETTE_CELL_SIZE - 1, cellY, 0, 1, PALETTE_CELL_SIZE, C2D_Color32(0x60, 0x60, 0x60, 0xFF));
                }
            }
        }

        float btnY = paletteStartY + PALETTE_ROWS * (PALETTE_CELL_SIZE + PALETTE_SPACING) + 2;
        float btnWidth = 24;
        float btnHeight = 24;
        float previewSize = 32;
        float btnSpacing = PALETTE_SPACING;
        float paletteWidth = PALETTE_COLS * PALETTE_CELL_SIZE + (PALETTE_COLS - 1) * PALETTE_SPACING;
        float paletteRightX = paletteStartX + paletteWidth;

        u8 previewR = (currentColor >> 24) & 0xFF;
        u8 previewG = (currentColor >> 16) & 0xFF;
        u8 previewB = (currentColor >> 8) & 0xFF;
        float previewX = paletteStartX + 2;
        float previewY = btnY;
        float previewInnerX = previewX;
        float previewInnerY = previewY + 2;

        C2D_DrawRectSolid(paletteStartX, previewY, 0, previewSize + 4, previewSize + 4, C2D_Color32(0xFF, 0xFF, 0xFF, 0xFF));

        int checkSize = 4;
        for (int cy = 0; cy < previewSize; cy += checkSize) {
            for (int cx = 0; cx < previewSize; cx += checkSize) {
                bool isLight = ((cx / checkSize) + (cy / checkSize)) % 2 == 0;
                u32 checkColor = isLight ? C2D_Color32(0xCC, 0xCC, 0xCC, 0xFF) : C2D_Color32(0x88, 0x88, 0x88, 0xFF);
                float drawW = (cx + checkSize > previewSize) ? (previewSize - cx) : checkSize;
                float drawH = (cy + checkSize > previewSize) ? (previewSize - cy) : checkSize;
                C2D_DrawRectSolid(previewInnerX + cx, previewInnerY + cy, 0, drawW, drawH, checkColor);
            }
        }

        C2D_DrawRectSolid(previewInnerX, previewInnerY, 0, previewSize, previewSize, C2D_Color32(previewR, previewG, previewB, brushAlpha));

        float delBtnX = paletteRightX - btnWidth;
        float addBtnX = delBtnX - btnWidth - btnSpacing;
        u32 addBtnColor = paletteDeleteMode ? UI_COLOR_GRAY_3 : UI_COLOR_ACTIVE;
        RectButtonConfig addBtn = {
            .x = addBtnX,
            .y = btnY,
            .width = btnWidth,
            .height = btnHeight,
            .drawBackground = true,
            .bgColor = addBtnColor,
            .drawTopBorder = false,
            .drawBottomBorder = false,
            .borderTopColor = 0,
            .borderBottomColor = 0,
            .icon = &palettePlusIconSprite,
            .iconScale = 0.3f,
            .iconColor = UI_COLOR_WHITE,
            .text = NULL,
            .textScale = 0.0f,
            .textColor = UI_COLOR_WHITE
        };
        drawRectButton(&addBtn);

        u32 delBtnColor = paletteDeleteMode ? C2D_Color32(0xAA, 0x00, 0x00, 0xFF) : UI_COLOR_GRAY_3;
        RectButtonConfig delBtn = {
            .x = delBtnX,
            .y = btnY,
            .width = btnWidth,
            .height = btnHeight,
            .drawBackground = true,
            .bgColor = delBtnColor,
            .drawTopBorder = false,
            .drawBottomBorder = false,
            .borderTopColor = 0,
            .borderBottomColor = 0,
            .icon = &paletteMinusIconSprite,
            .iconScale = 0.3f,
            .iconColor = UI_COLOR_WHITE,
            .text = NULL,
            .textScale = 0.0f,
            .textColor = UI_COLOR_WHITE
        };
        drawRectButton(&delBtn);
    } else if (currentMenuTab == TAB_LAYER) {
        float listStartY = MENU_CONTENT_Y + MENU_CONTENT_PADDING;
        float listItemSpacing = 2;
        float listBottomY = MENU_BTN_Y - MENU_CONTENT_PADDING;
        float listHeight = listBottomY - listStartY;
        float listItemHeight = (listHeight - listItemSpacing * (MAX_LAYERS - 1)) / MAX_LAYERS;
        if (listItemHeight < 1) listItemHeight = 1;
        float listItemWidth = 150;
        float listX = MENU_CONTENT_PADDING;
        float eyeBtnSize = 28;

        for (int i = MAX_LAYERS - 1; i >= 0; i--) {
            int displayIdx = MAX_LAYERS - 1 - i;
            float itemY = listStartY + displayIdx * (listItemHeight + listItemSpacing);
            float clipIndent = layers[i].clipping ? 6.0f : 0.0f;
            float itemX = listX + clipIndent;
            float itemWidth = listItemWidth - clipIndent;
            float eyeBtnX = itemX + itemWidth - eyeBtnSize - 4;

            if (itemY + listItemHeight > listBottomY) continue;

            u32 bgColor = (i == currentLayerIndex) ? UI_COLOR_ACTIVE_DARK : UI_COLOR_GRAY_2;
            u32 borderColor = (i == currentLayerIndex) ? UI_COLOR_ACTIVE : UI_COLOR_GRAY_3;

            char layerNumStr[16];
            snprintf(layerNumStr, sizeof(layerNumStr), "%d", i + 1);
            const char* layerLabel = (layers[i].name[0] != '\0') ? layers[i].name : layerNumStr;

            float textWidth, textHeight;
            C2D_TextBufClear(g_textBuf);
            C2D_Text layerText;
            C2D_TextParse(&layerText, g_textBuf, layerLabel);
            C2D_TextOptimize(&layerText);
            C2D_TextGetDimensions(&layerText, 0.5f, 0.5f, &textWidth, &textHeight);
            float textX = itemX + 6;
            float textY = itemY + (listItemHeight - textHeight) / 2;

            float eyeIconX = eyeBtnX + eyeBtnSize / 2;
            float eyeIconY = itemY + listItemHeight / 2;

            ListItemConfig layerItem = {
                .x = itemX,
                .y = itemY,
                .width = itemWidth,
                .height = listItemHeight,
                .bgColor = bgColor,
                .drawBorder = true,
                .borderColor = borderColor,
                .text = layerLabel,
                .textX = textX,
                .textY = textY,
                .textScale = 0.5f,
                .textColor = UI_COLOR_WHITE,
                .rightIcon = &eyeIconSprite,
                .rightIconX = eyeIconX,
                .rightIconY = eyeIconY,
                .rightIconScale = 0.45f,
                .rightIconColor = layers[i].visible ? UI_COLOR_WHITE : UI_COLOR_DISABLED
            };
            drawListItem(&layerItem);
        }

        float opX = listX + listItemWidth + MENU_CONTENT_PADDING;
        float opY = listStartY;
        float opBtnSize = 32;
        float opBtnSpacing = 4;
        float rightMargin = MENU_CONTENT_PADDING + 10;
        float sliderWidth = BOTTOM_SCREEN_WIDTH - opX - rightMargin;

        float col1X = opX;
        float col2X = col1X + opBtnSize + opBtnSpacing;
        float col3X = col2X + opBtnSize + opBtnSpacing;
        float col4X = col3X + opBtnSize + opBtnSpacing;

        ButtonConfig upBtn = {
            .x = col1X, .y = opY, .size = opBtnSize,
            .icon = &upArrowIconSprite, .label = NULL,
            .isActive = false, .isToggle = false, .isSkeleton = false,
            .useCustomColors = true, .bgColor = UI_COLOR_GRAY_3,
            .iconColor = (currentLayerIndex >= MAX_LAYERS - 1) ? UI_COLOR_DISABLED : UI_COLOR_WHITE,
            .labelColor = UI_COLOR_TEXT, .iconScale = 0.4f
        };
        drawButton(&upBtn);

        ButtonConfig downBtn = {
            .x = col2X, .y = opY, .size = opBtnSize,
            .icon = &downArrowIconSprite, .label = NULL,
            .isActive = false, .isToggle = false, .isSkeleton = false,
            .useCustomColors = true, .bgColor = UI_COLOR_GRAY_3,
            .iconColor = (currentLayerIndex <= 0) ? UI_COLOR_DISABLED : UI_COLOR_WHITE,
            .labelColor = UI_COLOR_TEXT, .iconScale = 0.4f
        };
        drawButton(&downBtn);

        ButtonConfig mergeBtn = {
            .x = col3X, .y = opY, .size = opBtnSize,
            .icon = &mergeIconSprite, .label = NULL,
            .isActive = false, .isToggle = false, .isSkeleton = false,
            .useCustomColors = true, .bgColor = UI_COLOR_GRAY_3,
            .iconColor = (currentLayerIndex <= 0) ? UI_COLOR_DISABLED : UI_COLOR_WHITE,
            .labelColor = UI_COLOR_TEXT, .iconScale = 0.4f
        };
        drawButton(&mergeBtn);

        ButtonConfig clearBtn = {
            .x = col4X, .y = opY, .size = opBtnSize,
            .icon = &clearIconSprite, .label = NULL,
            .isActive = false, .isToggle = false, .isSkeleton = false,
            .useCustomColors = true, .bgColor = UI_COLOR_GRAY_3,
            .iconColor = UI_COLOR_WHITE,
            .labelColor = UI_COLOR_TEXT, .iconScale = 0.4f
        };
        drawButton(&clearBtn);

        float row2Y = opY + opBtnSize + opBtnSpacing;

        ButtonConfig alphaLockBtn = {
            .x = col1X, .y = row2Y, .size = opBtnSize,
            .icon = &alphaLockIconSprite, .label = NULL,
            .isActive = layers[currentLayerIndex].alphaLock,
            .isToggle = true, .isSkeleton = false,
            .useCustomColors = true,
            .bgColor = layers[currentLayerIndex].alphaLock ? UI_COLOR_ACTIVE : UI_COLOR_GRAY_3,
            .iconColor = UI_COLOR_WHITE,
            .labelColor = UI_COLOR_TEXT, .iconScale = 0.4f
        };
        drawButton(&alphaLockBtn);

        ButtonConfig clippingBtn = {
            .x = col2X, .y = row2Y, .size = opBtnSize,
            .icon = &clippingIconSprite, .label = NULL,
            .isActive = (currentLayerIndex > 0) ? layers[currentLayerIndex].clipping : false,
            .isToggle = (currentLayerIndex > 0), .isSkeleton = false,
            .useCustomColors = true,
            .bgColor = (currentLayerIndex > 0 && layers[currentLayerIndex].clipping) ? UI_COLOR_ACTIVE : UI_COLOR_GRAY_3,
            .iconColor = (currentLayerIndex > 0) ? UI_COLOR_WHITE : UI_COLOR_DISABLED,
            .labelColor = UI_COLOR_TEXT, .iconScale = 0.4f
        };
        drawButton(&clippingBtn);

        ButtonConfig renameBtn = {
            .x = col3X, .y = row2Y, .size = opBtnSize,
            .icon = &pencilIconSprite, .label = NULL,
            .isActive = false, .isToggle = false, .isSkeleton = false,
            .useCustomColors = true, .bgColor = UI_COLOR_GRAY_3,
            .iconColor = UI_COLOR_WHITE,
            .labelColor = UI_COLOR_TEXT, .iconScale = 0.4f
        };
        drawButton(&renameBtn);

        float sliderY = row2Y + opBtnSize + 10;
        float sliderX = opX;
        float opacityRatio = layers[currentLayerIndex].opacity / 255.0f;

        SliderConfig opacitySlider = {
            .x = sliderX,
            .y = sliderY,
            .width = sliderWidth,
            .height = 8,
            .knobRadius = 10,
            .value = opacityRatio,
            .label = "Opacity",
            .showPercent = true
        };
        drawSlider(&opacitySlider);

        float blendBtnY = sliderY + 45;
        float blendBtnWidth = BOTTOM_SCREEN_WIDTH - MENU_CONTENT_PADDING - sliderX;
        float blendBtnHeight = 24;

        const char* blendModeName;
        switch (layers[currentLayerIndex].blendMode) {
            case BLEND_ADD: blendModeName = "Add"; break;
            case BLEND_MULTIPLY: blendModeName = "Multiply"; break;
            default: blendModeName = "Normal"; break;
        }
        RectButtonConfig blendBtn = {
            .x = sliderX,
            .y = blendBtnY,
            .width = blendBtnWidth,
            .height = blendBtnHeight,
            .drawBackground = true,
            .bgColor = UI_COLOR_GRAY_3,
            .drawTopBorder = false,
            .drawBottomBorder = false,
            .borderTopColor = 0,
            .borderBottomColor = 0,
            .icon = NULL,
            .iconScale = 0.0f,
            .iconColor = UI_COLOR_WHITE,
            .text = blendModeName,
            .textScale = 0.5f,
            .textColor = UI_COLOR_WHITE
        };
        drawRectButton(&blendBtn);
    }

    C2D_DrawRectSolid(0, MENU_BOTTOM_BAR_Y, 0, BOTTOM_SCREEN_WIDTH, MENU_BOTTOM_BAR_HEIGHT, UI_COLOR_GRAY_2);

    C2D_ImageTint tint;
    C2D_PlainImageTint(&tint, UI_COLOR_WHITE, 1.0f);
    C2D_SpriteSetPos(&closeIconSprite, MENU_BTN_X + MENU_BTN_SIZE / 2, MENU_BTN_Y + MENU_BTN_SIZE / 2);
    C2D_SpriteSetScale(&closeIconSprite, 0.3f, 0.3f);
    C2D_DrawSpriteTinted(&closeIconSprite, &tint);

    C2D_DrawRectSolid(MENU_BTN_SIZE, MENU_BOTTOM_BAR_Y + 4, 0, 1, MENU_BOTTOM_BAR_HEIGHT - 8, UI_COLOR_GRAY_3);

    RectButtonConfig saveExitBtn = {
        .x = SAVE_EXIT_BTN_X,
        .y = SAVE_EXIT_BTN_Y,
        .width = SAVE_EXIT_BTN_WIDTH,
        .height = MENU_BOTTOM_BAR_HEIGHT,
        .drawBackground = false,
        .bgColor = 0,
        .drawTopBorder = false,
        .drawBottomBorder = false,
        .borderTopColor = 0,
        .borderBottomColor = 0,
        .icon = NULL,
        .iconScale = 0.0f,
        .iconColor = UI_COLOR_WHITE,
        .text = "Save / Exit",
        .textScale = 0.5f,
        .textColor = UI_COLOR_WHITE
    };
    drawRectButton(&saveExitBtn);
}

void renderHomeMenu(C3D_RenderTarget* target) {
    C2D_TargetClear(target, UI_COLOR_GRAY_1);
    C2D_SceneBegin(target);

    float totalWidth = BTN_SIZE_LARGE * 3 + SAVE_MENU_ITEM_SPACING * 2;
    float startX = (BOTTOM_SCREEN_WIDTH - totalWidth) / 2;
    float itemY = SAVE_MENU_Y;

    C2D_Sprite* homeIcons[3] = { &newFileIconSprite, &folderIconSprite, &settingsIconSprite };
    const char* homeLabels[3] = { "New", "Open", "Settings" };

    for (int i = 0; i < 3; i++) {
        float btnX = startX + i * (BTN_SIZE_LARGE + SAVE_MENU_ITEM_SPACING);

        ButtonConfig btn = {
            .x = btnX,
            .y = itemY,
            .size = BTN_SIZE_LARGE,
            .icon = homeIcons[i],
            .label = homeLabels[i],
            .isActive = true,
            .isToggle = false,
            .isSkeleton = false
        };
        drawButton(&btn);
    }

    // Display credit sprite at bottom center
    if (creditSpriteSheet) {
        float creditW = 296.0f;
        float creditH = 18.0f;
        float drawX = (BOTTOM_SCREEN_WIDTH - creditW) / 2.0f;
        float drawY = BOTTOM_SCREEN_HEIGHT - creditH - 10.0f;
        C2D_SpriteSetPos(&creditSprite, drawX, drawY);
        C2D_SpriteSetScale(&creditSprite, 1.0f, 1.0f);
        C2D_DrawSprite(&creditSprite);
    }
}

void renderSettingsMenu(C3D_RenderTarget* target) {
    C2D_TargetClear(target, UI_COLOR_GRAY_1);
    C2D_SceneBegin(target);

    C2D_TextBufClear(g_textBuf);
    C2D_Text title;
    C2D_TextParse(&title, g_textBuf, "Settings");
    C2D_TextOptimize(&title);
    float tw, th;
    C2D_TextGetDimensions(&title, 0.6f, 0.6f, &tw, &th);
    C2D_DrawText(&title, C2D_WithColor, (BOTTOM_SCREEN_WIDTH - tw) / 2, 12, 0, 0.6f, 0.6f, UI_COLOR_WHITE);

    float checkboxX = MENU_CONTENT_PADDING;
    float checkboxY = MENU_CONTENT_Y + MENU_CONTENT_PADDING;
    float checkboxSize = 20.0f;
    drawCheckbox(checkboxX, checkboxY, checkboxSize, "Show menu button", showDrawMenuButton);

    C2D_DrawRectSolid(0, MENU_BOTTOM_BAR_Y, 0, BOTTOM_SCREEN_WIDTH, MENU_BOTTOM_BAR_HEIGHT, UI_COLOR_GRAY_2);

    C2D_ImageTint tint;
    C2D_PlainImageTint(&tint, UI_COLOR_WHITE, 1.0f);
    C2D_SpriteSetPos(&backArrowIconSprite, MENU_BTN_X + MENU_BTN_SIZE / 2, MENU_BTN_Y + MENU_BTN_SIZE / 2);
    C2D_SpriteSetScale(&backArrowIconSprite, 0.3f, 0.3f);
    C2D_DrawSpriteTinted(&backArrowIconSprite, &tint);

    C2D_DrawRectSolid(MENU_BTN_SIZE, MENU_BOTTOM_BAR_Y + 4, 0, 1, MENU_BOTTOM_BAR_HEIGHT - 8, UI_COLOR_GRAY_3);
}

void renderSettingsTop(C3D_RenderTarget* target) {
    C2D_TargetClear(target, UI_COLOR_GRAY_1);
    C2D_SceneBegin(target);

    if (guideSpriteSheet) {
        float guideW = 332.0f;
        float guideH = 180.0f;
        float drawX = (TOP_SCREEN_WIDTH - guideW) / 2.0f;
        float drawY = (TOP_SCREEN_HEIGHT - guideH) / 2.0f;
        C2D_SpriteSetPos(&guideSprite, drawX, drawY);
        C2D_SpriteSetScale(&guideSprite, 1.0f, 1.0f);
        C2D_DrawSprite(&guideSprite);
    }
}

void renderHomeTop(C3D_RenderTarget* target) {
    C2D_TargetClear(target, UI_COLOR_GRAY_1);
    C2D_SceneBegin(target);

    if (bannerSpriteSheet) {
        float bannerW = 352.0f;
        float bannerH = 146.0f;
        float drawX = (TOP_SCREEN_WIDTH - bannerW) / 2.0f;
        float drawY = (TOP_SCREEN_HEIGHT - bannerH) / 2.0f;
        C2D_SpriteSetPos(&bannerSprite, drawX, drawY);
        C2D_SpriteSetScale(&bannerSprite, 1.0f, 1.0f);
        C2D_DrawSprite(&bannerSprite);
    }

    C2D_TextBufClear(g_textBuf);
    C2D_Text versionText;
    C2D_TextParse(&versionText, g_textBuf, "Ver. 1.0.0");
    C2D_TextOptimize(&versionText);
    float tw, th;
    C2D_TextGetDimensions(&versionText, 0.5f, 0.5f, &tw, &th);
    float textX = TOP_SCREEN_WIDTH - tw - 6;
    float textY = TOP_SCREEN_HEIGHT - th - 6;
    C2D_DrawText(&versionText, C2D_WithColor, textX, textY, 0, 0.5f, 0.5f, UI_COLOR_TEXT_DIM);
}

void renderOpenMenu(C3D_RenderTarget* target) {
    C2D_TargetClear(target, UI_COLOR_GRAY_1);
    C2D_SceneBegin(target);

    float pad = MENU_CONTENT_PADDING;
    float listX = pad;
    float listY = 8;
    float listWidth = BOTTOM_SCREEN_WIDTH - pad * 2;
    float listHeight = MENU_BOTTOM_BAR_Y - listY - 4;
    float itemHeight = OPEN_LIST_ITEM_HEIGHT;

    C2D_DrawRectSolid(listX, listY, 0, listWidth, listHeight, UI_COLOR_GRAY_1);

    float totalContentHeight = openProjectCount * itemHeight;
    float maxScroll = totalContentHeight - listHeight;
    if (maxScroll < 0) maxScroll = 0;
    if (openListScrollY < 0) openListScrollY = 0;
    if (openListScrollY > maxScroll) openListScrollY = maxScroll;

    if (openProjectCount == 0) {
        C2D_TextBufClear(g_textBuf);
        C2D_Text emptyText;
        C2D_TextParse(&emptyText, g_textBuf, "No projects found");
        C2D_TextOptimize(&emptyText);
        float tw, th;
        C2D_TextGetDimensions(&emptyText, 0.5f, 0.5f, &tw, &th);
        C2D_DrawText(&emptyText, C2D_WithColor, listX + (listWidth - tw) / 2, listY + listHeight / 2 - th / 2, 0, 0.5f, 0.5f, UI_COLOR_TEXT_DIM);
    } else {
        for (int i = 0; i < openProjectCount; i++) {
            float itemY = listY + i * itemHeight - openListScrollY;

            if (itemY + itemHeight <= listY || itemY >= listY + listHeight) continue;

            bool fullyVisible = (itemY >= listY && itemY + itemHeight <= listY + listHeight);

            u32 itemBg = (i == openSelectedIndex) ? UI_COLOR_ACTIVE : UI_COLOR_GRAY_3;
            float textScale = 0.5f;
            float textX = listX + 10;
            float textY_pos = itemY + itemHeight / 2 - 8;

            if (fullyVisible) {
                ListItemConfig item = {
                    .x = listX + 2,
                    .y = itemY + 2,
                    .width = listWidth - 4,
                    .height = itemHeight - 4,
                    .bgColor = itemBg,
                    .drawBorder = false,
                    .borderColor = 0,
                    .text = openProjectNames[i],
                    .textX = textX,
                    .textY = textY_pos,
                    .textScale = textScale,
                    .textColor = UI_COLOR_WHITE,
                    .rightIcon = NULL,
                    .rightIconX = 0,
                    .rightIconY = 0,
                    .rightIconScale = 0.0f,
                    .rightIconColor = UI_COLOR_WHITE
                };
                drawListItem(&item);
            } else {
                float bgTop = (itemY + 2 < listY) ? listY : itemY + 2;
                float bgBottom = (itemY + itemHeight - 2 > listY + listHeight) ? listY + listHeight : itemY + itemHeight - 2;
                if (bgBottom > bgTop) {
                    C2D_DrawRectSolid(listX + 2, bgTop, 0, listWidth - 4, bgBottom - bgTop, itemBg);
                }
                if (textY_pos >= listY && textY_pos + 16 <= listY + listHeight) {
                    C2D_TextBufClear(g_textBuf);
                    C2D_Text nameText;
                    C2D_TextParse(&nameText, g_textBuf, openProjectNames[i]);
                    C2D_TextOptimize(&nameText);
                    C2D_DrawText(&nameText, C2D_WithColor, textX, textY_pos, 0, textScale, textScale, UI_COLOR_WHITE);
                }
            }
        }
    }

    C2D_DrawRectSolid(listX, listY, 0, listWidth, 2, UI_COLOR_GRAY_3);
    C2D_DrawRectSolid(listX, listY + listHeight - 2, 0, listWidth, 2, UI_COLOR_GRAY_3);
    C2D_DrawRectSolid(listX, listY, 0, 2, listHeight, UI_COLOR_GRAY_3);
    C2D_DrawRectSolid(listX + listWidth - 2, listY, 0, 2, listHeight, UI_COLOR_GRAY_3);

    C2D_DrawRectSolid(0, MENU_BOTTOM_BAR_Y, 0, BOTTOM_SCREEN_WIDTH, MENU_BOTTOM_BAR_HEIGHT, UI_COLOR_GRAY_2);

    C2D_ImageTint tint;
    C2D_PlainImageTint(&tint, UI_COLOR_WHITE, 1.0f);
    C2D_SpriteSetPos(&backArrowIconSprite, MENU_BTN_X + MENU_BTN_SIZE / 2, MENU_BTN_Y + MENU_BTN_SIZE / 2);
    C2D_SpriteSetScale(&backArrowIconSprite, 0.3f, 0.3f);
    C2D_DrawSpriteTinted(&backArrowIconSprite, &tint);

    C2D_DrawRectSolid(MENU_BTN_SIZE, MENU_BOTTOM_BAR_Y + 4, 0, 1, MENU_BOTTOM_BAR_HEIGHT - 8, UI_COLOR_GRAY_3);

    RectButtonConfig openBtn = {
        .x = SAVE_EXIT_BTN_X,
        .y = SAVE_EXIT_BTN_Y,
        .width = SAVE_EXIT_BTN_WIDTH,
        .height = MENU_BOTTOM_BAR_HEIGHT,
        .drawBackground = false,
        .bgColor = 0,
        .drawTopBorder = false,
        .drawBottomBorder = false,
        .borderTopColor = 0,
        .borderBottomColor = 0,
        .icon = NULL,
        .iconScale = 0.0f,
        .iconColor = UI_COLOR_WHITE,
        .text = "Open",
        .textScale = 0.5f,
        .textColor = UI_COLOR_WHITE
    };
    drawRectButton(&openBtn);
}

void renderNewProjectMenu(C3D_RenderTarget* target) {
    C2D_TargetClear(target, UI_COLOR_GRAY_1);
    C2D_SceneBegin(target);

    float pad = MENU_CONTENT_PADDING;
    float labelScale = 0.5f;
    float fieldX = pad;
    float fieldWidth = BOTTOM_SCREEN_WIDTH - pad * 2;
    float fieldHeight = 28;
    float curY = 12;
    char textBuf[64];
    C2D_Text text;
    float tw, th;

    C2D_TextBufClear(g_textBuf);
    C2D_TextParse(&text, g_textBuf, "New Project");
    C2D_TextOptimize(&text);
    C2D_TextGetDimensions(&text, 0.6f, 0.6f, &tw, &th);
    C2D_DrawText(&text, C2D_WithColor, (BOTTOM_SCREEN_WIDTH - tw) / 2, curY, 0, 0.6f, 0.6f, UI_COLOR_WHITE);
    curY += th + 12;

    C2D_TextBufClear(g_textBuf);
    C2D_TextParse(&text, g_textBuf, "Project Name");
    C2D_TextOptimize(&text);
    C2D_DrawText(&text, C2D_WithColor, fieldX, curY, 0, labelScale, labelScale, UI_COLOR_TEXT_DIM);
    curY += 16;

    C2D_DrawRectSolid(fieldX, curY, 0, fieldWidth, fieldHeight, UI_COLOR_GRAY_3);
    C2D_TextBufClear(g_textBuf);
    const char* nameDisplay = (newProjectName[0] != '\0') ? newProjectName : "Tap to enter name...";
    C2D_TextParse(&text, g_textBuf, nameDisplay);
    C2D_TextOptimize(&text);
    u32 nameColor = (newProjectName[0] != '\0') ? UI_COLOR_WHITE : UI_COLOR_DISABLED;
    C2D_DrawText(&text, C2D_WithColor, fieldX + 6, curY + 6, 0, labelScale, labelScale, nameColor);
    curY += fieldHeight + 10;

    C2D_TextBufClear(g_textBuf);
    snprintf(textBuf, sizeof(textBuf), "Width (max %d)", NEW_PROJECT_MAX_WIDTH);
    C2D_TextParse(&text, g_textBuf, textBuf);
    C2D_TextOptimize(&text);
    C2D_DrawText(&text, C2D_WithColor, fieldX, curY, 0, labelScale, labelScale, UI_COLOR_TEXT_DIM);
    curY += 16;

    float halfFieldW = (fieldWidth - pad) / 2;

    C2D_DrawRectSolid(fieldX, curY, 0, halfFieldW, fieldHeight, UI_COLOR_GRAY_3);
    C2D_TextBufClear(g_textBuf);
    snprintf(textBuf, sizeof(textBuf), "%d", newProjectWidth);
    C2D_TextParse(&text, g_textBuf, textBuf);
    C2D_TextOptimize(&text);
    C2D_DrawText(&text, C2D_WithColor, fieldX + 6, curY + 6, 0, labelScale, labelScale, UI_COLOR_WHITE);

    float rightFieldX = fieldX + halfFieldW + pad;
    C2D_DrawRectSolid(rightFieldX, curY, 0, halfFieldW, fieldHeight, UI_COLOR_GRAY_3);
    C2D_TextBufClear(g_textBuf);
    snprintf(textBuf, sizeof(textBuf), "%d", newProjectHeight);
    C2D_TextParse(&text, g_textBuf, textBuf);
    C2D_TextOptimize(&text);
    C2D_DrawText(&text, C2D_WithColor, rightFieldX + 6, curY + 6, 0, labelScale, labelScale, UI_COLOR_WHITE);

    C2D_TextBufClear(g_textBuf);
    snprintf(textBuf, sizeof(textBuf), "Height (max %d)", NEW_PROJECT_MAX_HEIGHT);
    C2D_TextParse(&text, g_textBuf, textBuf);
    C2D_TextOptimize(&text);
    C2D_DrawText(&text, C2D_WithColor, rightFieldX, curY - 16, 0, labelScale, labelScale, UI_COLOR_TEXT_DIM);

    curY += fieldHeight + 16;

    C2D_DrawRectSolid(0, MENU_BOTTOM_BAR_Y, 0, BOTTOM_SCREEN_WIDTH, MENU_BOTTOM_BAR_HEIGHT, UI_COLOR_GRAY_2);

    C2D_ImageTint tint;
    C2D_PlainImageTint(&tint, UI_COLOR_WHITE, 1.0f);
    C2D_SpriteSetPos(&backArrowIconSprite, MENU_BTN_X + MENU_BTN_SIZE / 2, MENU_BTN_Y + MENU_BTN_SIZE / 2);
    C2D_SpriteSetScale(&backArrowIconSprite, 0.3f, 0.3f);
    C2D_DrawSpriteTinted(&backArrowIconSprite, &tint);

    C2D_DrawRectSolid(MENU_BTN_SIZE, MENU_BOTTOM_BAR_Y + 4, 0, 1, MENU_BOTTOM_BAR_HEIGHT - 8, UI_COLOR_GRAY_3);

    RectButtonConfig okBtn = {
        .x = SAVE_EXIT_BTN_X,
        .y = SAVE_EXIT_BTN_Y,
        .width = SAVE_EXIT_BTN_WIDTH,
        .height = MENU_BOTTOM_BAR_HEIGHT,
        .drawBackground = false,
        .bgColor = 0,
        .drawTopBorder = false,
        .drawBottomBorder = false,
        .borderTopColor = 0,
        .borderBottomColor = 0,
        .icon = NULL,
        .iconScale = 0.0f,
        .iconColor = UI_COLOR_WHITE,
        .text = "OK",
        .textScale = 0.5f,
        .textColor = UI_COLOR_WHITE
    };
    drawRectButton(&okBtn);
}

void renderSaveMenu(C3D_RenderTarget* target) {
    C2D_TargetClear(target, UI_COLOR_GRAY_1);
    C2D_SceneBegin(target);

    float totalWidth = BTN_SIZE_LARGE * 3 + SAVE_MENU_ITEM_SPACING * 2;
    float startX = (BOTTOM_SCREEN_WIDTH - totalWidth) / 2;
    float itemY = SAVE_MENU_Y;

    C2D_Sprite* saveIcons[3] = { &saveIconSprite, &saveAsIconSprite, &exportIconSprite };
    const char* saveLabels[3] = { "Save", "Save As", "Export" };

    for (int i = 0; i < 3; i++) {
        float btnX = startX + i * (BTN_SIZE_LARGE + SAVE_MENU_ITEM_SPACING);

        ButtonConfig btn = {
            .x = btnX,
            .y = itemY,
            .size = BTN_SIZE_LARGE,
            .icon = saveIcons[i],
            .label = saveLabels[i],
            .isActive = true,
            .isToggle = false,
            .isSkeleton = false
        };
        drawButton(&btn);
    }

    C2D_DrawRectSolid(0, MENU_BOTTOM_BAR_Y, 0, BOTTOM_SCREEN_WIDTH, MENU_BOTTOM_BAR_HEIGHT, UI_COLOR_GRAY_2);

    C2D_DrawRectSolid(MENU_BTN_SIZE, MENU_BOTTOM_BAR_Y + 4, 0, 1, MENU_BOTTOM_BAR_HEIGHT - 8, UI_COLOR_GRAY_3);

    C2D_ImageTint tint;
    C2D_PlainImageTint(&tint, UI_COLOR_WHITE, 1.0f);
    C2D_SpriteSetPos(&backArrowIconSprite, MENU_BTN_X + MENU_BTN_SIZE / 2, MENU_BTN_Y + MENU_BTN_SIZE / 2);
    C2D_SpriteSetScale(&backArrowIconSprite, 0.3f, 0.3f);
    C2D_DrawSpriteTinted(&backArrowIconSprite, &tint);

    RectButtonConfig goHomeBtn = {
        .x = MENU_BTN_SIZE,
        .y = MENU_BOTTOM_BAR_Y,
        .width = BOTTOM_SCREEN_WIDTH - MENU_BTN_SIZE,
        .height = MENU_BOTTOM_BAR_HEIGHT,
        .drawBackground = false,
        .bgColor = 0,
        .drawTopBorder = false,
        .drawBottomBorder = false,
        .borderTopColor = 0,
        .borderBottomColor = 0,
        .icon = NULL,
        .iconScale = 0.0f,
        .iconColor = UI_COLOR_WHITE,
        .text = "Go to Home",
        .textScale = 0.5f,
        .textColor = UI_COLOR_WHITE
    };
    drawRectButton(&goHomeBtn);
}
