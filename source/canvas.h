#pragma once

#include "app_state.h"

/**
 * @file canvas.h
 * @brief Canvas texture upload helpers.
 */

void updateCanvasTexture(void);
void forceUpdateCanvasTexture(void);
void markCanvasDirtyFull(void);
void markCanvasDirtyRect(int minX, int minY, int maxX, int maxY);
