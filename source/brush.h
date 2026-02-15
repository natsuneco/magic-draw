#pragma once

#include "app_state.h"

/**
 * @file brush.h
 * @brief Brush, fill, and stroke rendering routines.
 */

void floodFill(int layerIndex, int startX, int startY, u32 fillColor, int expand);
void drawPixelToLayer(int layerIndex, int x, int y, u32 color);
void drawBrushToLayer(int layerIndex, int x, int y, int size, u32 color);
void drawLineToLayer(int layerIndex, int x0, int y0, int x1, int y1, int size, u32 color);
void startStroke(int layerIndex);
void endStroke(void);
