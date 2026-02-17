#include "canvas.h"

#include "layers.h"

static void clampDirtyRect(int* minX, int* minY, int* maxX, int* maxY) {
    if (*minX < 0) *minX = 0;
    if (*minY < 0) *minY = 0;
    if (*maxX >= CANVAS_WIDTH) *maxX = CANVAS_WIDTH - 1;
    if (*maxY >= CANVAS_HEIGHT) *maxY = CANVAS_HEIGHT - 1;
}

void markCanvasDirtyFull(void) {
    canvasNeedsUpdate = true;
    canvasDirtyValid = true;
    canvasDirtyMinX = 0;
    canvasDirtyMinY = 0;
    canvasDirtyMaxX = CANVAS_WIDTH - 1;
    canvasDirtyMaxY = CANVAS_HEIGHT - 1;
}

void markCanvasDirtyRect(int minX, int minY, int maxX, int maxY) {
    if (minX > maxX || minY > maxY) return;

    clampDirtyRect(&minX, &minY, &maxX, &maxY);
    if (minX > maxX || minY > maxY) return;

    canvasNeedsUpdate = true;
    if (!canvasDirtyValid) {
        canvasDirtyValid = true;
        canvasDirtyMinX = minX;
        canvasDirtyMinY = minY;
        canvasDirtyMaxX = maxX;
        canvasDirtyMaxY = maxY;
        return;
    }

    if (minX < canvasDirtyMinX) canvasDirtyMinX = minX;
    if (minY < canvasDirtyMinY) canvasDirtyMinY = minY;
    if (maxX > canvasDirtyMaxX) canvasDirtyMaxX = maxX;
    if (maxY > canvasDirtyMaxY) canvasDirtyMaxY = maxY;
}

void updateCanvasTexture(void) {
    if (!compositeBuffer || !canvasNeedsUpdate) return;

    if (!canvasDirtyValid) {
        markCanvasDirtyFull();
    }

    compositeAllLayers();

    GSPGPU_FlushDataCache(compositeBuffer, TEX_WIDTH * TEX_HEIGHT * sizeof(u32));
    C3D_SyncDisplayTransfer(
        (u32*)compositeBuffer, GX_BUFFER_DIM(TEX_WIDTH, TEX_HEIGHT),
        (u32*)canvasTex.data, GX_BUFFER_DIM(TEX_WIDTH, TEX_HEIGHT),
        (GX_TRANSFER_FLIP_VERT(1) | GX_TRANSFER_OUT_TILED(1) | GX_TRANSFER_RAW_COPY(0) |
         GX_TRANSFER_IN_FORMAT(GX_TRANSFER_FMT_RGBA8) | GX_TRANSFER_OUT_FORMAT(GX_TRANSFER_FMT_RGBA8) |
         GX_TRANSFER_SCALING(GX_TRANSFER_SCALE_NO))
    );

    canvasNeedsUpdate = false;
    canvasDirtyValid = false;
}

void forceUpdateCanvasTexture(void) {
    markCanvasDirtyFull();
    updateCanvasTexture();
}
