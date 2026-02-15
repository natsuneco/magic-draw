#include "canvas.h"

#include "layers.h"

void updateCanvasTexture(void) {
    if (!compositeBuffer || !canvasNeedsUpdate) return;

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
}

void forceUpdateCanvasTexture(void) {
    canvasNeedsUpdate = true;
    updateCanvasTexture();
}
