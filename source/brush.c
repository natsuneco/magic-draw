#include "brush.h"

#include <math.h>
#include <stdlib.h>
#include <string.h>

#include "canvas.h"

typedef struct {
    int x, y;
} Point;

// Stroke buffer: prevents alpha accumulation within a single stroke.
// Each pixel is blended from the original (pre-stroke) layer state,
// and only the maximum alpha per pixel during the stroke is applied.
static u32* strokeBackupBuffer = NULL;
static u8* strokeAlphaMap = NULL;
static int strokeLayerIdx = -1;

static void blendPixelOver(u32* outPixel, u32 srcR, u32 srcG, u32 srcB, u32 srcA,
                           u32 dst, bool alphaLock) {
    u32 dstR = (dst >> 24) & 0xFF;
    u32 dstG = (dst >> 16) & 0xFF;
    u32 dstB = (dst >> 8) & 0xFF;
    u32 dstA = dst & 0xFF;

    u32 invSrcA = 255 - srcA;
    u32 outA = srcA + (dstA * invSrcA) / 255;

    u32 oR, oG, oB;
    if (outA == 0) {
        oR = oG = oB = 0;
    } else {
        // Porter-Duff "over" for straight alpha
        oR = (srcR * srcA + dstR * dstA * invSrcA / 255) / outA;
        oG = (srcG * srcA + dstG * dstA * invSrcA / 255) / outA;
        oB = (srcB * srcA + dstB * dstA * invSrcA / 255) / outA;
        if (oR > 255) oR = 255;
        if (oG > 255) oG = 255;
        if (oB > 255) oB = 255;
    }

    if (alphaLock) {
        outA = dstA;
    }

    *outPixel = (oR << 24) | (oG << 16) | (oB << 8) | outA;
}

static void drawPixelBlended(int layerIndex, int x, int y, u32 color, u8 alpha) {
    if (layerIndex < 0 || layerIndex >= MAX_LAYERS) return;
    if (!layers[layerIndex].buffer) return;
    if (x < 0 || x >= CANVAS_WIDTH || y < 0 || y >= CANVAS_HEIGHT) return;
    if (alpha == 0) return;

    u32 srcR = (color >> 24) & 0xFF;
    u32 srcG = (color >> 16) & 0xFF;
    u32 srcB = (color >> 8) & 0xFF;
    u32 srcA = color & 0xFF;

    srcA = (srcA * alpha) / 255;
    if (srcA == 0) return;

    int idx = y * TEX_WIDTH + x;
    bool alphaLock = layers[layerIndex].alphaLock;

    // Stroke-level alpha: blend from original pixel, track max alpha per pixel
    if (strokeBackupBuffer && strokeAlphaMap && layerIndex == strokeLayerIdx) {
        int mapIdx = y * CANVAS_WIDTH + x;
        if ((u8)srcA <= strokeAlphaMap[mapIdx]) return;
        strokeAlphaMap[mapIdx] = (u8)srcA;
        u32 dst = strokeBackupBuffer[idx];
        blendPixelOver(&layers[layerIndex].buffer[idx], srcR, srcG, srcB, srcA, dst, alphaLock);
    } else {
        u32 dst = layers[layerIndex].buffer[idx];
        blendPixelOver(&layers[layerIndex].buffer[idx], srcR, srcG, srcB, srcA, dst, alphaLock);
    }
}

void drawPixelToLayer(int layerIndex, int x, int y, u32 color) {
    if (layerIndex < 0 || layerIndex >= MAX_LAYERS) return;
    if (!layers[layerIndex].buffer) return;
    if (x < 0 || x >= CANVAS_WIDTH || y < 0 || y >= CANVAS_HEIGHT) return;

    u32 srcA = color & 0xFF;
    if (srcA == 0) return;

    int idx = y * TEX_WIDTH + x;
    u32 srcR = (color >> 24) & 0xFF;
    u32 srcG = (color >> 16) & 0xFF;
    u32 srcB = (color >> 8) & 0xFF;
    bool alphaLock = layers[layerIndex].alphaLock;

    // Stroke-level alpha: blend from original pixel, track max alpha per pixel
    if (strokeBackupBuffer && strokeAlphaMap && layerIndex == strokeLayerIdx) {
        int mapIdx = y * CANVAS_WIDTH + x;
        if ((u8)srcA <= strokeAlphaMap[mapIdx]) return;
        strokeAlphaMap[mapIdx] = (u8)srcA;
        u32 dst = strokeBackupBuffer[idx];
        blendPixelOver(&layers[layerIndex].buffer[idx], srcR, srcG, srcB, srcA, dst, alphaLock);
    } else {
        u32 dst = layers[layerIndex].buffer[idx];
        blendPixelOver(&layers[layerIndex].buffer[idx], srcR, srcG, srcB, srcA, dst, alphaLock);
    }
}

static void drawBrushAntialias(int layerIndex, int x, int y, int size, u32 color) {
    float radius = (float)size;
    float radiusSq = radius * radius;
    float aaWidth = 1.5f;
    float innerRadiusSq = (radius - aaWidth) * (radius - aaWidth);
    if (innerRadiusSq < 0) innerRadiusSq = 0;

    int iRadius = size + 1;

    for (int dy = -iRadius; dy <= iRadius; dy++) {
        for (int dx = -iRadius; dx <= iRadius; dx++) {
            float distSq = (float)(dx * dx + dy * dy);

            if (distSq <= innerRadiusSq) {
                drawPixelToLayer(layerIndex, x + dx, y + dy, color);
            } else if (distSq <= radiusSq) {
                float dist = sqrtf(distSq);
                float alpha = (radius - dist) / aaWidth;
                if (alpha > 1.0f) alpha = 1.0f;
                if (alpha < 0.0f) alpha = 0.0f;
                u8 pixelAlpha = (u8)(alpha * 255.0f);
                drawPixelBlended(layerIndex, x + dx, y + dy, color, pixelAlpha);
            }
        }
    }
}

static void drawBrushPixel(int layerIndex, int x, int y, int size, u32 color) {
    int radius = size;
    int radiusSq = radius * radius;

    for (int dy = -radius; dy <= radius; dy++) {
        for (int dx = -radius; dx <= radius; dx++) {
            int distSq = dx * dx + dy * dy;
            if (distSq <= radiusSq) {
                drawPixelToLayer(layerIndex, x + dx, y + dy, color);
            }
        }
    }
}

static void drawBrushAirbrush(int layerIndex, int x, int y, int size, u32 color) {
    float radius = (float)size * 1.5f;
    float radiusSq = radius * radius;

    int iRadius = (int)radius + 1;

    for (int dy = -iRadius; dy <= iRadius; dy++) {
        for (int dx = -iRadius; dx <= iRadius; dx++) {
            float distSq = (float)(dx * dx + dy * dy);

            if (distSq <= radiusSq) {
                float dist = sqrtf(distSq);
                float t = dist / radius;
                float alpha = (1.0f - t * t) * 0.3f;
                if (alpha > 0.0f) {
                    u8 pixelAlpha = (u8)(alpha * 255.0f);
                    drawPixelBlended(layerIndex, x + dx, y + dy, color, pixelAlpha);
                }
            }
        }
    }
}

static float gpenPressure = 0.0f;
static bool gpenStrokeStarted = false;
static int gpenStrokeLength = 0;

#define GPEN_HISTORY_SIZE 20

typedef struct {
    int x, y;
    int size;
    u32 color;
    int layerIndex;
} GpenPoint;

static GpenPoint gpenHistory[GPEN_HISTORY_SIZE];
static int gpenHistoryCount = 0;
static int gpenHistoryIndex = 0;

static void recordGpenPoint(int layerIndex, int x, int y, int size, u32 color) {
    gpenHistory[gpenHistoryIndex].x = x;
    gpenHistory[gpenHistoryIndex].y = y;
    gpenHistory[gpenHistoryIndex].size = size;
    gpenHistory[gpenHistoryIndex].color = color;
    gpenHistory[gpenHistoryIndex].layerIndex = layerIndex;

    gpenHistoryIndex = (gpenHistoryIndex + 1) % GPEN_HISTORY_SIZE;
    if (gpenHistoryCount < GPEN_HISTORY_SIZE) {
        gpenHistoryCount++;
    }
}

static void drawBrushGPen(int layerIndex, int x, int y, int size, u32 color, float pressure) {
    float effectiveRadius = size * (0.3f + 0.7f * pressure);
    float radiusSq = effectiveRadius * effectiveRadius;
    float aaWidth = 1.0f;
    float innerRadiusSq = (effectiveRadius - aaWidth) * (effectiveRadius - aaWidth);
    if (innerRadiusSq < 0) innerRadiusSq = 0;

    int iRadius = (int)effectiveRadius + 1;

    for (int dy = -iRadius; dy <= iRadius; dy++) {
        for (int dx = -iRadius; dx <= iRadius; dx++) {
            float distSq = (float)(dx * dx + dy * dy);

            if (distSq <= innerRadiusSq) {
                drawPixelToLayer(layerIndex, x + dx, y + dy, color);
            } else if (distSq <= radiusSq) {
                float dist = sqrtf(distSq);
                float alpha = (effectiveRadius - dist) / aaWidth;
                if (alpha > 1.0f) alpha = 1.0f;
                if (alpha < 0.0f) alpha = 0.0f;
                u8 pixelAlpha = (u8)(alpha * 255.0f);
                drawPixelBlended(layerIndex, x + dx, y + dy, color, pixelAlpha);
            }
        }
    }
}

void drawBrushToLayer(int layerIndex, int x, int y, int size, u32 color) {
    projectHasUnsavedChanges = true;
    BrushType brushType = brushDefs[currentBrushType].type;

    switch (brushType) {
        case BRUSH_ANTIALIAS:
            drawBrushAntialias(layerIndex, x, y, size, color);
            break;
        case BRUSH_PIXEL:
            drawBrushPixel(layerIndex, x, y, size, color);
            break;
        case BRUSH_AIRBRUSH:
            drawBrushAirbrush(layerIndex, x, y, size, color);
            break;
        case BRUSH_GPEN:
            drawBrushGPen(layerIndex, x, y, size, color, gpenPressure);
            recordGpenPoint(layerIndex, x, y, size, color);
            break;
        default:
            drawBrushAntialias(layerIndex, x, y, size, color);
            break;
    }
}

void drawLineToLayer(int layerIndex, int x0, int y0, int x1, int y1, int size, u32 color) {
    projectHasUnsavedChanges = true;
    int dx = abs(x1 - x0);
    int dy = abs(y1 - y0);
    int sx = (x0 < x1) ? 1 : -1;
    int sy = (y0 < y1) ? 1 : -1;
    int err = dx - dy;

    BrushType brushType = brushDefs[currentBrushType].type;

    while (1) {
        if (brushType == BRUSH_GPEN) {
            float strokeProgress = (float)gpenStrokeLength / 30.0f;
            if (strokeProgress > 1.0f) strokeProgress = 1.0f;
            gpenPressure = strokeProgress;
            gpenStrokeLength++;
        }

        drawBrushToLayer(layerIndex, x0, y0, size, color);

        if (x0 == x1 && y0 == y1) break;

        int e2 = 2 * err;
        if (e2 > -dy) {
            err -= dy;
            x0 += sx;
        }
        if (e2 < dx) {
            err += dx;
            y0 += sy;
        }
    }
}

void startStroke(int layerIndex) {
    gpenPressure = 0.0f;
    gpenStrokeStarted = true;
    gpenStrokeLength = 0;
    gpenHistoryCount = 0;
    gpenHistoryIndex = 0;

    // Initialize stroke buffer for stroke-level alpha
    strokeLayerIdx = layerIndex;
    size_t bufSize = TEX_WIDTH * TEX_HEIGHT * sizeof(u32);
    strokeBackupBuffer = (u32*)malloc(bufSize);
    if (strokeBackupBuffer && layers[layerIndex].buffer) {
        memcpy(strokeBackupBuffer, layers[layerIndex].buffer, bufSize);
    }
    strokeAlphaMap = (u8*)calloc(CANVAS_WIDTH * CANVAS_HEIGHT, sizeof(u8));
}

static void applyGpenTaperOut(void) {
    if (gpenHistoryCount < 2) return;

    BrushType brushType = brushDefs[currentBrushType].type;
    if (brushType != BRUSH_GPEN) return;

    int startIdx;
    if (gpenHistoryCount < GPEN_HISTORY_SIZE) {
        startIdx = 0;
    } else {
        startIdx = gpenHistoryIndex;
    }

    int lastIdx = (startIdx + gpenHistoryCount - 1) % GPEN_HISTORY_SIZE;
    int prevIdx = (startIdx + gpenHistoryCount - 2) % GPEN_HISTORY_SIZE;

    GpenPoint* lastPt = &gpenHistory[lastIdx];
    GpenPoint* prevPt = &gpenHistory[prevIdx];

    float dx = (float)(lastPt->x - prevPt->x);
    float dy = (float)(lastPt->y - prevPt->y);
    float len = sqrtf(dx * dx + dy * dy);

    if (len < 0.1f) return;

    dx /= len;
    dy /= len;

    int taperLength = lastPt->size * 3;
    int steps = taperLength;

    for (int i = 1; i <= steps; i++) {
        float t = (float)i / (float)steps;
        float pressure = (1.0f - t);
        pressure = pressure * pressure;

        if (pressure < 0.05f) break;

        int x = lastPt->x + (int)(dx * i);
        int y = lastPt->y + (int)(dy * i);

        drawBrushGPen(lastPt->layerIndex, x, y, lastPt->size, lastPt->color, pressure);
    }
}

void endStroke(void) {
    applyGpenTaperOut();
    markCanvasDirtyFull();
    forceUpdateCanvasTexture();

    gpenStrokeStarted = false;
    gpenHistoryCount = 0;

    // Free stroke buffer
    if (strokeBackupBuffer) { free(strokeBackupBuffer); strokeBackupBuffer = NULL; }
    if (strokeAlphaMap) { free(strokeAlphaMap); strokeAlphaMap = NULL; }
    strokeLayerIdx = -1;
}

static bool colorWithinTolerance(u32 a, u32 b, int tolerance) {
    if (tolerance <= 0) return a == b;
    int aR = (a >> 24) & 0xFF, aG = (a >> 16) & 0xFF, aB = (a >> 8) & 0xFF, aA = a & 0xFF;
    int bR = (b >> 24) & 0xFF, bG = (b >> 16) & 0xFF, bB = (b >> 8) & 0xFF, bA = b & 0xFF;
    int dr = aR - bR, dg = aG - bG, db = aB - bB, da = aA - bA;
    if (dr < 0) dr = -dr;
    if (dg < 0) dg = -dg;
    if (db < 0) db = -db;
    if (da < 0) da = -da;
    int maxDiff = dr;
    if (dg > maxDiff) maxDiff = dg;
    if (db > maxDiff) maxDiff = db;
    if (da > maxDiff) maxDiff = da;
    // tolerance is 0-100%, map to 0-255
    int threshold = (tolerance * 255) / 100;
    return maxDiff <= threshold;
}

void floodFill(int layerIndex, int startX, int startY, u32 fillColor, int expand, int tolerancePct) {
    projectHasUnsavedChanges = true;
    if (layerIndex < 0 || layerIndex >= MAX_LAYERS) return;
    if (!layers[layerIndex].buffer || !compositeBuffer) return;
    if (startX < 0 || startX >= CANVAS_WIDTH || startY < 0 || startY >= CANVAS_HEIGHT) return;

    int startIdx = startY * TEX_WIDTH + startX;
    u32 targetColor = compositeBuffer[startIdx];

    // Only skip if fill color exactly matches target (tolerance doesn't apply here)
    if (targetColor == fillColor) return;

    const int maxStackSize = CANVAS_WIDTH * CANVAS_HEIGHT;
    Point* stack = (Point*)malloc(maxStackSize * sizeof(Point));
    if (!stack) return;

    bool* filled = (bool*)calloc(CANVAS_WIDTH * CANVAS_HEIGHT, sizeof(bool));
    if (!filled) {
        free(stack);
        return;
    }

    bool* visited = (bool*)calloc(CANVAS_WIDTH * CANVAS_HEIGHT, sizeof(bool));
    if (!visited) {
        free(filled);
        free(stack);
        return;
    }

    int stackSize = 0;
    stack[stackSize++] = (Point){startX, startY};
    visited[startY * CANVAS_WIDTH + startX] = true;

    while (stackSize > 0) {
        Point p = stack[--stackSize];
        int x = p.x;
        int y = p.y;

        drawPixelToLayer(layerIndex, x, y, fillColor);
        filled[y * CANVAS_WIDTH + x] = true;

        const int dx[] = {0, 0, -1, 1};
        const int dy[] = {-1, 1, 0, 0};

        for (int i = 0; i < 4; i++) {
            int nx = x + dx[i];
            int ny = y + dy[i];

            if (nx < 0 || nx >= CANVAS_WIDTH || ny < 0 || ny >= CANVAS_HEIGHT) continue;

            int visitIdx = ny * CANVAS_WIDTH + nx;
            if (visited[visitIdx]) continue;

            int nIdx = ny * TEX_WIDTH + nx;
            if (colorWithinTolerance(compositeBuffer[nIdx], targetColor, tolerancePct)) {
                if (stackSize < maxStackSize) {
                    stack[stackSize++] = (Point){nx, ny};
                    visited[visitIdx] = true;
                }
            }
        }
    }

    if (expand > 0) {
        bool* expanded = (bool*)calloc(CANVAS_WIDTH * CANVAS_HEIGHT, sizeof(bool));
        if (expanded) {
            memcpy(expanded, filled, CANVAS_WIDTH * CANVAS_HEIGHT * sizeof(bool));
            for (int pass = 0; pass < expand; pass++) {
                bool* nextExpanded = (bool*)calloc(CANVAS_WIDTH * CANVAS_HEIGHT, sizeof(bool));
                if (!nextExpanded) break;
                memcpy(nextExpanded, expanded, CANVAS_WIDTH * CANVAS_HEIGHT * sizeof(bool));
                for (int y = 0; y < CANVAS_HEIGHT; y++) {
                    for (int x = 0; x < CANVAS_WIDTH; x++) {
                        if (!expanded[y * CANVAS_WIDTH + x]) continue;
                        const int edx[] = {0, 0, -1, 1};
                        const int edy[] = {-1, 1, 0, 0};
                        for (int d = 0; d < 4; d++) {
                            int nx = x + edx[d];
                            int ny = y + edy[d];
                            if (nx < 0 || nx >= CANVAS_WIDTH || ny < 0 || ny >= CANVAS_HEIGHT) continue;
                            int ni = ny * CANVAS_WIDTH + nx;
                            if (!nextExpanded[ni]) {
                                nextExpanded[ni] = true;
                                drawPixelToLayer(layerIndex, nx, ny, fillColor);
                            }
                        }
                    }
                }
                free(expanded);
                expanded = nextExpanded;
            }
            free(expanded);
        }
    }

    free(visited);
    free(filled);
    free(stack);
}
