#pragma once

#include <stdbool.h>

#include "app_state.h"

/**
 * @file project_io.h
 * @brief Project save/load functions.
 */

typedef struct {
    u32 magic;
    u32 version;
    u32 canvasWidth;
    u32 canvasHeight;
    u32 numLayers;
    u32 currentLayer;
    u32 currentTool;
    u32 brushSize;
    u32 currentColor;
    u32 brushAlpha;
    u32 currentBrushType;
    float hue;
    float saturation;
    float value;
    u32 paletteCount;
} ProjectHeader;

bool saveProject(const char* projectName);
bool quickSaveProject(void);
bool loadProject(const char* projectName);
int findNextUntitledIndex(void);
