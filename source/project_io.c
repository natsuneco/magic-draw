#include "project_io.h"

#include <ctype.h>
#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <zlib.h>

#include "history.h"
#include "layers.h"
#include "util.h"

typedef struct {
    u32 x;
    u32 y;
    u32 w;
    u32 h;
} LayerRect;

#define LAYER_RECT_COMPRESSED_FLAG 0x80000000u
#define LAYER_RECT_WIDTH_MASK 0x7FFFFFFFu

static bool isLayerRectValid(LayerRect rect, int canvasW, int canvasH) {
    if (rect.w == 0 || rect.h == 0) {
        return rect.x == 0 && rect.y == 0;
    }

    if (rect.x >= (u32)canvasW || rect.y >= (u32)canvasH) return false;
    if (rect.w > (u32)canvasW || rect.h > (u32)canvasH) return false;
    if (rect.x + rect.w > (u32)canvasW) return false;
    if (rect.y + rect.h > (u32)canvasH) return false;
    return true;
}

static LayerRect computeLayerBounds(const u32* layerBuf, int canvasW, int canvasH) {
    LayerRect rect = {0, 0, 0, 0};
    if (!layerBuf || canvasW <= 0 || canvasH <= 0) return rect;

    int minX = canvasW;
    int minY = canvasH;
    int maxX = -1;
    int maxY = -1;

    for (int y = 0; y < canvasH; y++) {
        const u32* row = &layerBuf[(size_t)y * (size_t)TEX_WIDTH];
        for (int x = 0; x < canvasW; x++) {
            if ((row[x] & 0xFF) == 0) continue;
            if (x < minX) minX = x;
            if (y < minY) minY = y;
            if (x > maxX) maxX = x;
            if (y > maxY) maxY = y;
        }
    }

    if (maxX < minX || maxY < minY) return rect;

    rect.x = (u32)minX;
    rect.y = (u32)minY;
    rect.w = (u32)(maxX - minX + 1);
    rect.h = (u32)(maxY - minY + 1);
    return rect;
}

static bool writeLayerPixelsV2(FILE* fp, const u32* layerBuf, int canvasW, int canvasH) {
    if (!fp || !layerBuf || canvasW <= 0 || canvasH <= 0) return false;
    for (int y = 0; y < canvasH; y++) {
        if (fwrite(&layerBuf[(size_t)y * (size_t)TEX_WIDTH], sizeof(u32), canvasW, fp) != (size_t)canvasW) {
            return false;
        }
    }
    return true;
}

static bool writeLayerPixelsV3(FILE* fp, const u32* layerBuf, int canvasW, int canvasH) {
    if (!fp || !layerBuf || canvasW <= 0 || canvasH <= 0) return false;

    LayerRect rect = computeLayerBounds(layerBuf, canvasW, canvasH);
    if (rect.w == 0 || rect.h == 0) {
        return fwrite(&rect, sizeof(rect), 1, fp) == 1;
    }

    size_t rawBytes = (size_t)rect.w * (size_t)rect.h * sizeof(u32);
    u8* rawBuf = (u8*)malloc(rawBytes);
    if (!rawBuf) {
        /* Fallback: write uncompressed row by row directly from layer buffer */
        if (fwrite(&rect, sizeof(rect), 1, fp) != 1) return false;
        for (u32 y = 0; y < rect.h; y++) {
            const u32* srcRow = &layerBuf[(size_t)(rect.y + y) * (size_t)TEX_WIDTH + (size_t)rect.x];
            if (fwrite(srcRow, sizeof(u32), rect.w, fp) != (size_t)rect.w) return false;
        }
        return true;
    }

    for (u32 y = 0; y < rect.h; y++) {
        const u32* srcRow = &layerBuf[(size_t)(rect.y + y) * (size_t)TEX_WIDTH + (size_t)rect.x];
        memcpy(&rawBuf[(size_t)y * (size_t)rect.w * sizeof(u32)], srcRow, (size_t)rect.w * sizeof(u32));
    }

    uLongf compressedCapacity = compressBound((uLong)rawBytes);
    u8* compressedBuf = (u8*)malloc((size_t)compressedCapacity);
    bool useCompressed = false;
    uLongf compressedSize = 0;
    if (compressedBuf) {
        compressedSize = compressedCapacity;
        if (compress2(compressedBuf, &compressedSize, rawBuf, (uLong)rawBytes, Z_BEST_SPEED) == Z_OK &&
            compressedSize > 0 &&
            compressedSize <= 0xFFFFFFFFu &&
            compressedSize + sizeof(u32) < rawBytes) {
            useCompressed = true;
        }
    }

    LayerRect writtenRect = rect;
    if (useCompressed) {
        writtenRect.w |= LAYER_RECT_COMPRESSED_FLAG;
    }
    if (fwrite(&writtenRect, sizeof(writtenRect), 1, fp) != 1) {
        free(compressedBuf);
        free(rawBuf);
        return false;
    }

    if (useCompressed) {
        u32 compressedSize32 = (u32)compressedSize;
        bool ok = fwrite(&compressedSize32, sizeof(compressedSize32), 1, fp) == 1 &&
                  fwrite(compressedBuf, 1, (size_t)compressedSize32, fp) == (size_t)compressedSize32;
        free(compressedBuf);
        free(rawBuf);
        return ok;
    }

    bool ok = fwrite(rawBuf, 1, rawBytes, fp) == rawBytes;
    free(compressedBuf);
    free(rawBuf);
    return ok;
}

static bool readLayerPixelsV2(FILE* fp, u32* layerBuf, int canvasW, int canvasH) {
    if (!fp || !layerBuf || canvasW <= 0 || canvasH <= 0) return false;
    for (int y = 0; y < canvasH; y++) {
        if (fread(&layerBuf[(size_t)y * (size_t)TEX_WIDTH], sizeof(u32), canvasW, fp) != (size_t)canvasW) {
            return false;
        }
    }
    return true;
}

static bool readLayerPixelsV3(FILE* fp, u32* layerBuf, int canvasW, int canvasH) {
    if (!fp || !layerBuf || canvasW <= 0 || canvasH <= 0) return false;

    LayerRect storedRect;
    if (fread(&storedRect, sizeof(storedRect), 1, fp) != 1) return false;

    bool compressed = (storedRect.w & LAYER_RECT_COMPRESSED_FLAG) != 0;
    LayerRect rect = storedRect;
    rect.w &= LAYER_RECT_WIDTH_MASK;

    if (rect.w == 0 || rect.h == 0) {
        if (!isLayerRectValid(rect, canvasW, canvasH)) return false;
        return true;
    }

    if (!isLayerRectValid(rect, canvasW, canvasH)) return false;

    if (compressed) {
        u32 compressedSize = 0;
        if (fread(&compressedSize, sizeof(compressedSize), 1, fp) != 1) return false;
        if (compressedSize == 0) return false;

        size_t rawBytes = (size_t)rect.w * (size_t)rect.h * sizeof(u32);
        u8* compressedBuf = (u8*)malloc(compressedSize);
        u8* rawBuf = (u8*)malloc(rawBytes);
        if (!compressedBuf || !rawBuf) {
            free(compressedBuf);
            free(rawBuf);
            return false;
        }

        if (fread(compressedBuf, 1, compressedSize, fp) != compressedSize) {
            free(compressedBuf);
            free(rawBuf);
            return false;
        }

        uLongf outSize = (uLongf)rawBytes;
        if (uncompress(rawBuf, &outSize, compressedBuf, compressedSize) != Z_OK || outSize != rawBytes) {
            free(compressedBuf);
            free(rawBuf);
            return false;
        }

        for (u32 y = 0; y < rect.h; y++) {
            u32* dstRow = &layerBuf[(size_t)(rect.y + y) * (size_t)TEX_WIDTH + (size_t)rect.x];
            memcpy(dstRow, &rawBuf[(size_t)y * (size_t)rect.w * sizeof(u32)], (size_t)rect.w * sizeof(u32));
        }

        free(compressedBuf);
        free(rawBuf);
        return true;
    }

    u32 startX = rect.x;
    u32 startY = rect.y;
    u32 endY = startY + rect.h;

    for (u32 y = startY; y < endY; y++) {
        u32* row = &layerBuf[(size_t)y * (size_t)TEX_WIDTH + (size_t)startX];
        if (fread(row, sizeof(u32), rect.w, fp) != (size_t)rect.w) {
            return false;
        }
    }

    return true;
}

static bool skipLayerPixelsV3(FILE* fp, int canvasW, int canvasH) {
    if (!fp || canvasW <= 0 || canvasH <= 0) return false;

    LayerRect storedRect;
    if (fread(&storedRect, sizeof(storedRect), 1, fp) != 1) return false;

    bool compressed = (storedRect.w & LAYER_RECT_COMPRESSED_FLAG) != 0;
    LayerRect rect = storedRect;
    rect.w &= LAYER_RECT_WIDTH_MASK;

    if (rect.w == 0 || rect.h == 0) {
        if (!isLayerRectValid(rect, canvasW, canvasH)) return false;
        return true;
    }

    if (!isLayerRectValid(rect, canvasW, canvasH)) return false;

    if (compressed) {
        u32 compressedSize = 0;
        if (fread(&compressedSize, sizeof(compressedSize), 1, fp) != 1) return false;
        if (compressedSize == 0) return false;
        return fseek(fp, (long)compressedSize, SEEK_CUR) == 0;
    }

    return fseek(fp, (long)((size_t)rect.w * (size_t)rect.h * sizeof(u32)), SEEK_CUR) == 0;
}

int findNextUntitledIndex(void) {
    const char* prefix = "Untitled ";
    const char* suffix = ".mgdw";
    size_t prefixLen = strlen(prefix);
    size_t suffixLen = strlen(suffix);
    int maxIndex = 0;

    ensureDirectoryExists(SAVE_DIR);
    DIR* dir = opendir(SAVE_DIR);
    if (dir) {
        struct dirent* ent;
        while ((ent = readdir(dir)) != NULL) {
            const char* name = ent->d_name;
            size_t len = strlen(name);
            if (len <= prefixLen + suffixLen) continue;
            if (strncmp(name, prefix, prefixLen) != 0) continue;
            if (strcmp(name + len - suffixLen, suffix) != 0) continue;

            size_t numLen = len - prefixLen - suffixLen;
            if (numLen == 0 || numLen >= 12) continue;
            char numBuf[12];
            memcpy(numBuf, name + prefixLen, numLen);
            numBuf[numLen] = '\0';

            bool allDigits = true;
            for (size_t i = 0; i < numLen; i++) {
                if (!isdigit((unsigned char)numBuf[i])) {
                    allDigits = false;
                    break;
                }
            }
            if (!allDigits) continue;

            int val = atoi(numBuf);
            if (val > maxIndex) maxIndex = val;
        }
        closedir(dir);
    }

    int candidate = maxIndex + 1;
    while (1) {
        char filePath[256];
        snprintf(filePath, sizeof(filePath), "%s/Untitled %d.mgdw", SAVE_DIR, candidate);
        if (!fileExists(filePath)) break;
        candidate++;
    }
    return candidate;
}

bool saveProject(const char* projectName) {
    ensureDirectoryExists(SAVE_DIR);

    char filePath[256];
    snprintf(filePath, sizeof(filePath), "%s/%s.mgdw", SAVE_DIR, projectName);

    if (fileExists(filePath)) {
        int suffix = 1;
        do {
            snprintf(filePath, sizeof(filePath), "%s/%s_%d.mgdw", SAVE_DIR, projectName, suffix);
            suffix++;
        } while (fileExists(filePath) && suffix < 100);

        if (suffix >= 100) {
            return false;
        }
    }

    FILE* fp = fopen(filePath, "wb");
    if (!fp) {
        return false;
    }

    setvbuf(fp, NULL, _IOFBF, 1 << 20);

    ProjectHeader header;
    header.magic = PROJECT_FILE_MAGIC;
    header.version = PROJECT_FILE_VERSION;
    header.canvasWidth = CANVAS_WIDTH;
    header.canvasHeight = CANVAS_HEIGHT;
    header.numLayers = MAX_LAYERS;
    header.currentLayer = currentLayerIndex;
    header.currentTool = currentTool;
    header.brushSize = getCurrentBrushSize();
    header.currentColor = currentColor;
    header.brushAlpha = brushAlpha;
    header.currentBrushType = (u32)currentBrushType;
    header.hue = currentHue;
    header.saturation = currentSaturation;
    header.value = currentValue;

    header.paletteCount = 0;
    for (int i = 0; i < PALETTE_MAX_COLORS; i++) {
        if (paletteUsed[i]) header.paletteCount++;
    }

    if (fwrite(&header, sizeof(ProjectHeader), 1, fp) != 1) {
        fclose(fp);
        return false;
    }

    for (int i = 0; i < MAX_LAYERS; i++) {
        if (fwrite(&layers[i].visible, sizeof(bool), 1, fp) != 1 ||
            fwrite(&layers[i].opacity, sizeof(u8), 1, fp) != 1 ||
            fwrite(&layers[i].blendMode, sizeof(BlendMode), 1, fp) != 1 ||
            fwrite(&layers[i].alphaLock, sizeof(bool), 1, fp) != 1 ||
            fwrite(&layers[i].clipping, sizeof(bool), 1, fp) != 1 ||
            fwrite(layers[i].name, sizeof(layers[i].name), 1, fp) != 1) {
            fclose(fp);
            return false;
        }

        if (PROJECT_FILE_VERSION >= 3) {
            if (!writeLayerPixelsV3(fp, layers[i].buffer, CANVAS_WIDTH, CANVAS_HEIGHT)) {
                fclose(fp);
                return false;
            }
        } else {
            if (!writeLayerPixelsV2(fp, layers[i].buffer, CANVAS_WIDTH, CANVAS_HEIGHT)) {
                fclose(fp);
                return false;
            }
        }
    }

    if (fwrite(brushSizesByType, sizeof(brushSizesByType[0]), BRUSH_TYPE_COUNT, fp) != BRUSH_TYPE_COUNT) {
        fclose(fp);
        return false;
    }

    if (fwrite(paletteUsed, sizeof(bool), PALETTE_MAX_COLORS, fp) != PALETTE_MAX_COLORS ||
        fwrite(paletteColors, sizeof(u32), PALETTE_MAX_COLORS, fp) != PALETTE_MAX_COLORS) {
        fclose(fp);
        return false;
    }

    fclose(fp);

    strncpy(currentProjectName, projectName, PROJECT_NAME_MAX - 1);
    currentProjectName[PROJECT_NAME_MAX - 1] = '\0';
    projectHasName = true;
    projectHasUnsavedChanges = false;

    return true;
}

bool quickSaveProject(void) {
    if (!projectHasName || currentProjectName[0] == '\0') {
        return false;
    }

    ensureDirectoryExists(SAVE_DIR);

    char filePath[256];
    snprintf(filePath, sizeof(filePath), "%s/%s.mgdw", SAVE_DIR, currentProjectName);

    FILE* fp = fopen(filePath, "wb");
    if (!fp) {
        return false;
    }

    setvbuf(fp, NULL, _IOFBF, 1 << 20);

    ProjectHeader header;
    header.magic = PROJECT_FILE_MAGIC;
    header.version = PROJECT_FILE_VERSION;
    header.canvasWidth = CANVAS_WIDTH;
    header.canvasHeight = CANVAS_HEIGHT;
    header.numLayers = MAX_LAYERS;
    header.currentLayer = currentLayerIndex;
    header.currentTool = currentTool;
    header.brushSize = getCurrentBrushSize();
    header.currentColor = currentColor;
    header.brushAlpha = brushAlpha;
    header.currentBrushType = (u32)currentBrushType;
    header.hue = currentHue;
    header.saturation = currentSaturation;
    header.value = currentValue;

    header.paletteCount = 0;
    for (int i = 0; i < PALETTE_MAX_COLORS; i++) {
        if (paletteUsed[i]) header.paletteCount++;
    }

    if (fwrite(&header, sizeof(ProjectHeader), 1, fp) != 1) {
        fclose(fp);
        return false;
    }

    for (int i = 0; i < MAX_LAYERS; i++) {
        if (fwrite(&layers[i].visible, sizeof(bool), 1, fp) != 1 ||
            fwrite(&layers[i].opacity, sizeof(u8), 1, fp) != 1 ||
            fwrite(&layers[i].blendMode, sizeof(BlendMode), 1, fp) != 1 ||
            fwrite(&layers[i].alphaLock, sizeof(bool), 1, fp) != 1 ||
            fwrite(&layers[i].clipping, sizeof(bool), 1, fp) != 1 ||
            fwrite(layers[i].name, sizeof(layers[i].name), 1, fp) != 1) {
            fclose(fp);
            return false;
        }

        if (PROJECT_FILE_VERSION >= 3) {
            if (!writeLayerPixelsV3(fp, layers[i].buffer, CANVAS_WIDTH, CANVAS_HEIGHT)) {
                fclose(fp);
                return false;
            }
        } else {
            if (!writeLayerPixelsV2(fp, layers[i].buffer, CANVAS_WIDTH, CANVAS_HEIGHT)) {
                fclose(fp);
                return false;
            }
        }
    }

    if (fwrite(brushSizesByType, sizeof(brushSizesByType[0]), BRUSH_TYPE_COUNT, fp) != BRUSH_TYPE_COUNT) {
        fclose(fp);
        return false;
    }

    if (fwrite(paletteUsed, sizeof(bool), PALETTE_MAX_COLORS, fp) != PALETTE_MAX_COLORS ||
        fwrite(paletteColors, sizeof(u32), PALETTE_MAX_COLORS, fp) != PALETTE_MAX_COLORS) {
        fclose(fp);
        return false;
    }

    fclose(fp);

    projectHasUnsavedChanges = false;

    return true;
}

bool loadProject(const char* projectName) {
    char filePath[256];
    snprintf(filePath, sizeof(filePath), "%s/%s.mgdw", SAVE_DIR, projectName);

    FILE* fp = fopen(filePath, "rb");
    if (!fp) return false;

    setvbuf(fp, NULL, _IOFBF, 1 << 20);

    ProjectHeader header;
    if (fread(&header, sizeof(ProjectHeader), 1, fp) != 1) { fclose(fp); return false; }
    if (header.magic != PROJECT_FILE_MAGIC) { fclose(fp); return false; }

    int cw = header.canvasWidth;
    int ch = header.canvasHeight;
    if (cw <= 0 || cw > MAX_CANVAS_DIM || ch <= 0 || ch > MAX_CANVAS_DIM) { fclose(fp); return false; }

    if (!applyCanvasSize(cw, ch)) {
        fclose(fp);
        return false;
    }

    int numLayersLocal = header.numLayers;
    if (numLayersLocal > MAX_LAYERS) numLayersLocal = MAX_LAYERS;

    for (int i = 0; i < (int)header.numLayers; i++) {
        bool visible;
        u8 opacity;
        BlendMode blendMode;
        bool alphaLock, clipping;
        char layerName[32];

        if (fread(&visible, sizeof(bool), 1, fp) != 1 ||
            fread(&opacity, sizeof(u8), 1, fp) != 1 ||
            fread(&blendMode, sizeof(BlendMode), 1, fp) != 1 ||
            fread(&alphaLock, sizeof(bool), 1, fp) != 1 ||
            fread(&clipping, sizeof(bool), 1, fp) != 1 ||
            fread(layerName, sizeof(layerName), 1, fp) != 1) {
            fclose(fp);
            return false;
        }

        if (i < numLayersLocal && layers[i].buffer) {
            layers[i].visible = visible;
            layers[i].opacity = opacity;
            layers[i].blendMode = blendMode;
            layers[i].alphaLock = alphaLock;
            layers[i].clipping = clipping;
            memcpy(layers[i].name, layerName, sizeof(layers[i].name));

            memset(layers[i].buffer, 0, TEX_WIDTH * TEX_HEIGHT * sizeof(u32));

            if (header.version >= 3) {
                if (!readLayerPixelsV3(fp, layers[i].buffer, cw, ch)) {
                    fclose(fp);
                    return false;
                }
            } else {
                if (!readLayerPixelsV2(fp, layers[i].buffer, cw, ch)) {
                    fclose(fp);
                    return false;
                }
            }
        } else {
            if (header.version >= 3) {
                if (!skipLayerPixelsV3(fp, cw, ch)) {
                    fclose(fp);
                    return false;
                }
            } else {
                if (fseek(fp, cw * ch * sizeof(u32), SEEK_CUR) != 0) {
                    fclose(fp);
                    return false;
                }
            }
        }
    }

    if (header.version >= 2) {
        long payloadStart = ftell(fp);
        fseek(fp, 0, SEEK_END);
        long payloadEnd = ftell(fp);
        fseek(fp, payloadStart, SEEK_SET);

        size_t paletteBytes = PALETTE_MAX_COLORS * sizeof(bool) + PALETTE_MAX_COLORS * sizeof(u32);
        size_t fullBrushBytes = BRUSH_TYPE_COUNT * sizeof(brushSizesByType[0]);
        int brushCountToRead = BRUSH_TYPE_COUNT;

        if (payloadEnd >= payloadStart) {
            size_t remainingBytes = (size_t)(payloadEnd - payloadStart);
            if (remainingBytes < paletteBytes + fullBrushBytes) {
                brushCountToRead = BRUSH_TYPE_COUNT - 1;
            }
        }

        if (fread(brushSizesByType, sizeof(brushSizesByType[0]), brushCountToRead, fp) != (size_t)brushCountToRead) {
            fclose(fp);
            return false;
        }
        for (int i = brushCountToRead; i < BRUSH_TYPE_COUNT; i++) {
            brushSizesByType[i] = brushSizesByType[0];
        }

        if (fread(paletteUsed, sizeof(bool), PALETTE_MAX_COLORS, fp) != PALETTE_MAX_COLORS ||
            fread(paletteColors, sizeof(u32), PALETTE_MAX_COLORS, fp) != PALETTE_MAX_COLORS) {
            fclose(fp);
            return false;
        }
    }

    fclose(fp);

    currentLayerIndex = header.currentLayer;
    if (currentLayerIndex >= MAX_LAYERS) currentLayerIndex = 0;
    currentTool = (ToolType)header.currentTool;
    setCurrentBrushSize(header.brushSize);
    currentColor = header.currentColor;
    brushAlpha = header.brushAlpha;
    currentBrushType = header.currentBrushType;
    if (currentBrushType < 0 || currentBrushType >= (int)NUM_BRUSHES) {
        currentBrushType = 0;
    }
    currentHue = header.hue;
    currentSaturation = header.saturation;
    currentValue = header.value;

    strncpy(currentProjectName, projectName, PROJECT_NAME_MAX - 1);
    currentProjectName[PROJECT_NAME_MAX - 1] = '\0';
    projectHasName = true;
    projectHasUnsavedChanges = false;

    exitHistory();
    initHistory();
    canvasPanX = 0.0f;
    canvasPanY = 0.0f;
    canvasZoom = 1.0f;
    canvasNeedsUpdate = true;

    return true;
}
