#include "layers.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "blend.h"
#include "util.h"

void initLayers(void) {
    texWidth = nextPowerOf2(canvasWidth);
    texHeight = nextPowerOf2(canvasHeight);

    size_t bufferSize = TEX_WIDTH * TEX_HEIGHT * sizeof(u32);

    compositeBuffer = (u32*)linearAlloc(bufferSize);
    if (!compositeBuffer) return;

    for (int i = 0; i < MAX_LAYERS; i++) {
        layers[i].buffer = (u32*)malloc(bufferSize);
        layers[i].visible = true;
        layers[i].opacity = 255;
        layers[i].blendMode = BLEND_NORMAL;
        snprintf(layers[i].name, sizeof(layers[i].name), "Layer %d", i + 1);
        layers[i].alphaLock = false;
        layers[i].clipping = false;

        if (layers[i].buffer) {
            u32 clearColor = 0x00000000;
            for (int y = 0; y < TEX_HEIGHT; y++) {
                for (int x = 0; x < TEX_WIDTH; x++) {
                    layers[i].buffer[y * TEX_WIDTH + x] = clearColor;
                }
            }
        }
    }

    C3D_TexInit(&canvasTex, TEX_WIDTH, TEX_HEIGHT, GPU_RGBA8);
    C3D_TexSetFilter(&canvasTex, GPU_LINEAR, GPU_LINEAR);
    C3D_TexSetWrap(&canvasTex, GPU_CLAMP_TO_EDGE, GPU_CLAMP_TO_EDGE);

    canvasSubTex.width = CANVAS_WIDTH;
    canvasSubTex.height = CANVAS_HEIGHT;
    canvasSubTex.left = 0.0f;
    canvasSubTex.top = (float)CANVAS_HEIGHT / TEX_HEIGHT;
    canvasSubTex.right = (float)CANVAS_WIDTH / TEX_WIDTH;
    canvasSubTex.bottom = 0.0f;

    canvasImage.tex = &canvasTex;
    canvasImage.subtex = &canvasSubTex;

    compositeAllLayers();
}

void resetLayersForNewProject(void) {
    for (int i = 0; i < MAX_LAYERS; i++) {
        layers[i].visible = true;
        layers[i].opacity = 255;
        layers[i].blendMode = BLEND_NORMAL;
        layers[i].alphaLock = false;
        layers[i].clipping = false;
        snprintf(layers[i].name, sizeof(layers[i].name), "Layer %d", i + 1);

        if (layers[i].buffer) {
            u32 clearColor = 0x00000000;
            for (int y = 0; y < TEX_HEIGHT; y++) {
                for (int x = 0; x < TEX_WIDTH; x++) {
                    layers[i].buffer[y * TEX_WIDTH + x] = clearColor;
                }
            }
        }
    }
    currentLayerIndex = 0;
}

void applyCanvasSize(int width, int height) {
    canvasWidth = width;
    canvasHeight = height;

    int newTexW = nextPowerOf2(width);
    int newTexH = nextPowerOf2(height);

    if (newTexW != texWidth || newTexH != texHeight) {
        texWidth = newTexW;
        texHeight = newTexH;
        size_t bufferSize = TEX_WIDTH * TEX_HEIGHT * sizeof(u32);

        if (compositeBuffer) {
            linearFree(compositeBuffer);
        }
        compositeBuffer = (u32*)linearAlloc(bufferSize);

        for (int i = 0; i < MAX_LAYERS; i++) {
            if (layers[i].buffer) {
                free(layers[i].buffer);
            }
            layers[i].buffer = (u32*)malloc(bufferSize);
            if (layers[i].buffer) {
                memset(layers[i].buffer, 0, bufferSize);
            }
        }

        C3D_TexDelete(&canvasTex);
        C3D_TexInit(&canvasTex, TEX_WIDTH, TEX_HEIGHT, GPU_RGBA8);
        C3D_TexSetFilter(&canvasTex, GPU_LINEAR, GPU_LINEAR);
        C3D_TexSetWrap(&canvasTex, GPU_CLAMP_TO_EDGE, GPU_CLAMP_TO_EDGE);
    }

    canvasSubTex.width = canvasWidth;
    canvasSubTex.height = canvasHeight;
    canvasSubTex.left = 0.0f;
    canvasSubTex.top = (float)canvasHeight / TEX_HEIGHT;
    canvasSubTex.right = (float)canvasWidth / TEX_WIDTH;
    canvasSubTex.bottom = 0.0f;

    canvasImage.tex = &canvasTex;
    canvasImage.subtex = &canvasSubTex;

    canvasNeedsUpdate = true;
}

void exitLayers(void) {
    for (int i = 0; i < MAX_LAYERS; i++) {
        if (layers[i].buffer) {
            free(layers[i].buffer);
            layers[i].buffer = NULL;
        }
    }

    if (compositeBuffer) {
        linearFree(compositeBuffer);
        compositeBuffer = NULL;
    }

    C3D_TexDelete(&canvasTex);
}

void clearLayer(int layerIndex, u32 color) {
    projectHasUnsavedChanges = true;
    if (layerIndex < 0 || layerIndex >= MAX_LAYERS) return;
    if (!layers[layerIndex].buffer) return;

    for (int y = 0; y < TEX_HEIGHT; y++) {
        for (int x = 0; x < TEX_WIDTH; x++) {
            layers[layerIndex].buffer[y * TEX_WIDTH + x] = color;
        }
    }
}

void compositeAllLayers(void) {
    if (!compositeBuffer) return;

    int minX = 0;
    int minY = 0;
    int maxX = CANVAS_WIDTH - 1;
    int maxY = CANVAS_HEIGHT - 1;
    if (canvasDirtyValid) {
        minX = canvasDirtyMinX;
        minY = canvasDirtyMinY;
        maxX = canvasDirtyMaxX;
        maxY = canvasDirtyMaxY;

        if (minX < 0) minX = 0;
        if (minY < 0) minY = 0;
        if (maxX >= CANVAS_WIDTH) maxX = CANVAS_WIDTH - 1;
        if (maxY >= CANVAS_HEIGHT) maxY = CANVAS_HEIGHT - 1;
        if (minX > maxX || minY > maxY) return;
    }

    int firstVisibleLayer = -1;
    int visibleCount = 0;
    for (int i = 0; i < numLayers; i++) {
        if (layers[i].visible && layers[i].buffer && layers[i].opacity > 0) {
            if (firstVisibleLayer < 0) firstVisibleLayer = i;
            visibleCount++;
        }
    }

    for (int y = minY; y <= maxY; y++) {
        int rowStart = y * TEX_WIDTH;
        for (int x = minX; x <= maxX; x++) {
            compositeBuffer[rowStart + x] = 0xFFFFFFFF;
        }
    }

    if (visibleCount == 0) {
        return;
    }

    for (int i = 0; i < numLayers; i++) {
        if (!layers[i].visible || !layers[i].buffer || layers[i].opacity == 0) continue;

        u8 layerOpacity = layers[i].opacity;
        BlendMode blendMode = layers[i].blendMode;
        u32* layerBuf = layers[i].buffer;

        bool isClipped = layers[i].clipping && i > 0 && layers[i - 1].buffer;
        u32* clipBuf = isClipped ? layers[i - 1].buffer : NULL;

        for (int y = minY; y <= maxY; y++) {
            int rowStart = y * TEX_WIDTH;
            for (int x = minX; x <= maxX; x++) {
                int idx = rowStart + x;
                u32 src = layerBuf[idx];

                u8 srcA = src & 0xFF;
                if (srcA == 0) continue;

                if (clipBuf) {
                    u8 clipA = clipBuf[idx] & 0xFF;
                    if (clipA == 0) continue;
                    srcA = (srcA * clipA) / 255;
                    if (srcA == 0) continue;
                    src = (src & 0xFFFFFF00) | srcA;
                }

                u32 dst = compositeBuffer[idx];
                compositeBuffer[idx] = blendPixel(dst, src, blendMode, layerOpacity);
            }
        }
    }
}
