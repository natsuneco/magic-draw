# Copilot Instructions

## Project Snapshot (as of 2026-02)
- Nintendo 3DS homebrew paint app using devkitPro with citro2d/citro3d.
- Single executable UX: bottom screen for canvas and controls, top screen for preview/overlay.
- Rendering pipeline: draw into per-layer RGBA buffers -> compositeAllLayers() -> updateCanvasTexture().
- Codebase was split from a monolithic `main.c` into focused modules (app_state, blend, brush, canvas, color_utils, history, layers, preview, project_io, ui_screens, util).

## Build
- Use devkitPro MSYS2 bash:
  - `c:\devkitPro\msys2\usr\bin\bash.exe -lc "cd /path/to/magic-draw && make"`
- Main output artifact: `magic-draw.3dsx`.

## Core Files and Ownership
- `source/main.c`: app lifecycle and high-level loop wiring.
- `source/app_state.c/.h`: shared runtime state and app-level control flow.
- `source/canvas.c/.h`: canvas update path and texture upload.
- `source/layers.c/.h`: layer operations, ordering, metadata handling.
- `source/history.c/.h`: snapshot-based undo/redo (all layers + metadata).
- `source/project_io.c/.h`: project save path and related format handling.
- `source/ui_components.c/.h`: reusable UI widgets.
- `source/ui_screens.c/.h`: per-screen UI composition and interactions.
- `source/ui_theme.h`: UI color/theme macros.
- `romfs/gfx/icons.t3x`: icon spritesheet used by the app.

## Data Model and Rendering Rules
- `Layer` includes: buffer, visible, opacity, blendMode, alphaLock, clipping, name[32].
- Clipping is evaluated during compositing (mask by lower-layer alpha), not at stroke write time.
- Alpha lock preserves destination alpha while allowing RGB updates.

## Save Format
- Current project format: `PROJECT_FILE_VERSION 2`.
- Header stores canvas settings, current layer/tool, brush settings (size/alpha/type/color), HSV, and palette count.
- Per-layer payload stores visibility/opacity/blendMode/alphaLock/clipping/name[32]/pixel data.
- Project-level payload stores `brushSizesByType[]`, `paletteUsed[]`, and `paletteColors[]`.

## Undo/Redo
- History stores full-layer snapshots (pixels + metadata) and `currentLayerIndex`.
- Undo targets drawing and structural edits (clear, merge, blend mode, alpha lock, clipping, layer order).
- History capacity is 10 entries.
- Large-canvas stability updates are already applied:
  - canvas-area snapshot storage (instead of full texture area),
  - safe history reset on canvas size changes,
  - oldest-entry drop with allocation retry under memory pressure.

## UI and Code Conventions
- Prefer reusable controls from `ui_components` and call `uiSetTextBuf()` for text rendering.
- Use color/theme macros from `source/ui_theme.h`.
- Keep header declarations documented with Doxygen-style comments.
- Keep edits ASCII unless the target file already contains non-ASCII text.
- Existing UX additions include:
  - Settings screen toggle: "Show menu button",
  - reduced L-button overlay button sizes,
  - loosened zoom limits,
  - lightweight top-preview rendering during drawing,
  - dirty-rect compositing and adaptive draw update interval.

## Maintenance Policy for This File
- Do not treat this file as an append-only change log.
- When behavior, architecture, or workflow changes, rewrite this document as a whole so it always represents the latest project state.
- Keep only the information that is currently true and operationally useful.

## Icon Indices
- 0 bucket, 1 tool, 2 brush, 3 eraser, 4 close, 5 delete, 6 minus, 7 plus, 8 clear, 9 eye
- 10 up_arrow, 11 down_arrow, 12 merge, 13 zoom_out, 14 zoom_in, 15 undo, 16 redo
- 17 save, 18 save_as, 19 export, 20 pallet_plus, 21 pallet_minus, 22 layer_duplicate
- 23 clipping, 24 alpha_lock, 25 pencil, 26 cross_arrow, 27 check, 28 folder
- 29 settings, 30 new_file
