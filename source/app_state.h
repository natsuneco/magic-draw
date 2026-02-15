#pragma once

#include <citro2d.h>
#include <3ds.h>
#include <stdbool.h>
#include <stddef.h>

/**
 * @file app_state.h
 * @brief Shared application state, constants, and core types.
 */

// Screen dimensions
#define TOP_SCREEN_WIDTH  400
#define TOP_SCREEN_HEIGHT 240
#define BOTTOM_SCREEN_WIDTH  320
#define BOTTOM_SCREEN_HEIGHT 240

// Max canvas dimension (GPU texture limit on 3DS)
#define MAX_CANVAS_DIM 1024

// Layer settings
#define MAX_LAYERS 4

// Canvas settings (using bottom screen size for now)
extern int canvasWidth;
extern int canvasHeight;
#define CANVAS_WIDTH  canvasWidth
#define CANVAS_HEIGHT canvasHeight

// Texture size (power-of-2, computed from canvas size)
extern int texWidth;
extern int texHeight;
#define TEX_WIDTH  texWidth
#define TEX_HEIGHT texHeight

// Blend modes
typedef enum {
    BLEND_NORMAL,
    BLEND_ADD,
    BLEND_MULTIPLY
} BlendMode;

// Layer structure - designed to be extensible for tiled canvas
typedef struct {
    u32* buffer;          /**< Pixel buffer (RGBA8 format). */
    bool visible;         /**< Layer visibility. */
    u8 opacity;           /**< Layer opacity (0-255). */
    BlendMode blendMode;  /**< Blend mode. */
    bool alphaLock;       /**< Alpha lock (opacity lock). */
    bool clipping;        /**< Clipping to layer below. */
    char name[32];        /**< Layer name. */
} Layer;

// Brush types
typedef enum {
    BRUSH_ANTIALIAS,
    BRUSH_GPEN,
    BRUSH_PIXEL,
    BRUSH_AIRBRUSH,
    BRUSH_TYPE_COUNT
} BrushType;

// Brush definition structure
typedef struct {
    const char* name;
    BrushType type;
} BrushDef;

// Available brushes
extern const BrushDef brushDefs[];
extern const size_t brushDefsCount;
#define NUM_BRUSHES (brushDefsCount)

// Color picker settings
#define CP_SV_X 20
#define CP_SV_Y 20
#define CP_SV_WIDTH 180
#define CP_SV_HEIGHT 180
#define CP_HUE_X 220
#define CP_HUE_Y 20
#define CP_HUE_WIDTH 30
#define CP_HUE_HEIGHT 180
#define CP_PREVIEW_X 270
#define CP_PREVIEW_Y 20
#define CP_PREVIEW_SIZE 40

// Application modes
typedef enum {
    MODE_HOME,
    MODE_NEW_PROJECT,
    MODE_OPEN,
    MODE_DRAW,
    MODE_COLOR_PICKER,
    MODE_MENU,
    MODE_SAVE_MENU
} AppMode;

// Tool types
typedef enum {
    TOOL_BRUSH,
    TOOL_ERASER,
    TOOL_FILL
} ToolType;

// Menu tabs
typedef enum {
    TAB_TOOL,
    TAB_BRUSH,
    TAB_COLOR,
    TAB_LAYER
} MenuTab;

extern AppMode currentMode;
extern ToolType currentTool;
extern MenuTab currentMenuTab;

// HSV color state (for color picker)
extern float currentHue;        /**< 0-360. */
extern float currentSaturation; /**< 0-1. */
extern float currentValue;      /**< 0-1. */

// Layer system
extern Layer layers[MAX_LAYERS];
extern int currentLayerIndex;
extern int numLayers;

// Composite buffer (result of merging all layers)
extern u32* compositeBuffer;
extern C3D_Tex canvasTex;
extern Tex3DS_SubTexture canvasSubTex;
extern C2D_Image canvasImage;

// Current drawing state
extern u32 currentColor;  /**< RGBA format: 0xRRGGBBAA. */
extern int brushSizesByType[BRUSH_TYPE_COUNT];
extern u8 brushAlpha;  /**< Brush opacity (0-255). */
extern bool isDrawing;
extern float lastCanvasX;
extern float lastCanvasY;
extern bool brushSizeSliderActive;

// Brush selection state
extern int currentBrushType;
extern float brushListScrollY;
extern float brushListLastTouchY;
extern bool brushListDragging;

// Fill tool settings
extern int fillExpand;  /**< Fill expansion in pixels (0-10). */
extern bool fillExpandSliderActive;
#define FILL_EXPAND_MAX 10

// Icon sprites
extern C2D_SpriteSheet iconSpriteSheet;
extern C2D_SpriteSheet bannerSpriteSheet;
extern C2D_SpriteSheet menuButtonBgSpriteSheet;
extern C2D_Sprite brushIconSprite;
extern C2D_Sprite eraserIconSprite;
extern C2D_Sprite bucketIconSprite;
extern C2D_Sprite wrenchIconSprite;
extern C2D_Sprite closeIconSprite;
extern C2D_Sprite plusIconSprite;
extern C2D_Sprite minusIconSprite;
extern C2D_Sprite eyeIconSprite;
extern C2D_Sprite clearIconSprite;
extern C2D_Sprite upArrowIconSprite;
extern C2D_Sprite downArrowIconSprite;
extern C2D_Sprite mergeIconSprite;
extern C2D_Sprite zoomInIconSprite;
extern C2D_Sprite zoomOutIconSprite;
extern C2D_Sprite undoIconSprite;
extern C2D_Sprite redoIconSprite;
extern C2D_Sprite saveIconSprite;
extern C2D_Sprite saveAsIconSprite;
extern C2D_Sprite exportIconSprite;
extern C2D_Sprite palettePlusIconSprite;
extern C2D_Sprite paletteMinusIconSprite;
extern C2D_Sprite clippingIconSprite;
extern C2D_Sprite alphaLockIconSprite;
extern C2D_Sprite pencilIconSprite;
extern C2D_Sprite crossArrowIconSprite;
extern C2D_Sprite checkIconSprite;
extern C2D_Sprite folderIconSprite;
extern C2D_Sprite settingsIconSprite;
extern C2D_Sprite newFileIconSprite;
extern C2D_Sprite backArrowIconSprite;
extern C2D_Sprite bannerSprite;
extern C2D_Sprite menuButtonBgSprite;

// Screen targets (for dialog rendering)
extern C3D_RenderTarget* g_topScreen;
extern C3D_RenderTarget* g_bottomScreen;

// FPS counter
extern u64 lastFrameTime;
extern float currentFPS;

// Canvas update optimization
extern bool canvasNeedsUpdate;
extern int updateFrameCounter;
#define UPDATE_INTERVAL_DRAWING 3

// Zoom and pan state
extern float canvasZoom;
extern float canvasPanX;
extern float canvasPanY;
extern bool isPanning;
extern touchPosition panStartTouch;

// Zoom limits
#define ZOOM_MIN 0.5f
#define ZOOM_MAX 4.0f
#define ZOOM_STEP 0.5f

// L-button overlay button positions
#define OVERLAY_BTN_SIZE 40
#define OVERLAY_MARGIN 8

// Draw mode menu button position (with margin)
#define DRAW_MENU_BTN_X 4
#define DRAW_MENU_BTN_Y (BOTTOM_SCREEN_HEIGHT - 25 - 4)
#define DRAW_MENU_BTN_SIZE 25

// Text rendering
extern C2D_TextBuf g_textBuf;

// Menu bottom bar
#define MENU_BOTTOM_BAR_HEIGHT 25
#define MENU_BOTTOM_BAR_Y (BOTTOM_SCREEN_HEIGHT - MENU_BOTTOM_BAR_HEIGHT)

// Close button position (left side of bottom bar)
#define MENU_BTN_X 0
#define MENU_BTN_Y MENU_BOTTOM_BAR_Y
#define MENU_BTN_SIZE MENU_BOTTOM_BAR_HEIGHT

// Save/Exit button position (rest of bottom bar)
#define SAVE_EXIT_BTN_X MENU_BTN_SIZE
#define SAVE_EXIT_BTN_Y MENU_BOTTOM_BAR_Y
#define SAVE_EXIT_BTN_WIDTH (BOTTOM_SCREEN_WIDTH - MENU_BTN_SIZE)
#define SAVE_EXIT_BTN_HEIGHT MENU_BOTTOM_BAR_HEIGHT

// Save menu item layout
#define SAVE_MENU_ITEM_SIZE 64
#define SAVE_MENU_ITEM_SPACING 20
#define SAVE_MENU_Y (BOTTOM_SCREEN_HEIGHT / 2 - SAVE_MENU_ITEM_SIZE / 2)

// Save/Project settings
#define SAVE_DIR "sdmc:/3ds/magicdraw"
#define PROJECT_NAME_MAX 32
#define PROJECT_FILE_MAGIC 0x4D474457  /**< "MGDW" */
#define PROJECT_FILE_VERSION 2

// Current project state
extern char currentProjectName[PROJECT_NAME_MAX];
extern bool projectHasName;
extern bool projectHasUnsavedChanges;

// Open project browser state
#define OPEN_MAX_PROJECTS 64
#define OPEN_LIST_ITEM_HEIGHT 36
extern char openProjectNames[OPEN_MAX_PROJECTS][PROJECT_NAME_MAX];
extern int openProjectCount;
extern int openSelectedIndex;
extern float openListScrollY;
extern int openListLastTouchX;
extern float openListLastTouchY;
extern bool openListDragging;

// Preview texture for open browser
extern C3D_Tex openPreviewTex;
extern Tex3DS_SubTexture openPreviewSubTex;
extern C2D_Image openPreviewImage;
extern bool openPreviewValid;
extern int openPreviewWidth;
extern int openPreviewHeight;

// New project creation state
#define NEW_PROJECT_MAX_WIDTH  MAX_CANVAS_DIM
#define NEW_PROJECT_MAX_HEIGHT MAX_CANVAS_DIM
extern char newProjectName[PROJECT_NAME_MAX];
extern int newProjectWidth;
extern int newProjectHeight;

// Menu layout
#define MENU_TAB_HEIGHT 40
#define MENU_TAB_WIDTH 80
#define MENU_CONTENT_Y 42
#define MENU_CONTENT_PADDING 8

// Color palette settings
#define PALETTE_MAX_COLORS 24
#define PALETTE_COLS 6
#define PALETTE_ROWS 4
#define PALETTE_CELL_SIZE 24
#define PALETTE_SPACING 4

extern u32 paletteColors[PALETTE_MAX_COLORS];
extern bool paletteUsed[PALETTE_MAX_COLORS];
extern bool paletteDeleteMode;

// Mini color picker layout (for color tab)
#define MCP_SV_X MENU_CONTENT_PADDING
#define MCP_SV_Y (MENU_CONTENT_Y + MENU_CONTENT_PADDING)
#define MCP_SV_SIZE 100
#define MCP_HUE_X (MCP_SV_X + MCP_SV_SIZE + 5)
#define MCP_HUE_Y (MENU_CONTENT_Y + MENU_CONTENT_PADDING)
#define MCP_HUE_WIDTH 20
#define MCP_HUE_HEIGHT 100
#define MCP_ALPHA_SLIDER_X MCP_SV_X
#define MCP_ALPHA_SLIDER_Y (MCP_SV_Y + MCP_SV_SIZE + 8)
#define MCP_ALPHA_SLIDER_WIDTH (MCP_HUE_X + MCP_HUE_WIDTH - MCP_SV_X)

/** @brief Get the current brush size for the active brush type. */
int getCurrentBrushSize(void);

/** @brief Set the current brush size for the active brush type. */
void setCurrentBrushSize(int size);

/** @brief Initialize the default palette colors. */
void initPalette(void);
