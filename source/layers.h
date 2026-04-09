#pragma once

#include "app_state.h"

/**
 * @file layers.h
 * @brief Layer storage and canvas buffer management.
 */

void initLayers(void);
void exitLayers(void);
void resetLayersForNewProject(void);
bool applyCanvasSize(int width, int height);
void setCanvasTextureFiltering(bool smooth);
void clearLayer(int layerIndex, u32 color);
void compositeAllLayers(void);
