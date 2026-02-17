// Magic Draw - 3DS Drawing Application

#include <citro2d.h>
#include <3ds.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

#include "app_state.h"
#include "blend.h"
#include "brush.h"
#include "canvas.h"
#include "color_utils.h"
#include "export.h"
#include "history.h"
#include "layers.h"
#include "preview.h"
#include "project_io.h"
#include "ui_components.h"
#include "ui_screens.h"
#include "ui_theme.h"
#include "util.h"

//---------------------------------------------------------------------------------
// Main function
//---------------------------------------------------------------------------------
int main(int argc, char* argv[]) {
    // Initialize libraries
    gfxInitDefault();
    romfsInit();
    C3D_Init(C3D_DEFAULT_CMDBUF_SIZE);
    C2D_Init(C2D_DEFAULT_MAX_OBJECTS);
    C2D_Prepare();

    // Create render targets
    C3D_RenderTarget* topScreen = C2D_CreateScreenTarget(GFX_TOP, GFX_LEFT);
    C3D_RenderTarget* bottomScreen = C2D_CreateScreenTarget(GFX_BOTTOM, GFX_LEFT);

    // Store in global variables for dialog rendering
    g_topScreen = topScreen;
    g_bottomScreen = bottomScreen;

    // Initialize icons
    initIcons();

    // Initialize text rendering
    g_textBuf = C2D_TextBufNew(256);
    uiSetTextBuf(g_textBuf);

    // Initialize layers
    initLayers();

    // Initialize history system
    initHistory();

    // Initialize color palette
    initPalette();

    // Initialize HSV from current color
    rgbToHsv(currentColor, &currentHue, &currentSaturation, &currentValue);

    // Initialize frame time for FPS calculation
    lastFrameTime = osGetTime();

    // Main loop
    while (aptMainLoop()) {
        // Calculate FPS
        u64 currentTime = osGetTime();
        u64 deltaTime = currentTime - lastFrameTime;
        if (deltaTime > 0) {
            currentFPS = 1000.0f / deltaTime;
        }
        lastFrameTime = currentTime;

        hidScanInput();

        u32 kDown = hidKeysDown();
        u32 kHeld = hidKeysHeld();
        u32 kUp = hidKeysUp();

        // START: quick save
        if (kDown & KEY_START) {
            if (projectHasName && currentProjectName[0] != '\0') {
                quickSaveProject();
            } else {
                showDialog(g_topScreen, g_bottomScreen,
                           "No Project Name",
                           "Create a project from Home\nso a name is set.");
            }
        }

        if (currentMode == MODE_HOME) {
            // === HOME MODE ===
            touchPosition touch;
            hidTouchRead(&touch);

            if (kDown & KEY_TOUCH) {
                // Calculate home menu button positions (same as renderHomeMenu)
                float totalWidth = BTN_SIZE_LARGE * 3 + SAVE_MENU_ITEM_SPACING * 2;
                float startX = (BOTTOM_SCREEN_WIDTH - totalWidth) / 2;
                float itemY = SAVE_MENU_Y;

                // Check New button (index 0)
                float newBtnX = startX;
                if (touch.px >= newBtnX && touch.px < newBtnX + BTN_SIZE_LARGE &&
                    touch.py >= itemY && touch.py < itemY + BTN_SIZE_LARGE) {
                    // Initialize new project defaults
                    int untitledIndex = findNextUntitledIndex();
                    snprintf(newProjectName, sizeof(newProjectName), "Untitled %d", untitledIndex);
                    newProjectWidth = BOTTOM_SCREEN_WIDTH;
                    newProjectHeight = BOTTOM_SCREEN_HEIGHT;
                    currentMode = MODE_NEW_PROJECT;
                }

                // Check Open button (index 1)
                float openBtnX = startX + BTN_SIZE_LARGE + SAVE_MENU_ITEM_SPACING;
                if (touch.px >= openBtnX && touch.px < openBtnX + BTN_SIZE_LARGE &&
                    touch.py >= itemY && touch.py < itemY + BTN_SIZE_LARGE) {
                    // Scan for existing projects
                    scanProjectFiles();
                    openSelectedIndex = -1;
                    openListScrollY = 0;
                    openListDragging = false;
                    openPreviewValid = false;
                    currentMode = MODE_OPEN;
                }

                // Check Settings button (index 2)
                float settingsBtnX = startX + (BTN_SIZE_LARGE + SAVE_MENU_ITEM_SPACING) * 2;
                if (touch.px >= settingsBtnX && touch.px < settingsBtnX + BTN_SIZE_LARGE &&
                    touch.py >= itemY && touch.py < itemY + BTN_SIZE_LARGE) {
                    currentMode = MODE_SETTINGS;
                }
            }

            // Render frame
            C3D_FrameBegin(C3D_FRAME_SYNCDRAW);
            renderHomeTop(topScreen);
            renderHomeMenu(bottomScreen);
            C3D_FrameEnd(0);

        } else if (currentMode == MODE_SETTINGS) {
            // === SETTINGS MODE ===
            touchPosition touch;
            hidTouchRead(&touch);

            if (kDown & KEY_TOUCH) {
                float checkboxX = MENU_CONTENT_PADDING;
                float checkboxY = MENU_CONTENT_Y + MENU_CONTENT_PADDING;
                float checkboxSize = 20.0f;

                if (touch.px >= checkboxX && touch.px < checkboxX + checkboxSize &&
                    touch.py >= checkboxY && touch.py < checkboxY + checkboxSize) {
                    showDrawMenuButton = !showDrawMenuButton;
                }

                // Back icon (bottom bar left)
                if (touch.px >= MENU_BTN_X && touch.px < MENU_BTN_X + MENU_BTN_SIZE &&
                    touch.py >= MENU_BTN_Y && touch.py < MENU_BTN_Y + MENU_BTN_SIZE) {
                    currentMode = MODE_HOME;
                }
            }

            if (kDown & KEY_B) {
                currentMode = MODE_HOME;
            }

            C3D_FrameBegin(C3D_FRAME_SYNCDRAW);
            renderSettingsTop(topScreen);
            renderSettingsMenu(bottomScreen);
            C3D_FrameEnd(0);

        } else if (currentMode == MODE_NEW_PROJECT) {
            // === NEW PROJECT MODE ===
            touchPosition touch;
            hidTouchRead(&touch);

            if (kDown & KEY_TOUCH) {
                float pad = MENU_CONTENT_PADDING;
                float fieldX = pad;
                float fieldWidth = BOTTOM_SCREEN_WIDTH - pad * 2;
                float fieldHeight = 28;
                float halfFieldW = (fieldWidth - pad) / 2;

                // Compute Y positions to match renderNewProjectMenu layout
                float curY = 12;  // title Y
                curY += 24;       // title height + gap (approx th + 12)
                // Project Name label
                curY += 16;
                // Project Name field
                float nameFieldY = curY;
                curY += fieldHeight + 10;
                // Width/Height label
                curY += 16;
                // Width/Height fields
                float sizeFieldY = curY;
                float rightFieldX = fieldX + halfFieldW + pad;
                curY += fieldHeight + 16;

                // --- Touch: Project Name field ---
                if (touch.px >= fieldX && touch.px < fieldX + fieldWidth &&
                    touch.py >= nameFieldY && touch.py < nameFieldY + fieldHeight) {
                    char buf[PROJECT_NAME_MAX];
                    strncpy(buf, newProjectName, sizeof(buf));
                    buf[sizeof(buf) - 1] = '\0';
                    if (showKeyboard("Project name", buf, sizeof(buf))) {
                        strncpy(newProjectName, buf, sizeof(newProjectName));
                        newProjectName[sizeof(newProjectName) - 1] = '\0';
                    }
                }
                // --- Touch: Width field ---
                else if (touch.px >= fieldX && touch.px < fieldX + halfFieldW &&
                         touch.py >= sizeFieldY && touch.py < sizeFieldY + fieldHeight) {
                    int val = showNumericKeyboard("Width", newProjectWidth, 1, NEW_PROJECT_MAX_WIDTH);
                    if (val > 0) newProjectWidth = val;
                }
                // --- Touch: Height field ---
                else if (touch.px >= rightFieldX && touch.px < rightFieldX + halfFieldW &&
                         touch.py >= sizeFieldY && touch.py < sizeFieldY + fieldHeight) {
                    int val = showNumericKeyboard("Height", newProjectHeight, 1, NEW_PROJECT_MAX_HEIGHT);
                    if (val > 0) newProjectHeight = val;
                }
                // --- Touch: OK button (bottom bar right) ---
                else if (touch.px >= SAVE_EXIT_BTN_X && touch.px < SAVE_EXIT_BTN_X + SAVE_EXIT_BTN_WIDTH &&
                         touch.py >= SAVE_EXIT_BTN_Y && touch.py < SAVE_EXIT_BTN_Y + SAVE_EXIT_BTN_HEIGHT) {
                    if (newProjectName[0] == '\0') {
                        // Show error dialog for empty name
                        showDialog(topScreen, bottomScreen, "No Name", "Please enter a project name.");
                    } else {
                        // Check if project with this name already exists
                        char filePath[256];
                        snprintf(filePath, sizeof(filePath), "%s/%s.mgdw", SAVE_DIR, newProjectName);
                        if (fileExists(filePath)) {
                            // Show error dialog
                            showDialog(topScreen, bottomScreen, "Project Exists",
                                      "A project with this name\nalready exists.");
                        } else {
                            strncpy(currentProjectName, newProjectName, PROJECT_NAME_MAX);
                            currentProjectName[PROJECT_NAME_MAX - 1] = '\0';
                            projectHasName = true;
                            projectHasUnsavedChanges = false;  // Reset unsaved changes flag for new project
                            applyCanvasSize(newProjectWidth, newProjectHeight);
                            resetLayersForNewProject();
                            exitHistory();
                            initHistory();
                            canvasPanX = 0.0f;
                            canvasPanY = 0.0f;
                            canvasZoom = 1.0f;
                            currentMode = MODE_DRAW;
                            canvasNeedsUpdate = true;
                        }
                    }
                }
                // --- Touch: Back icon (bottom bar left) ---
                else if (touch.px >= MENU_BTN_X && touch.px < MENU_BTN_X + MENU_BTN_SIZE &&
                         touch.py >= MENU_BTN_Y && touch.py < MENU_BTN_Y + MENU_BTN_SIZE) {
                    currentMode = MODE_HOME;
                }
            }

            // B button goes back
            if (kDown & KEY_B) {
                currentMode = MODE_HOME;
            }

            // Render frame
            C3D_FrameBegin(C3D_FRAME_SYNCDRAW);
            renderHomeTop(topScreen);
            renderNewProjectMenu(bottomScreen);
            C3D_FrameEnd(0);

        } else if (currentMode == MODE_OPEN) {
            // === OPEN PROJECT MODE ===
            touchPosition touch;
            hidTouchRead(&touch);

            float pad = MENU_CONTENT_PADDING;
            float listX = pad;
            float listY = 8;
            float listWidth = BOTTOM_SCREEN_WIDTH - pad * 2;
            float listHeight = MENU_BOTTOM_BAR_Y - listY - 4;
            float itemHeight = OPEN_LIST_ITEM_HEIGHT;

            // Handle touch start (record position)
            if (kDown & KEY_TOUCH) {
                openListLastTouchX = touch.px;
                openListLastTouchY = touch.py;
                openListDragging = false;
            }

            // Handle touch drag (scrolling)
            if (kHeld & KEY_TOUCH) {
                if (touch.px >= listX && touch.px < listX + listWidth &&
                    touch.py >= listY && touch.py < listY + listHeight) {
                    float deltaY = openListLastTouchY - touch.py;
                    if (fabsf(deltaY) > 3) {
                        openListDragging = true;
                        openListScrollY += deltaY;
                        float totalContentHeight = openProjectCount * itemHeight;
                        float maxScroll = totalContentHeight - listHeight;
                        if (maxScroll < 0) maxScroll = 0;
                        if (openListScrollY < 0) openListScrollY = 0;
                        if (openListScrollY > maxScroll) openListScrollY = maxScroll;
                    }
                    openListLastTouchX = touch.px;
                    openListLastTouchY = touch.py;
                }
            }

            // Handle touch release (selection and buttons)
            if (kUp & KEY_TOUCH) {
                int releaseX = openListLastTouchX;
                int releaseY = (int)openListLastTouchY;

                // Check list item selection (only if not dragging)
                if (!openListDragging &&
                    releaseX >= listX && releaseX < listX + listWidth &&
                    releaseY >= listY && releaseY < listY + listHeight) {
                    int touchedIndex = (int)((releaseY - listY + openListScrollY) / itemHeight);
                    if (touchedIndex >= 0 && touchedIndex < openProjectCount) {
                        if (openSelectedIndex != touchedIndex) {
                            openSelectedIndex = touchedIndex;
                            openPreviewValid = false;  // プレビューを無効化

                            // "Loading..." 表示のために1フレームレンダリング
                            C3D_FrameBegin(C3D_FRAME_SYNCDRAW);
                            C2D_TargetClear(topScreen, C2D_Color32(0x2A, 0x2A, 0x2A, 0xFF));
                            C2D_SceneBegin(topScreen);
                            // "Loading..." テキストを中央に表示
                            C2D_TextBufClear(g_textBuf);
                            C2D_Text text;
                            C2D_TextParse(&text, g_textBuf, "Loading...");
                            C2D_TextOptimize(&text);
                            float tw, th;
                            C2D_TextGetDimensions(&text, 0.7f, 0.7f, &tw, &th);
                            C2D_DrawText(&text, C2D_WithColor, (TOP_SCREEN_WIDTH - tw) / 2, (TOP_SCREEN_HEIGHT - th) / 2, 0, 0.7f, 0.7f, UI_COLOR_WHITE);
                            renderOpenMenu(bottomScreen);
                            C3D_FrameEnd(0);

                            // プレビュー読み込み
                            loadProjectPreview(openProjectNames[openSelectedIndex]);
                        }
                    }
                }
                // Check Open button
                else if (releaseX >= SAVE_EXIT_BTN_X && releaseX < SAVE_EXIT_BTN_X + SAVE_EXIT_BTN_WIDTH &&
                         releaseY >= SAVE_EXIT_BTN_Y && releaseY < SAVE_EXIT_BTN_Y + SAVE_EXIT_BTN_HEIGHT) {
                    if (openSelectedIndex >= 0 && openSelectedIndex < openProjectCount) {
                        if (loadProject(openProjectNames[openSelectedIndex])) {
                            freeOpenPreview();
                            currentMode = MODE_DRAW;
                        }
                    }
                }
                // Check Back button
                else if (releaseX >= MENU_BTN_X && releaseX < MENU_BTN_X + MENU_BTN_SIZE &&
                         releaseY >= MENU_BTN_Y && releaseY < MENU_BTN_Y + MENU_BTN_SIZE) {
                    freeOpenPreview();
                    currentMode = MODE_HOME;
                }

                openListDragging = false;
            }

            // B button goes back
            if (kDown & KEY_B) {
                freeOpenPreview();
                currentMode = MODE_HOME;
            }

            // Render frame
            C3D_FrameBegin(C3D_FRAME_SYNCDRAW);
            // Top screen: show preview if available
            C2D_TargetClear(topScreen, C2D_Color32(0x2A, 0x2A, 0x2A, 0xFF));
            C2D_SceneBegin(topScreen);
            if (openPreviewValid) {
                float scaleX = (float)TOP_SCREEN_WIDTH / openPreviewWidth;
                float scaleY = (float)TOP_SCREEN_HEIGHT / openPreviewHeight;
                float scale = (scaleX < scaleY) ? scaleX : scaleY;
                float drawW = openPreviewWidth * scale;
                float drawH = openPreviewHeight * scale;
                float drawX = (TOP_SCREEN_WIDTH - drawW) / 2;
                float drawY = (TOP_SCREEN_HEIGHT - drawH) / 2;
                C2D_DrawImageAt(openPreviewImage, drawX, drawY, 0, NULL, scale, scale);
            }
            renderOpenMenu(bottomScreen);
            C3D_FrameEnd(0);

        } else if (currentMode == MODE_DRAW) {
            // === DRAW MODE ===

            // Toggle menu with D-Pad Up
            if (kDown & KEY_DUP) {
                currentMode = MODE_MENU;
                currentMenuTab = TAB_TOOL;
                isDrawing = false;
            }

            // Undo with D-Pad Left
            if (kDown & KEY_DLEFT) {
                undo();
            }

            // Redo with D-Pad Right
            if (kDown & KEY_DRIGHT) {
                redo();
            }

            // Toggle tool with A button (Brush <-> Eraser)
            if (kDown & KEY_A) {
                currentTool = (currentTool == TOOL_BRUSH) ? TOOL_ERASER : TOOL_BRUSH;
            }

            // Handle touch input
            touchPosition touch;
            hidTouchRead(&touch);

            // Check if L button is held (zoom/pan mode)
            bool lHeld = (kHeld & KEY_L) != 0;

            if (lHeld) {
                // L-button mode: zoom/pan controls
                float btnY = BOTTOM_SCREEN_HEIGHT - OVERLAY_BTN_SIZE - OVERLAY_MARGIN;
                float zoomOutX = OVERLAY_MARGIN;
                float zoomInX = OVERLAY_MARGIN + OVERLAY_BTN_SIZE + OVERLAY_MARGIN;
                float redoX = BOTTOM_SCREEN_WIDTH - OVERLAY_MARGIN - OVERLAY_BTN_SIZE;
                float undoX = redoX - OVERLAY_MARGIN - OVERLAY_BTN_SIZE;

                if (kDown & KEY_TOUCH) {
                    // Check zoom out button
                    if (touch.px >= zoomOutX && touch.px < zoomOutX + OVERLAY_BTN_SIZE &&
                        touch.py >= btnY && touch.py < btnY + OVERLAY_BTN_SIZE) {
                        canvasZoom -= ZOOM_STEP;
                        if (canvasZoom < ZOOM_MIN) canvasZoom = ZOOM_MIN;
                    }
                    // Check zoom in button
                    else if (touch.px >= zoomInX && touch.px < zoomInX + OVERLAY_BTN_SIZE &&
                             touch.py >= btnY && touch.py < btnY + OVERLAY_BTN_SIZE) {
                        canvasZoom += ZOOM_STEP;
                        if (canvasZoom > ZOOM_MAX) canvasZoom = ZOOM_MAX;
                    }
                    // Check undo button
                    else if (touch.px >= undoX && touch.px < undoX + OVERLAY_BTN_SIZE &&
                             touch.py >= btnY && touch.py < btnY + OVERLAY_BTN_SIZE) {
                        undo();
                    }
                    // Check redo button
                    else if (touch.px >= redoX && touch.px < redoX + OVERLAY_BTN_SIZE &&
                             touch.py >= btnY && touch.py < btnY + OVERLAY_BTN_SIZE) {
                        redo();
                    }
                    // Start panning (anywhere else)
                    else {
                        isPanning = true;
                        panStartTouch.px = touch.px;
                        panStartTouch.py = touch.py;
                    }
                }

                // Handle panning drag
                if ((kHeld & KEY_TOUCH) && isPanning) {
                    canvasPanX += (touch.px - panStartTouch.px);
                    canvasPanY += (touch.py - panStartTouch.py);
                    panStartTouch.px = touch.px;
                    panStartTouch.py = touch.py;
                }

                if (kUp & KEY_TOUCH) {
                    isPanning = false;
                }

                isDrawing = false;
            } else {
                // Normal drawing mode
                isPanning = false;

                if (kDown & KEY_TOUCH) {
                    // Check if touching menu button
                    if (showDrawMenuButton &&
                        touch.px >= DRAW_MENU_BTN_X && touch.px < DRAW_MENU_BTN_X + DRAW_MENU_BTN_SIZE &&
                        touch.py >= DRAW_MENU_BTN_Y && touch.py < DRAW_MENU_BTN_Y + DRAW_MENU_BTN_SIZE) {
                        currentMode = MODE_MENU;
                        currentMenuTab = TAB_TOOL;
                        isDrawing = false;
                    } else {
                        // Convert screen coordinates to canvas coordinates (accounting for zoom/pan)
                        float canvasX = (touch.px - canvasPanX - (BOTTOM_SCREEN_WIDTH - CANVAS_WIDTH * canvasZoom) / 2) / canvasZoom;
                        float canvasY = (touch.py - canvasPanY - (BOTTOM_SCREEN_HEIGHT - CANVAS_HEIGHT * canvasZoom) / 2) / canvasZoom;

                        int drawX = (int)canvasX;
                        int drawY = CANVAS_HEIGHT - 1 - (int)canvasY;

                        if (currentTool == TOOL_FILL) {
                            // Fill tool: flood fill on tap
                            pushHistory();

                            // Update composite buffer first to get current canvas state
                            forceUpdateCanvasTexture();

                            // Get fill color
                            u8 r = (currentColor >> 24) & 0xFF;
                            u8 g = (currentColor >> 16) & 0xFF;
                            u8 b = (currentColor >> 8) & 0xFF;
                            u32 fillColor = (r << 24) | (g << 16) | (b << 8) | brushAlpha;

                            floodFill(currentLayerIndex, drawX, drawY, fillColor, fillExpand, fillTolerance);
                            canvasNeedsUpdate = true;
                            isDrawing = false;  // No dragging for fill tool
                        } else {
                            // Brush/Eraser tool: start drawing
                            pushHistory();

                            // Initialize last position for smooth line drawing
                            lastCanvasX = canvasX;
                            lastCanvasY = canvasY;
                            isDrawing = true;

                            // Start new stroke (for G-Pen pressure)
                            startStroke(currentLayerIndex);

                            // Draw initial point
                            u32 drawColor;
                            if (currentTool == TOOL_ERASER) {
                                // Eraser: always transparent (white background added during compositing)
                                drawColor = 0x00000000;
                            } else {
                                // Apply brush alpha to color
                                u8 r = (currentColor >> 24) & 0xFF;
                                u8 g = (currentColor >> 16) & 0xFF;
                                u8 b = (currentColor >> 8) & 0xFF;
                                drawColor = (r << 24) | (g << 16) | (b << 8) | brushAlpha;
                            }
                            drawBrushToLayer(currentLayerIndex, drawX, drawY, getCurrentBrushSize(), drawColor);
                            canvasNeedsUpdate = true;  // Mark canvas as dirty
                            updateFrameCounter = 0;    // Reset update counter for new stroke
                        }
                    }
                }

                if (kHeld & KEY_TOUCH && currentMode == MODE_DRAW && isDrawing) {
                    // Convert screen coordinates to canvas coordinates
                    float canvasX = (touch.px - canvasPanX - (BOTTOM_SCREEN_WIDTH - CANVAS_WIDTH * canvasZoom) / 2) / canvasZoom;
                    float canvasY = (touch.py - canvasPanY - (BOTTOM_SCREEN_HEIGHT - CANVAS_HEIGHT * canvasZoom) / 2) / canvasZoom;

                    // Check if NOT touching menu button area
                    bool touchingMenuBtn = showDrawMenuButton &&
                                            (touch.px >= DRAW_MENU_BTN_X && touch.px < DRAW_MENU_BTN_X + DRAW_MENU_BTN_SIZE &&
                                             touch.py >= DRAW_MENU_BTN_Y && touch.py < DRAW_MENU_BTN_Y + DRAW_MENU_BTN_SIZE);

                    if (!touchingMenuBtn) {
                        // Flip Y coordinate for correct orientation
                        int drawX = (int)canvasX;
                        int drawY = CANVAS_HEIGHT - 1 - (int)canvasY;
                        int lastDrawX = (int)lastCanvasX;
                        int lastDrawY = CANVAS_HEIGHT - 1 - (int)lastCanvasY;

                        // Get the color to use based on current tool
                        u32 drawColor;
                        if (currentTool == TOOL_ERASER) {
                            // Eraser: always transparent (white background added during compositing)
                            drawColor = 0x00000000;
                        } else {
                            // Apply brush alpha to color
                            u8 r = (currentColor >> 24) & 0xFF;
                            u8 g = (currentColor >> 16) & 0xFF;
                            u8 b = (currentColor >> 8) & 0xFF;
                            drawColor = (r << 24) | (g << 16) | (b << 8) | brushAlpha;
                        }

                        // Draw line from last position to current position
                        drawLineToLayer(currentLayerIndex, lastDrawX, lastDrawY, drawX, drawY, getCurrentBrushSize(), drawColor);
                        lastCanvasX = canvasX;
                        lastCanvasY = canvasY;
                        canvasNeedsUpdate = true;  // Mark canvas as dirty
                    } else {
                        isDrawing = false;
                    }
                }

                // Reset drawing state when touch is released
                if (kUp & KEY_TOUCH) {
                    if (isDrawing) {
                        endStroke();  // End stroke (for G-Pen)
                    }
                    isDrawing = false;
                    // Force immediate update when stroke ends for final result
                    if (canvasNeedsUpdate) {
                        forceUpdateCanvasTexture();
                    }
                }
            }

            // Update texture with composited layers (throttled during drawing)
            if (isDrawing) {
                // During drawing: update every N frames for performance
                updateFrameCounter++;
                if (updateFrameCounter >= UPDATE_INTERVAL_DRAWING) {
                    updateCanvasTexture();
                    updateFrameCounter = 0;
                }
            } else {
                // Not drawing: update only when needed
                updateCanvasTexture();
            }

            // Render frame
            C3D_FrameBegin(C3D_FRAME_SYNCDRAW);
            renderUI(topScreen);
            renderCanvas(bottomScreen, lHeld);
            C3D_FrameEnd(0);

        } else if (currentMode == MODE_MENU) {
            // === MENU MODE ===

            touchPosition touch;
            hidTouchRead(&touch);

            if (kDown & KEY_TOUCH) {
                // Check close button
                if (touch.px >= MENU_BTN_X && touch.px < MENU_BTN_X + MENU_BTN_SIZE &&
                    touch.py >= MENU_BTN_Y && touch.py < MENU_BTN_Y + MENU_BTN_SIZE) {
                    currentMode = MODE_DRAW;
                }
                // Check Save/Exit button
                else if (touch.px >= SAVE_EXIT_BTN_X && touch.px < SAVE_EXIT_BTN_X + SAVE_EXIT_BTN_WIDTH &&
                         touch.py >= SAVE_EXIT_BTN_Y && touch.py < SAVE_EXIT_BTN_Y + SAVE_EXIT_BTN_HEIGHT) {
                    currentMode = MODE_SAVE_MENU;
                }
                // Check tab selection
                else if (touch.py < MENU_TAB_HEIGHT) {
                    int tabIndex = touch.px / MENU_TAB_WIDTH;
                    if (tabIndex >= 0 && tabIndex < 4) {
                        currentMenuTab = (MenuTab)tabIndex;
                    }
                }

                // Handle tool tab content touch - 4x2 grid
                if (currentMenuTab == TAB_TOOL) {
                    const int GRID_COLS = 4;
                    const int GRID_ROWS = 2;
                    const float GRID_GAP = 8;
                    float gridStartX = (BOTTOM_SCREEN_WIDTH - (BTN_SIZE_LARGE * GRID_COLS + GRID_GAP * (GRID_COLS - 1))) / 2;
                    float gridStartY = MENU_CONTENT_Y + 8;  // Match drawing code

                    for (int row = 0; row < GRID_ROWS; row++) {
                        for (int col = 0; col < GRID_COLS; col++) {
                            int idx = row * GRID_COLS + col;
                            if (idx >= 3) continue; // Skip skeletons

                            float btnX = gridStartX + col * (BTN_SIZE_LARGE + GRID_GAP);
                            float btnY = gridStartY + row * (BTN_SIZE_LARGE + GRID_GAP + 8);

                            ButtonConfig btn = { .x = btnX, .y = btnY, .size = BTN_SIZE_LARGE };
                            if (isButtonTouched(&btn, touch.px, touch.py)) {
                                currentTool = (ToolType)idx;
                            }
                        }
                    }
                }
            }

            // Handle brush size slider (continuous touch)
            if (kHeld & KEY_TOUCH) {
                if (currentMenuTab == TAB_BRUSH) {
                    if (currentTool == TOOL_FILL) {
                        // Fill tool: Tolerance + Expand sliders (must match rendering layout)
                        float settingsX = MENU_CONTENT_PADDING;
                        float settingsY = MENU_CONTENT_Y + MENU_CONTENT_PADDING;
                        float settingsWidth = BOTTOM_SCREEN_WIDTH - MENU_CONTENT_PADDING * 2;
                        float knobRadius = 10;
                        float sliderX = settingsX + knobRadius;
                        float sliderY = settingsY;
                        float sliderWidth = settingsWidth - knobRadius * 2;

                        // Tolerance slider track Y
                        float toleranceTrackY = sliderY + 14 + knobRadius;

                        // Expand slider track Y (shifted down to match rendering)
                        float expandSliderY = sliderY + 14 + knobRadius * 2 + 12;
                        float expandTrackY = expandSliderY + 14 + knobRadius;

                        // Reset state when touch released
                        if (!(kHeld & KEY_TOUCH)) {
                            fillToleranceSliderActive = false;
                            fillExpandSliderActive = false;
                        }

                        // Check if touching tolerance slider area
                        if (touch.px >= sliderX - 5 && touch.px <= sliderX + sliderWidth + 5 &&
                            touch.py >= toleranceTrackY - 15 && touch.py <= toleranceTrackY + 15) {
                            fillToleranceSliderActive = true;
                            float ratio = (float)(touch.px - sliderX) / sliderWidth;
                            if (ratio < 0) ratio = 0;
                            if (ratio > 1) ratio = 1;
                            int newTolerance = (int)(ratio * FILL_TOLERANCE_MAX + 0.5f);
                            if (newTolerance > FILL_TOLERANCE_MAX) newTolerance = FILL_TOLERANCE_MAX;
                            if (newTolerance < 0) newTolerance = 0;
                            fillTolerance = newTolerance;
                        }

                        // Check if touching expand slider area
                        if (touch.px >= sliderX - 5 && touch.px <= sliderX + sliderWidth + 5 &&
                            touch.py >= expandTrackY - 15 && touch.py <= expandTrackY + 15) {
                            fillExpandSliderActive = true;
                            float ratio = (float)(touch.px - sliderX) / sliderWidth;
                            if (ratio < 0) ratio = 0;
                            if (ratio > 1) ratio = 1;
                            int newExpand = (int)(ratio * FILL_EXPAND_MAX + 0.5f);
                            if (newExpand > FILL_EXPAND_MAX) newExpand = FILL_EXPAND_MAX;
                            if (newExpand < 0) newExpand = 0;
                            fillExpand = newExpand;
                        }
                    } else {
                        // Layout constants (must match rendering)
                        float listX = MENU_CONTENT_PADDING;
                        float listY = MENU_CONTENT_Y + MENU_CONTENT_PADDING;
                        float listWidth = 150;
                        float listHeight = MENU_BTN_Y - listY - MENU_CONTENT_PADDING;
                        float itemHeight = 40;

                        float settingsX = listX + listWidth + MENU_CONTENT_PADDING;
                        float settingsY = MENU_CONTENT_Y + MENU_CONTENT_PADDING;
                        float settingsWidth = BOTTOM_SCREEN_WIDTH - settingsX - MENU_CONTENT_PADDING;

                        float sliderX = settingsX;
                        float sliderY = settingsY + 25;  // Match drawing code
                        float sliderWidth = settingsWidth - 10;  // Match drawing code
                        float sliderTrackY = sliderY + 14 + 10 + 10;  // labelHeight(14) + offset(10) + knobRadius(10)
                        int minSize = 1;
                        int maxSize = 16;

                        // Handle brush list scrolling
                        if (touch.px >= listX && touch.px < listX + listWidth &&
                            touch.py >= listY && touch.py < listY + listHeight) {
                            if (kDown & KEY_TOUCH) {
                                // Start of touch - record position and check for selection
                                brushListLastTouchY = touch.py;
                                brushListDragging = false;
                            } else {
                                // Continued touch - check for scroll
                                float deltaY = brushListLastTouchY - touch.py;
                                if (fabsf(deltaY) > 3) {
                                    brushListDragging = true;
                                    brushListScrollY += deltaY;

                                    // Clamp scroll
                                    float totalContentHeight = NUM_BRUSHES * itemHeight;
                                    float maxScroll = totalContentHeight - listHeight;
                                    if (maxScroll < 0) maxScroll = 0;
                                    if (brushListScrollY < 0) brushListScrollY = 0;
                                    if (brushListScrollY > maxScroll) brushListScrollY = maxScroll;
                                }
                                brushListLastTouchY = touch.py;
                            }
                        }

                        // Check brush list touch for selection (on kDown, only if not dragging)
                        if ((kDown & KEY_TOUCH) && !brushListDragging) {
                            if (touch.px >= listX && touch.px < listX + listWidth &&
                                touch.py >= listY && touch.py < listY + listHeight) {
                                // Calculate which brush was touched
                                int touchedIndex = (int)((touch.py - listY + brushListScrollY) / itemHeight);
                                if (touchedIndex >= 0 && touchedIndex < (int)NUM_BRUSHES) {
                                    currentBrushType = touchedIndex;
                                }
                            }
                        }

                        // Reset dragging state when touch released
                        if (!(kHeld & KEY_TOUCH)) {
                            brushListDragging = false;
                            brushSizeSliderActive = false;
                        }

                        // Check if touching slider area (extended touch area)
                        if (touch.px >= sliderX - 5 && touch.px <= sliderX + sliderWidth + 5 &&
                            touch.py >= sliderTrackY - 15 && touch.py <= sliderTrackY + 15) {
                            brushSizeSliderActive = true;
                            float ratio = (float)(touch.px - sliderX) / sliderWidth;
                            if (ratio < 0) ratio = 0;
                            if (ratio > 1) ratio = 1;
                            int newSize = minSize + (int)(ratio * (maxSize - minSize) + 0.5f);
                            if (newSize > maxSize) newSize = maxSize;
                            if (newSize < minSize) newSize = minSize;
                            setCurrentBrushSize(newSize);
                        }
                    }  // end if (currentTool == TOOL_FILL) else
                }

                // Handle color tab continuous touch (for HSV picker and alpha slider)
                if (currentMenuTab == TAB_COLOR) {
                    // SV Square touch
                    if (touch.px >= MCP_SV_X && touch.px < MCP_SV_X + MCP_SV_SIZE &&
                        touch.py >= MCP_SV_Y && touch.py < MCP_SV_Y + MCP_SV_SIZE) {
                        currentSaturation = (float)(touch.px - MCP_SV_X) / (MCP_SV_SIZE - 1);
                        currentValue = 1.0f - (float)(touch.py - MCP_SV_Y) / (MCP_SV_SIZE - 1);
                        if (currentSaturation < 0) currentSaturation = 0;
                        if (currentSaturation > 1) currentSaturation = 1;
                        if (currentValue < 0) currentValue = 0;
                        if (currentValue > 1) currentValue = 1;
                        currentColor = hsvToRgb(currentHue, currentSaturation, currentValue);
                    }

                    // Hue bar touch
                    if (touch.px >= MCP_HUE_X && touch.px < MCP_HUE_X + MCP_HUE_WIDTH &&
                        touch.py >= MCP_HUE_Y && touch.py < MCP_HUE_Y + MCP_HUE_HEIGHT) {
                        currentHue = (float)(touch.py - MCP_HUE_Y) / (MCP_HUE_HEIGHT - 1) * 360.0f;
                        if (currentHue < 0) currentHue = 0;
                        if (currentHue >= 360) currentHue = 359.9f;
                        currentColor = hsvToRgb(currentHue, currentSaturation, currentValue);
                    }

                    // Alpha slider touch (track is offset by label + knob radius)
                    float alphaTrackY = MCP_ALPHA_SLIDER_Y + 14 + 10;  // labelHeight(14) + knobRadius(10)
                    if (touch.py >= alphaTrackY - 15 && touch.py <= alphaTrackY + 15 &&
                        touch.px >= MCP_ALPHA_SLIDER_X - 5 && touch.px <= MCP_ALPHA_SLIDER_X + MCP_ALPHA_SLIDER_WIDTH + 5) {
                        float ratio = (float)(touch.px - MCP_ALPHA_SLIDER_X) / MCP_ALPHA_SLIDER_WIDTH;
                        if (ratio < 0) ratio = 0;
                        if (ratio > 1) ratio = 1;
                        brushAlpha = (u8)(ratio * 255 + 0.5f);
                    }
                }

                // Handle layer tab opacity slider (continuous touch)
                if (currentMenuTab == TAB_LAYER) {
                    float listStartY = MENU_CONTENT_Y + MENU_CONTENT_PADDING;
                    float listX = MENU_CONTENT_PADDING;
                    float listItemWidth = 150;
                    float opX = listX + listItemWidth + MENU_CONTENT_PADDING;
                    float opBtnSize = 32;
                    float opBtnSpacing = 4;
                    float rightMargin = MENU_CONTENT_PADDING + 10;  // Match drawing code
                    float row2Y = listStartY + opBtnSize + opBtnSpacing;
                    float sliderY = row2Y + opBtnSize + 10;
                    float sliderTrackY = sliderY + 14 + 10;  // labelHeight(14) + knobRadius(10)
                    float sliderX = opX;
                    float sliderWidth = BOTTOM_SCREEN_WIDTH - opX - rightMargin;

                    // Check if touching opacity slider area
                    if (touch.py >= sliderTrackY - 15 && touch.py <= sliderTrackY + 15 &&
                        touch.px >= sliderX - 5 && touch.px <= sliderX + sliderWidth + 5) {
                        float ratio = (float)(touch.px - sliderX) / sliderWidth;
                        if (ratio < 0) ratio = 0;
                        if (ratio > 1) ratio = 1;
                        u8 newOpacity = (u8)(ratio * 255 + 0.5f);
                        if (newOpacity != layers[currentLayerIndex].opacity) {
                            layers[currentLayerIndex].opacity = newOpacity;
                            canvasNeedsUpdate = true;  // Opacity changed
                        }
                    }
                }
            }

            // Reset brush size preview when touch is released
            if (kUp & KEY_TOUCH) {
                brushSizeSliderActive = false;
            }

            // Handle color tab kDown touch (for palette and buttons)
            if (kDown & KEY_TOUCH) {
                if (currentMenuTab == TAB_COLOR) {
                    float paletteStartX = 148;  // Match drawing code
                    float paletteStartY = MENU_CONTENT_Y + MENU_CONTENT_PADDING;

                    // Check palette cells
                    for (int row = 0; row < PALETTE_ROWS; row++) {
                        for (int col = 0; col < PALETTE_COLS; col++) {
                            int idx = row * PALETTE_COLS + col;
                            float cellX = paletteStartX + col * (PALETTE_CELL_SIZE + PALETTE_SPACING);
                            float cellY = paletteStartY + row * (PALETTE_CELL_SIZE + PALETTE_SPACING);

                            if (touch.px >= cellX && touch.px < cellX + PALETTE_CELL_SIZE &&
                                touch.py >= cellY && touch.py < cellY + PALETTE_CELL_SIZE) {

                                if (paletteDeleteMode) {
                                    // Delete mode: remove color from palette
                                    if (paletteUsed[idx]) {
                                        paletteUsed[idx] = false;
                                        paletteColors[idx] = 0x00000000;
                                    }
                                } else {
                                    // Add/Select mode
                                    if (paletteUsed[idx]) {
                                        // Select this color
                                        currentColor = paletteColors[idx];
                                        rgbToHsv(currentColor, &currentHue, &currentSaturation, &currentValue);
                                    } else {
                                        // Add current color to this slot
                                        paletteColors[idx] = currentColor;
                                        paletteUsed[idx] = true;
                                    }
                                }
                            }
                        }
                    }

                    // Check Add/Delete mode buttons
                    float btnY = paletteStartY + PALETTE_ROWS * (PALETTE_CELL_SIZE + PALETTE_SPACING) + 2;  // Match drawing code
                    float btnWidth = 24;
                    float btnHeight = 24;
                    float btnSpacing = PALETTE_SPACING;
                    float paletteWidth = PALETTE_COLS * PALETTE_CELL_SIZE + (PALETTE_COLS - 1) * PALETTE_SPACING;
                    float paletteRightX = paletteStartX + paletteWidth;
                    float delBtnX = paletteRightX - btnWidth;
                    float addBtnX = delBtnX - btnWidth - btnSpacing;

                    // Add button
                    if (touch.px >= addBtnX && touch.px < addBtnX + btnWidth &&
                        touch.py >= btnY && touch.py < btnY + btnHeight) {
                        paletteDeleteMode = false;
                    }

                    // Delete button
                    if (touch.px >= delBtnX && touch.px < delBtnX + btnWidth &&
                        touch.py >= btnY && touch.py < btnY + btnHeight) {
                        paletteDeleteMode = true;
                    }
                }

                // Handle layer tab touch (layer selection, visibility toggle, and operations)
                if (currentMenuTab == TAB_LAYER) {
                    float listStartY = MENU_CONTENT_Y + MENU_CONTENT_PADDING;
                    float listItemSpacing = 2;
                    float listBottomY = MENU_BTN_Y - MENU_CONTENT_PADDING;
                    float listHeight = listBottomY - listStartY;
                    float listItemHeight = (listHeight - listItemSpacing * (MAX_LAYERS - 1)) / MAX_LAYERS;
                    if (listItemHeight < 1) listItemHeight = 1;
                    float listX = MENU_CONTENT_PADDING;
                    float listItemWidth = 150;
                    float eyeBtnSize = 28;

                    // Layer list touch
                    for (int i = MAX_LAYERS - 1; i >= 0; i--) {
                        int displayIdx = MAX_LAYERS - 1 - i;
                        float itemY = listStartY + displayIdx * (listItemHeight + listItemSpacing);
                        float clipIndent = layers[i].clipping ? 6.0f : 0.0f;
                        float itemX = listX + clipIndent;
                        float itemWidth = listItemWidth - clipIndent;
                        float eyeBtnX = itemX + itemWidth - eyeBtnSize - 4;

                        if (itemY + listItemHeight > listBottomY) continue;

                        // Check if touching eye icon area
                        if (touch.px >= eyeBtnX && touch.px < eyeBtnX + eyeBtnSize &&
                            touch.py >= itemY && touch.py < itemY + listItemHeight) {
                            layers[i].visible = !layers[i].visible;
                            projectHasUnsavedChanges = true;  // Mark as changed
                            canvasNeedsUpdate = true;  // Visibility change affects display
                        }
                        // Check if touching layer item (but not eye icon)
                        else if (touch.px >= itemX && touch.px < eyeBtnX &&
                            touch.py >= itemY && touch.py < itemY + listItemHeight) {
                            currentLayerIndex = i;
                        }
                    }

                    // Layer operation buttons touch (4 columns x 2 rows)
                    float opX = listX + listItemWidth + MENU_CONTENT_PADDING;
                    float opY = listStartY;
                    float opBtnSize = 32;
                    float opBtnSpacing = 4;
                    float col1X = opX;
                    float col2X = col1X + opBtnSize + opBtnSpacing;
                    float col3X = col2X + opBtnSize + opBtnSpacing;
                    float col4X = col3X + opBtnSize + opBtnSpacing;
                    float row2Y = opY + opBtnSize + opBtnSpacing;

                    // Row 1: Up arrow - swap with layer above
                    if (touch.px >= col1X && touch.px < col1X + opBtnSize &&
                        touch.py >= opY && touch.py < opY + opBtnSize) {
                        if (currentLayerIndex < MAX_LAYERS - 1) {
                            pushHistory();
                            int src = currentLayerIndex;
                            int dst = currentLayerIndex + 1;
                            // Swap all layer properties
                            Layer tempLayer = layers[src];
                            layers[src] = layers[dst];
                            layers[dst] = tempLayer;
                            // Move selection up
                            currentLayerIndex++;
                            canvasNeedsUpdate = true;
                        }
                    }

                    // Row 1: Down arrow - swap with layer below
                    if (touch.px >= col2X && touch.px < col2X + opBtnSize &&
                        touch.py >= opY && touch.py < opY + opBtnSize) {
                        if (currentLayerIndex > 0) {
                            pushHistory();
                            int src = currentLayerIndex;
                            int dst = currentLayerIndex - 1;
                            // Swap all layer properties
                            Layer tempLayer = layers[src];
                            layers[src] = layers[dst];
                            layers[dst] = tempLayer;
                            // Move selection down
                            currentLayerIndex--;
                            if (currentLayerIndex == 0 && layers[currentLayerIndex].clipping) {
                                layers[currentLayerIndex].clipping = false;
                            }
                            canvasNeedsUpdate = true;
                        }
                    }

                    // Row 1: Merge down button
                    if (touch.px >= col3X && touch.px < col3X + opBtnSize &&
                        touch.py >= opY && touch.py < opY + opBtnSize) {
                        if (currentLayerIndex > 0) {
                            pushHistory();
                            // Merge current layer onto layer below
                            int srcIdx = currentLayerIndex;
                            int dstIdx = currentLayerIndex - 1;
                            for (int y = 0; y < TEX_HEIGHT; y++) {
                                for (int x = 0; x < TEX_WIDTH; x++) {
                                    int idx = y * TEX_WIDTH + x;
                                    u32 srcColor = layers[srcIdx].buffer[idx];
                                    u32 dstColor = layers[dstIdx].buffer[idx];
                                    layers[dstIdx].buffer[idx] = blendPixel(dstColor, srcColor, BLEND_NORMAL, 255);
                                }
                            }
                            // Clear source layer
                            u32 clearColor = 0x00000000;
                            for (int y = 0; y < TEX_HEIGHT; y++) {
                                for (int x = 0; x < TEX_WIDTH; x++) {
                                    layers[srcIdx].buffer[y * TEX_WIDTH + x] = clearColor;
                                }
                            }
                            // Select destination layer
                            currentLayerIndex = dstIdx;
                            canvasNeedsUpdate = true;
                        }
                    }

                    // Row 1: Clear button
                    if (touch.px >= col4X && touch.px < col4X + opBtnSize &&
                        touch.py >= opY && touch.py < opY + opBtnSize) {
                        pushHistory();
                        clearLayer(currentLayerIndex, 0x00000000);
                        canvasNeedsUpdate = true;
                    }

                    // Row 2: Alpha Lock toggle
                    if (touch.px >= col1X && touch.px < col1X + opBtnSize &&
                        touch.py >= row2Y && touch.py < row2Y + opBtnSize) {
                        pushHistory();
                        projectHasUnsavedChanges = true;  // Mark as changed
                        layers[currentLayerIndex].alphaLock = !layers[currentLayerIndex].alphaLock;
                    }

                    // Row 2: Clipping toggle
                    if (touch.px >= col2X && touch.px < col2X + opBtnSize &&
                        touch.py >= row2Y && touch.py < row2Y + opBtnSize) {
                        if (currentLayerIndex > 0) {
                            pushHistory();
                            projectHasUnsavedChanges = true;  // Mark as changed
                            layers[currentLayerIndex].clipping = !layers[currentLayerIndex].clipping;
                            canvasNeedsUpdate = true;
                        }
                    }

                    // Row 2: Rename button
                    if (touch.px >= col3X && touch.px < col3X + opBtnSize &&
                        touch.py >= row2Y && touch.py < row2Y + opBtnSize) {
                        char nameBuf[32];
                        if (layers[currentLayerIndex].name[0] != '\0') {
                            strncpy(nameBuf, layers[currentLayerIndex].name, sizeof(nameBuf));
                            nameBuf[sizeof(nameBuf) - 1] = '\0';
                        } else {
                            snprintf(nameBuf, sizeof(nameBuf), "Layer %d", currentLayerIndex + 1);
                        }
                        if (showKeyboard("Layer name", nameBuf, sizeof(nameBuf))) {
                            strncpy(layers[currentLayerIndex].name, nameBuf, sizeof(layers[currentLayerIndex].name));
                            layers[currentLayerIndex].name[sizeof(layers[currentLayerIndex].name) - 1] = '\0';
                        }
                    }

                    // Blend mode button touch
                    float sliderX = opX;
                    float sliderY = row2Y + opBtnSize + 15;
                    float sliderHeight = 8;
                    float blendBtnY = sliderY + sliderHeight + 28;
                    float blendBtnWidth = BOTTOM_SCREEN_WIDTH - MENU_CONTENT_PADDING - sliderX;
                    float blendBtnHeight = 24;

                    if (touch.px >= sliderX && touch.px < sliderX + blendBtnWidth &&
                        touch.py >= blendBtnY && touch.py < blendBtnY + blendBtnHeight) {
                        pushHistory();
                        projectHasUnsavedChanges = true;  // Mark as changed
                        // Cycle blend mode: Normal -> Add -> Multiply -> Normal
                        layers[currentLayerIndex].blendMode = (layers[currentLayerIndex].blendMode + 1) % 3;
                        canvasNeedsUpdate = true;  // Blend mode changed
                    }
                }
            }

            // B button closes menu
            if (kDown & KEY_B) {
                currentMode = MODE_DRAW;
            }

            // Update texture with composited layers (for real-time preview)
            updateCanvasTexture();

            // Render frame
            C3D_FrameBegin(C3D_FRAME_SYNCDRAW);
            renderUI(topScreen);
            renderMenu(bottomScreen);
            C3D_FrameEnd(0);

        } else if (currentMode == MODE_SAVE_MENU) {
            // === SAVE MENU MODE ===

            touchPosition touch;
            hidTouchRead(&touch);

            if (kDown & KEY_TOUCH) {
                // Check back button (left side of bottom bar)
                if (touch.px >= MENU_BTN_X && touch.px < MENU_BTN_X + MENU_BTN_SIZE &&
                    touch.py >= MENU_BOTTOM_BAR_Y && touch.py < MENU_BOTTOM_BAR_Y + MENU_BOTTOM_BAR_HEIGHT) {
                    currentMode = MODE_MENU;
                }

                // Check Go to Home button (right side of bottom bar)
                if (touch.px >= MENU_BTN_SIZE && touch.px < BOTTOM_SCREEN_WIDTH &&
                    touch.py >= MENU_BOTTOM_BAR_Y && touch.py < MENU_BOTTOM_BAR_Y + MENU_BOTTOM_BAR_HEIGHT) {
                    // Go to Home with unsaved changes check
                    if (projectHasUnsavedChanges) {
                        bool goHome = showConfirmDialog(g_topScreen, g_bottomScreen,
                                                        "Unsaved Changes",
                                                        "Are you sure you want to\ngo to Home without saving?");
                        if (goHome) {
                            currentMode = MODE_HOME;
                            projectHasUnsavedChanges = false;
                        }
                    } else {
                        currentMode = MODE_HOME;
                    }
                }

                // Calculate save menu item positions
                float totalWidth = BTN_SIZE_LARGE * 3 + SAVE_MENU_ITEM_SPACING * 2;
                float startX = (BOTTOM_SCREEN_WIDTH - totalWidth) / 2;
                float itemY = SAVE_MENU_Y;

                // Check Save button (overwrite using existing project name)
                float item1X = startX;
                if (touch.px >= item1X && touch.px < item1X + BTN_SIZE_LARGE &&
                    touch.py >= itemY && touch.py < itemY + BTN_SIZE_LARGE) {
                    if (projectHasName && currentProjectName[0] != '\0') {
                        quickSaveProject();
                        currentMode = MODE_MENU;
                    } else {
                        showDialog(g_topScreen, g_bottomScreen,
                                   "No Project Name",
                                   "Create a project from Home\nso a name is set.");
                    }
                }

                // Check Save As button (Always ask for new name)
                float item2X = startX + BTN_SIZE_LARGE + SAVE_MENU_ITEM_SPACING;
                if (touch.px >= item2X && touch.px < item2X + BTN_SIZE_LARGE &&
                    touch.py >= itemY && touch.py < itemY + BTN_SIZE_LARGE) {
                    char nameBuf[PROJECT_NAME_MAX] = "";
                    if (showKeyboard("Enter project name", nameBuf, sizeof(nameBuf))) {
                        // Check if name is empty
                        if (nameBuf[0] == '\0') {
                            showDialog(g_topScreen, g_bottomScreen, "No Name", "Please enter a project name.");
                        } else {
                            // Check if project with this name already exists
                            char filePath[256];
                            snprintf(filePath, sizeof(filePath), "%s/%s.mgdw", SAVE_DIR, nameBuf);
                            if (fileExists(filePath)) {
                                showDialog(g_topScreen, g_bottomScreen, "Project Exists",
                                          "A project with this name\nalready exists.");
                            } else {
                                saveProject(nameBuf);
                                currentMode = MODE_MENU;
                            }
                        }
                    }
                }

                // Check Export button
                float item3X = startX + (BTN_SIZE_LARGE + SAVE_MENU_ITEM_SPACING) * 2;
                if (touch.px >= item3X && touch.px < item3X + BTN_SIZE_LARGE &&
                    touch.py >= itemY && touch.py < itemY + BTN_SIZE_LARGE) {
                    char exportPath[256];
                    if (exportCanvasPNG(exportPath, sizeof(exportPath))) {
                        const char* filename = exportPath;
                        const char* slash = strrchr(exportPath, '/');
                        if (slash && *(slash + 1) != '\0') {
                            filename = slash + 1;
                        }
                        char msg[300];
                        snprintf(msg, sizeof(msg), "Exported to\n%s", filename);
                        showDialog(g_topScreen, g_bottomScreen, "Export Complete", msg);
                    } else {
                        showDialog(g_topScreen, g_bottomScreen, "Export Failed", "Failed to export PNG.");
                    }
                }
            }

            // B button goes back to menu
            if (kDown & KEY_B) {
                currentMode = MODE_MENU;
            }

            // Update texture with composited layers (for preview)
            updateCanvasTexture();

            // Render frame
            C3D_FrameBegin(C3D_FRAME_SYNCDRAW);
            renderUI(topScreen);
            renderSaveMenu(bottomScreen);
            C3D_FrameEnd(0);
        }
    }

    // Cleanup
    C2D_TextBufDelete(g_textBuf);
    exitIcons();
    exitHistory();
    exitLayers();
    C2D_Fini();
    C3D_Fini();
    romfsExit();
    gfxExit();

    return 0;
}
