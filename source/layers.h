#pragma once

#include "app_state.h"

/**
 * @file layers.h
 * @brief Layer storage and canvas buffer management.
 */

void initLayers(void);
void exitLayers(void);
void resetLayersForNewProject(void);
void applyCanvasSize(int width, int height);
void clearLayer(int layerIndex, u32 color);
void compositeAllLayers(void);
