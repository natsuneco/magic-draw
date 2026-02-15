#include "export.h"

#include "app_state.h"
#include "canvas.h"
#include "layers.h"
#include "util.h"

#include <3ds.h>
#include <png.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define EXPORT_DIR SAVE_DIR "/exports"

bool exportCanvasPNG(char* outPath, size_t outPathSize) {
    // Ensure composite buffer is up to date
    forceUpdateCanvasTexture();

    // Ensure export directory exists
    ensureDirectoryExists(SAVE_DIR);
    ensureDirectoryExists(EXPORT_DIR);

    // Build filename: [name]_YYYYMMDD_HHMMSS.png
    time_t t = time(NULL);
    struct tm* tm = localtime(&t);

    char filePath[256];
    snprintf(filePath, sizeof(filePath),
             "%s/%s_%04d%02d%02d_%02d%02d%02d.png",
             EXPORT_DIR,
             currentProjectName,
             tm->tm_year + 1900, tm->tm_mon + 1, tm->tm_mday,
             tm->tm_hour, tm->tm_min, tm->tm_sec);

    FILE* fp = fopen(filePath, "wb");
    if (!fp) return false;

    png_structp png = png_create_write_struct(PNG_LIBPNG_VER_STRING,
                                              NULL, NULL, NULL);
    if (!png) {
        fclose(fp);
        return false;
    }

    png_infop info = png_create_info_struct(png);
    if (!info) {
        png_destroy_write_struct(&png, NULL);
        fclose(fp);
        return false;
    }

    if (setjmp(png_jmpbuf(png))) {
        png_destroy_write_struct(&png, &info);
        fclose(fp);
        return false;
    }

    png_init_io(png, fp);

    png_set_IHDR(png, info,
                 CANVAS_WIDTH, CANVAS_HEIGHT,
                 8,
                 PNG_COLOR_TYPE_RGBA,
                 PNG_INTERLACE_NONE,
                 PNG_COMPRESSION_TYPE_DEFAULT,
                 PNG_FILTER_TYPE_DEFAULT);

    png_write_info(png, info);

    // Allocate a row buffer (4 bytes per pixel: R, G, B, A)
    png_bytep row = (png_bytep)malloc(CANVAS_WIDTH * 4);
    if (!row) {
        png_destroy_write_struct(&png, &info);
        fclose(fp);
        return false;
    }

    for (int y = 0; y < CANVAS_HEIGHT; y++) {
        for (int x = 0; x < CANVAS_WIDTH; x++) {
            u32 pixel = compositeBuffer[y * TEX_WIDTH + x];
            // compositeBuffer is RGBA8: (R << 24) | (G << 16) | (B << 8) | A
            row[x * 4 + 0] = (pixel >> 24) & 0xFF; // R
            row[x * 4 + 1] = (pixel >> 16) & 0xFF; // G
            row[x * 4 + 2] = (pixel >>  8) & 0xFF; // B
            row[x * 4 + 3] =  pixel        & 0xFF; // A
        }
        png_write_row(png, row);
    }

    free(row);
    png_write_end(png, NULL);
    png_destroy_write_struct(&png, &info);
    fclose(fp);

    if (outPath && outPathSize > 0) {
        snprintf(outPath, outPathSize, "%s", filePath);
    }

    return true;
}
