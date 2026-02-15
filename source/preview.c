#include "preview.h"

#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "blend.h"
#include "project_io.h"
#include "util.h"

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
    if (openPreviewValid) {
        C3D_TexDelete(&openPreviewTex);
        openPreviewValid = false;
    }
}

bool loadProjectPreview(const char* projectName) {
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

    int tw = nextPowerOf2(cw);
    int th = nextPowerOf2(ch);

    u32* tempLayer = (u32*)malloc(tw * th * sizeof(u32));
    u32* composite = (u32*)malloc(tw * th * sizeof(u32));
    if (!tempLayer || !composite) {
        free(tempLayer);
        free(composite);
        fclose(fp);
        return false;
    }

    for (int y = 0; y < ch; y++) {
        for (int x = 0; x < cw; x++) {
            composite[y * tw + x] = 0xFFFFFFFF;
        }
    }

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

        memset(tempLayer, 0, tw * th * sizeof(u32));

        for (int y = 0; y < ch; y++) {
            fread(&tempLayer[y * tw], sizeof(u32), cw, fp);
        }

        if (!visible || opacity == 0) continue;
        if (i >= numLayersLocal) continue;

        for (int y = 0; y < ch; y++) {
            for (int x = 0; x < cw; x++) {
                int idx = y * tw + x;
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

    freeOpenPreview();
    C3D_TexInit(&openPreviewTex, tw, th, GPU_RGBA8);
    C3D_TexSetFilter(&openPreviewTex, GPU_LINEAR, GPU_LINEAR);

    u32* gpuBuf = (u32*)linearAlloc(tw * th * sizeof(u32));
    if (gpuBuf) {
        memcpy(gpuBuf, composite, tw * th * sizeof(u32));
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
    openPreviewValid = true;
    openPreviewWidth = cw;
    openPreviewHeight = ch;

    return true;
}
