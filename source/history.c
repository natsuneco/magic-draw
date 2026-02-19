#include "history.h"

#include <stdlib.h>
#include <string.h>

#define HISTORY_MAX 10

typedef struct {
    u32* buffer;
    bool visible;
    u8 opacity;
    BlendMode blendMode;
    bool alphaLock;
    bool clipping;
    char name[32];
} LayerSnapshot;

typedef struct {
    int currentLayerIndex;
    LayerSnapshot layers[MAX_LAYERS];
} HistoryEntry;

static HistoryEntry historyStack[HISTORY_MAX];
static int historyCount = 0;
static int historyIndex = -1;
static bool historyInitialized = false;
static int historyCanvasWidth = 0;
static int historyCanvasHeight = 0;

static size_t getHistoryBufferSize(void) {
    return (size_t)CANVAS_WIDTH * (size_t)CANVAS_HEIGHT * sizeof(u32);
}

static void clearHistoryEntries(void) {
    for (int i = 0; i < HISTORY_MAX; i++) {
        historyStack[i].currentLayerIndex = -1;
        for (int j = 0; j < MAX_LAYERS; j++) {
            if (historyStack[i].layers[j].buffer) {
                free(historyStack[i].layers[j].buffer);
                historyStack[i].layers[j].buffer = NULL;
            }
        }
    }
    historyCount = 0;
    historyIndex = -1;
}

static void copyLayerToSnapshot(u32* dst, const u32* src) {
    for (int y = 0; y < CANVAS_HEIGHT; y++) {
        memcpy(&dst[y * CANVAS_WIDTH], &src[y * TEX_WIDTH], CANVAS_WIDTH * sizeof(u32));
    }
}

static void copySnapshotToLayer(u32* dst, const u32* src) {
    for (int y = 0; y < CANVAS_HEIGHT; y++) {
        memcpy(&dst[y * TEX_WIDTH], &src[y * CANVAS_WIDTH], CANVAS_WIDTH * sizeof(u32));
    }
}

static void swapLayerWithSnapshot(u32* layerBuf, u32* snapshotBuf, u32* tempBuf) {
    copyLayerToSnapshot(tempBuf, layerBuf);
    copySnapshotToLayer(layerBuf, snapshotBuf);
    memcpy(snapshotBuf, tempBuf, getHistoryBufferSize());
}

void initHistory(void) {
    clearHistoryEntries();
    historyCanvasWidth = CANVAS_WIDTH;
    historyCanvasHeight = CANVAS_HEIGHT;
    historyInitialized = true;
}

void exitHistory(void) {
    clearHistoryEntries();
    historyCanvasWidth = 0;
    historyCanvasHeight = 0;
    historyInitialized = false;
}

void pushHistory(void) {
    if (!historyInitialized) return;
    for (int i = 0; i < MAX_LAYERS; i++) {
        if (!layers[i].buffer) return;
    }

    if (historyCanvasWidth != CANVAS_WIDTH || historyCanvasHeight != CANVAS_HEIGHT) {
        clearHistoryEntries();
        historyCanvasWidth = CANVAS_WIDTH;
        historyCanvasHeight = CANVAS_HEIGHT;
    }

    size_t bufferSize = getHistoryBufferSize();

    if (historyIndex < historyCount - 1) {
        for (int i = historyIndex + 1; i < historyCount; i++) {
            for (int j = 0; j < MAX_LAYERS; j++) {
                if (historyStack[i].layers[j].buffer) {
                    free(historyStack[i].layers[j].buffer);
                    historyStack[i].layers[j].buffer = NULL;
                }
            }
        }
        historyCount = historyIndex + 1;
    }

    if (historyCount >= HISTORY_MAX) {
        for (int j = 0; j < MAX_LAYERS; j++) {
            if (historyStack[0].layers[j].buffer) {
                free(historyStack[0].layers[j].buffer);
                historyStack[0].layers[j].buffer = NULL;
            }
        }
        for (int i = 0; i < HISTORY_MAX - 1; i++) {
            historyStack[i] = historyStack[i + 1];
        }
        historyStack[HISTORY_MAX - 1].currentLayerIndex = -1;
        for (int j = 0; j < MAX_LAYERS; j++) {
            historyStack[HISTORY_MAX - 1].layers[j].buffer = NULL;
        }
        historyCount = HISTORY_MAX - 1;
        historyIndex = historyCount - 1;
    }

    for (int j = 0; j < MAX_LAYERS; j++) {
        if (!historyStack[historyCount].layers[j].buffer) {
            historyStack[historyCount].layers[j].buffer = (u32*)malloc(bufferSize);
            if (!historyStack[historyCount].layers[j].buffer) {
                for (int k = 0; k < MAX_LAYERS; k++) {
                    if (historyStack[historyCount].layers[k].buffer) {
                        free(historyStack[historyCount].layers[k].buffer);
                        historyStack[historyCount].layers[k].buffer = NULL;
                    }
                }
                return;
            }
        }
    }

    historyStack[historyCount].currentLayerIndex = currentLayerIndex;
    for (int j = 0; j < MAX_LAYERS; j++) {
        copyLayerToSnapshot(historyStack[historyCount].layers[j].buffer, layers[j].buffer);
        historyStack[historyCount].layers[j].visible = layers[j].visible;
        historyStack[historyCount].layers[j].opacity = layers[j].opacity;
        historyStack[historyCount].layers[j].blendMode = layers[j].blendMode;
        historyStack[historyCount].layers[j].alphaLock = layers[j].alphaLock;
        historyStack[historyCount].layers[j].clipping = layers[j].clipping;
        strncpy(historyStack[historyCount].layers[j].name, layers[j].name, sizeof(historyStack[historyCount].layers[j].name));
        historyStack[historyCount].layers[j].name[sizeof(historyStack[historyCount].layers[j].name) - 1] = '\0';
    }
    historyCount++;
    historyIndex = historyCount - 1;
}

void undo(void) {
    if (!historyInitialized) return;
    if (historyIndex < 0) return;
    if (historyCanvasWidth != CANVAS_WIDTH || historyCanvasHeight != CANVAS_HEIGHT) {
        clearHistoryEntries();
        return;
    }

    HistoryEntry* entry = &historyStack[historyIndex];
    size_t bufferSize = getHistoryBufferSize();
    u32* tempBuffer = (u32*)malloc(bufferSize);
    if (!tempBuffer) return;

    int tempLayerIndex = currentLayerIndex;
    currentLayerIndex = entry->currentLayerIndex;
    entry->currentLayerIndex = tempLayerIndex;

    for (int j = 0; j < MAX_LAYERS; j++) {
        if (layers[j].buffer && entry->layers[j].buffer) {
            swapLayerWithSnapshot(layers[j].buffer, entry->layers[j].buffer, tempBuffer);
        }

        bool tempVisible = layers[j].visible;
        layers[j].visible = entry->layers[j].visible;
        entry->layers[j].visible = tempVisible;

        u8 tempOpacity = layers[j].opacity;
        layers[j].opacity = entry->layers[j].opacity;
        entry->layers[j].opacity = tempOpacity;

        BlendMode tempBlend = layers[j].blendMode;
        layers[j].blendMode = entry->layers[j].blendMode;
        entry->layers[j].blendMode = tempBlend;

        bool tempAlphaLock = layers[j].alphaLock;
        layers[j].alphaLock = entry->layers[j].alphaLock;
        entry->layers[j].alphaLock = tempAlphaLock;

        bool tempClipping = layers[j].clipping;
        layers[j].clipping = entry->layers[j].clipping;
        entry->layers[j].clipping = tempClipping;

        char tempName[32];
        memcpy(tempName, layers[j].name, sizeof(tempName));
        memcpy(layers[j].name, entry->layers[j].name, sizeof(layers[j].name));
        memcpy(entry->layers[j].name, tempName, sizeof(entry->layers[j].name));
        layers[j].name[sizeof(layers[j].name) - 1] = '\0';
        entry->layers[j].name[sizeof(entry->layers[j].name) - 1] = '\0';
    }

    free(tempBuffer);
    historyIndex--;
    canvasNeedsUpdate = true;
}

void redo(void) {
    if (!historyInitialized) return;
    if (historyIndex >= historyCount - 1) return;
    if (historyCanvasWidth != CANVAS_WIDTH || historyCanvasHeight != CANVAS_HEIGHT) {
        clearHistoryEntries();
        return;
    }

    historyIndex++;

    HistoryEntry* entry = &historyStack[historyIndex];
    size_t bufferSize = getHistoryBufferSize();
    u32* tempBuffer = (u32*)malloc(bufferSize);
    if (!tempBuffer) {
        historyIndex--;
        return;
    }

    int tempLayerIndex = currentLayerIndex;
    currentLayerIndex = entry->currentLayerIndex;
    entry->currentLayerIndex = tempLayerIndex;

    for (int j = 0; j < MAX_LAYERS; j++) {
        if (layers[j].buffer && entry->layers[j].buffer) {
            swapLayerWithSnapshot(layers[j].buffer, entry->layers[j].buffer, tempBuffer);
        }

        bool tempVisible = layers[j].visible;
        layers[j].visible = entry->layers[j].visible;
        entry->layers[j].visible = tempVisible;

        u8 tempOpacity = layers[j].opacity;
        layers[j].opacity = entry->layers[j].opacity;
        entry->layers[j].opacity = tempOpacity;

        BlendMode tempBlend = layers[j].blendMode;
        layers[j].blendMode = entry->layers[j].blendMode;
        entry->layers[j].blendMode = tempBlend;

        bool tempAlphaLock = layers[j].alphaLock;
        layers[j].alphaLock = entry->layers[j].alphaLock;
        entry->layers[j].alphaLock = tempAlphaLock;

        bool tempClipping = layers[j].clipping;
        layers[j].clipping = entry->layers[j].clipping;
        entry->layers[j].clipping = tempClipping;

        char tempName[32];
        memcpy(tempName, layers[j].name, sizeof(tempName));
        memcpy(layers[j].name, entry->layers[j].name, sizeof(layers[j].name));
        memcpy(entry->layers[j].name, tempName, sizeof(entry->layers[j].name));
        layers[j].name[sizeof(layers[j].name) - 1] = '\0';
        entry->layers[j].name[sizeof(entry->layers[j].name) - 1] = '\0';
    }

    free(tempBuffer);
    canvasNeedsUpdate = true;
}
