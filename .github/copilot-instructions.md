# Copilot Instructions

## Project Overview
- Nintendo 3DS homebrew paint app using devkitPro, citro2d/citro3d.
- Single executable: bottom screen is canvas + UI, top screen is preview/overlay.
- Rendering flow: draw into per-layer RGBA buffers -> compositeAllLayers() -> updateCanvasTexture().

## Build and Run
- Build with devkitPro MSYS2 bash:
  - `c:\devkitPro\msys2\usr\bin\bash.exe -lc "cd /path/to/magic-draw && make"`
- Output: magic-draw.3dsx

## Key Files and Responsibilities
- source/main.c: core app loop, input, layer system, compositing, save (no load yet).
- source/ui_components.c/.h: reusable UI components (ButtonConfig, SliderConfig, ListItemConfig).
- source/ui_theme.h: UI color macros.
- romfs/gfx/icons.t3x: icon spritesheet.

## Data Model Notes
- Layer struct includes buffer, visible, opacity, blendMode, alphaLock, clipping, name[32].
- Clipping is applied at composite time (mask by below layer alpha), not during drawing.
- Alpha lock preserves destination alpha (RGB updates only).

## Save Format (PROJECT_FILE_VERSION 2)
- Header: canvas, current layer/tool, brush size, color, brush alpha, brush type, HSV, palette count.
- Per layer: visible, opacity, blendMode, alphaLock, clipping, name[32], pixel data.
- Per project: brushSizesByType[], paletteUsed[], paletteColors[].
- Load implementation is not present yet; be careful when adding it (support v1/v2).

## Undo/Redo Behavior
- History snapshots store all layers (pixel buffers + metadata) and currentLayerIndex.
- Undo applies to draw, clear, merge, blend mode change, alpha lock, clipping, order changes.
- History size is 10.

## UI Conventions
- Prefer UI components in source/ui_components.c; connect text buffer via uiSetTextBuf().
- Use color macros from source/ui_theme.h.
- Header files should include Doxygen-style documentation comments.
- Keep edits ASCII unless file already uses non-ASCII.

## Maintenance
- When you make changes worth keeping for the future (e.g., adding functions or new behavior), update this file to record them.

## Icons (indices)
- 0 bucket, 1 tool, 2 brush, 3 eraser, 4 close, 5 delete, 6 minus, 7 plus, 8 clear, 9 eye
- 10 up_arrow, 11 down_arrow, 12 merge, 13 zoom_out, 14 zoom_in, 15 undo, 16 redo
- 17 save, 18 save_as, 19 export, 20 pallet_plus, 21 pallet_minus, 22 layer_duplicate
- 23 clipping, 24 alpha_lock, 25 pencil, 26 cross_arrow, 27 check, 28 folder
- 29 settings, 30 new_file
