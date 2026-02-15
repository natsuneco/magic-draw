#include "project_io.h"

#include <ctype.h>
#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "history.h"
#include "layers.h"
#include "util.h"

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

    fwrite(&header, sizeof(ProjectHeader), 1, fp);

    for (int i = 0; i < MAX_LAYERS; i++) {
        fwrite(&layers[i].visible, sizeof(bool), 1, fp);
        fwrite(&layers[i].opacity, sizeof(u8), 1, fp);
        fwrite(&layers[i].blendMode, sizeof(BlendMode), 1, fp);
        fwrite(&layers[i].alphaLock, sizeof(bool), 1, fp);
        fwrite(&layers[i].clipping, sizeof(bool), 1, fp);
        fwrite(layers[i].name, sizeof(layers[i].name), 1, fp);

        for (int y = 0; y < CANVAS_HEIGHT; y++) {
            fwrite(&layers[i].buffer[y * TEX_WIDTH], sizeof(u32), CANVAS_WIDTH, fp);
        }
    }

    fwrite(brushSizesByType, sizeof(brushSizesByType[0]), BRUSH_TYPE_COUNT, fp);

    fwrite(paletteUsed, sizeof(bool), PALETTE_MAX_COLORS, fp);
    fwrite(paletteColors, sizeof(u32), PALETTE_MAX_COLORS, fp);

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

    fwrite(&header, sizeof(ProjectHeader), 1, fp);

    for (int i = 0; i < MAX_LAYERS; i++) {
        fwrite(&layers[i].visible, sizeof(bool), 1, fp);
        fwrite(&layers[i].opacity, sizeof(u8), 1, fp);
        fwrite(&layers[i].blendMode, sizeof(BlendMode), 1, fp);
        fwrite(&layers[i].alphaLock, sizeof(bool), 1, fp);
        fwrite(&layers[i].clipping, sizeof(bool), 1, fp);
        fwrite(layers[i].name, sizeof(layers[i].name), 1, fp);

        for (int y = 0; y < CANVAS_HEIGHT; y++) {
            fwrite(&layers[i].buffer[y * TEX_WIDTH], sizeof(u32), CANVAS_WIDTH, fp);
        }
    }

    fwrite(brushSizesByType, sizeof(brushSizesByType[0]), BRUSH_TYPE_COUNT, fp);

    fwrite(paletteUsed, sizeof(bool), PALETTE_MAX_COLORS, fp);
    fwrite(paletteColors, sizeof(u32), PALETTE_MAX_COLORS, fp);

    fclose(fp);

    projectHasUnsavedChanges = false;

    return true;
}

bool loadProject(const char* projectName) {
    char filePath[256];
    snprintf(filePath, sizeof(filePath), "%s/%s.mgdw", SAVE_DIR, projectName);

    FILE* fp = fopen(filePath, "rb");
    if (!fp) return false;

    ProjectHeader header;
    if (fread(&header, sizeof(ProjectHeader), 1, fp) != 1) { fclose(fp); return false; }
    if (header.magic != PROJECT_FILE_MAGIC) { fclose(fp); return false; }

    int cw = header.canvasWidth;
    int ch = header.canvasHeight;
    if (cw <= 0 || cw > MAX_CANVAS_DIM || ch <= 0 || ch > MAX_CANVAS_DIM) { fclose(fp); return false; }

    applyCanvasSize(cw, ch);

    int numLayersLocal = header.numLayers;
    if (numLayersLocal > MAX_LAYERS) numLayersLocal = MAX_LAYERS;

    for (int i = 0; i < (int)header.numLayers; i++) {
        bool visible;
        u8 opacity;
        BlendMode blendMode;
        bool alphaLock, clipping;
        char layerName[32];

        fread(&visible, sizeof(bool), 1, fp);
        fread(&opacity, sizeof(u8), 1, fp);
        fread(&blendMode, sizeof(BlendMode), 1, fp);
        fread(&alphaLock, sizeof(bool), 1, fp);
        fread(&clipping, sizeof(bool), 1, fp);
        fread(layerName, sizeof(layerName), 1, fp);

        if (i < numLayersLocal && layers[i].buffer) {
            layers[i].visible = visible;
            layers[i].opacity = opacity;
            layers[i].blendMode = blendMode;
            layers[i].alphaLock = alphaLock;
            layers[i].clipping = clipping;
            memcpy(layers[i].name, layerName, sizeof(layers[i].name));

            memset(layers[i].buffer, 0, TEX_WIDTH * TEX_HEIGHT * sizeof(u32));

            for (int y = 0; y < ch; y++) {
                fread(&layers[i].buffer[y * TEX_WIDTH], sizeof(u32), cw, fp);
            }
        } else {
            fseek(fp, cw * ch * sizeof(u32), SEEK_CUR);
        }
    }

    if (header.version >= 2) {
        fread(brushSizesByType, sizeof(brushSizesByType[0]), BRUSH_TYPE_COUNT, fp);
        fread(paletteUsed, sizeof(bool), PALETTE_MAX_COLORS, fp);
        fread(paletteColors, sizeof(u32), PALETTE_MAX_COLORS, fp);
    }

    fclose(fp);

    currentLayerIndex = header.currentLayer;
    if (currentLayerIndex >= MAX_LAYERS) currentLayerIndex = 0;
    currentTool = (ToolType)header.currentTool;
    setCurrentBrushSize(header.brushSize);
    currentColor = header.currentColor;
    brushAlpha = header.brushAlpha;
    currentBrushType = header.currentBrushType;
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
