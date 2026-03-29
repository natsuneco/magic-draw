#include "preview.h"

#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <zlib.h>

#include "blend.h"
#include "project_io.h"
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

static bool readLayerPixelsV2(FILE* fp, u32* layerBuf, int stride, int canvasW, int canvasH) {
    if (!fp || !layerBuf || stride < canvasW || canvasW <= 0 || canvasH <= 0) return false;
    for (int y = 0; y < canvasH; y++) {
        if (fread(&layerBuf[(size_t)y * (size_t)stride], sizeof(u32), canvasW, fp) != (size_t)canvasW) {
            return false;
        }
    }
    return true;
}

static bool readLayerPixelsV3(FILE* fp, u32* layerBuf, int stride, int canvasW, int canvasH) {
    if (!fp || !layerBuf || stride < canvasW || canvasW <= 0 || canvasH <= 0) return false;

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
            u32* dstRow = &layerBuf[(size_t)(rect.y + y) * (size_t)stride + (size_t)rect.x];
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
        if (fread(&layerBuf[(size_t)y * (size_t)stride + (size_t)startX], sizeof(u32), rect.w, fp) != (size_t)rect.w) {
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

void scanProjectFiles(void) {
    openProjectCount = 0;
    DIR* dir = opendir(SAVE_DIR);
    if (!dir) return;

    struct dirent* entry;
    while ((entry = readdir(dir)) != NULL && openProjectCount < OPEN_MAX_PROJECTS) {
        const char* name = entry->d_name;
        size_t len = strlen(name);
        if (len > 5 && strcmp(name + len - 5, ".mgdw") == 0) {
            size_t nameLen = len - 5;
            if (nameLen >= PROJECT_NAME_MAX) nameLen = PROJECT_NAME_MAX - 1;
            memcpy(openProjectNames[openProjectCount], name, nameLen);
            openProjectNames[openProjectCount][nameLen] = '\0';
            openProjectCount++;
        }
    }
    closedir(dir);
}

void freeOpenPreview(void) {
    /* Always delete the texture, regardless of openPreviewValid flag.
       This prevents VRAM leaks when switching previews rapidly. */
    C3D_TexDelete(&openPreviewTex);
    openPreviewValid = false;
}

bool loadProjectPreview(const char* projectName) {
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

    int tw = nextPowerOf2(cw);
    int th = nextPowerOf2(ch);

    size_t canvasPixels = (size_t)cw * (size_t)ch;
    u32* tempLayer = (u32*)malloc(canvasPixels * sizeof(u32));
    u32* composite = (u32*)malloc(canvasPixels * sizeof(u32));
    if (!tempLayer || !composite) {
        free(tempLayer);
        free(composite);
        fclose(fp);
        return false;
    }

    memset(composite, 0xFF, canvasPixels * sizeof(u32));

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
            free(tempLayer);
            free(composite);
            fclose(fp);
            return false;
        }

        if (!visible || opacity == 0 || i >= numLayersLocal) {
            if (header.version >= 3) {
                if (!skipLayerPixelsV3(fp, cw, ch)) {
                    free(tempLayer);
                    free(composite);
                    fclose(fp);
                    return false;
                }
            } else {
                if (fseek(fp, cw * ch * sizeof(u32), SEEK_CUR) != 0) {
                    free(tempLayer);
                    free(composite);
                    fclose(fp);
                    return false;
                }
            }
            continue;
        }

        memset(tempLayer, 0, canvasPixels * sizeof(u32));

        if (header.version >= 3) {
            if (!readLayerPixelsV3(fp, tempLayer, cw, cw, ch)) {
                free(tempLayer);
                free(composite);
                fclose(fp);
                return false;
            }
        } else {
            if (!readLayerPixelsV2(fp, tempLayer, cw, cw, ch)) {
                free(tempLayer);
                free(composite);
                fclose(fp);
                return false;
            }
        }

        for (int y = 0; y < ch; y++) {
            for (int x = 0; x < cw; x++) {
                int idx = y * cw + x;
                u32 src = tempLayer[idx];
                u8 srcA = src & 0xFF;
                if (srcA == 0) continue;
                u32 dst = composite[idx];
                composite[idx] = blendPixel(dst, src, blendMode, opacity);
            }
        }
    }

    fclose(fp);
    free(tempLayer);

    /* Ensure old texture is cleaned up before creating a new one. */
    freeOpenPreview();
    
    if (!C3D_TexInit(&openPreviewTex, tw, th, GPU_RGBA8)) {
        free(composite);
        return false;  /* Texture init failed; unable to create preview. */
    }
    C3D_TexSetFilter(&openPreviewTex, GPU_LINEAR, GPU_LINEAR);

    u32* gpuBuf = (u32*)linearAlloc(tw * th * sizeof(u32));
    if (gpuBuf) {
        memset(gpuBuf, 0, tw * th * sizeof(u32));
        for (int y = 0; y < ch; y++) {
            memcpy(&gpuBuf[y * tw], &composite[y * cw], cw * sizeof(u32));
        }
        GSPGPU_FlushDataCache(gpuBuf, tw * th * sizeof(u32));
        C3D_SyncDisplayTransfer(
            gpuBuf, GX_BUFFER_DIM(tw, th),
            (u32*)openPreviewTex.data, GX_BUFFER_DIM(tw, th),
            (GX_TRANSFER_FLIP_VERT(1) | GX_TRANSFER_OUT_TILED(1) | GX_TRANSFER_RAW_COPY(0) |
             GX_TRANSFER_IN_FORMAT(GX_TRANSFER_FMT_RGBA8) | GX_TRANSFER_OUT_FORMAT(GX_TRANSFER_FMT_RGBA8) |
             GX_TRANSFER_SCALING(GX_TRANSFER_SCALE_NO))
        );
        linearFree(gpuBuf);
    }
    free(composite);

    openPreviewSubTex.width = cw;
    openPreviewSubTex.height = ch;
    openPreviewSubTex.left = 0.0f;
    openPreviewSubTex.top = (float)ch / th;
    openPreviewSubTex.right = (float)cw / tw;
    openPreviewSubTex.bottom = 0.0f;

    openPreviewImage.tex = &openPreviewTex;
    openPreviewImage.subtex = &openPreviewSubTex;
    openPreviewValid = true;  /* Only set to true after successful init. */
    openPreviewWidth = cw;
    openPreviewHeight = ch;

    return true;
}
