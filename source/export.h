#pragma once

#include <stdbool.h>
#include <stddef.h>

/**
 * @file export.h
 * @brief PNG export functionality for the canvas.
 */

/**
 * @brief Export the composited canvas to a PNG file.
 *
 * Composites all visible layers and writes the result as a PNG image.
 * The file is saved to sdmc:/3ds/magicdraw/exports/[name]_YYYYMMDD_HHMMSS.png.
 *
 * @param outPath If non-NULL, receives the exported file path on success.
 * @param outPathSize Size of the outPath buffer.
 * @return true on success, false on failure.
 */
bool exportCanvasPNG(char* outPath, size_t outPathSize);
