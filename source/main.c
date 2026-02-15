// Magic Draw - 3DS Drawing Application

#include <citro2d.h>
#include <3ds.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <sys/stat.h>
#include <dirent.h>
#include <ctype.h>

#include "ui_theme.h"
#include "ui_components.h"

// Screen dimensions
#define TOP_SCREEN_WIDTH  400
#define TOP_SCREEN_HEIGHT 240
#define BOTTOM_SCREEN_WIDTH  320
#define BOTTOM_SCREEN_HEIGHT 240

// Canvas settings (using bottom screen size for now)
static int canvasWidth = BOTTOM_SCREEN_WIDTH;
static int canvasHeight = BOTTOM_SCREEN_HEIGHT;
#define CANVAS_WIDTH  canvasWidth
#define CANVAS_HEIGHT canvasHeight

// Texture size (power-of-2, computed from canvas size)
static int texWidth = 512;
static int texHeight = 256;
#define TEX_WIDTH  texWidth
#define TEX_HEIGHT texHeight

// Max canvas dimension (GPU texture limit on 3DS)
#define MAX_CANVAS_DIM 1024

// Layer settings
#define MAX_LAYERS 4

// Blend modes
typedef enum {
    BLEND_NORMAL,
    BLEND_ADD,
    BLEND_MULTIPLY
} BlendMode;

// Layer structure - designed to be extensible for tiled canvas
typedef struct {
    u32* buffer;          // Pixel buffer (RGBA8 format)
    bool visible;         // Layer visibility
    u8 opacity;           // Layer opacity (0-255)
    BlendMode blendMode;  // Blend mode
    bool alphaLock;       // Alpha lock (opacity lock)
    bool clipping;        // Clipping to layer below
    char name[32];        // Layer name
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
static const BrushDef brushDefs[] = {
    {"Antialias Pen", BRUSH_ANTIALIAS},
    {"G-Pen", BRUSH_GPEN},
    {"Pixel Pen", BRUSH_PIXEL},
    {"Airbrush", BRUSH_AIRBRUSH}
};
#define NUM_BRUSHES (sizeof(brushDefs) / sizeof(brushDefs[0]))

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

static AppMode currentMode = MODE_HOME;
static ToolType currentTool = TOOL_BRUSH;
static MenuTab currentMenuTab = TAB_TOOL;

// HSV color state (for color picker)
static float currentHue = 0.0f;        // 0-360
static float currentSaturation = 1.0f; // 0-1
static float currentValue = 0.0f;      // 0-1

// Layer system
static Layer layers[MAX_LAYERS];
static int currentLayerIndex = 0;
static int numLayers = MAX_LAYERS;

// Composite buffer (result of merging all layers)
static u32* compositeBuffer = NULL;
static C3D_Tex canvasTex;
static Tex3DS_SubTexture canvasSubTex;
static C2D_Image canvasImage;

// Current drawing state
static u32 currentColor = 0x000000FF;  // Black (RGBA format: 0xRRGGBBAA)
static int brushSizesByType[BRUSH_TYPE_COUNT] = {2, 2, 2, 2};
static u8 brushAlpha = 255;  // Brush opacity (0-255)
static bool isDrawing = false;
static float lastCanvasX = 0.0f;
static float lastCanvasY = 0.0f;
static bool brushSizeSliderActive = false;  // For brush size preview on top screen

// Brush selection state
static int currentBrushType = 0;  // Index into brushDefs
static float brushListScrollY = 0;  // Scroll position for brush list
static float brushListLastTouchY = 0;  // For scroll tracking
static bool brushListDragging = false;  // Whether dragging the list

// Fill tool settings
static int fillExpand = 0;  // Fill expansion in pixels (0-10)
static bool fillExpandSliderActive = false;  // For slider tracking
#define FILL_EXPAND_MAX 10

static int getCurrentBrushSize(void) {
    return brushSizesByType[currentBrushType];
}

static void setCurrentBrushSize(int size) {
    brushSizesByType[currentBrushType] = size;
}

// Icon sprites
static C2D_SpriteSheet iconSpriteSheet;
static C2D_Sprite brushIconSprite;
static C2D_Sprite eraserIconSprite;
static C2D_Sprite bucketIconSprite;
static C2D_Sprite wrenchIconSprite;
static C2D_Sprite closeIconSprite;
static C2D_Sprite plusIconSprite;
static C2D_Sprite minusIconSprite;
static C2D_Sprite eyeIconSprite;
static C2D_Sprite clearIconSprite;
static C2D_Sprite upArrowIconSprite;
static C2D_Sprite downArrowIconSprite;
static C2D_Sprite mergeIconSprite;
static C2D_Sprite zoomInIconSprite;
static C2D_Sprite zoomOutIconSprite;
static C2D_Sprite undoIconSprite;
static C2D_Sprite redoIconSprite;
static C2D_Sprite saveIconSprite;
static C2D_Sprite saveAsIconSprite;
static C2D_Sprite exportIconSprite;
static C2D_Sprite palettePlusIconSprite;
static C2D_Sprite paletteMinusIconSprite;
static C2D_Sprite clippingIconSprite;
static C2D_Sprite alphaLockIconSprite;
static C2D_Sprite pencilIconSprite;
static C2D_Sprite crossArrowIconSprite;
static C2D_Sprite checkIconSprite;
static C2D_Sprite folderIconSprite;
static C2D_Sprite settingsIconSprite;
static C2D_Sprite newFileIconSprite;
static C2D_Sprite backArrowIconSprite;

// Screen targets (for dialog rendering)
static C3D_RenderTarget* g_topScreen = NULL;
static C3D_RenderTarget* g_bottomScreen = NULL;

// FPS counter
static u64 lastFrameTime = 0;
static float currentFPS = 0.0f;

// Canvas update optimization
static bool canvasNeedsUpdate = true;  // Dirty flag for canvas
static int updateFrameCounter = 0;     // Frame counter for update throttling
#define UPDATE_INTERVAL_DRAWING 3      // Update every N frames while drawing

// Zoom and pan state
static float canvasZoom = 1.0f;
static float canvasPanX = 0.0f;
static float canvasPanY = 0.0f;
static bool isPanning = false;
static touchPosition panStartTouch;

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
static C2D_TextBuf g_textBuf;

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
#define PROJECT_FILE_MAGIC 0x4D474457  // "MGDW"
#define PROJECT_FILE_VERSION 2

// Current project state
static char currentProjectName[PROJECT_NAME_MAX] = "";
static bool projectHasName = false;
static bool projectHasUnsavedChanges = false;

// Open project browser state
#define OPEN_MAX_PROJECTS 64
#define OPEN_LIST_ITEM_HEIGHT 36
static char openProjectNames[OPEN_MAX_PROJECTS][PROJECT_NAME_MAX];
static int  openProjectCount = 0;
static int  openSelectedIndex = -1;  // -1 = none selected
static float openListScrollY = 0;
static int  openListLastTouchX = 0;
static float openListLastTouchY = 0;
static bool  openListDragging = false;

// Preview texture for open browser
static C3D_Tex openPreviewTex;
static Tex3DS_SubTexture openPreviewSubTex;
static C2D_Image openPreviewImage;
static bool openPreviewValid = false;
static int openPreviewWidth = 0;
static int openPreviewHeight = 0;

// New project creation state
#define NEW_PROJECT_MAX_WIDTH  MAX_CANVAS_DIM
#define NEW_PROJECT_MAX_HEIGHT MAX_CANVAS_DIM
static char newProjectName[PROJECT_NAME_MAX] = "";
static int  newProjectWidth = BOTTOM_SCREEN_WIDTH;
static int  newProjectHeight = BOTTOM_SCREEN_HEIGHT;

// Menu layout
#define MENU_TAB_HEIGHT 40
#define MENU_TAB_WIDTH 80
#define MENU_CONTENT_Y 42  // Tab height + small gap
#define MENU_CONTENT_PADDING 8  // Common padding for tab content

// Color palette settings
#define PALETTE_MAX_COLORS 24
#define PALETTE_COLS 6
#define PALETTE_ROWS 4
#define PALETTE_CELL_SIZE 24
#define PALETTE_SPACING 4

static u32 paletteColors[PALETTE_MAX_COLORS];
static bool paletteUsed[PALETTE_MAX_COLORS];
static bool paletteDeleteMode = false;  // false = add mode, true = delete mode

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
#define MCP_ALPHA_SLIDER_WIDTH (MCP_HUE_X + MCP_HUE_WIDTH - MCP_SV_X)  // Full width of SV + Hue

// Undo/Redo history settings
#define HISTORY_MAX 10

// History entry structure
typedef struct {
    u32* buffer;          // Snapshot of the layer's pixel data
    bool visible;         // Layer visibility
    u8 opacity;           // Layer opacity (0-255)
    BlendMode blendMode;  // Blend mode
    bool alphaLock;       // Alpha lock
    bool clipping;        // Clipping
    char name[32];        // Layer name
} LayerSnapshot;

typedef struct {
    int currentLayerIndex;          // Selected layer index
    LayerSnapshot layers[MAX_LAYERS];
} HistoryEntry;

// History system
static HistoryEntry historyStack[HISTORY_MAX];
static int historyCount = 0;      // Number of entries in history
static int historyIndex = -1;     // Current position in history (-1 = no history)
static bool historyInitialized = false;

// Function prototypes
static int nextPowerOf2(int n);
static void initLayers(void);
static void exitLayers(void);
static void resetLayersForNewProject(void);
static void applyCanvasSize(int width, int height);
static void clearLayer(int layerIndex, u32 color);
static void floodFill(int layerIndex, int startX, int startY, u32 fillColor, int expand);
static void drawPixelToLayer(int layerIndex, int x, int y, u32 color);
static void drawBrushToLayer(int layerIndex, int x, int y, int size, u32 color);
static void drawLineToLayer(int layerIndex, int x0, int y0, int x1, int y1, int size, u32 color);
static void compositeAllLayers(void);
static void updateCanvasTexture(void);
static void forceUpdateCanvasTexture(void);
static void renderUI(C3D_RenderTarget* target);
static void renderCanvas(C3D_RenderTarget* target, bool showOverlay);
static void renderColorPicker(C3D_RenderTarget* target);
static void renderMenu(C3D_RenderTarget* target);
static void renderHomeMenu(C3D_RenderTarget* target);
static void renderOpenMenu(C3D_RenderTarget* target);
static void renderNewProjectMenu(C3D_RenderTarget* target);
static void scanProjectFiles(void);
static void freeOpenPreview(void);
static bool loadProjectPreview(const char* projectName);
static bool loadProject(const char* projectName);
static void initIcons(void);
static void exitIcons(void);
static u32 hsvToRgb(float h, float s, float v);
static void rgbToHsv(u32 color, float* h, float* s, float* v);
static u32 blendPixel(u32 dst, u32 src, BlendMode mode, u8 opacity);
static void initHistory(void);
static void exitHistory(void);
static void pushHistory(void);
static void undo(void);
static void redo(void);

//---------------------------------------------------------------------------------
// Blend two pixels with specified blend mode and opacity
//---------------------------------------------------------------------------------
static u32 blendPixel(u32 dst, u32 src, BlendMode mode, u8 opacity) {
    // Extract RGBA components (format: 0xRRGGBBAA)
    u8 srcR = (src >> 24) & 0xFF;
    u8 srcG = (src >> 16) & 0xFF;
    u8 srcB = (src >> 8) & 0xFF;
    u8 srcA = src & 0xFF;
    
    u8 dstR = (dst >> 24) & 0xFF;
    u8 dstG = (dst >> 16) & 0xFF;
    u8 dstB = (dst >> 8) & 0xFF;
    u8 dstA = dst & 0xFF;
    
    // Apply layer opacity to source alpha
    srcA = (srcA * opacity) / 255;
    
    // If source is fully transparent, return destination
    if (srcA == 0) return dst;
    
    u8 outR, outG, outB, outA;
    
    switch (mode) {
        case BLEND_ADD:
            // Additive blending (clamped to 255)
            outR = (dstR + (srcR * srcA / 255) > 255) ? 255 : dstR + (srcR * srcA / 255);
            outG = (dstG + (srcG * srcA / 255) > 255) ? 255 : dstG + (srcG * srcA / 255);
            outB = (dstB + (srcB * srcA / 255) > 255) ? 255 : dstB + (srcB * srcA / 255);
            outA = (dstA > srcA) ? dstA : srcA;
            break;
            
        case BLEND_MULTIPLY:
            // Multiply blending
            {
                u8 mulR = (dstR * srcR) / 255;
                u8 mulG = (dstG * srcG) / 255;
                u8 mulB = (dstB * srcB) / 255;
                // Blend between original and multiplied based on alpha
                outR = dstR + ((mulR - dstR) * srcA) / 255;
                outG = dstG + ((mulG - dstG) * srcA) / 255;
                outB = dstB + ((mulB - dstB) * srcA) / 255;
                outA = (dstA > srcA) ? dstA : srcA;
            }
            break;
            
        case BLEND_NORMAL:
        default:
            // Normal alpha blending
            if (srcA == 255) {
                outR = srcR;
                outG = srcG;
                outB = srcB;
                outA = 255;
            } else {
                u16 invSrcA = 255 - srcA;
                outR = (srcR * srcA + dstR * invSrcA) / 255;
                outG = (srcG * srcA + dstG * invSrcA) / 255;
                outB = (srcB * srcA + dstB * invSrcA) / 255;
                outA = srcA + (dstA * invSrcA) / 255;
            }
            break;
    }
    
    return (outR << 24) | (outG << 16) | (outB << 8) | outA;
}

//---------------------------------------------------------------------------------
// Convert HSV to RGB (returns RGBA format for GPU texture: 0xRRGGBBAA)
//---------------------------------------------------------------------------------
static u32 hsvToRgb(float h, float s, float v) {
    float r, g, b;
    
    int i = (int)(h / 60.0f) % 6;
    float f = (h / 60.0f) - i;
    float p = v * (1.0f - s);
    float q = v * (1.0f - f * s);
    float t = v * (1.0f - (1.0f - f) * s);
    
    switch (i) {
        case 0: r = v; g = t; b = p; break;
        case 1: r = q; g = v; b = p; break;
        case 2: r = p; g = v; b = t; break;
        case 3: r = p; g = q; b = v; break;
        case 4: r = t; g = p; b = v; break;
        case 5: r = v; g = p; b = q; break;
        default: r = v; g = t; b = p; break;
    }
    
    u8 rb = (u8)(r * 255);
    u8 gb = (u8)(g * 255);
    u8 bb = (u8)(b * 255);
    
    // Return in RGBA format (0xRRGGBBAA) for GPU_RGBA8 texture
    return (rb << 24) | (gb << 16) | (bb << 8) | 0xFF;
}

//---------------------------------------------------------------------------------
// Convert RGB (RGBA format: 0xRRGGBBAA) to HSV
//---------------------------------------------------------------------------------
static void rgbToHsv(u32 color, float* h, float* s, float* v) {
    float r = ((color >> 24) & 0xFF) / 255.0f;
    float g = ((color >> 16) & 0xFF) / 255.0f;
    float b = ((color >> 8) & 0xFF) / 255.0f;
    
    float maxC = r > g ? (r > b ? r : b) : (g > b ? g : b);
    float minC = r < g ? (r < b ? r : b) : (g < b ? g : b);
    float delta = maxC - minC;
    
    *v = maxC;
    
    if (maxC == 0) {
        *s = 0;
        *h = 0;
        return;
    }
    
    *s = delta / maxC;
    
    if (delta == 0) {
        *h = 0;
        return;
    }
    
    if (maxC == r) {
        *h = 60.0f * ((g - b) / delta);
    } else if (maxC == g) {
        *h = 60.0f * (2.0f + (b - r) / delta);
    } else {
        *h = 60.0f * (4.0f + (r - g) / delta);
    }
    
    if (*h < 0) *h += 360.0f;
}

//---------------------------------------------------------------------------------
// Initialize icon sprites
//---------------------------------------------------------------------------------
static void initIcons(void) {
    iconSpriteSheet = C2D_SpriteSheetLoad("romfs:/gfx/icons.t3x");
    
    // Bucket/Fill icon (index 0)
    C2D_SpriteFromSheet(&bucketIconSprite, iconSpriteSheet, 0);
    C2D_SpriteSetCenter(&bucketIconSprite, 0.5f, 0.5f);
    
    // Wrench/Tool icon (index 1)
    C2D_SpriteFromSheet(&wrenchIconSprite, iconSpriteSheet, 1);
    C2D_SpriteSetCenter(&wrenchIconSprite, 0.5f, 0.5f);
    
    // Brush icon (index 2)
    C2D_SpriteFromSheet(&brushIconSprite, iconSpriteSheet, 2);
    C2D_SpriteSetCenter(&brushIconSprite, 0.5f, 0.5f);
    
    // Eraser icon (index 3)
    C2D_SpriteFromSheet(&eraserIconSprite, iconSpriteSheet, 3);
    C2D_SpriteSetCenter(&eraserIconSprite, 0.5f, 0.5f);
    
    // Close icon (index 4)
    C2D_SpriteFromSheet(&closeIconSprite, iconSpriteSheet, 4);
    C2D_SpriteSetCenter(&closeIconSprite, 0.5f, 0.5f);
    
    // Plus icon (index 7)
    C2D_SpriteFromSheet(&plusIconSprite, iconSpriteSheet, 7);
    C2D_SpriteSetCenter(&plusIconSprite, 0.5f, 0.5f);
    
    // Minus icon (index 6)
    C2D_SpriteFromSheet(&minusIconSprite, iconSpriteSheet, 6);
    C2D_SpriteSetCenter(&minusIconSprite, 0.5f, 0.5f);
    
    // Eye icon (index 9)
    C2D_SpriteFromSheet(&eyeIconSprite, iconSpriteSheet, 9);
    C2D_SpriteSetCenter(&eyeIconSprite, 0.5f, 0.5f);
    
    // Clear icon (index 8)
    C2D_SpriteFromSheet(&clearIconSprite, iconSpriteSheet, 8);
    C2D_SpriteSetCenter(&clearIconSprite, 0.5f, 0.5f);
    
    // Up arrow icon (index 10)
    C2D_SpriteFromSheet(&upArrowIconSprite, iconSpriteSheet, 10);
    C2D_SpriteSetCenter(&upArrowIconSprite, 0.5f, 0.5f);
    
    // Down arrow icon (index 11)
    C2D_SpriteFromSheet(&downArrowIconSprite, iconSpriteSheet, 11);
    C2D_SpriteSetCenter(&downArrowIconSprite, 0.5f, 0.5f);
    
    // Merge icon (index 12)
    C2D_SpriteFromSheet(&mergeIconSprite, iconSpriteSheet, 12);
    C2D_SpriteSetCenter(&mergeIconSprite, 0.5f, 0.5f);
    
    // Zoom out icon (index 13)
    C2D_SpriteFromSheet(&zoomOutIconSprite, iconSpriteSheet, 13);
    C2D_SpriteSetCenter(&zoomOutIconSprite, 0.5f, 0.5f);
    
    // Zoom in icon (index 14)
    C2D_SpriteFromSheet(&zoomInIconSprite, iconSpriteSheet, 14);
    C2D_SpriteSetCenter(&zoomInIconSprite, 0.5f, 0.5f);
    
    // Undo icon (index 15)
    C2D_SpriteFromSheet(&undoIconSprite, iconSpriteSheet, 15);
    C2D_SpriteSetCenter(&undoIconSprite, 0.5f, 0.5f);
    
    // Redo icon (index 16)
    C2D_SpriteFromSheet(&redoIconSprite, iconSpriteSheet, 16);
    C2D_SpriteSetCenter(&redoIconSprite, 0.5f, 0.5f);
    
    // Save icon (index 17)
    C2D_SpriteFromSheet(&saveIconSprite, iconSpriteSheet, 17);
    C2D_SpriteSetCenter(&saveIconSprite, 0.5f, 0.5f);
    
    // Save As icon (index 18)
    C2D_SpriteFromSheet(&saveAsIconSprite, iconSpriteSheet, 18);
    C2D_SpriteSetCenter(&saveAsIconSprite, 0.5f, 0.5f);
    
    // Export icon (index 19)
    C2D_SpriteFromSheet(&exportIconSprite, iconSpriteSheet, 19);
    C2D_SpriteSetCenter(&exportIconSprite, 0.5f, 0.5f);
    
    // Palette Plus icon (index 20)
    C2D_SpriteFromSheet(&palettePlusIconSprite, iconSpriteSheet, 20);
    C2D_SpriteSetCenter(&palettePlusIconSprite, 0.5f, 0.5f);
    
    // Palette Minus icon (index 21)
    C2D_SpriteFromSheet(&paletteMinusIconSprite, iconSpriteSheet, 21);
    C2D_SpriteSetCenter(&paletteMinusIconSprite, 0.5f, 0.5f);
    
    // Clipping icon (index 23)
    C2D_SpriteFromSheet(&clippingIconSprite, iconSpriteSheet, 23);
    C2D_SpriteSetCenter(&clippingIconSprite, 0.5f, 0.5f);
    
    // Alpha Lock icon (index 24)
    C2D_SpriteFromSheet(&alphaLockIconSprite, iconSpriteSheet, 24);
    C2D_SpriteSetCenter(&alphaLockIconSprite, 0.5f, 0.5f);
    
    // Pencil icon (index 25)
    C2D_SpriteFromSheet(&pencilIconSprite, iconSpriteSheet, 25);
    C2D_SpriteSetCenter(&pencilIconSprite, 0.5f, 0.5f);

    // Cross Arrow icon (index 26)
    C2D_SpriteFromSheet(&crossArrowIconSprite, iconSpriteSheet, 26);
    C2D_SpriteSetCenter(&crossArrowIconSprite, 0.5f, 0.5f);

    // Check icon (index 27)
    C2D_SpriteFromSheet(&checkIconSprite, iconSpriteSheet, 27);
    C2D_SpriteSetCenter(&checkIconSprite, 0.5f, 0.5f);

    // Folder icon (index 28)
    C2D_SpriteFromSheet(&folderIconSprite, iconSpriteSheet, 28);
    C2D_SpriteSetCenter(&folderIconSprite, 0.5f, 0.5f);

    // Settings icon (index 29)
    C2D_SpriteFromSheet(&settingsIconSprite, iconSpriteSheet, 29);
    C2D_SpriteSetCenter(&settingsIconSprite, 0.5f, 0.5f);

    // New file icon (index 30)
    C2D_SpriteFromSheet(&newFileIconSprite, iconSpriteSheet, 30);
    C2D_SpriteSetCenter(&newFileIconSprite, 0.5f, 0.5f);

    // Back arrow icon (index 31)
    C2D_SpriteFromSheet(&backArrowIconSprite, iconSpriteSheet, 31);
    C2D_SpriteSetCenter(&backArrowIconSprite, 0.5f, 0.5f);
}

//---------------------------------------------------------------------------------
// Initialize palette with default colors
//---------------------------------------------------------------------------------
static void initPalette(void) {
    // Clear all palette slots
    for (int i = 0; i < PALETTE_MAX_COLORS; i++) {
        paletteColors[i] = 0x00000000;
        paletteUsed[i] = false;
    }
    
    // Set some default colors
    paletteColors[0] = 0x000000FF;  // Black
    paletteUsed[0] = true;
    paletteColors[1] = 0xFFFFFFFF;  // White
    paletteUsed[1] = true;
    paletteColors[2] = 0xFF0000FF;  // Red
    paletteUsed[2] = true;
    paletteColors[3] = 0x00FF00FF;  // Green
    paletteUsed[3] = true;
    paletteColors[4] = 0x0000FFFF;  // Blue
    paletteUsed[4] = true;
    paletteColors[5] = 0xFFFF00FF;  // Yellow
    paletteUsed[5] = true;
    paletteColors[6] = 0xFF00FFFF;  // Magenta
    paletteUsed[6] = true;
    paletteColors[7] = 0x00FFFFFF;  // Cyan
    paletteUsed[7] = true;
}

//---------------------------------------------------------------------------------
// Create directory if it doesn't exist
//---------------------------------------------------------------------------------
static void ensureDirectoryExists(const char* path) {
    struct stat st = {0};
    if (stat(path, &st) == -1) {
        mkdir(path, 0777);
    }
}

//---------------------------------------------------------------------------------
// Show software keyboard for text input
//---------------------------------------------------------------------------------
static bool showKeyboard(const char* hintText, char* outBuf, size_t bufSize) {
    SwkbdState swkbd;
    swkbdInit(&swkbd, SWKBD_TYPE_NORMAL, 2, bufSize - 1);
    swkbdSetHintText(&swkbd, hintText);
    swkbdSetButton(&swkbd, SWKBD_BUTTON_LEFT, "Cancel", false);
    swkbdSetButton(&swkbd, SWKBD_BUTTON_RIGHT, "OK", true);
    swkbdSetValidation(&swkbd, SWKBD_NOTEMPTY_NOTBLANK, 0, 0);
    swkbdSetFeatures(&swkbd, SWKBD_PREDICTIVE_INPUT);
    
    SwkbdButton button = swkbdInputText(&swkbd, outBuf, bufSize);
    return (button == SWKBD_BUTTON_RIGHT);
}

//---------------------------------------------------------------------------------
// Show numeric keyboard for integer input, returns -1 on cancel
//---------------------------------------------------------------------------------
static int showNumericKeyboard(const char* hintText, int currentValue, int minVal, int maxVal) {
    char buf[16];
    snprintf(buf, sizeof(buf), "%d", currentValue);
    SwkbdState swkbd;
    swkbdInit(&swkbd, SWKBD_TYPE_NUMPAD, 2, 8);
    swkbdSetHintText(&swkbd, hintText);
    swkbdSetButton(&swkbd, SWKBD_BUTTON_LEFT, "Cancel", false);
    swkbdSetButton(&swkbd, SWKBD_BUTTON_RIGHT, "OK", true);
    swkbdSetValidation(&swkbd, SWKBD_NOTEMPTY_NOTBLANK, 0, 0);
    swkbdSetInitialText(&swkbd, buf);
    SwkbdButton button = swkbdInputText(&swkbd, buf, sizeof(buf));
    if (button != SWKBD_BUTTON_RIGHT) return -1;
    int val = atoi(buf);
    if (val < minVal) val = minVal;
    if (val > maxVal) val = maxVal;
    return val;
}

//---------------------------------------------------------------------------------
// Forward declaration
//---------------------------------------------------------------------------------
static bool fileExists(const char* path);

//---------------------------------------------------------------------------------
// Find next available Untitled n project index
//---------------------------------------------------------------------------------
static int findNextUntitledIndex(void) {
    const char* prefix = "Untitled ";
    const char* suffix = ".mgdw";
    size_t prefixLen = strlen(prefix);
    size_t suffixLen = strlen(suffix);
    int maxIndex = 0;

    ensureDirectoryExists(SAVE_DIR);
    DIR* dir = opendir(SAVE_DIR);
    if (dir) {
        struct dirent* ent;
        while ((ent = readdir(dir)) != NULL) {
            const char* name = ent->d_name;
            size_t len = strlen(name);
            if (len <= prefixLen + suffixLen) continue;
            if (strncmp(name, prefix, prefixLen) != 0) continue;
            if (strcmp(name + len - suffixLen, suffix) != 0) continue;

            size_t numLen = len - prefixLen - suffixLen;
            if (numLen == 0 || numLen >= 12) continue;
            char numBuf[12];
            memcpy(numBuf, name + prefixLen, numLen);
            numBuf[numLen] = '\0';

            bool allDigits = true;
            for (size_t i = 0; i < numLen; i++) {
                if (!isdigit((unsigned char)numBuf[i])) {
                    allDigits = false;
                    break;
                }
            }
            if (!allDigits) continue;

            int val = atoi(numBuf);
            if (val > maxIndex) maxIndex = val;
        }
        closedir(dir);
    }

    int candidate = maxIndex + 1;
    while (1) {
        char filePath[256];
        snprintf(filePath, sizeof(filePath), "%s/Untitled %d.mgdw", SAVE_DIR, candidate);
        if (!fileExists(filePath)) break;
        candidate++;
    }
    return candidate;
}

//---------------------------------------------------------------------------------
// Check if a file exists
//---------------------------------------------------------------------------------
static bool fileExists(const char* path) {
    struct stat st;
    return (stat(path, &st) == 0);
}

//---------------------------------------------------------------------------------
// Project file header structure
//---------------------------------------------------------------------------------
typedef struct {
    u32 magic;              // File magic number
    u32 version;            // File format version
    u32 canvasWidth;        // Canvas width
    u32 canvasHeight;       // Canvas height
    u32 numLayers;          // Number of layers
    u32 currentLayer;       // Current selected layer
    u32 currentTool;        // Current tool
    u32 brushSize;          // Brush size
    u32 currentColor;       // Current drawing color
    u32 brushAlpha;         // Brush alpha (0-255)
    u32 currentBrushType;   // Current brush type
    float hue;              // HSV hue
    float saturation;       // HSV saturation
    float value;            // HSV value
    u32 paletteCount;       // Number of palette colors used
} ProjectHeader;

//---------------------------------------------------------------------------------
// Save project to file
//---------------------------------------------------------------------------------
static bool saveProject(const char* projectName) {
    // Ensure save directory exists
    ensureDirectoryExists(SAVE_DIR);
    
    // Build file path
    char filePath[256];
    snprintf(filePath, sizeof(filePath), "%s/%s.mgdw", SAVE_DIR, projectName);
    
    // Check if file already exists - if so, add number suffix
    if (fileExists(filePath)) {
        int suffix = 1;
        do {
            snprintf(filePath, sizeof(filePath), "%s/%s_%d.mgdw", SAVE_DIR, projectName, suffix);
            suffix++;
        } while (fileExists(filePath) && suffix < 100);
        
        if (suffix >= 100) {
            return false;  // Too many files with same name
        }
    }
    
    FILE* fp = fopen(filePath, "wb");
    if (!fp) {
        return false;
    }
    
    // Prepare header
    ProjectHeader header;
    header.magic = PROJECT_FILE_MAGIC;
    header.version = PROJECT_FILE_VERSION;
    header.canvasWidth = CANVAS_WIDTH;
    header.canvasHeight = CANVAS_HEIGHT;
    header.numLayers = MAX_LAYERS;
    header.currentLayer = currentLayerIndex;
    header.currentTool = currentTool;
    header.brushSize = getCurrentBrushSize();
    header.currentColor = currentColor;
    header.brushAlpha = brushAlpha;
    header.currentBrushType = (u32)currentBrushType;
    header.hue = currentHue;
    header.saturation = currentSaturation;
    header.value = currentValue;
    
    // Count used palette colors
    header.paletteCount = 0;
    for (int i = 0; i < PALETTE_MAX_COLORS; i++) {
        if (paletteUsed[i]) header.paletteCount++;
    }
    
    // Write header
    fwrite(&header, sizeof(ProjectHeader), 1, fp);
    
    // Write layer data
    for (int i = 0; i < MAX_LAYERS; i++) {
        // Write layer properties
        fwrite(&layers[i].visible, sizeof(bool), 1, fp);
        fwrite(&layers[i].opacity, sizeof(u8), 1, fp);
        fwrite(&layers[i].blendMode, sizeof(BlendMode), 1, fp);
        fwrite(&layers[i].alphaLock, sizeof(bool), 1, fp);
        fwrite(&layers[i].clipping, sizeof(bool), 1, fp);
        fwrite(layers[i].name, sizeof(layers[i].name), 1, fp);
        
        // Write layer pixel data (only canvas area)
        for (int y = 0; y < CANVAS_HEIGHT; y++) {
            fwrite(&layers[i].buffer[y * TEX_WIDTH], sizeof(u32), CANVAS_WIDTH, fp);
        }
    }

    // Write brush size per type
    fwrite(brushSizesByType, sizeof(brushSizesByType[0]), BRUSH_TYPE_COUNT, fp);
    
    // Write palette data
    fwrite(paletteUsed, sizeof(bool), PALETTE_MAX_COLORS, fp);
    fwrite(paletteColors, sizeof(u32), PALETTE_MAX_COLORS, fp);
    
    fclose(fp);
    
    // Update project name
    strncpy(currentProjectName, projectName, PROJECT_NAME_MAX - 1);
    currentProjectName[PROJECT_NAME_MAX - 1] = '\0';
    projectHasName = true;
    projectHasUnsavedChanges = false;  // Reset unsaved changes flag
    
    return true;
}

//---------------------------------------------------------------------------------
// Quick save (overwrite) project - uses same filename, no suffix added
//---------------------------------------------------------------------------------
static bool quickSaveProject(void) {
    if (!projectHasName || currentProjectName[0] == '\0') {
        return false;
    }
    
    // Ensure save directory exists
    ensureDirectoryExists(SAVE_DIR);
    
    // Build file path
    char filePath[256];
    snprintf(filePath, sizeof(filePath), "%s/%s.mgdw", SAVE_DIR, currentProjectName);
    
    FILE* fp = fopen(filePath, "wb");
    if (!fp) {
        return false;
    }
    
    // Prepare header
    ProjectHeader header;
    header.magic = PROJECT_FILE_MAGIC;
    header.version = PROJECT_FILE_VERSION;
    header.canvasWidth = CANVAS_WIDTH;
    header.canvasHeight = CANVAS_HEIGHT;
    header.numLayers = MAX_LAYERS;
    header.currentLayer = currentLayerIndex;
    header.currentTool = currentTool;
    header.brushSize = getCurrentBrushSize();
    header.currentColor = currentColor;
    header.brushAlpha = brushAlpha;
    header.currentBrushType = (u32)currentBrushType;
    header.hue = currentHue;
    header.saturation = currentSaturation;
    header.value = currentValue;
    
    // Count used palette colors
    header.paletteCount = 0;
    for (int i = 0; i < PALETTE_MAX_COLORS; i++) {
        if (paletteUsed[i]) header.paletteCount++;
    }
    
    // Write header
    fwrite(&header, sizeof(ProjectHeader), 1, fp);
    
    // Write layer data
    for (int i = 0; i < MAX_LAYERS; i++) {
        // Write layer properties
        fwrite(&layers[i].visible, sizeof(bool), 1, fp);
        fwrite(&layers[i].opacity, sizeof(u8), 1, fp);
        fwrite(&layers[i].blendMode, sizeof(BlendMode), 1, fp);
        fwrite(&layers[i].alphaLock, sizeof(bool), 1, fp);
        fwrite(&layers[i].clipping, sizeof(bool), 1, fp);
        fwrite(layers[i].name, sizeof(layers[i].name), 1, fp);
        
        // Write layer pixel data (only canvas area)
        for (int y = 0; y < CANVAS_HEIGHT; y++) {
            fwrite(&layers[i].buffer[y * TEX_WIDTH], sizeof(u32), CANVAS_WIDTH, fp);
        }
    }

    // Write brush size per type
    fwrite(brushSizesByType, sizeof(brushSizesByType[0]), BRUSH_TYPE_COUNT, fp);
    
    // Write palette data
    fwrite(paletteUsed, sizeof(bool), PALETTE_MAX_COLORS, fp);
    fwrite(paletteColors, sizeof(u32), PALETTE_MAX_COLORS, fp);
    
    fclose(fp);
    
    projectHasUnsavedChanges = false;  // Reset unsaved changes flag
    
    return true;
}

//---------------------------------------------------------------------------------
// Scan SAVE_DIR for .mgdw project files
//---------------------------------------------------------------------------------
static void scanProjectFiles(void) {
    openProjectCount = 0;
    DIR* dir = opendir(SAVE_DIR);
    if (!dir) return;
    
    struct dirent* entry;
    while ((entry = readdir(dir)) != NULL && openProjectCount < OPEN_MAX_PROJECTS) {
        const char* name = entry->d_name;
        size_t len = strlen(name);
        if (len > 5 && strcmp(name + len - 5, ".mgdw") == 0) {
            // Strip .mgdw extension
            size_t nameLen = len - 5;
            if (nameLen >= PROJECT_NAME_MAX) nameLen = PROJECT_NAME_MAX - 1;
            memcpy(openProjectNames[openProjectCount], name, nameLen);
            openProjectNames[openProjectCount][nameLen] = '\0';
            openProjectCount++;
        }
    }
    closedir(dir);
}

//---------------------------------------------------------------------------------
// Free preview texture resources
//---------------------------------------------------------------------------------
static void freeOpenPreview(void) {
    if (openPreviewValid) {
        C3D_TexDelete(&openPreviewTex);
        openPreviewValid = false;
    }
}

//---------------------------------------------------------------------------------
// Load project preview (composite thumbnail from file)
//---------------------------------------------------------------------------------
static bool loadProjectPreview(const char* projectName) {
    char filePath[256];
    snprintf(filePath, sizeof(filePath), "%s/%s.mgdw", SAVE_DIR, projectName);
    
    FILE* fp = fopen(filePath, "rb");
    if (!fp) return false;
    
    // Read header
    ProjectHeader header;
    if (fread(&header, sizeof(ProjectHeader), 1, fp) != 1) { fclose(fp); return false; }
    if (header.magic != PROJECT_FILE_MAGIC) { fclose(fp); return false; }
    
    int cw = header.canvasWidth;
    int ch = header.canvasHeight;
    if (cw <= 0 || cw > MAX_CANVAS_DIM || ch <= 0 || ch > MAX_CANVAS_DIM) { fclose(fp); return false; }
    
    int tw = nextPowerOf2(cw);
    int th = nextPowerOf2(ch);
    
    // Allocate temporary buffers for compositing
    u32* tempLayer = (u32*)malloc(tw * th * sizeof(u32));
    u32* composite = (u32*)malloc(tw * th * sizeof(u32));
    if (!tempLayer || !composite) {
        free(tempLayer);
        free(composite);
        fclose(fp);
        return false;
    }
    
    // Start with white background
    for (int y = 0; y < ch; y++) {
        for (int x = 0; x < cw; x++) {
            composite[y * tw + x] = 0xFFFFFFFF;
        }
    }
    
    // Read and composite each layer
    int numLayers = header.numLayers;
    if (numLayers > MAX_LAYERS) numLayers = MAX_LAYERS;
    
    for (int i = 0; i < (int)header.numLayers; i++) {
        // Read layer properties
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
        
        // Clear temp buffer
        memset(tempLayer, 0, tw * th * sizeof(u32));
        
        // Read pixel data
        for (int y = 0; y < ch; y++) {
            fread(&tempLayer[y * tw], sizeof(u32), cw, fp);
        }
        
        // Skip non-visible or zero-opacity layers (but we still read the data above)
        if (!visible || opacity == 0) continue;
        if (i >= numLayers) continue;
        
        // Composite this layer onto the result
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
    
    // Create or recreate preview texture
    freeOpenPreview();
    C3D_TexInit(&openPreviewTex, tw, th, GPU_RGBA8);
    C3D_TexSetFilter(&openPreviewTex, GPU_LINEAR, GPU_LINEAR);
    
    // Transfer composite data to the texture
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
    
    // Setup subtexture
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

//---------------------------------------------------------------------------------
// Load a project from file into the current canvas
//---------------------------------------------------------------------------------
static bool loadProject(const char* projectName) {
    char filePath[256];
    snprintf(filePath, sizeof(filePath), "%s/%s.mgdw", SAVE_DIR, projectName);
    
    FILE* fp = fopen(filePath, "rb");
    if (!fp) return false;
    
    // Read header
    ProjectHeader header;
    if (fread(&header, sizeof(ProjectHeader), 1, fp) != 1) { fclose(fp); return false; }
    if (header.magic != PROJECT_FILE_MAGIC) { fclose(fp); return false; }
    
    int cw = header.canvasWidth;
    int ch = header.canvasHeight;
    if (cw <= 0 || cw > MAX_CANVAS_DIM || ch <= 0 || ch > MAX_CANVAS_DIM) { fclose(fp); return false; }
    
    // Apply canvas size (this reallocates buffers if needed)
    applyCanvasSize(cw, ch);
    
    // Restore layer data
    int numLayers = header.numLayers;
    if (numLayers > MAX_LAYERS) numLayers = MAX_LAYERS;
    
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
        
        if (i < numLayers && layers[i].buffer) {
            layers[i].visible = visible;
            layers[i].opacity = opacity;
            layers[i].blendMode = blendMode;
            layers[i].alphaLock = alphaLock;
            layers[i].clipping = clipping;
            memcpy(layers[i].name, layerName, sizeof(layers[i].name));
            
            // Clear buffer first
            memset(layers[i].buffer, 0, TEX_WIDTH * TEX_HEIGHT * sizeof(u32));
            
            // Read pixel data
            for (int y = 0; y < ch; y++) {
                fread(&layers[i].buffer[y * TEX_WIDTH], sizeof(u32), cw, fp);
            }
        } else {
            // Skip pixel data for extra layers
            fseek(fp, cw * ch * sizeof(u32), SEEK_CUR);
        }
    }
    
    // Read brush sizes and palette (v2+)
    if (header.version >= 2) {
        fread(brushSizesByType, sizeof(brushSizesByType[0]), BRUSH_TYPE_COUNT, fp);
        fread(paletteUsed, sizeof(bool), PALETTE_MAX_COLORS, fp);
        fread(paletteColors, sizeof(u32), PALETTE_MAX_COLORS, fp);
    }
    
    fclose(fp);
    
    // Restore state from header
    currentLayerIndex = header.currentLayer;
    if (currentLayerIndex >= MAX_LAYERS) currentLayerIndex = 0;
    currentTool = (ToolType)header.currentTool;
    setCurrentBrushSize(header.brushSize);
    currentColor = header.currentColor;
    brushAlpha = header.brushAlpha;
    currentBrushType = header.currentBrushType;
    currentHue = header.hue;
    currentSaturation = header.saturation;
    currentValue = header.value;
    
    // Set project name
    strncpy(currentProjectName, projectName, PROJECT_NAME_MAX - 1);
    currentProjectName[PROJECT_NAME_MAX - 1] = '\0';
    projectHasName = true;
    projectHasUnsavedChanges = false;
    
    // Reset history and view
    exitHistory();
    initHistory();
    canvasPanX = 0.0f;
    canvasPanY = 0.0f;
    canvasZoom = 1.0f;
    canvasNeedsUpdate = true;
    
    return true;
}

//---------------------------------------------------------------------------------
// Clean up icon resources
//---------------------------------------------------------------------------------
static void exitIcons(void) {
    C2D_SpriteSheetFree(iconSpriteSheet);
}

//---------------------------------------------------------------------------------
// Round up to next power of 2
//---------------------------------------------------------------------------------
static int nextPowerOf2(int n) {
    if (n <= 0) return 1;
    n--;
    n |= n >> 1;
    n |= n >> 2;
    n |= n >> 4;
    n |= n >> 8;
    n |= n >> 16;
    return n + 1;
}

//---------------------------------------------------------------------------------
// Initialize all layers
//---------------------------------------------------------------------------------
static void initLayers(void) {
    // Compute texture size from canvas size
    texWidth = nextPowerOf2(canvasWidth);
    texHeight = nextPowerOf2(canvasHeight);

    size_t bufferSize = TEX_WIDTH * TEX_HEIGHT * sizeof(u32);
    
    // Allocate composite buffer
    compositeBuffer = (u32*)linearAlloc(bufferSize);
    if (!compositeBuffer) return;
    
    // Initialize each layer
    for (int i = 0; i < MAX_LAYERS; i++) {
        layers[i].buffer = (u32*)malloc(bufferSize);
        layers[i].visible = true;
        layers[i].opacity = 255;
        layers[i].blendMode = BLEND_NORMAL;
        snprintf(layers[i].name, sizeof(layers[i].name), "Layer %d", i + 1);
        layers[i].alphaLock = false;
        layers[i].clipping = false;
        
        if (layers[i].buffer) {
            // All layers start transparent (white background added during compositing)
            u32 clearColor = 0x00000000;
            for (int y = 0; y < TEX_HEIGHT; y++) {
                for (int x = 0; x < TEX_WIDTH; x++) {
                    layers[i].buffer[y * TEX_WIDTH + x] = clearColor;
                }
            }
        }
    }
    
    // Initialize texture
    C3D_TexInit(&canvasTex, TEX_WIDTH, TEX_HEIGHT, GPU_RGBA8);
    C3D_TexSetFilter(&canvasTex, GPU_LINEAR, GPU_LINEAR);
    C3D_TexSetWrap(&canvasTex, GPU_CLAMP_TO_EDGE, GPU_CLAMP_TO_EDGE);
    
    // Setup subtexture info
    // Reference top 240 pixels of texture (Y=0 to 239 in buffer)
    canvasSubTex.width = CANVAS_WIDTH;
    canvasSubTex.height = CANVAS_HEIGHT;
    canvasSubTex.left = 0.0f;
    canvasSubTex.top = (float)CANVAS_HEIGHT / TEX_HEIGHT;  // 240/256 = 0.9375
    canvasSubTex.right = (float)CANVAS_WIDTH / TEX_WIDTH;
    canvasSubTex.bottom = 0.0f;
    
    // Setup C2D_Image
    canvasImage.tex = &canvasTex;
    canvasImage.subtex = &canvasSubTex;
    
    // Initial composite
    compositeAllLayers();
}

//---------------------------------------------------------------------------------
// Reset layer state for a new project
//---------------------------------------------------------------------------------
static void resetLayersForNewProject(void) {
    for (int i = 0; i < MAX_LAYERS; i++) {
        layers[i].visible = true;
        layers[i].opacity = 255;
        layers[i].blendMode = BLEND_NORMAL;
        layers[i].alphaLock = false;
        layers[i].clipping = false;
        snprintf(layers[i].name, sizeof(layers[i].name), "Layer %d", i + 1);

        if (layers[i].buffer) {
            u32 clearColor = 0x00000000;
            for (int y = 0; y < TEX_HEIGHT; y++) {
                for (int x = 0; x < TEX_WIDTH; x++) {
                    layers[i].buffer[y * TEX_WIDTH + x] = clearColor;
                }
            }
        }
    }
    currentLayerIndex = 0;
}

//---------------------------------------------------------------------------------
// Apply canvas size and reallocate texture/buffers as needed
//---------------------------------------------------------------------------------
static void applyCanvasSize(int width, int height) {
    canvasWidth = width;
    canvasHeight = height;

    int newTexW = nextPowerOf2(width);
    int newTexH = nextPowerOf2(height);

    // Reallocate buffers if texture dimensions changed
    if (newTexW != texWidth || newTexH != texHeight) {
        texWidth = newTexW;
        texHeight = newTexH;
        size_t bufferSize = TEX_WIDTH * TEX_HEIGHT * sizeof(u32);

        // Reallocate composite buffer (must be linear memory for GPU transfer)
        if (compositeBuffer) {
            linearFree(compositeBuffer);
        }
        compositeBuffer = (u32*)linearAlloc(bufferSize);

        // Reallocate layer buffers
        for (int i = 0; i < MAX_LAYERS; i++) {
            if (layers[i].buffer) {
                free(layers[i].buffer);
            }
            layers[i].buffer = (u32*)malloc(bufferSize);
            if (layers[i].buffer) {
                memset(layers[i].buffer, 0, bufferSize);
            }
        }

        // Recreate GPU texture
        C3D_TexDelete(&canvasTex);
        C3D_TexInit(&canvasTex, TEX_WIDTH, TEX_HEIGHT, GPU_RGBA8);
        C3D_TexSetFilter(&canvasTex, GPU_LINEAR, GPU_LINEAR);
        C3D_TexSetWrap(&canvasTex, GPU_CLAMP_TO_EDGE, GPU_CLAMP_TO_EDGE);
    }

    // Update subtexture mapping
    canvasSubTex.width = canvasWidth;
    canvasSubTex.height = canvasHeight;
    canvasSubTex.left = 0.0f;
    canvasSubTex.top = (float)canvasHeight / TEX_HEIGHT;
    canvasSubTex.right = (float)canvasWidth / TEX_WIDTH;
    canvasSubTex.bottom = 0.0f;

    canvasImage.tex = &canvasTex;
    canvasImage.subtex = &canvasSubTex;

    canvasNeedsUpdate = true;
}

//---------------------------------------------------------------------------------
// Clean up layer resources
//---------------------------------------------------------------------------------
static void exitLayers(void) {
    for (int i = 0; i < MAX_LAYERS; i++) {
        if (layers[i].buffer) {
            free(layers[i].buffer);
            layers[i].buffer = NULL;
        }
    }
    
    if (compositeBuffer) {
        linearFree(compositeBuffer);
        compositeBuffer = NULL;
    }
    
    C3D_TexDelete(&canvasTex);
}

//---------------------------------------------------------------------------------
// Initialize history system
//---------------------------------------------------------------------------------
static void initHistory(void) {
    for (int i = 0; i < HISTORY_MAX; i++) {
        historyStack[i].currentLayerIndex = -1;
        for (int j = 0; j < MAX_LAYERS; j++) {
            historyStack[i].layers[j].buffer = NULL;
        }
    }
    historyCount = 0;
    historyIndex = -1;
    historyInitialized = true;
}

//---------------------------------------------------------------------------------
// Clean up history resources
//---------------------------------------------------------------------------------
static void exitHistory(void) {
    for (int i = 0; i < HISTORY_MAX; i++) {
        for (int j = 0; j < MAX_LAYERS; j++) {
            if (historyStack[i].layers[j].buffer) {
                free(historyStack[i].layers[j].buffer);
                historyStack[i].layers[j].buffer = NULL;
            }
        }
    }
    historyCount = 0;
    historyIndex = -1;
    historyInitialized = false;
}

//---------------------------------------------------------------------------------
// Push current layer state to history (call before modifying)
//---------------------------------------------------------------------------------
static void pushHistory(void) {
    if (!historyInitialized) return;
    for (int i = 0; i < MAX_LAYERS; i++) {
        if (!layers[i].buffer) return;
    }
    
    size_t bufferSize = TEX_WIDTH * TEX_HEIGHT * sizeof(u32);
    
    // If we're not at the end of history, discard redo entries
    if (historyIndex < historyCount - 1) {
        for (int i = historyIndex + 1; i < historyCount; i++) {
            for (int j = 0; j < MAX_LAYERS; j++) {
                if (historyStack[i].layers[j].buffer) {
                    free(historyStack[i].layers[j].buffer);
                    historyStack[i].layers[j].buffer = NULL;
                }
            }
        }
        historyCount = historyIndex + 1;
    }
    
    // If history is full, shift everything down
    if (historyCount >= HISTORY_MAX) {
        for (int j = 0; j < MAX_LAYERS; j++) {
            if (historyStack[0].layers[j].buffer) {
                free(historyStack[0].layers[j].buffer);
                historyStack[0].layers[j].buffer = NULL;
            }
        }
        for (int i = 0; i < HISTORY_MAX - 1; i++) {
            historyStack[i] = historyStack[i + 1];
        }
        historyStack[HISTORY_MAX - 1].currentLayerIndex = -1;
        for (int j = 0; j < MAX_LAYERS; j++) {
            historyStack[HISTORY_MAX - 1].layers[j].buffer = NULL;
        }
        historyCount = HISTORY_MAX - 1;
        historyIndex = historyCount - 1;
    }
    
    // Allocate new history entry buffers if needed
    for (int j = 0; j < MAX_LAYERS; j++) {
        if (!historyStack[historyCount].layers[j].buffer) {
            historyStack[historyCount].layers[j].buffer = (u32*)malloc(bufferSize);
            if (!historyStack[historyCount].layers[j].buffer) {
                for (int k = 0; k < MAX_LAYERS; k++) {
                    if (historyStack[historyCount].layers[k].buffer) {
                        free(historyStack[historyCount].layers[k].buffer);
                        historyStack[historyCount].layers[k].buffer = NULL;
                    }
                }
                return;
            }
        }
    }

    // Copy current state for all layers
    historyStack[historyCount].currentLayerIndex = currentLayerIndex;
    for (int j = 0; j < MAX_LAYERS; j++) {
        memcpy(historyStack[historyCount].layers[j].buffer, layers[j].buffer, bufferSize);
        historyStack[historyCount].layers[j].visible = layers[j].visible;
        historyStack[historyCount].layers[j].opacity = layers[j].opacity;
        historyStack[historyCount].layers[j].blendMode = layers[j].blendMode;
        historyStack[historyCount].layers[j].alphaLock = layers[j].alphaLock;
        historyStack[historyCount].layers[j].clipping = layers[j].clipping;
        strncpy(historyStack[historyCount].layers[j].name, layers[j].name, sizeof(historyStack[historyCount].layers[j].name));
        historyStack[historyCount].layers[j].name[sizeof(historyStack[historyCount].layers[j].name) - 1] = '\0';
    }
    historyCount++;
    historyIndex = historyCount - 1;
}

//---------------------------------------------------------------------------------
// Undo last operation
//---------------------------------------------------------------------------------
static void undo(void) {
    if (!historyInitialized) return;
    if (historyIndex < 0) return;
    
    HistoryEntry* entry = &historyStack[historyIndex];
    size_t bufferSize = TEX_WIDTH * TEX_HEIGHT * sizeof(u32);
    u32* tempBuffer = (u32*)malloc(bufferSize);
    if (!tempBuffer) return;

    int tempLayerIndex = currentLayerIndex;
    currentLayerIndex = entry->currentLayerIndex;
    entry->currentLayerIndex = tempLayerIndex;

    for (int j = 0; j < MAX_LAYERS; j++) {
        if (layers[j].buffer && entry->layers[j].buffer) {
            memcpy(tempBuffer, layers[j].buffer, bufferSize);
            memcpy(layers[j].buffer, entry->layers[j].buffer, bufferSize);
            memcpy(entry->layers[j].buffer, tempBuffer, bufferSize);
        }

        bool tempVisible = layers[j].visible;
        layers[j].visible = entry->layers[j].visible;
        entry->layers[j].visible = tempVisible;

        u8 tempOpacity = layers[j].opacity;
        layers[j].opacity = entry->layers[j].opacity;
        entry->layers[j].opacity = tempOpacity;

        BlendMode tempBlend = layers[j].blendMode;
        layers[j].blendMode = entry->layers[j].blendMode;
        entry->layers[j].blendMode = tempBlend;

        bool tempAlphaLock = layers[j].alphaLock;
        layers[j].alphaLock = entry->layers[j].alphaLock;
        entry->layers[j].alphaLock = tempAlphaLock;

        bool tempClipping = layers[j].clipping;
        layers[j].clipping = entry->layers[j].clipping;
        entry->layers[j].clipping = tempClipping;

        char tempName[32];
        memcpy(tempName, layers[j].name, sizeof(tempName));
        memcpy(layers[j].name, entry->layers[j].name, sizeof(layers[j].name));
        memcpy(entry->layers[j].name, tempName, sizeof(entry->layers[j].name));
        layers[j].name[sizeof(layers[j].name) - 1] = '\0';
        entry->layers[j].name[sizeof(entry->layers[j].name) - 1] = '\0';
    }

    free(tempBuffer);
    historyIndex--;
    canvasNeedsUpdate = true;  // Canvas changed
}

//---------------------------------------------------------------------------------
// Redo last undone operation
//---------------------------------------------------------------------------------
static void redo(void) {
    if (!historyInitialized) return;
    if (historyIndex >= historyCount - 1) return;
    
    historyIndex++;
    
    HistoryEntry* entry = &historyStack[historyIndex];
    size_t bufferSize = TEX_WIDTH * TEX_HEIGHT * sizeof(u32);
    u32* tempBuffer = (u32*)malloc(bufferSize);
    if (!tempBuffer) {
        historyIndex--;
        return;
    }

    int tempLayerIndex = currentLayerIndex;
    currentLayerIndex = entry->currentLayerIndex;
    entry->currentLayerIndex = tempLayerIndex;

    for (int j = 0; j < MAX_LAYERS; j++) {
        if (layers[j].buffer && entry->layers[j].buffer) {
            memcpy(tempBuffer, layers[j].buffer, bufferSize);
            memcpy(layers[j].buffer, entry->layers[j].buffer, bufferSize);
            memcpy(entry->layers[j].buffer, tempBuffer, bufferSize);
        }

        bool tempVisible = layers[j].visible;
        layers[j].visible = entry->layers[j].visible;
        entry->layers[j].visible = tempVisible;

        u8 tempOpacity = layers[j].opacity;
        layers[j].opacity = entry->layers[j].opacity;
        entry->layers[j].opacity = tempOpacity;

        BlendMode tempBlend = layers[j].blendMode;
        layers[j].blendMode = entry->layers[j].blendMode;
        entry->layers[j].blendMode = tempBlend;

        bool tempAlphaLock = layers[j].alphaLock;
        layers[j].alphaLock = entry->layers[j].alphaLock;
        entry->layers[j].alphaLock = tempAlphaLock;

        bool tempClipping = layers[j].clipping;
        layers[j].clipping = entry->layers[j].clipping;
        entry->layers[j].clipping = tempClipping;

        char tempName[32];
        memcpy(tempName, layers[j].name, sizeof(tempName));
        memcpy(layers[j].name, entry->layers[j].name, sizeof(layers[j].name));
        memcpy(entry->layers[j].name, tempName, sizeof(entry->layers[j].name));
        layers[j].name[sizeof(layers[j].name) - 1] = '\0';
        entry->layers[j].name[sizeof(entry->layers[j].name) - 1] = '\0';
    }

    free(tempBuffer);
    canvasNeedsUpdate = true;  // Canvas changed
}

//---------------------------------------------------------------------------------
// Clear a specific layer
//---------------------------------------------------------------------------------
static void clearLayer(int layerIndex, u32 color) {
    projectHasUnsavedChanges = true;  // Mark project as having unsaved changes
    if (layerIndex < 0 || layerIndex >= MAX_LAYERS) return;
    if (!layers[layerIndex].buffer) return;
    
    for (int y = 0; y < TEX_HEIGHT; y++) {
        for (int x = 0; x < TEX_WIDTH; x++) {
            layers[layerIndex].buffer[y * TEX_WIDTH + x] = color;
        }
    }
}

//---------------------------------------------------------------------------------
// Flood fill algorithm (uses composite canvas for boundary detection)
//---------------------------------------------------------------------------------
typedef struct {
    int x, y;
} Point;

static void floodFill(int layerIndex, int startX, int startY, u32 fillColor, int expand) {
    projectHasUnsavedChanges = true;
    if (layerIndex < 0 || layerIndex >= MAX_LAYERS) return;
    if (!layers[layerIndex].buffer || !compositeBuffer) return;
    if (startX < 0 || startX >= CANVAS_WIDTH || startY < 0 || startY >= CANVAS_HEIGHT) return;

    // Get target color from composite canvas (all layers)
    int startIdx = startY * TEX_WIDTH + startX;
    u32 targetColor = compositeBuffer[startIdx];
    
    // If target color is same as fill color, nothing to do
    if (targetColor == fillColor) return;

    // Allocate stack for flood fill (max canvas pixels)
    const int maxStackSize = CANVAS_WIDTH * CANVAS_HEIGHT;
    Point* stack = (Point*)malloc(maxStackSize * sizeof(Point));
    if (!stack) return;
    
    // Track filled pixels (for expansion pass)
    bool* filled = (bool*)calloc(CANVAS_WIDTH * CANVAS_HEIGHT, sizeof(bool));
    if (!filled) {
        free(stack);
        return;
    }

    // Track visited pixels to avoid revisiting
    bool* visited = (bool*)calloc(CANVAS_WIDTH * CANVAS_HEIGHT, sizeof(bool));
    if (!visited) {
        free(filled);
        free(stack);
        return;
    }

    int stackSize = 0;
    stack[stackSize++] = (Point){startX, startY};
    visited[startY * CANVAS_WIDTH + startX] = true;

    while (stackSize > 0) {
        Point p = stack[--stackSize];
        int x = p.x;
        int y = p.y;

        // Draw to active layer
        drawPixelToLayer(layerIndex, x, y, fillColor);
        filled[y * CANVAS_WIDTH + x] = true;

        // Check 4 neighbors
        const int dx[] = {0, 0, -1, 1};
        const int dy[] = {-1, 1, 0, 0};
        
        for (int i = 0; i < 4; i++) {
            int nx = x + dx[i];
            int ny = y + dy[i];
            
            if (nx < 0 || nx >= CANVAS_WIDTH || ny < 0 || ny >= CANVAS_HEIGHT) continue;
            
            int visitIdx = ny * CANVAS_WIDTH + nx;
            if (visited[visitIdx]) continue;
            
            int nIdx = ny * TEX_WIDTH + nx;
            if (compositeBuffer[nIdx] == targetColor) {
                if (stackSize < maxStackSize) {
                    stack[stackSize++] = (Point){nx, ny};
                    visited[visitIdx] = true;
                }
            }
        }
    }

    // Expansion pass: expand filled region by 'expand' pixels
    if (expand > 0) {
        bool* expanded = (bool*)calloc(CANVAS_WIDTH * CANVAS_HEIGHT, sizeof(bool));
        if (expanded) {
            memcpy(expanded, filled, CANVAS_WIDTH * CANVAS_HEIGHT * sizeof(bool));
            for (int pass = 0; pass < expand; pass++) {
                bool* nextExpanded = (bool*)calloc(CANVAS_WIDTH * CANVAS_HEIGHT, sizeof(bool));
                if (!nextExpanded) break;
                memcpy(nextExpanded, expanded, CANVAS_WIDTH * CANVAS_HEIGHT * sizeof(bool));
                for (int y = 0; y < CANVAS_HEIGHT; y++) {
                    for (int x = 0; x < CANVAS_WIDTH; x++) {
                        if (!expanded[y * CANVAS_WIDTH + x]) continue;
                        // Expand to 4 neighbors
                        const int edx[] = {0, 0, -1, 1};
                        const int edy[] = {-1, 1, 0, 0};
                        for (int d = 0; d < 4; d++) {
                            int nx = x + edx[d];
                            int ny = y + edy[d];
                            if (nx < 0 || nx >= CANVAS_WIDTH || ny < 0 || ny >= CANVAS_HEIGHT) continue;
                            int ni = ny * CANVAS_WIDTH + nx;
                            if (!nextExpanded[ni]) {
                                nextExpanded[ni] = true;
                                drawPixelToLayer(layerIndex, nx, ny, fillColor);
                            }
                        }
                    }
                }
                free(expanded);
                expanded = nextExpanded;
            }
            free(expanded);
        }
    }

    free(visited);
    free(filled);
    free(stack);
}

//---------------------------------------------------------------------------------
// Draw a pixel to a specific layer with alpha blending
//---------------------------------------------------------------------------------
static void drawPixelToLayer(int layerIndex, int x, int y, u32 color) {
    if (layerIndex < 0 || layerIndex >= MAX_LAYERS) return;
    if (!layers[layerIndex].buffer) return;
    if (x < 0 || x >= CANVAS_WIDTH || y < 0 || y >= CANVAS_HEIGHT) return;

    int idx = y * TEX_WIDTH + x;
    if (layers[layerIndex].alphaLock) {
        u32 dst = layers[layerIndex].buffer[idx];
        u32 srcR = (color >> 24) & 0xFF;
        u32 srcG = (color >> 16) & 0xFF;
        u32 srcB = (color >> 8) & 0xFF;
        u32 srcA = color & 0xFF;
        if (srcA == 0) return;

        u32 dstR = (dst >> 24) & 0xFF;
        u32 dstG = (dst >> 16) & 0xFF;
        u32 dstB = (dst >> 8) & 0xFF;
        u32 dstA = dst & 0xFF;

        u32 outR, outG, outB;
        if (srcA < 255) {
            outR = dstR + (srcR * srcA) / 255;
            outG = dstG + (srcG * srcA) / 255;
            outB = dstB + (srcB * srcA) / 255;
            if (outR > 255) outR = 255;
            if (outG > 255) outG = 255;
            if (outB > 255) outB = 255;
        } else {
            outR = srcR;
            outG = srcG;
            outB = srcB;
        }

        layers[layerIndex].buffer[idx] = (outR << 24) | (outG << 16) | (outB << 8) | dstA;
        return;
    }

    u8 srcA = color & 0xFF;
    if (srcA < 255) {
        u32 dst = layers[layerIndex].buffer[idx];
        u32 srcR = (color >> 24) & 0xFF;
        u32 srcG = (color >> 16) & 0xFF;
        u32 srcB = (color >> 8) & 0xFF;
        u32 dstR = (dst >> 24) & 0xFF;
        u32 dstG = (dst >> 16) & 0xFF;
        u32 dstB = (dst >> 8) & 0xFF;
        u32 dstA = dst & 0xFF;

        u32 outR = dstR + (srcR * srcA) / 255;
        u32 outG = dstG + (srcG * srcA) / 255;
        u32 outB = dstB + (srcB * srcA) / 255;
        if (outR > 255) outR = 255;
        if (outG > 255) outG = 255;
        if (outB > 255) outB = 255;
        u32 outA = dstA + srcA;
        if (outA > 255) outA = 255;

        layers[layerIndex].buffer[idx] = (outR << 24) | (outG << 16) | (outB << 8) | outA;
    } else {
        layers[layerIndex].buffer[idx] = color;
    }
}

//---------------------------------------------------------------------------------
// Draw a pixel with alpha blending (for anti-aliasing)
//---------------------------------------------------------------------------------
static void drawPixelBlended(int layerIndex, int x, int y, u32 color, u8 alpha) {
    if (layerIndex < 0 || layerIndex >= MAX_LAYERS) return;
    if (!layers[layerIndex].buffer) return;
    if (x < 0 || x >= CANVAS_WIDTH || y < 0 || y >= CANVAS_HEIGHT) return;
    if (alpha == 0) return;
    
    int idx = y * TEX_WIDTH + x;
    u32 dst = layers[layerIndex].buffer[idx];
    
    // Extract source color components (use u32 to avoid overflow)
    u32 srcR = (color >> 24) & 0xFF;
    u32 srcG = (color >> 16) & 0xFF;
    u32 srcB = (color >> 8) & 0xFF;
    u32 srcA = color & 0xFF;
    
    // Apply brush alpha to source alpha
    srcA = (srcA * alpha) / 255;
    if (srcA == 0) return;
    
    // Extract destination color components
    u32 dstR = (dst >> 24) & 0xFF;
    u32 dstG = (dst >> 16) & 0xFF;
    u32 dstB = (dst >> 8) & 0xFF;
    u32 dstA = dst & 0xFF;
    
    // Alpha lock: preserve destination alpha, update RGB only
    if (layers[layerIndex].alphaLock) {
        u32 outR, outG, outB;
        if (srcA < 255) {
            outR = dstR + (srcR * srcA) / 255;
            outG = dstG + (srcG * srcA) / 255;
            outB = dstB + (srcB * srcA) / 255;
            if (outR > 255) outR = 255;
            if (outG > 255) outG = 255;
            if (outB > 255) outB = 255;
        } else {
            outR = srcR;
            outG = srcG;
            outB = srcB;
        }
        layers[layerIndex].buffer[idx] = (outR << 24) | (outG << 16) | (outB << 8) | dstA;
        return;
    }

    // Semi-transparent strokes accumulate by additive RGB
    if (srcA < 255) {
        u32 outR = dstR + (srcR * srcA) / 255;
        u32 outG = dstG + (srcG * srcA) / 255;
        u32 outB = dstB + (srcB * srcA) / 255;
        if (outR > 255) outR = 255;
        if (outG > 255) outG = 255;
        if (outB > 255) outB = 255;
        u32 outA = dstA + srcA;
        if (outA > 255) outA = 255;
        layers[layerIndex].buffer[idx] = (outR << 24) | (outG << 16) | (outB << 8) | outA;
        return;
    }

    // Alpha blend with proper handling of destination alpha
    u32 outR, outG, outB, outA;
    if (srcA == 255) {
        outR = srcR;
        outG = srcG;
        outB = srcB;
        outA = 255;
    } else if (dstA == 0) {
        // Destination is fully transparent - just use source
        outR = srcR;
        outG = srcG;
        outB = srcB;
        outA = srcA;
    } else {
        // Proper alpha blending using Porter-Duff "over" operator
        // out_alpha = src_alpha + dst_alpha * (1 - src_alpha)
        u32 invSrcA = 255 - srcA;
        outA = srcA + (dstA * invSrcA) / 255;
        
        if (outA > 0) {
            // out_color = (src_color * src_alpha + dst_color * dst_alpha * (1 - src_alpha)) / out_alpha
            u32 srcContrib = srcA * 255;  // Scale up for precision
            u32 dstContrib = (dstA * invSrcA) / 255 * 255;
            u32 totalContrib = srcContrib + dstContrib;
            
            if (totalContrib > 0) {
                outR = (srcR * srcContrib + dstR * dstContrib) / totalContrib;
                outG = (srcG * srcContrib + dstG * dstContrib) / totalContrib;
                outB = (srcB * srcContrib + dstB * dstContrib) / totalContrib;
            } else {
                outR = srcR;
                outG = srcG;
                outB = srcB;
            }
        } else {
            outR = outG = outB = 0;
        }
    }
    
    layers[layerIndex].buffer[idx] = (outR << 24) | (outG << 16) | (outB << 8) | outA;
}

//---------------------------------------------------------------------------------
// Draw Antialias Pen - smooth edges
//---------------------------------------------------------------------------------
static void drawBrushAntialias(int layerIndex, int x, int y, int size, u32 color) {
    float radius = (float)size;
    float radiusSq = radius * radius;
    float aaWidth = 1.5f;
    float innerRadiusSq = (radius - aaWidth) * (radius - aaWidth);
    if (innerRadiusSq < 0) innerRadiusSq = 0;
    
    int iRadius = size + 1;
    
    for (int dy = -iRadius; dy <= iRadius; dy++) {
        for (int dx = -iRadius; dx <= iRadius; dx++) {
            float distSq = (float)(dx * dx + dy * dy);
            
            if (distSq <= innerRadiusSq) {
                drawPixelToLayer(layerIndex, x + dx, y + dy, color);
            } else if (distSq <= radiusSq) {
                float dist = sqrtf(distSq);
                float alpha = (radius - dist) / aaWidth;
                if (alpha > 1.0f) alpha = 1.0f;
                if (alpha < 0.0f) alpha = 0.0f;
                u8 pixelAlpha = (u8)(alpha * 255.0f);
                drawPixelBlended(layerIndex, x + dx, y + dy, color, pixelAlpha);
            }
        }
    }
}

//---------------------------------------------------------------------------------
// Draw Pixel Pen - hard edges, no anti-aliasing
//---------------------------------------------------------------------------------
static void drawBrushPixel(int layerIndex, int x, int y, int size, u32 color) {
    int radius = size;
    int radiusSq = radius * radius;
    
    for (int dy = -radius; dy <= radius; dy++) {
        for (int dx = -radius; dx <= radius; dx++) {
            int distSq = dx * dx + dy * dy;
            if (distSq <= radiusSq) {
                drawPixelToLayer(layerIndex, x + dx, y + dy, color);
            }
        }
    }
}

//---------------------------------------------------------------------------------
// Draw Airbrush - soft spray effect with transparency falloff
//---------------------------------------------------------------------------------
static void drawBrushAirbrush(int layerIndex, int x, int y, int size, u32 color) {
    float radius = (float)size * 1.5f;  // Airbrush has larger soft area
    float radiusSq = radius * radius;
    
    int iRadius = (int)radius + 1;
    
    for (int dy = -iRadius; dy <= iRadius; dy++) {
        for (int dx = -iRadius; dx <= iRadius; dx++) {
            float distSq = (float)(dx * dx + dy * dy);
            
            if (distSq <= radiusSq) {
                float dist = sqrtf(distSq);
                // Gaussian-like falloff for soft airbrush effect
                float t = dist / radius;
                float alpha = (1.0f - t * t) * 0.3f;  // Soft, low opacity
                if (alpha > 0.0f) {
                    u8 pixelAlpha = (u8)(alpha * 255.0f);
                    drawPixelBlended(layerIndex, x + dx, y + dy, color, pixelAlpha);
                }
            }
        }
    }
}

//---------------------------------------------------------------------------------
// G-Pen state for pressure simulation
//---------------------------------------------------------------------------------
static float gpenPressure = 0.0f;  // Simulated pressure (0 to 1)
static bool gpenStrokeStarted = false;
static int gpenStrokeLength = 0;  // Track stroke length for taper

// G-Pen stroke history for taper-out effect
#define GPEN_HISTORY_SIZE 20
typedef struct {
    int x, y;
    int size;
    u32 color;
    int layerIndex;
} GpenPoint;
static GpenPoint gpenHistory[GPEN_HISTORY_SIZE];
static int gpenHistoryCount = 0;
static int gpenHistoryIndex = 0;  // Circular buffer index

//---------------------------------------------------------------------------------
// Record a point in G-Pen history for taper-out
//---------------------------------------------------------------------------------
static void recordGpenPoint(int layerIndex, int x, int y, int size, u32 color) {
    gpenHistory[gpenHistoryIndex].x = x;
    gpenHistory[gpenHistoryIndex].y = y;
    gpenHistory[gpenHistoryIndex].size = size;
    gpenHistory[gpenHistoryIndex].color = color;
    gpenHistory[gpenHistoryIndex].layerIndex = layerIndex;
    
    gpenHistoryIndex = (gpenHistoryIndex + 1) % GPEN_HISTORY_SIZE;
    if (gpenHistoryCount < GPEN_HISTORY_SIZE) {
        gpenHistoryCount++;
    }
}

//---------------------------------------------------------------------------------
// Draw G-Pen - tapered line with simulated pressure
//---------------------------------------------------------------------------------
static void drawBrushGPen(int layerIndex, int x, int y, int size, u32 color, float pressure) {
    // Pressure affects size: minimum 30% of size at pressure 0
    float effectiveRadius = size * (0.3f + 0.7f * pressure);
    float radiusSq = effectiveRadius * effectiveRadius;
    float aaWidth = 1.0f;
    float innerRadiusSq = (effectiveRadius - aaWidth) * (effectiveRadius - aaWidth);
    if (innerRadiusSq < 0) innerRadiusSq = 0;
    
    int iRadius = (int)effectiveRadius + 1;
    
    for (int dy = -iRadius; dy <= iRadius; dy++) {
        for (int dx = -iRadius; dx <= iRadius; dx++) {
            float distSq = (float)(dx * dx + dy * dy);
            
            if (distSq <= innerRadiusSq) {
                drawPixelToLayer(layerIndex, x + dx, y + dy, color);
            } else if (distSq <= radiusSq) {
                float dist = sqrtf(distSq);
                float alpha = (effectiveRadius - dist) / aaWidth;
                if (alpha > 1.0f) alpha = 1.0f;
                if (alpha < 0.0f) alpha = 0.0f;
                u8 pixelAlpha = (u8)(alpha * 255.0f);
                drawPixelBlended(layerIndex, x + dx, y + dy, color, pixelAlpha);
            }
        }
    }
}

//---------------------------------------------------------------------------------
// Draw a brush stroke to a specific layer (dispatches based on brush type)
//---------------------------------------------------------------------------------
static void drawBrushToLayer(int layerIndex, int x, int y, int size, u32 color) {
    projectHasUnsavedChanges = true;  // Mark project as having unsaved changes
    BrushType brushType = brushDefs[currentBrushType].type;
    
    switch (brushType) {
        case BRUSH_ANTIALIAS:
            drawBrushAntialias(layerIndex, x, y, size, color);
            break;
        case BRUSH_PIXEL:
            drawBrushPixel(layerIndex, x, y, size, color);
            break;
        case BRUSH_AIRBRUSH:
            drawBrushAirbrush(layerIndex, x, y, size, color);
            break;
        case BRUSH_GPEN:
            drawBrushGPen(layerIndex, x, y, size, color, gpenPressure);
            // Record point for taper-out effect
            recordGpenPoint(layerIndex, x, y, size, color);
            break;
        default:
            drawBrushAntialias(layerIndex, x, y, size, color);
            break;
    }
}

//---------------------------------------------------------------------------------
// Draw a line to a specific layer (with G-Pen pressure interpolation)
//---------------------------------------------------------------------------------
static void drawLineToLayer(int layerIndex, int x0, int y0, int x1, int y1, int size, u32 color) {
    projectHasUnsavedChanges = true;  // Mark project as having unsaved changes
    int dx = abs(x1 - x0);
    int dy = abs(y1 - y0);
    int sx = (x0 < x1) ? 1 : -1;
    int sy = (y0 < y1) ? 1 : -1;
    int err = dx - dy;
    
    // For G-Pen: get brush type
    BrushType brushType = brushDefs[currentBrushType].type;
    
    while (1) {
        // Update G-Pen pressure based on stroke progress
        if (brushType == BRUSH_GPEN) {
            // Smooth pressure buildup at start, maintain in middle
            float strokeProgress = (float)gpenStrokeLength / 30.0f;  // Ramp up over ~30 pixels
            if (strokeProgress > 1.0f) strokeProgress = 1.0f;
            gpenPressure = strokeProgress;
            gpenStrokeLength++;
        }
        
        drawBrushToLayer(layerIndex, x0, y0, size, color);
        
        if (x0 == x1 && y0 == y1) break;
        
        int e2 = 2 * err;
        if (e2 > -dy) {
            err -= dy;
            x0 += sx;
        }
        if (e2 < dx) {
            err += dx;
            y0 += sy;
        }
    }
}

//---------------------------------------------------------------------------------
// Start a new stroke (reset G-Pen state)
//---------------------------------------------------------------------------------
static void startStroke(void) {
    gpenPressure = 0.0f;
    gpenStrokeStarted = true;
    gpenStrokeLength = 0;
    gpenHistoryCount = 0;
    gpenHistoryIndex = 0;
}

//---------------------------------------------------------------------------------
// Apply G-Pen taper-out effect at end of stroke
//---------------------------------------------------------------------------------
static void applyGpenTaperOut(void) {
    if (gpenHistoryCount < 2) return;
    
    BrushType brushType = brushDefs[currentBrushType].type;
    if (brushType != BRUSH_GPEN) return;
    
    // Get the last two recorded points to determine direction
    int startIdx;
    if (gpenHistoryCount < GPEN_HISTORY_SIZE) {
        startIdx = 0;
    } else {
        startIdx = gpenHistoryIndex;
    }
    
    // Get the last point and second-to-last point
    int lastIdx = (startIdx + gpenHistoryCount - 1) % GPEN_HISTORY_SIZE;
    int prevIdx = (startIdx + gpenHistoryCount - 2) % GPEN_HISTORY_SIZE;
    
    GpenPoint* lastPt = &gpenHistory[lastIdx];
    GpenPoint* prevPt = &gpenHistory[prevIdx];
    
    // Calculate direction vector
    float dx = (float)(lastPt->x - prevPt->x);
    float dy = (float)(lastPt->y - prevPt->y);
    float len = sqrtf(dx * dx + dy * dy);
    
    if (len < 0.1f) return;  // No movement, skip taper
    
    // Normalize direction
    dx /= len;
    dy /= len;
    
    // Draw taper-out extension (extending beyond the last point)
    int taperLength = lastPt->size * 3;  // Taper length based on brush size
    int steps = taperLength;
    
    for (int i = 1; i <= steps; i++) {
        float t = (float)i / (float)steps;
        float pressure = (1.0f - t);
        pressure = pressure * pressure;  // Ease out
        
        if (pressure < 0.05f) break;  // Stop when too thin
        
        int x = lastPt->x + (int)(dx * i);
        int y = lastPt->y + (int)(dy * i);
        
        drawBrushGPen(lastPt->layerIndex, x, y, lastPt->size, lastPt->color, pressure);
    }
}

//---------------------------------------------------------------------------------
// End a stroke (apply taper-out for G-Pen)
//---------------------------------------------------------------------------------
static void endStroke(void) {
    // Apply G-Pen taper-out effect
    applyGpenTaperOut();
    // Mark canvas dirty so taper is shown immediately
    canvasNeedsUpdate = true;
    forceUpdateCanvasTexture();

    gpenStrokeStarted = false;
    gpenHistoryCount = 0;
}

//---------------------------------------------------------------------------------
// Composite all visible layers into the composite buffer
//---------------------------------------------------------------------------------
static void compositeAllLayers(void) {
    if (!compositeBuffer) return;
    
    // Count visible layers first
    int firstVisibleLayer = -1;
    int visibleCount = 0;
    for (int i = 0; i < numLayers; i++) {
        if (layers[i].visible && layers[i].buffer && layers[i].opacity > 0) {
            if (firstVisibleLayer < 0) firstVisibleLayer = i;
            visibleCount++;
        }
    }
    
    // Start with white background (simulates paper)
    for (int y = 0; y < CANVAS_HEIGHT; y++) {
        int rowStart = y * TEX_WIDTH;
        for (int x = 0; x < CANVAS_WIDTH; x++) {
            compositeBuffer[rowStart + x] = 0xFFFFFFFF;  // White with full alpha
        }
    }
    
    // If no visible layers, just keep white background
    if (visibleCount == 0) {
        return;
    }
    
    // Composite layers from bottom to top onto white background
    for (int i = 0; i < numLayers; i++) {
        if (!layers[i].visible || !layers[i].buffer || layers[i].opacity == 0) continue;
        
        u8 layerOpacity = layers[i].opacity;
        BlendMode blendMode = layers[i].blendMode;
        u32* layerBuf = layers[i].buffer;
        
        // Clipping: use alpha of the layer below as a mask
        bool isClipped = layers[i].clipping && i > 0 && layers[i - 1].buffer;
        u32* clipBuf = isClipped ? layers[i - 1].buffer : NULL;
        
        // Optimized inner loop
        for (int y = 0; y < CANVAS_HEIGHT; y++) {
            int rowStart = y * TEX_WIDTH;
            for (int x = 0; x < CANVAS_WIDTH; x++) {
                int idx = rowStart + x;
                u32 src = layerBuf[idx];
                
                // Skip fully transparent source pixels
                u8 srcA = src & 0xFF;
                if (srcA == 0) continue;
                
                // Clipping: mask by the layer below's alpha
                if (clipBuf) {
                    u8 clipA = clipBuf[idx] & 0xFF;
                    if (clipA == 0) continue;
                    // Scale source alpha by clip alpha
                    srcA = (srcA * clipA) / 255;
                    if (srcA == 0) continue;
                    src = (src & 0xFFFFFF00) | srcA;
                }
                
                u32 dst = compositeBuffer[idx];
                compositeBuffer[idx] = blendPixel(dst, src, blendMode, layerOpacity);
            }
        }
    }
}

//---------------------------------------------------------------------------------
// Upload composite buffer to GPU texture (only when needed)
//---------------------------------------------------------------------------------
static void updateCanvasTexture(void) {
    if (!compositeBuffer || !canvasNeedsUpdate) return;
    
    // First composite all layers
    compositeAllLayers();
    
    // Flush data cache and copy to texture
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

// Force canvas texture update (bypasses dirty flag)
static void forceUpdateCanvasTexture(void) {
    canvasNeedsUpdate = true;
    updateCanvasTexture();
}

//---------------------------------------------------------------------------------
// Render UI elements on top screen
//---------------------------------------------------------------------------------
static void renderUI(C3D_RenderTarget* target) {
    // Background
    C2D_TargetClear(target, C2D_Color32(0x30, 0x30, 0x30, 0xFF));
    C2D_SceneBegin(target);
    
    // Draw canvas preview (scaled to fit entire top screen)
    float scaleX = (float)TOP_SCREEN_WIDTH / CANVAS_WIDTH;
    float scaleY = (float)TOP_SCREEN_HEIGHT / CANVAS_HEIGHT;
    float scale = (scaleX < scaleY) ? scaleX : scaleY;
    
    float previewWidth = CANVAS_WIDTH * scale;
    float previewHeight = CANVAS_HEIGHT * scale;
    float previewX = (TOP_SCREEN_WIDTH - previewWidth) / 2;
    float previewY = (TOP_SCREEN_HEIGHT - previewHeight) / 2;
    
    // Draw canvas preview
    C2D_DrawImageAt(canvasImage, previewX, previewY, 0, NULL, scale, scale);
    
    // Draw blue rectangle showing the current viewport on bottom screen
    // Calculate the visible area in canvas coordinates
    float drawX = canvasPanX + (BOTTOM_SCREEN_WIDTH - CANVAS_WIDTH * canvasZoom) / 2;
    float drawY = canvasPanY + (BOTTOM_SCREEN_HEIGHT - CANVAS_HEIGHT * canvasZoom) / 2;
    
    // Visible area in canvas coordinates
    float viewLeft = -drawX / canvasZoom;
    float viewTop = -drawY / canvasZoom;
    float viewWidth = BOTTOM_SCREEN_WIDTH / canvasZoom;
    float viewHeight = BOTTOM_SCREEN_HEIGHT / canvasZoom;
    
    // Clamp to canvas bounds
    if (viewLeft < 0) viewLeft = 0;
    if (viewTop < 0) viewTop = 0;
    if (viewLeft + viewWidth > CANVAS_WIDTH) viewWidth = CANVAS_WIDTH - viewLeft;
    if (viewTop + viewHeight > CANVAS_HEIGHT) viewHeight = CANVAS_HEIGHT - viewTop;
    
    // Convert to preview coordinates
    float rectX = previewX + viewLeft * scale;
    float rectY = previewY + viewTop * scale;
    float rectW = viewWidth * scale;
    float rectH = viewHeight * scale;
    
    // Draw blue border (4 lines forming a rectangle)
    u32 borderColor = C2D_Color32(0x00, 0x80, 0xFF, 0xFF);
    float borderThickness = 2.0f;
    
    // Top line
    C2D_DrawRectSolid(rectX, rectY, 0, rectW, borderThickness, borderColor);
    // Bottom line
    C2D_DrawRectSolid(rectX, rectY + rectH - borderThickness, 0, rectW, borderThickness, borderColor);
    // Left line
    C2D_DrawRectSolid(rectX, rectY, 0, borderThickness, rectH, borderColor);
    // Right line
    C2D_DrawRectSolid(rectX + rectW - borderThickness, rectY, 0, borderThickness, rectH, borderColor);
    
    // === Brush size preview (when adjusting slider) ===
    if (brushSizeSliderActive) {
        // Draw circle at center of canvas preview showing brush size
        float previewCenterX = previewX + previewWidth / 2;
        float previewCenterY = previewY + previewHeight / 2;
        float previewRadius = getCurrentBrushSize() * scale;  // Scale brush size to preview
        
        // Draw white outline
        C2D_DrawCircleSolid(previewCenterX, previewCenterY, 0, previewRadius + 2, C2D_Color32(0xFF, 0xFF, 0xFF, 0xFF));
        // Draw brush color circle
        u8 curR = (currentColor >> 24) & 0xFF;
        u8 curG = (currentColor >> 16) & 0xFF;
        u8 curB = (currentColor >> 8) & 0xFF;
        C2D_DrawCircleSolid(previewCenterX, previewCenterY, 0, previewRadius, C2D_Color32(curR, curG, curB, brushAlpha));
    }
    
    // === Information display ===
    float textScale = 0.4f;
    float lineHeight = 12.0f;
    u32 textColor = C2D_Color32(0xAA, 0xAA, 0xAA, 0xFF);  // Gray for all text
    char textBuf[64];
    C2D_Text text;
    float textWidth, textHeight;
    float rightMargin = 4.0f;
    
    // --- Right top: System & Canvas info (right-aligned) ---
    float infoY = 4;
    
    // Canvas size
    C2D_TextBufClear(g_textBuf);
    snprintf(textBuf, sizeof(textBuf), "Canvas: %dx%d", CANVAS_WIDTH, CANVAS_HEIGHT);
    C2D_TextParse(&text, g_textBuf, textBuf);
    C2D_TextOptimize(&text);
    C2D_TextGetDimensions(&text, textScale, textScale, &textWidth, &textHeight);
    C2D_DrawText(&text, C2D_WithColor, TOP_SCREEN_WIDTH - textWidth - rightMargin, infoY, 0, textScale, textScale, textColor);
    infoY += lineHeight;
    
    // FPS
    C2D_TextBufClear(g_textBuf);
    snprintf(textBuf, sizeof(textBuf), "FPS: %.1f", currentFPS);
    C2D_TextParse(&text, g_textBuf, textBuf);
    C2D_TextOptimize(&text);
    C2D_TextGetDimensions(&text, textScale, textScale, &textWidth, &textHeight);
    C2D_DrawText(&text, C2D_WithColor, TOP_SCREEN_WIDTH - textWidth - rightMargin, infoY, 0, textScale, textScale, textColor);
    infoY += lineHeight;
    
    // Memory usage (used/total)
    u32 memUsed = osGetMemRegionUsed(MEMREGION_ALL);
    u32 memTotal = osGetMemRegionSize(MEMREGION_ALL);
    C2D_TextBufClear(g_textBuf);
    snprintf(textBuf, sizeof(textBuf), "Mem: %.1f/%.1fMB", memUsed / (1024.0f * 1024.0f), memTotal / (1024.0f * 1024.0f));
    C2D_TextParse(&text, g_textBuf, textBuf);
    C2D_TextOptimize(&text);
    C2D_TextGetDimensions(&text, textScale, textScale, &textWidth, &textHeight);
    C2D_DrawText(&text, C2D_WithColor, TOP_SCREEN_WIDTH - textWidth - rightMargin, infoY, 0, textScale, textScale, textColor);
    infoY += lineHeight;
    
    // Zoom level
    C2D_TextBufClear(g_textBuf);
    snprintf(textBuf, sizeof(textBuf), "Zoom: x%.1f", canvasZoom);
    C2D_TextParse(&text, g_textBuf, textBuf);
    C2D_TextOptimize(&text);
    C2D_TextGetDimensions(&text, textScale, textScale, &textWidth, &textHeight);
    C2D_DrawText(&text, C2D_WithColor, TOP_SCREEN_WIDTH - textWidth - rightMargin, infoY, 0, textScale, textScale, textColor);
    
    // --- Right bottom: Tool info (right-aligned) ---
    float toolY = TOP_SCREEN_HEIGHT - 26;
    
    // Current tool
    const char* toolName;
    switch (currentTool) {
        case TOOL_ERASER: toolName = "Eraser"; break;
        case TOOL_FILL: toolName = "Fill"; break;
        default: toolName = "Brush"; break;
    }
    C2D_TextBufClear(g_textBuf);
    snprintf(textBuf, sizeof(textBuf), "Tool: %s", toolName);
    C2D_TextParse(&text, g_textBuf, textBuf);
    C2D_TextOptimize(&text);
    C2D_TextGetDimensions(&text, textScale, textScale, &textWidth, &textHeight);
    C2D_DrawText(&text, C2D_WithColor, TOP_SCREEN_WIDTH - textWidth - rightMargin, toolY, 0, textScale, textScale, textColor);
    toolY += lineHeight;
    
    // Brush size + Color preview (on same conceptual line, right-aligned)
    C2D_TextBufClear(g_textBuf);
    snprintf(textBuf, sizeof(textBuf), "Size: %d", getCurrentBrushSize());
    C2D_TextParse(&text, g_textBuf, textBuf);
    C2D_TextOptimize(&text);
    C2D_TextGetDimensions(&text, textScale, textScale, &textWidth, &textHeight);
    
    // Color preview square (to the right of size text)
    u8 r = (currentColor >> 24) & 0xFF;
    u8 g = (currentColor >> 16) & 0xFF;
    u8 b = (currentColor >> 8) & 0xFF;
    float colorBoxSize = 10;
    float colorBoxX = TOP_SCREEN_WIDTH - colorBoxSize - rightMargin;
    float colorBoxY = toolY + 1;
    // Border
    C2D_DrawRectSolid(colorBoxX - 1, colorBoxY - 1, 0, colorBoxSize + 2, colorBoxSize + 2, C2D_Color32(0x80, 0x80, 0x80, 0xFF));
    // Color
    C2D_DrawRectSolid(colorBoxX, colorBoxY, 0, colorBoxSize, colorBoxSize, C2D_Color32(r, g, b, 0xFF));
    
    // Size text to the left of color box
    C2D_DrawText(&text, C2D_WithColor, colorBoxX - textWidth - 8, toolY, 0, textScale, textScale, textColor);
    
    // --- Left bottom: Layer info ---
    float layerX = 4;
    float layerY = TOP_SCREEN_HEIGHT - 26;
    
    // Layer number
    C2D_TextBufClear(g_textBuf);
    snprintf(textBuf, sizeof(textBuf), "Layer: %d/%d", currentLayerIndex + 1, MAX_LAYERS);
    C2D_TextParse(&text, g_textBuf, textBuf);
    C2D_TextOptimize(&text);
    C2D_DrawText(&text, C2D_WithColor, layerX, layerY, 0, textScale, textScale, textColor);
    layerY += lineHeight;
    
    // Opacity + Blend mode on same line
    int opacityPercent = (int)(layers[currentLayerIndex].opacity * 100 / 255);
    const char* blendName;
    switch (layers[currentLayerIndex].blendMode) {
        case BLEND_ADD: blendName = "Add"; break;
        case BLEND_MULTIPLY: blendName = "Multiply"; break;
        default: blendName = "Normal"; break;
    }
    C2D_TextBufClear(g_textBuf);
    snprintf(textBuf, sizeof(textBuf), "%d%% / %s", opacityPercent, blendName);
    C2D_TextParse(&text, g_textBuf, textBuf);
    C2D_TextOptimize(&text);
    C2D_DrawText(&text, C2D_WithColor, layerX, layerY, 0, textScale, textScale, textColor);
}

//---------------------------------------------------------------------------------
// Render canvas on bottom screen
//---------------------------------------------------------------------------------
static void renderCanvas(C3D_RenderTarget* target, bool showOverlay) {
    C2D_TargetClear(target, C2D_Color32(0x80, 0x80, 0x80, 0xFF));
    C2D_SceneBegin(target);
    
    // Calculate canvas position with zoom and pan
    float drawX = canvasPanX + (BOTTOM_SCREEN_WIDTH - CANVAS_WIDTH * canvasZoom) / 2;
    float drawY = canvasPanY + (BOTTOM_SCREEN_HEIGHT - CANVAS_HEIGHT * canvasZoom) / 2;
    
    // Draw canvas with zoom
    C2D_DrawImageAt(canvasImage, drawX, drawY, 0, NULL, canvasZoom, canvasZoom);
    
    // Draw menu button (small square in bottom-left corner with margin)
    // Different color based on current tool
    u32 menuBtnColor;
    switch (currentTool) {
        case TOOL_ERASER: menuBtnColor = C2D_Color32(0x80, 0x30, 0x30, 0xFF); break;  // Red-ish for eraser
        case TOOL_FILL: menuBtnColor = C2D_Color32(0x30, 0x80, 0x30, 0xFF); break;    // Green-ish for fill
        default: menuBtnColor = C2D_Color32(0x30, 0x30, 0x80, 0xFF); break;           // Blue-ish for brush
    }
    C2D_DrawRectSolid(DRAW_MENU_BTN_X, DRAW_MENU_BTN_Y, 0, DRAW_MENU_BTN_SIZE, DRAW_MENU_BTN_SIZE, C2D_Color32(0x30, 0x30, 0x30, 0xFF));
    C2D_DrawRectSolid(DRAW_MENU_BTN_X + 2, DRAW_MENU_BTN_Y + 2, 0, DRAW_MENU_BTN_SIZE - 4, DRAW_MENU_BTN_SIZE - 4, menuBtnColor);
    
    // Draw current tool icon on menu button
    C2D_ImageTint tint;
    C2D_PlainImageTint(&tint, C2D_Color32(0xFF, 0xFF, 0xFF, 0xFF), 1.0f);
    
    C2D_Sprite* toolIcon;
    switch (currentTool) {
        case TOOL_ERASER: toolIcon = &eraserIconSprite; break;
        case TOOL_FILL: toolIcon = &bucketIconSprite; break;
        default: toolIcon = &brushIconSprite; break;
    }
    C2D_SpriteSetPos(toolIcon, DRAW_MENU_BTN_X + DRAW_MENU_BTN_SIZE / 2, DRAW_MENU_BTN_Y + DRAW_MENU_BTN_SIZE / 2);
    C2D_SpriteSetScale(toolIcon, 0.4f, 0.4f);
    C2D_DrawSpriteTinted(toolIcon, &tint);
    
    // Draw L-button overlay (zoom and undo/redo buttons)
    if (showOverlay) {
        // Semi-transparent overlay background
        C2D_DrawRectSolid(0, BOTTOM_SCREEN_HEIGHT - OVERLAY_BTN_SIZE - OVERLAY_MARGIN * 2, 0,
                          BOTTOM_SCREEN_WIDTH, OVERLAY_BTN_SIZE + OVERLAY_MARGIN * 2,
                          C2D_Color32(0x00, 0x00, 0x00, 0x80));
        
        // Left side: Zoom out, Zoom in buttons
        float zoomOutX = OVERLAY_MARGIN;
        float zoomInX = OVERLAY_MARGIN + OVERLAY_BTN_SIZE + OVERLAY_MARGIN;
        float btnY = BOTTOM_SCREEN_HEIGHT - OVERLAY_BTN_SIZE - OVERLAY_MARGIN;
        
        // Zoom out button
        ButtonConfig zoomOutBtn = {
            .x = zoomOutX,
            .y = btnY,
            .size = BTN_SIZE_MEDIUM,
            .icon = &zoomOutIconSprite,
            .label = NULL,
            .isActive = false,
            .isToggle = false,
            .isSkeleton = false,
            .useCustomColors = true,
            .bgColor = UI_COLOR_GRAY_3,
            .iconColor = UI_COLOR_WHITE,
            .labelColor = UI_COLOR_TEXT,
            .iconScale = 0.6f
        };
        drawButton(&zoomOutBtn);
        
        // Zoom in button
        ButtonConfig zoomInBtn = {
            .x = zoomInX,
            .y = btnY,
            .size = BTN_SIZE_MEDIUM,
            .icon = &zoomInIconSprite,
            .label = NULL,
            .isActive = false,
            .isToggle = false,
            .isSkeleton = false,
            .useCustomColors = true,
            .bgColor = UI_COLOR_GRAY_3,
            .iconColor = UI_COLOR_WHITE,
            .labelColor = UI_COLOR_TEXT,
            .iconScale = 0.6f
        };
        drawButton(&zoomInBtn);
        
        // Zoom level text
        C2D_TextBufClear(g_textBuf);
        char zoomStr[16];
        snprintf(zoomStr, sizeof(zoomStr), "x%.1f", canvasZoom);
        C2D_Text zoomText;
        C2D_TextParse(&zoomText, g_textBuf, zoomStr);
        C2D_TextOptimize(&zoomText);
        C2D_DrawText(&zoomText, C2D_WithColor, zoomInX + OVERLAY_BTN_SIZE + 8, btnY + OVERLAY_BTN_SIZE / 2 - 6, 0, 0.5f, 0.5f, C2D_Color32(0xFF, 0xFF, 0xFF, 0xFF));
        
        // Right side: Undo, Redo buttons
        float redoX = BOTTOM_SCREEN_WIDTH - OVERLAY_MARGIN - OVERLAY_BTN_SIZE;
        float undoX = redoX - OVERLAY_MARGIN - OVERLAY_BTN_SIZE;
        
        // Undo button
        ButtonConfig undoBtn = {
            .x = undoX,
            .y = btnY,
            .size = BTN_SIZE_MEDIUM,
            .icon = &undoIconSprite,
            .label = NULL,
            .isActive = false,
            .isToggle = false,
            .isSkeleton = false,
            .useCustomColors = true,
            .bgColor = UI_COLOR_GRAY_3,
            .iconColor = UI_COLOR_WHITE,
            .labelColor = UI_COLOR_TEXT,
            .iconScale = 0.6f
        };
        drawButton(&undoBtn);
        
        // Redo button
        ButtonConfig redoBtn = {
            .x = redoX,
            .y = btnY,
            .size = BTN_SIZE_MEDIUM,
            .icon = &redoIconSprite,
            .label = NULL,
            .isActive = false,
            .isToggle = false,
            .isSkeleton = false,
            .useCustomColors = true,
            .bgColor = UI_COLOR_GRAY_3,
            .iconColor = UI_COLOR_WHITE,
            .labelColor = UI_COLOR_TEXT,
            .iconScale = 0.6f
        };
        drawButton(&redoBtn);
    }
}

//---------------------------------------------------------------------------------
// Render color picker on bottom screen
//---------------------------------------------------------------------------------
static void renderColorPicker(C3D_RenderTarget* target) {
    C2D_TargetClear(target, C2D_Color32(0x40, 0x40, 0x40, 0xFF));
    C2D_SceneBegin(target);
    
    // Draw Saturation-Value gradient box
    int gridSize = 4;
    for (int y = 0; y < CP_SV_HEIGHT; y += gridSize) {
        for (int x = 0; x < CP_SV_WIDTH; x += gridSize) {
            float s = (float)x / CP_SV_WIDTH;
            float v = 1.0f - (float)y / CP_SV_HEIGHT;
            u32 col = hsvToRgb(currentHue, s, v);
            u8 r = (col >> 24) & 0xFF;
            u8 g = (col >> 16) & 0xFF;
            u8 b = (col >> 8) & 0xFF;
            C2D_DrawRectSolid(CP_SV_X + x, CP_SV_Y + y, 0, gridSize, gridSize, C2D_Color32(r, g, b, 0xFF));
        }
    }
    
    // Draw border around SV box
    C2D_DrawRectSolid(CP_SV_X - 2, CP_SV_Y - 2, 0, CP_SV_WIDTH + 4, 2, C2D_Color32(0xFF, 0xFF, 0xFF, 0xFF));
    C2D_DrawRectSolid(CP_SV_X - 2, CP_SV_Y + CP_SV_HEIGHT, 0, CP_SV_WIDTH + 4, 2, C2D_Color32(0xFF, 0xFF, 0xFF, 0xFF));
    C2D_DrawRectSolid(CP_SV_X - 2, CP_SV_Y, 0, 2, CP_SV_HEIGHT, C2D_Color32(0xFF, 0xFF, 0xFF, 0xFF));
    C2D_DrawRectSolid(CP_SV_X + CP_SV_WIDTH, CP_SV_Y, 0, 2, CP_SV_HEIGHT, C2D_Color32(0xFF, 0xFF, 0xFF, 0xFF));
    
    // Draw SV cursor
    int svCursorX = CP_SV_X + (int)(currentSaturation * CP_SV_WIDTH);
    int svCursorY = CP_SV_Y + (int)((1.0f - currentValue) * CP_SV_HEIGHT);
    C2D_DrawCircleSolid(svCursorX, svCursorY, 0, 6, C2D_Color32(0xFF, 0xFF, 0xFF, 0xFF));
    C2D_DrawCircleSolid(svCursorX, svCursorY, 0, 4, C2D_Color32(0x00, 0x00, 0x00, 0xFF));
    
    // Draw Hue bar
    int hueGridSize = 4;
    for (int y = 0; y < CP_HUE_HEIGHT; y += hueGridSize) {
        float h = (float)y / CP_HUE_HEIGHT * 360.0f;
        u32 col = hsvToRgb(h, 1.0f, 1.0f);
        u8 r = (col >> 24) & 0xFF;
        u8 g = (col >> 16) & 0xFF;
        u8 b = (col >> 8) & 0xFF;
        C2D_DrawRectSolid(CP_HUE_X, CP_HUE_Y + y, 0, CP_HUE_WIDTH, hueGridSize, C2D_Color32(r, g, b, 0xFF));
    }
    
    // Draw border around Hue bar
    C2D_DrawRectSolid(CP_HUE_X - 2, CP_HUE_Y - 2, 0, CP_HUE_WIDTH + 4, 2, C2D_Color32(0xFF, 0xFF, 0xFF, 0xFF));
    C2D_DrawRectSolid(CP_HUE_X - 2, CP_HUE_Y + CP_HUE_HEIGHT, 0, CP_HUE_WIDTH + 4, 2, C2D_Color32(0xFF, 0xFF, 0xFF, 0xFF));
    C2D_DrawRectSolid(CP_HUE_X - 2, CP_HUE_Y, 0, 2, CP_HUE_HEIGHT, C2D_Color32(0xFF, 0xFF, 0xFF, 0xFF));
    C2D_DrawRectSolid(CP_HUE_X + CP_HUE_WIDTH, CP_HUE_Y, 0, 2, CP_HUE_HEIGHT, C2D_Color32(0xFF, 0xFF, 0xFF, 0xFF));
    
    // Draw Hue cursor
    int hueCursorY = CP_HUE_Y + (int)(currentHue / 360.0f * CP_HUE_HEIGHT);
    C2D_DrawRectSolid(CP_HUE_X - 4, hueCursorY - 2, 0, CP_HUE_WIDTH + 8, 4, C2D_Color32(0xFF, 0xFF, 0xFF, 0xFF));
    C2D_DrawRectSolid(CP_HUE_X - 2, hueCursorY - 1, 0, CP_HUE_WIDTH + 4, 2, C2D_Color32(0x00, 0x00, 0x00, 0xFF));
    
    // Draw color preview
    u8 pr = (currentColor >> 24) & 0xFF;
    u8 pg = (currentColor >> 16) & 0xFF;
    u8 pb = (currentColor >> 8) & 0xFF;
    C2D_DrawRectSolid(CP_PREVIEW_X - 2, CP_PREVIEW_Y - 2, 0, CP_PREVIEW_SIZE + 4, CP_PREVIEW_SIZE + 4, C2D_Color32(0xFF, 0xFF, 0xFF, 0xFF));
    C2D_DrawRectSolid(CP_PREVIEW_X, CP_PREVIEW_Y, 0, CP_PREVIEW_SIZE, CP_PREVIEW_SIZE, C2D_Color32(pr, pg, pb, 0xFF));
    
    // Draw OK button
    C2D_DrawRectSolid(CP_PREVIEW_X, CP_PREVIEW_Y + CP_PREVIEW_SIZE + 20, 0, CP_PREVIEW_SIZE, 30, C2D_Color32(0x00, 0xAA, 0x00, 0xFF));
    
    // Draw instruction text area
    C2D_DrawRectSolid(10, BOTTOM_SCREEN_HEIGHT - 25, 0, BOTTOM_SCREEN_WIDTH - 20, 20, C2D_Color32(0x20, 0x20, 0x20, 0xFF));
}

//---------------------------------------------------------------------------------
// Render menu on bottom screen
//---------------------------------------------------------------------------------
static void renderMenu(C3D_RenderTarget* target) {
    C2D_TargetClear(target, C2D_Color32(0x2A, 0x2A, 0x2A, 0xFF));
    C2D_SceneBegin(target);
    
    // Draw tab bar at top
    u32 tabBgColor = C2D_Color32(0x3A, 0x3A, 0x3A, 0xFF);
    u32 tabActiveColor = C2D_Color32(0x50, 0x50, 0x50, 0xFF);
    // Draw tab backgrounds
    for (int i = 0; i < 4; i++) {
        u32 bgCol = (i == currentMenuTab) ? tabActiveColor : tabBgColor;
        C2D_DrawRectSolid(i * MENU_TAB_WIDTH, 0, 0, MENU_TAB_WIDTH, MENU_TAB_HEIGHT, bgCol);
        
        // Draw separator line
        if (i > 0) {
            C2D_DrawRectSolid(i * MENU_TAB_WIDTH, 5, 0, 1, MENU_TAB_HEIGHT - 10, C2D_Color32(0x20, 0x20, 0x20, 0xFF));
        }
    }
    
    // Draw tab icons
    float tabCenterY = MENU_TAB_HEIGHT / 2;
    
    // Tab 0: Tool (current tool icon)
    C2D_Sprite* currentToolIcon;
    switch (currentTool) {
        case TOOL_ERASER: currentToolIcon = &eraserIconSprite; break;
        case TOOL_FILL: currentToolIcon = &bucketIconSprite; break;
        default: currentToolIcon = &brushIconSprite; break;
    }
    drawTabIcon(MENU_TAB_WIDTH * 0.5f, tabCenterY, currentToolIcon, currentMenuTab == TAB_TOOL);
    
    // Tab 1: Brush settings (Wrench icon)
    drawTabIcon(MENU_TAB_WIDTH * 1.5f, tabCenterY, &wrenchIconSprite, currentMenuTab == TAB_BRUSH);
    
    // Tab 2: Color (Current color square)
    {
        float colorTabX = MENU_TAB_WIDTH * 2 + (MENU_TAB_WIDTH - 20) / 2;
        float colorTabY = (MENU_TAB_HEIGHT - 20) / 2;
        u8 cr = (currentColor >> 24) & 0xFF;
        u8 cg = (currentColor >> 16) & 0xFF;
        u8 cb = (currentColor >> 8) & 0xFF;
        drawTabColorSwatch(colorTabX, colorTabY, 20, currentMenuTab == TAB_COLOR, C2D_Color32(cr, cg, cb, 0xFF));
    }
    
    // Tab 3: Layer (Number in square)
    {
        float layerTabX = MENU_TAB_WIDTH * 3 + (MENU_TAB_WIDTH - 24) / 2;
        float layerTabY = (MENU_TAB_HEIGHT - 24) / 2;
        drawTabNumber(layerTabX, layerTabY, 24, currentMenuTab == TAB_LAYER, currentLayerIndex + 1);
    }
    
    // Draw active tab indicator line
    C2D_DrawRectSolid(currentMenuTab * MENU_TAB_WIDTH, MENU_TAB_HEIGHT - 3, 0, MENU_TAB_WIDTH, 3, UI_COLOR_ACTIVE);
    
    // Draw tab content
    if (currentMenuTab == TAB_TOOL) {
        // Tool selection content - 4x2 grid layout
        const int GRID_COLS = 4;
        const int GRID_ROWS = 2;
        const float GRID_GAP = 8;
        float gridStartX = (BOTTOM_SCREEN_WIDTH - (BTN_SIZE_LARGE * GRID_COLS + GRID_GAP * (GRID_COLS - 1))) / 2;
        float gridStartY = MENU_CONTENT_Y + 8;  // Extra padding for tool grid
        
        // Tool definitions for grid
        C2D_Sprite* toolIcons[3] = { &brushIconSprite, &eraserIconSprite, &bucketIconSprite };
        const char* toolLabels[3] = { "Brush", "Eraser", "Fill" };
        
        // Draw 2x4 grid (3 tools + 5 skeletons)
        for (int row = 0; row < GRID_ROWS; row++) {
            for (int col = 0; col < GRID_COLS; col++) {
                int idx = row * GRID_COLS + col;
                float btnX = gridStartX + col * (BTN_SIZE_LARGE + GRID_GAP);
                float btnY = gridStartY + row * (BTN_SIZE_LARGE + GRID_GAP + 8); // +8 for label space
                
                ButtonConfig btn = {
                    .x = btnX,
                    .y = btnY,
                    .size = BTN_SIZE_LARGE,
                    .icon = (idx < 3) ? toolIcons[idx] : NULL,
                    .label = (idx < 3) ? toolLabels[idx] : NULL,
                    .isActive = (idx < 3 && idx == currentTool),
                    .isToggle = false,
                    .isSkeleton = (idx >= 3)
                };
                drawButton(&btn);
            }
        }
    } else if (currentMenuTab == TAB_BRUSH) {
      if (currentTool == TOOL_FILL) {
        // === Fill Tool Settings Tab ===
        float settingsX = MENU_CONTENT_PADDING;
        float settingsY = MENU_CONTENT_Y + MENU_CONTENT_PADDING;
        float settingsWidth = BOTTOM_SCREEN_WIDTH - MENU_CONTENT_PADDING * 2;

        // Expand slider (offset by knob radius so handle doesn't overflow)
        float knobRadius = 10;
        float sliderX = settingsX + knobRadius;
        float sliderY = settingsY;
        float sliderWidth = settingsWidth - knobRadius * 2;

        // Draw Expand label and value
        C2D_TextBufClear(g_textBuf);
        C2D_Text expandLabel;
        C2D_TextParse(&expandLabel, g_textBuf, "Expand");
        C2D_TextOptimize(&expandLabel);
        C2D_DrawText(&expandLabel, C2D_WithColor, settingsX, sliderY, 0, 0.4f, 0.4f, UI_COLOR_TEXT);

        // Draw expand value right-aligned
        C2D_TextBufClear(g_textBuf);
        char expandValBuf[8];
        snprintf(expandValBuf, sizeof(expandValBuf), "%d", fillExpand);
        C2D_Text expandVal;
        C2D_TextParse(&expandVal, g_textBuf, expandValBuf);
        C2D_TextOptimize(&expandVal);
        float expandValWidth, expandValHeight;
        C2D_TextGetDimensions(&expandVal, 0.4f, 0.4f, &expandValWidth, &expandValHeight);
        C2D_DrawText(&expandVal, C2D_WithColor, settingsX + settingsWidth - expandValWidth, sliderY, 0, 0.4f, 0.4f, UI_COLOR_WHITE);

        // Draw slider
        float expandFillRatio = (float)fillExpand / FILL_EXPAND_MAX;
        SliderConfig expandSlider = {
            .x = sliderX,
            .y = sliderY + 14,
            .width = sliderWidth,
            .height = 8,
            .knobRadius = knobRadius,
            .value = expandFillRatio,
            .label = NULL,
            .showPercent = false
        };
        drawSlider(&expandSlider);
      } else {
        // === Brush Tab: Left side = brush list, Right side = brush settings ===
        
        // Layout constants
        float listX = MENU_CONTENT_PADDING;
        float listY = MENU_CONTENT_Y + MENU_CONTENT_PADDING;
        float listWidth = 150;
        float listHeight = MENU_BTN_Y - listY - MENU_CONTENT_PADDING;
        float itemHeight = 40;
        
        float settingsX = listX + listWidth + MENU_CONTENT_PADDING;
        float settingsY = MENU_CONTENT_Y + MENU_CONTENT_PADDING;
        float settingsWidth = BOTTOM_SCREEN_WIDTH - settingsX - MENU_CONTENT_PADDING;
        
        // Draw brush list background
        C2D_DrawRectSolid(listX, listY, 0, listWidth, listHeight, UI_COLOR_GRAY_1);
        
        // Calculate max scroll
        float totalContentHeight = NUM_BRUSHES * itemHeight;
        float maxScroll = totalContentHeight - listHeight;
        if (maxScroll < 0) maxScroll = 0;
        if (brushListScrollY < 0) brushListScrollY = 0;
        if (brushListScrollY > maxScroll) brushListScrollY = maxScroll;
        
        // Draw brush list items (with manual clipping)
        for (int i = 0; i < (int)NUM_BRUSHES; i++) {
            float itemY = listY + i * itemHeight - brushListScrollY;
            
            // Skip if completely out of visible area
            if (itemY + itemHeight <= listY || itemY >= listY + listHeight) continue;
            
            // Calculate clipping
            float clipTop = (itemY < listY) ? listY : itemY;
            float clipBottom = (itemY + itemHeight > listY + listHeight) ? listY + listHeight : itemY + itemHeight;
            float clipHeight = clipBottom - clipTop;
            
            // Skip if nothing visible
            if (clipHeight <= 0) continue;
            
            u32 itemBg = (i == currentBrushType) ? UI_COLOR_ACTIVE : UI_COLOR_GRAY_3;
            float textScale = 0.55f;
            float textX = listX + 10;
            float textY = itemY + itemHeight / 2 - 6;
            bool fullyVisible = (itemY >= listY && itemY + itemHeight <= listY + listHeight);
            
            if (fullyVisible) {
                char itemLabel[64];
                snprintf(itemLabel, sizeof(itemLabel), "%s", brushDefs[i].name);
                ListItemConfig item = {
                    .x = listX + 2,
                    .y = itemY + 2,
                    .width = listWidth - 4,
                    .height = itemHeight - 4,
                    .bgColor = itemBg,
                    .drawBorder = false,
                    .borderColor = 0,
                    .text = itemLabel,
                    .textX = textX,
                    .textY = textY,
                    .textScale = textScale,
                    .textColor = UI_COLOR_WHITE,
                    .rightIcon = NULL,
                    .rightIconX = 0,
                    .rightIconY = 0,
                    .rightIconScale = 0.0f,
                    .rightIconColor = UI_COLOR_WHITE
                };
                drawListItem(&item);
            } else {
                // Draw item background (clipped)
                float bgTop = (itemY + 2 < listY) ? listY : itemY + 2;
                float bgBottom = (itemY + itemHeight - 2 > listY + listHeight) ? listY + listHeight : itemY + itemHeight - 2;
                if (bgBottom > bgTop) {
                    drawClippedRect(listX + 2, bgTop, listWidth - 4, bgBottom - bgTop, listY, listY + listHeight, itemBg);
                }
                
                // Draw brush name text
                char itemLabel[64];
                snprintf(itemLabel, sizeof(itemLabel), "%s", brushDefs[i].name);
                C2D_TextBufClear(g_textBuf);
                C2D_Text nameText;
                C2D_TextParse(&nameText, g_textBuf, itemLabel);
                C2D_TextOptimize(&nameText);
                C2D_DrawText(&nameText, C2D_WithColor, textX, textY, 0, textScale, textScale, UI_COLOR_WHITE);
            }
        }
        
        // Draw list border
        C2D_DrawRectSolid(listX, listY, 0, listWidth, 2, UI_COLOR_GRAY_3);
        C2D_DrawRectSolid(listX, listY + listHeight - 2, 0, listWidth, 2, UI_COLOR_GRAY_3);
        C2D_DrawRectSolid(listX, listY, 0, 2, listHeight, UI_COLOR_GRAY_3);
        C2D_DrawRectSolid(listX + listWidth - 2, listY, 0, 2, listHeight, UI_COLOR_GRAY_3);
        
        // === Right side: Brush settings ===
        // Draw brush name
        char brushNameBuf[64];
        snprintf(brushNameBuf, sizeof(brushNameBuf), "%s", brushDefs[currentBrushType].name);
        C2D_TextBufClear(g_textBuf);
        C2D_Text brushNameText;
        C2D_TextParse(&brushNameText, g_textBuf, brushNameBuf);
        C2D_TextOptimize(&brushNameText);
        C2D_DrawText(&brushNameText, C2D_WithColor, settingsX, settingsY, 0, 0.6f, 0.6f, C2D_Color32(0xFF, 0xFF, 0xFF, 0xFF));
        
        // Size slider using component
        float sliderX = settingsX;
        float sliderY = settingsY + 25;
        float sliderWidth = settingsWidth - 10;  // Slider bar to padding edge, knob extends into margin
        
        int minSize = 1;
        int maxSize = 16;
        float fillRatio = (float)(getCurrentBrushSize() - minSize) / (maxSize - minSize);
        
        // Draw Size label and value on same row (like other sliders)
        C2D_TextBufClear(g_textBuf);
        C2D_Text sizeLabel;
        C2D_TextParse(&sizeLabel, g_textBuf, "Size");
        C2D_TextOptimize(&sizeLabel);
        C2D_DrawText(&sizeLabel, C2D_WithColor, sliderX, sliderY, 0, 0.4f, 0.4f, UI_COLOR_TEXT);
        
        // Draw size value right-aligned
        C2D_TextBufClear(g_textBuf);
        char sizeValBuf[8];
        snprintf(sizeValBuf, sizeof(sizeValBuf), "%d", getCurrentBrushSize());
        C2D_Text sizeVal;
        C2D_TextParse(&sizeVal, g_textBuf, sizeValBuf);
        C2D_TextOptimize(&sizeVal);
        float sizeValWidth, sizeValHeight;
        C2D_TextGetDimensions(&sizeVal, 0.4f, 0.4f, &sizeValWidth, &sizeValHeight);
        C2D_DrawText(&sizeVal, C2D_WithColor, sliderX + sliderWidth - sizeValWidth, sliderY, 0, 0.4f, 0.4f, UI_COLOR_WHITE);
        
        // Draw slider (without label since we drew it manually)
        SliderConfig sizeSlider = {
            .x = sliderX,
            .y = sliderY + 14,  // Offset to account for manual label
            .width = sliderWidth,
            .height = 8,
            .knobRadius = 10,
            .value = fillRatio,
            .label = NULL,
            .showPercent = false
        };
        drawSlider(&sizeSlider);
      }  // end if (currentTool == TOOL_FILL) else
    } else if (currentMenuTab == TAB_COLOR) {
        // === Color Tab: HSV picker on left, palette on right ===
        
        // --- LEFT SIDE: Mini HSV Color Picker ---
        // Draw SV square (Saturation-Value) - use larger blocks to reduce draw calls
        int svBlockSize = 5;  // 5x5 pixel blocks = 20x20 = 400 rectangles
        for (int sy = 0; sy < MCP_SV_SIZE; sy += svBlockSize) {
            for (int sx = 0; sx < MCP_SV_SIZE; sx += svBlockSize) {
                float s = (float)sx / (MCP_SV_SIZE - 1);
                float v = 1.0f - (float)sy / (MCP_SV_SIZE - 1);
                u32 col = hsvToRgb(currentHue, s, v);
                u8 r = (col >> 24) & 0xFF;
                u8 g = (col >> 16) & 0xFF;
                u8 b = (col >> 8) & 0xFF;
                C2D_DrawRectSolid(MCP_SV_X + sx, MCP_SV_Y + sy, 0, svBlockSize, svBlockSize, C2D_Color32(r, g, b, 0xFF));
            }
        }
        
        // Draw SV cursor
        int svCursorX = MCP_SV_X + (int)(currentSaturation * (MCP_SV_SIZE - 1));
        int svCursorY = MCP_SV_Y + (int)((1.0f - currentValue) * (MCP_SV_SIZE - 1));
        C2D_DrawCircleSolid(svCursorX, svCursorY, 0, 6, C2D_Color32(0x00, 0x00, 0x00, 0xFF));
        C2D_DrawCircleSolid(svCursorX, svCursorY, 0, 4, C2D_Color32(0xFF, 0xFF, 0xFF, 0xFF));
        
        // Draw Hue bar - use larger blocks
        int hueBlockSize = 5;
        for (int hy = 0; hy < MCP_HUE_HEIGHT; hy += hueBlockSize) {
            float h = (float)hy / (MCP_HUE_HEIGHT - 1) * 360.0f;
            u32 col = hsvToRgb(h, 1.0f, 1.0f);
            u8 r = (col >> 24) & 0xFF;
            u8 g = (col >> 16) & 0xFF;
            u8 b = (col >> 8) & 0xFF;
            C2D_DrawRectSolid(MCP_HUE_X, MCP_HUE_Y + hy, 0, MCP_HUE_WIDTH, hueBlockSize, C2D_Color32(r, g, b, 0xFF));
        }
        
        // Draw Hue cursor
        int hueCursorY = MCP_HUE_Y + (int)(currentHue / 360.0f * (MCP_HUE_HEIGHT - 1));
        C2D_DrawRectSolid(MCP_HUE_X - 3, hueCursorY - 2, 0, MCP_HUE_WIDTH + 6, 5, C2D_Color32(0xFF, 0xFF, 0xFF, 0xFF));
        C2D_DrawRectSolid(MCP_HUE_X - 2, hueCursorY - 1, 0, MCP_HUE_WIDTH + 4, 3, C2D_Color32(0x00, 0x00, 0x00, 0xFF));
        
        // Draw Alpha slider (below SV and Hue)
        float alphaRatio = brushAlpha / 255.0f;
        SliderConfig alphaSlider = {
            .x = MCP_ALPHA_SLIDER_X,
            .y = MCP_ALPHA_SLIDER_Y,
            .width = MCP_ALPHA_SLIDER_WIDTH,
            .height = 8,
            .knobRadius = 10,
            .value = alphaRatio,
            .label = "Alpha",
            .showPercent = true
        };
        drawSlider(&alphaSlider);
        
        // --- RIGHT SIDE: Color Palette ---
        float paletteStartX = 148;  // 2px left of original position
        float paletteStartY = MENU_CONTENT_Y + MENU_CONTENT_PADDING;
        
        // Draw palette grid
        for (int row = 0; row < PALETTE_ROWS; row++) {
            for (int col = 0; col < PALETTE_COLS; col++) {
                int idx = row * PALETTE_COLS + col;
                float cellX = paletteStartX + col * (PALETTE_CELL_SIZE + PALETTE_SPACING);
                float cellY = paletteStartY + row * (PALETTE_CELL_SIZE + PALETTE_SPACING);
                
                if (paletteUsed[idx]) {
                    // Draw filled color cell
                    u8 r = (paletteColors[idx] >> 24) & 0xFF;
                    u8 g = (paletteColors[idx] >> 16) & 0xFF;
                    u8 b = (paletteColors[idx] >> 8) & 0xFF;
                    C2D_DrawRectSolid(cellX, cellY, 0, PALETTE_CELL_SIZE, PALETTE_CELL_SIZE, C2D_Color32(r, g, b, 0xFF));
                    
                    // Border (white or red for delete mode)
                    u32 borderColor = paletteDeleteMode ? C2D_Color32(0xFF, 0x50, 0x50, 0xFF) : UI_COLOR_GRAY_3;
                    C2D_DrawRectSolid(cellX, cellY, 0, PALETTE_CELL_SIZE, 1, borderColor);
                    C2D_DrawRectSolid(cellX, cellY + PALETTE_CELL_SIZE - 1, 0, PALETTE_CELL_SIZE, 1, borderColor);
                    C2D_DrawRectSolid(cellX, cellY, 0, 1, PALETTE_CELL_SIZE, borderColor);
                    C2D_DrawRectSolid(cellX + PALETTE_CELL_SIZE - 1, cellY, 0, 1, PALETTE_CELL_SIZE, borderColor);
                } else {
                    // Draw empty cell
                    C2D_DrawRectSolid(cellX, cellY, 0, PALETTE_CELL_SIZE, PALETTE_CELL_SIZE, UI_COLOR_GRAY_2);
                    // Dashed border effect
                    C2D_DrawRectSolid(cellX, cellY, 0, PALETTE_CELL_SIZE, 1, C2D_Color32(0x60, 0x60, 0x60, 0xFF));
                    C2D_DrawRectSolid(cellX, cellY + PALETTE_CELL_SIZE - 1, 0, PALETTE_CELL_SIZE, 1, C2D_Color32(0x60, 0x60, 0x60, 0xFF));
                    C2D_DrawRectSolid(cellX, cellY, 0, 1, PALETTE_CELL_SIZE, C2D_Color32(0x60, 0x60, 0x60, 0xFF));
                    C2D_DrawRectSolid(cellX + PALETTE_CELL_SIZE - 1, cellY, 0, 1, PALETTE_CELL_SIZE, C2D_Color32(0x60, 0x60, 0x60, 0xFF));
                }
            }
        }
        
        // Draw Add/Delete mode toggle buttons and color preview
        float btnY = paletteStartY + PALETTE_ROWS * (PALETTE_CELL_SIZE + PALETTE_SPACING) + 2;  // 8px higher
        float btnWidth = 24;
        float btnHeight = 24;
        float previewSize = 32;  // 12px larger than before
        float btnSpacing = PALETTE_SPACING;
        float paletteWidth = PALETTE_COLS * PALETTE_CELL_SIZE + (PALETTE_COLS - 1) * PALETTE_SPACING;
        float paletteRightX = paletteStartX + paletteWidth;
        
        // Color preview (aligned with palette left edge)
        u8 previewR = (currentColor >> 24) & 0xFF;
        u8 previewG = (currentColor >> 16) & 0xFF;
        u8 previewB = (currentColor >> 8) & 0xFF;
        float previewX = paletteStartX + 2;  // Inner position (2px border)
        float previewY = btnY;  // Align top with buttons
        float previewInnerX = previewX;
        float previewInnerY = previewY + 2;
        
        // Draw white border
        C2D_DrawRectSolid(paletteStartX, previewY, 0, previewSize + 4, previewSize + 4, C2D_Color32(0xFF, 0xFF, 0xFF, 0xFF));
        
        // Draw checkerboard pattern for transparency visualization
        int checkSize = 4;  // Size of each checker square
        for (int cy = 0; cy < previewSize; cy += checkSize) {
            for (int cx = 0; cx < previewSize; cx += checkSize) {
                bool isLight = ((cx / checkSize) + (cy / checkSize)) % 2 == 0;
                u32 checkColor = isLight ? C2D_Color32(0xCC, 0xCC, 0xCC, 0xFF) : C2D_Color32(0x88, 0x88, 0x88, 0xFF);
                float drawW = (cx + checkSize > previewSize) ? (previewSize - cx) : checkSize;
                float drawH = (cy + checkSize > previewSize) ? (previewSize - cy) : checkSize;
                C2D_DrawRectSolid(previewInnerX + cx, previewInnerY + cy, 0, drawW, drawH, checkColor);
            }
        }
        
        // Draw color with alpha on top of checkerboard
        C2D_DrawRectSolid(previewInnerX, previewInnerY, 0, previewSize, previewSize, C2D_Color32(previewR, previewG, previewB, brushAlpha));
        
        // Add/Delete buttons aligned to palette right edge
        float delBtnX = paletteRightX - btnWidth;
        float addBtnX = delBtnX - btnWidth - btnSpacing;
        u32 addBtnColor = paletteDeleteMode ? UI_COLOR_GRAY_3 : UI_COLOR_ACTIVE;
        RectButtonConfig addBtn = {
            .x = addBtnX,
            .y = btnY,
            .width = btnWidth,
            .height = btnHeight,
            .drawBackground = true,
            .bgColor = addBtnColor,
            .drawTopBorder = false,
            .drawBottomBorder = false,
            .borderTopColor = 0,
            .borderBottomColor = 0,
            .icon = &palettePlusIconSprite,
            .iconScale = 0.3f,
            .iconColor = UI_COLOR_WHITE,
            .text = NULL,
            .textScale = 0.0f,
            .textColor = UI_COLOR_WHITE
        };
        drawRectButton(&addBtn);
        
        // Delete button (rightmost)
        u32 delBtnColor = paletteDeleteMode ? C2D_Color32(0xAA, 0x00, 0x00, 0xFF) : UI_COLOR_GRAY_3;
        RectButtonConfig delBtn = {
            .x = delBtnX,
            .y = btnY,
            .width = btnWidth,
            .height = btnHeight,
            .drawBackground = true,
            .bgColor = delBtnColor,
            .drawTopBorder = false,
            .drawBottomBorder = false,
            .borderTopColor = 0,
            .borderBottomColor = 0,
            .icon = &paletteMinusIconSprite,
            .iconScale = 0.3f,
            .iconColor = UI_COLOR_WHITE,
            .text = NULL,
            .textScale = 0.0f,
            .textColor = UI_COLOR_WHITE
        };
        drawRectButton(&delBtn);
    } else if (currentMenuTab == TAB_LAYER) {
        // === Layer Tab: Layer list on left, operations on right ===
        float listStartY = MENU_CONTENT_Y + MENU_CONTENT_PADDING;
        float listItemSpacing = 2;
        float listBottomY = MENU_BTN_Y - MENU_CONTENT_PADDING;
        float listHeight = listBottomY - listStartY;
        float listItemHeight = (listHeight - listItemSpacing * (MAX_LAYERS - 1)) / MAX_LAYERS;
        if (listItemHeight < 1) listItemHeight = 1;
        float listItemWidth = 150;  // Narrower to leave space for operations UI
        float listX = MENU_CONTENT_PADDING;
        float eyeBtnSize = 28;
        
        // Draw layer list (from top layer to bottom)
        for (int i = MAX_LAYERS - 1; i >= 0; i--) {
            int displayIdx = MAX_LAYERS - 1 - i;
            float itemY = listStartY + displayIdx * (listItemHeight + listItemSpacing);
            float clipIndent = layers[i].clipping ? 6.0f : 0.0f;
            float itemX = listX + clipIndent;
            float itemWidth = listItemWidth - clipIndent;
            float eyeBtnX = itemX + itemWidth - eyeBtnSize - 4;
            
            if (itemY + listItemHeight > listBottomY) continue;
            
            // Background - highlight current layer
            u32 bgColor = (i == currentLayerIndex) ? UI_COLOR_ACTIVE_DARK : UI_COLOR_GRAY_2;
            u32 borderColor = (i == currentLayerIndex) ? UI_COLOR_ACTIVE : UI_COLOR_GRAY_3;
            
            char layerNumStr[16];
            snprintf(layerNumStr, sizeof(layerNumStr), "%d", i + 1);
            const char* layerLabel = (layers[i].name[0] != '\0') ? layers[i].name : layerNumStr;
            
            float textWidth, textHeight;
            C2D_TextBufClear(g_textBuf);
            C2D_Text layerText;
            C2D_TextParse(&layerText, g_textBuf, layerLabel);
            C2D_TextOptimize(&layerText);
            C2D_TextGetDimensions(&layerText, 0.5f, 0.5f, &textWidth, &textHeight);
            float textX = itemX + 6;
            float textY = itemY + (listItemHeight - textHeight) / 2;
            
            float eyeIconX = eyeBtnX + eyeBtnSize / 2;
            float eyeIconY = itemY + listItemHeight / 2;
            
            ListItemConfig layerItem = {
                .x = itemX,
                .y = itemY,
                .width = itemWidth,
                .height = listItemHeight,
                .bgColor = bgColor,
                .drawBorder = true,
                .borderColor = borderColor,
                .text = layerLabel,
                .textX = textX,
                .textY = textY,
                .textScale = 0.5f,
                .textColor = UI_COLOR_WHITE,
                .rightIcon = &eyeIconSprite,
                .rightIconX = eyeIconX,
                .rightIconY = eyeIconY,
                .rightIconScale = 0.45f,
                .rightIconColor = layers[i].visible ? UI_COLOR_WHITE : UI_COLOR_DISABLED
            };
            drawListItem(&layerItem);
        }
        
        // === Right side: Layer operation buttons (4 columns x 2 rows) ===
        float opX = listX + listItemWidth + MENU_CONTENT_PADDING;
        float opY = listStartY;
        float opBtnSize = 32;
        float opBtnSpacing = 4;
        float rightMargin = MENU_CONTENT_PADDING + 10;  // Extra room for knob radius
        float sliderWidth = BOTTOM_SCREEN_WIDTH - opX - rightMargin;
        
        float col1X = opX;
        float col2X = col1X + opBtnSize + opBtnSpacing;
        float col3X = col2X + opBtnSize + opBtnSpacing;
        float col4X = col3X + opBtnSize + opBtnSpacing;
        
        // --- Row 1: Up, Down, Merge, Clear ---
        ButtonConfig upBtn = {
            .x = col1X, .y = opY, .size = opBtnSize,
            .icon = &upArrowIconSprite, .label = NULL,
            .isActive = false, .isToggle = false, .isSkeleton = false,
            .useCustomColors = true, .bgColor = UI_COLOR_GRAY_3,
            .iconColor = (currentLayerIndex >= MAX_LAYERS - 1) ? UI_COLOR_DISABLED : UI_COLOR_WHITE,
            .labelColor = UI_COLOR_TEXT, .iconScale = 0.4f
        };
        drawButton(&upBtn);
        
        ButtonConfig downBtn = {
            .x = col2X, .y = opY, .size = opBtnSize,
            .icon = &downArrowIconSprite, .label = NULL,
            .isActive = false, .isToggle = false, .isSkeleton = false,
            .useCustomColors = true, .bgColor = UI_COLOR_GRAY_3,
            .iconColor = (currentLayerIndex <= 0) ? UI_COLOR_DISABLED : UI_COLOR_WHITE,
            .labelColor = UI_COLOR_TEXT, .iconScale = 0.4f
        };
        drawButton(&downBtn);
        
        ButtonConfig mergeBtn = {
            .x = col3X, .y = opY, .size = opBtnSize,
            .icon = &mergeIconSprite, .label = NULL,
            .isActive = false, .isToggle = false, .isSkeleton = false,
            .useCustomColors = true, .bgColor = UI_COLOR_GRAY_3,
            .iconColor = (currentLayerIndex <= 0) ? UI_COLOR_DISABLED : UI_COLOR_WHITE,
            .labelColor = UI_COLOR_TEXT, .iconScale = 0.4f
        };
        drawButton(&mergeBtn);
        
        ButtonConfig clearBtn = {
            .x = col4X, .y = opY, .size = opBtnSize,
            .icon = &clearIconSprite, .label = NULL,
            .isActive = false, .isToggle = false, .isSkeleton = false,
            .useCustomColors = true, .bgColor = UI_COLOR_GRAY_3,
            .iconColor = UI_COLOR_WHITE,
            .labelColor = UI_COLOR_TEXT, .iconScale = 0.4f
        };
        drawButton(&clearBtn);
        
        // --- Row 2: Alpha Lock (toggle), Clipping (toggle), Rename ---
        float row2Y = opY + opBtnSize + opBtnSpacing;
        
        ButtonConfig alphaLockBtn = {
            .x = col1X, .y = row2Y, .size = opBtnSize,
            .icon = &alphaLockIconSprite, .label = NULL,
            .isActive = layers[currentLayerIndex].alphaLock,
            .isToggle = true, .isSkeleton = false,
            .useCustomColors = true,
            .bgColor = layers[currentLayerIndex].alphaLock ? UI_COLOR_ACTIVE : UI_COLOR_GRAY_3,
            .iconColor = UI_COLOR_WHITE,
            .labelColor = UI_COLOR_TEXT, .iconScale = 0.4f
        };
        drawButton(&alphaLockBtn);
        
        ButtonConfig clippingBtn = {
            .x = col2X, .y = row2Y, .size = opBtnSize,
            .icon = &clippingIconSprite, .label = NULL,
            .isActive = (currentLayerIndex > 0) ? layers[currentLayerIndex].clipping : false,
            .isToggle = (currentLayerIndex > 0), .isSkeleton = false,
            .useCustomColors = true,
            .bgColor = (currentLayerIndex > 0 && layers[currentLayerIndex].clipping) ? UI_COLOR_ACTIVE : UI_COLOR_GRAY_3,
            .iconColor = (currentLayerIndex > 0) ? UI_COLOR_WHITE : UI_COLOR_DISABLED,
            .labelColor = UI_COLOR_TEXT, .iconScale = 0.4f
        };
        drawButton(&clippingBtn);

        ButtonConfig renameBtn = {
            .x = col3X, .y = row2Y, .size = opBtnSize,
            .icon = &pencilIconSprite, .label = NULL,
            .isActive = false, .isToggle = false, .isSkeleton = false,
            .useCustomColors = true, .bgColor = UI_COLOR_GRAY_3,
            .iconColor = UI_COLOR_WHITE,
            .labelColor = UI_COLOR_TEXT, .iconScale = 0.4f
        };
        drawButton(&renameBtn);
        
        // === Opacity slider using component ===
        float sliderY = row2Y + opBtnSize + 10;
        float sliderX = opX;
        float opacityRatio = layers[currentLayerIndex].opacity / 255.0f;
        
        SliderConfig opacitySlider = {
            .x = sliderX,
            .y = sliderY,
            .width = sliderWidth,
            .height = 8,
            .knobRadius = 10,
            .value = opacityRatio,
            .label = "Opacity",
            .showPercent = true
        };
        drawSlider(&opacitySlider);
        
        // === Blend mode button ===
        float blendBtnY = sliderY + 45;
        float blendBtnWidth = BOTTOM_SCREEN_WIDTH - MENU_CONTENT_PADDING - sliderX;
        float blendBtnHeight = 24;
        
        // Blend mode text
        const char* blendModeName;
        switch (layers[currentLayerIndex].blendMode) {
            case BLEND_ADD: blendModeName = "Add"; break;
            case BLEND_MULTIPLY: blendModeName = "Multiply"; break;
            default: blendModeName = "Normal"; break;
        }
        RectButtonConfig blendBtn = {
            .x = sliderX,
            .y = blendBtnY,
            .width = blendBtnWidth,
            .height = blendBtnHeight,
            .drawBackground = true,
            .bgColor = UI_COLOR_GRAY_3,
            .drawTopBorder = false,
            .drawBottomBorder = false,
            .borderTopColor = 0,
            .borderBottomColor = 0,
            .icon = NULL,
            .iconScale = 0.0f,
            .iconColor = UI_COLOR_WHITE,
            .text = blendModeName,
            .textScale = 0.5f,
            .textColor = UI_COLOR_WHITE
        };
        drawRectButton(&blendBtn);
    }
    
    // Draw bottom bar background
    C2D_DrawRectSolid(0, MENU_BOTTOM_BAR_Y, 0, BOTTOM_SCREEN_WIDTH, MENU_BOTTOM_BAR_HEIGHT, UI_COLOR_GRAY_2);
    
    // Draw close button (left side) - just icon, no decoration
    C2D_ImageTint tint;
    C2D_PlainImageTint(&tint, UI_COLOR_WHITE, 1.0f);
    C2D_SpriteSetPos(&closeIconSprite, MENU_BTN_X + MENU_BTN_SIZE / 2, MENU_BTN_Y + MENU_BTN_SIZE / 2);
    C2D_SpriteSetScale(&closeIconSprite, 0.3f, 0.3f);
    C2D_DrawSpriteTinted(&closeIconSprite, &tint);
    
    // Draw separator line between close and save/exit
    C2D_DrawRectSolid(MENU_BTN_SIZE, MENU_BOTTOM_BAR_Y + 4, 0, 1, MENU_BOTTOM_BAR_HEIGHT - 8, UI_COLOR_GRAY_3);
    
    // Draw Save/Exit text (rest of bar is the button, no decoration)
    RectButtonConfig saveExitBtn = {
        .x = SAVE_EXIT_BTN_X,
        .y = SAVE_EXIT_BTN_Y,
        .width = SAVE_EXIT_BTN_WIDTH,
        .height = MENU_BOTTOM_BAR_HEIGHT,
        .drawBackground = false,
        .bgColor = 0,
        .drawTopBorder = false,
        .drawBottomBorder = false,
        .borderTopColor = 0,
        .borderBottomColor = 0,
        .icon = NULL,
        .iconScale = 0.0f,
        .iconColor = UI_COLOR_WHITE,
        .text = "Save / Exit",
        .textScale = 0.5f,
        .textColor = UI_COLOR_WHITE
    };
    drawRectButton(&saveExitBtn);
}

//---------------------------------------------------------------------------------
// Render home menu on bottom screen
//---------------------------------------------------------------------------------
static void renderHomeMenu(C3D_RenderTarget* target) {
    C2D_TargetClear(target, C2D_Color32(0x28, 0x28, 0x28, 0xFF));
    C2D_SceneBegin(target);

    // Calculate positions for 3 icons centered horizontally
    float totalWidth = BTN_SIZE_LARGE * 3 + SAVE_MENU_ITEM_SPACING * 2;
    float startX = (BOTTOM_SCREEN_WIDTH - totalWidth) / 2;
    float itemY = SAVE_MENU_Y;

    // Icons and labels for home menu
    C2D_Sprite* homeIcons[3] = { &newFileIconSprite, &folderIconSprite, &settingsIconSprite };
    const char* homeLabels[3] = { "New", "Open", "Settings" };

    // Draw 3 buttons
    for (int i = 0; i < 3; i++) {
        float btnX = startX + i * (BTN_SIZE_LARGE + SAVE_MENU_ITEM_SPACING);

        ButtonConfig btn = {
            .x = btnX,
            .y = itemY,
            .size = BTN_SIZE_LARGE,
            .icon = homeIcons[i],
            .label = homeLabels[i],
            .isActive = true,
            .isToggle = false,
            .isSkeleton = false
        };
        drawButton(&btn);
    }
}

//---------------------------------------------------------------------------------
// Render open project browser on bottom screen
//---------------------------------------------------------------------------------
static void renderOpenMenu(C3D_RenderTarget* target) {
    C2D_TargetClear(target, C2D_Color32(0x28, 0x28, 0x28, 0xFF));
    C2D_SceneBegin(target);

    float pad = MENU_CONTENT_PADDING;
    float listX = pad;
    float listY = 8;
    float listWidth = BOTTOM_SCREEN_WIDTH - pad * 2;
    float listHeight = MENU_BOTTOM_BAR_Y - listY - 4;
    float itemHeight = OPEN_LIST_ITEM_HEIGHT;

    // Draw list background
    C2D_DrawRectSolid(listX, listY, 0, listWidth, listHeight, UI_COLOR_GRAY_1);

    // Calculate scroll limits
    float totalContentHeight = openProjectCount * itemHeight;
    float maxScroll = totalContentHeight - listHeight;
    if (maxScroll < 0) maxScroll = 0;
    if (openListScrollY < 0) openListScrollY = 0;
    if (openListScrollY > maxScroll) openListScrollY = maxScroll;

    if (openProjectCount == 0) {
        // No projects found
        C2D_TextBufClear(g_textBuf);
        C2D_Text emptyText;
        C2D_TextParse(&emptyText, g_textBuf, "No projects found");
        C2D_TextOptimize(&emptyText);
        float tw, th;
        C2D_TextGetDimensions(&emptyText, 0.5f, 0.5f, &tw, &th);
        C2D_DrawText(&emptyText, C2D_WithColor, listX + (listWidth - tw) / 2, listY + listHeight / 2 - th / 2, 0, 0.5f, 0.5f, UI_COLOR_TEXT_DIM);
    } else {
        // Draw project list items
        for (int i = 0; i < openProjectCount; i++) {
            float itemY = listY + i * itemHeight - openListScrollY;

            // Skip items outside visible area
            if (itemY + itemHeight <= listY || itemY >= listY + listHeight) continue;

            // Clipping
            bool fullyVisible = (itemY >= listY && itemY + itemHeight <= listY + listHeight);

            u32 itemBg = (i == openSelectedIndex) ? UI_COLOR_ACTIVE : UI_COLOR_GRAY_3;
            float textScale = 0.5f;
            float textX = listX + 10;
            float textY_pos = itemY + itemHeight / 2 - 8;

            if (fullyVisible) {
                ListItemConfig item = {
                    .x = listX + 2,
                    .y = itemY + 2,
                    .width = listWidth - 4,
                    .height = itemHeight - 4,
                    .bgColor = itemBg,
                    .drawBorder = false,
                    .borderColor = 0,
                    .text = openProjectNames[i],
                    .textX = textX,
                    .textY = textY_pos,
                    .textScale = textScale,
                    .textColor = UI_COLOR_WHITE,
                    .rightIcon = NULL,
                    .rightIconX = 0,
                    .rightIconY = 0,
                    .rightIconScale = 0.0f,
                    .rightIconColor = UI_COLOR_WHITE
                };
                drawListItem(&item);
            } else {
                // Partial visibility: draw clipped background + text
                float bgTop = (itemY + 2 < listY) ? listY : itemY + 2;
                float bgBottom = (itemY + itemHeight - 2 > listY + listHeight) ? listY + listHeight : itemY + itemHeight - 2;
                if (bgBottom > bgTop) {
                    C2D_DrawRectSolid(listX + 2, bgTop, 0, listWidth - 4, bgBottom - bgTop, itemBg);
                }
                if (textY_pos >= listY && textY_pos + 16 <= listY + listHeight) {
                    C2D_TextBufClear(g_textBuf);
                    C2D_Text nameText;
                    C2D_TextParse(&nameText, g_textBuf, openProjectNames[i]);
                    C2D_TextOptimize(&nameText);
                    C2D_DrawText(&nameText, C2D_WithColor, textX, textY_pos, 0, textScale, textScale, UI_COLOR_WHITE);
                }
            }
        }
    }

    // Draw list border
    C2D_DrawRectSolid(listX, listY, 0, listWidth, 2, UI_COLOR_GRAY_3);
    C2D_DrawRectSolid(listX, listY + listHeight - 2, 0, listWidth, 2, UI_COLOR_GRAY_3);
    C2D_DrawRectSolid(listX, listY, 0, 2, listHeight, UI_COLOR_GRAY_3);
    C2D_DrawRectSolid(listX + listWidth - 2, listY, 0, 2, listHeight, UI_COLOR_GRAY_3);

    // --- Bottom bar with Back (icon) and Open (text) ---
    C2D_DrawRectSolid(0, MENU_BOTTOM_BAR_Y, 0, BOTTOM_SCREEN_WIDTH, MENU_BOTTOM_BAR_HEIGHT, UI_COLOR_GRAY_2);

    // Back icon on left
    C2D_ImageTint tint;
    C2D_PlainImageTint(&tint, UI_COLOR_WHITE, 1.0f);
    C2D_SpriteSetPos(&backArrowIconSprite, MENU_BTN_X + MENU_BTN_SIZE / 2, MENU_BTN_Y + MENU_BTN_SIZE / 2);
    C2D_SpriteSetScale(&backArrowIconSprite, 0.3f, 0.3f);
    C2D_DrawSpriteTinted(&backArrowIconSprite, &tint);

    // Separator line
    C2D_DrawRectSolid(MENU_BTN_SIZE, MENU_BOTTOM_BAR_Y + 4, 0, 1, MENU_BOTTOM_BAR_HEIGHT - 8, UI_COLOR_GRAY_3);

    // Open text button on right
    RectButtonConfig openBtn = {
        .x = SAVE_EXIT_BTN_X,
        .y = SAVE_EXIT_BTN_Y,
        .width = SAVE_EXIT_BTN_WIDTH,
        .height = MENU_BOTTOM_BAR_HEIGHT,
        .drawBackground = false,
        .bgColor = 0,
        .drawTopBorder = false,
        .drawBottomBorder = false,
        .borderTopColor = 0,
        .borderBottomColor = 0,
        .icon = NULL,
        .iconScale = 0.0f,
        .iconColor = UI_COLOR_WHITE,
        .text = "Open",
        .textScale = 0.5f,
        .textColor = UI_COLOR_WHITE
    };
    drawRectButton(&openBtn);
}

//---------------------------------------------------------------------------------
// Render new project settings on bottom screen
//---------------------------------------------------------------------------------
static void renderNewProjectMenu(C3D_RenderTarget* target) {
    C2D_TargetClear(target, C2D_Color32(0x28, 0x28, 0x28, 0xFF));
    C2D_SceneBegin(target);

    float pad = MENU_CONTENT_PADDING;
    float labelScale = 0.5f;
    float fieldX = pad;
    float fieldWidth = BOTTOM_SCREEN_WIDTH - pad * 2;
    float fieldHeight = 28;
    float curY = 12;
    char textBuf[64];
    C2D_Text text;
    float tw, th;

    // --- Title ---
    C2D_TextBufClear(g_textBuf);
    C2D_TextParse(&text, g_textBuf, "New Project");
    C2D_TextOptimize(&text);
    C2D_TextGetDimensions(&text, 0.6f, 0.6f, &tw, &th);
    C2D_DrawText(&text, C2D_WithColor, (BOTTOM_SCREEN_WIDTH - tw) / 2, curY, 0, 0.6f, 0.6f, UI_COLOR_WHITE);
    curY += th + 12;

    // --- Project Name ---
    C2D_TextBufClear(g_textBuf);
    C2D_TextParse(&text, g_textBuf, "Project Name");
    C2D_TextOptimize(&text);
    C2D_DrawText(&text, C2D_WithColor, fieldX, curY, 0, labelScale, labelScale, UI_COLOR_TEXT_DIM);
    curY += 16;

    // Text field background
    C2D_DrawRectSolid(fieldX, curY, 0, fieldWidth, fieldHeight, UI_COLOR_GRAY_3);
    // Text field value
    C2D_TextBufClear(g_textBuf);
    const char* nameDisplay = (newProjectName[0] != '\0') ? newProjectName : "Tap to enter name...";
    C2D_TextParse(&text, g_textBuf, nameDisplay);
    C2D_TextOptimize(&text);
    u32 nameColor = (newProjectName[0] != '\0') ? UI_COLOR_WHITE : UI_COLOR_DISABLED;
    C2D_DrawText(&text, C2D_WithColor, fieldX + 6, curY + 6, 0, labelScale, labelScale, nameColor);
    curY += fieldHeight + 10;

    // --- Canvas Width ---
    C2D_TextBufClear(g_textBuf);
    snprintf(textBuf, sizeof(textBuf), "Width (max %d)", NEW_PROJECT_MAX_WIDTH);
    C2D_TextParse(&text, g_textBuf, textBuf);
    C2D_TextOptimize(&text);
    C2D_DrawText(&text, C2D_WithColor, fieldX, curY, 0, labelScale, labelScale, UI_COLOR_TEXT_DIM);
    curY += 16;

    float halfFieldW = (fieldWidth - pad) / 2;

    C2D_DrawRectSolid(fieldX, curY, 0, halfFieldW, fieldHeight, UI_COLOR_GRAY_3);
    C2D_TextBufClear(g_textBuf);
    snprintf(textBuf, sizeof(textBuf), "%d", newProjectWidth);
    C2D_TextParse(&text, g_textBuf, textBuf);
    C2D_TextOptimize(&text);
    C2D_DrawText(&text, C2D_WithColor, fieldX + 6, curY + 6, 0, labelScale, labelScale, UI_COLOR_WHITE);

    // --- Canvas Height ---
    float rightFieldX = fieldX + halfFieldW + pad;
    C2D_DrawRectSolid(rightFieldX, curY, 0, halfFieldW, fieldHeight, UI_COLOR_GRAY_3);
    C2D_TextBufClear(g_textBuf);
    snprintf(textBuf, sizeof(textBuf), "%d", newProjectHeight);
    C2D_TextParse(&text, g_textBuf, textBuf);
    C2D_TextOptimize(&text);
    C2D_DrawText(&text, C2D_WithColor, rightFieldX + 6, curY + 6, 0, labelScale, labelScale, UI_COLOR_WHITE);

    // Height label (above right field, same row as width label)
    C2D_TextBufClear(g_textBuf);
    snprintf(textBuf, sizeof(textBuf), "Height (max %d)", NEW_PROJECT_MAX_HEIGHT);
    C2D_TextParse(&text, g_textBuf, textBuf);
    C2D_TextOptimize(&text);
    C2D_DrawText(&text, C2D_WithColor, rightFieldX, curY - 16, 0, labelScale, labelScale, UI_COLOR_TEXT_DIM);

    curY += fieldHeight + 16;

    // --- Bottom bar with Back (icon) and OK (text) ---
    C2D_DrawRectSolid(0, MENU_BOTTOM_BAR_Y, 0, BOTTOM_SCREEN_WIDTH, MENU_BOTTOM_BAR_HEIGHT, UI_COLOR_GRAY_2);

    // Back icon on left
    C2D_ImageTint tint;
    C2D_PlainImageTint(&tint, UI_COLOR_WHITE, 1.0f);
    C2D_SpriteSetPos(&backArrowIconSprite, MENU_BTN_X + MENU_BTN_SIZE / 2, MENU_BTN_Y + MENU_BTN_SIZE / 2);
    C2D_SpriteSetScale(&backArrowIconSprite, 0.3f, 0.3f);
    C2D_DrawSpriteTinted(&backArrowIconSprite, &tint);

    // Separator line between back icon and OK button
    C2D_DrawRectSolid(MENU_BTN_SIZE, MENU_BOTTOM_BAR_Y + 4, 0, 1, MENU_BOTTOM_BAR_HEIGHT - 8, UI_COLOR_GRAY_3);

    // OK text button on right
    RectButtonConfig okBtn = {
        .x = SAVE_EXIT_BTN_X,
        .y = SAVE_EXIT_BTN_Y,
        .width = SAVE_EXIT_BTN_WIDTH,
        .height = MENU_BOTTOM_BAR_HEIGHT,
        .drawBackground = false,
        .bgColor = 0,
        .drawTopBorder = false,
        .drawBottomBorder = false,
        .borderTopColor = 0,
        .borderBottomColor = 0,
        .icon = NULL,
        .iconScale = 0.0f,
        .iconColor = UI_COLOR_WHITE,
        .text = "OK",
        .textScale = 0.5f,
        .textColor = UI_COLOR_WHITE
    };
    drawRectButton(&okBtn);
}

//---------------------------------------------------------------------------------
// Render save menu on bottom screen
//---------------------------------------------------------------------------------
static void renderSaveMenu(C3D_RenderTarget* target) {
    C2D_TargetClear(target, C2D_Color32(0x28, 0x28, 0x28, 0xFF));
    C2D_SceneBegin(target);
    
    // Calculate positions for 3 icons centered horizontally
    float totalWidth = BTN_SIZE_LARGE * 3 + SAVE_MENU_ITEM_SPACING * 2;
    float startX = (BOTTOM_SCREEN_WIDTH - totalWidth) / 2;
    float itemY = SAVE_MENU_Y;
    
    // Icons and labels for save menu
    C2D_Sprite* saveIcons[3] = { &saveIconSprite, &saveAsIconSprite, &exportIconSprite };
    const char* saveLabels[3] = { "Save", "Save As", "Export" };
    
    // Draw 3 buttons
    for (int i = 0; i < 3; i++) {
        float btnX = startX + i * (BTN_SIZE_LARGE + SAVE_MENU_ITEM_SPACING);
        
        ButtonConfig btn = {
            .x = btnX,
            .y = itemY,
            .size = BTN_SIZE_LARGE,
            .icon = saveIcons[i],
            .label = saveLabels[i],
            .isActive = true,  // Use UI_COLOR_ACTIVE for all save menu buttons
            .isToggle = false,
            .isSkeleton = false
        };
        drawButton(&btn);
    }
    
    // Draw bottom bar with Back button on left and Go to Home on right
    C2D_DrawRectSolid(0, MENU_BOTTOM_BAR_Y, 0, BOTTOM_SCREEN_WIDTH, MENU_BOTTOM_BAR_HEIGHT, UI_COLOR_GRAY_2);
    
    // Draw separator line between sections
    C2D_DrawRectSolid(MENU_BTN_SIZE, MENU_BOTTOM_BAR_Y + 4, 0, 1, MENU_BOTTOM_BAR_HEIGHT - 8, UI_COLOR_GRAY_3);
    
    // Draw back button on left (icon only)
    C2D_ImageTint tint;
    C2D_PlainImageTint(&tint, UI_COLOR_WHITE, 1.0f);
    C2D_SpriteSetPos(&backArrowIconSprite, MENU_BTN_X + MENU_BTN_SIZE / 2, MENU_BTN_Y + MENU_BTN_SIZE / 2);
    C2D_SpriteSetScale(&backArrowIconSprite, 0.3f, 0.3f);
    C2D_DrawSpriteTinted(&backArrowIconSprite, &tint);
    
    // Draw "Go to Home" text on right
    RectButtonConfig goHomeBtn = {
        .x = MENU_BTN_SIZE,
        .y = MENU_BOTTOM_BAR_Y,
        .width = BOTTOM_SCREEN_WIDTH - MENU_BTN_SIZE,
        .height = MENU_BOTTOM_BAR_HEIGHT,
        .drawBackground = false,
        .bgColor = 0,
        .drawTopBorder = false,
        .drawBottomBorder = false,
        .borderTopColor = 0,
        .borderBottomColor = 0,
        .icon = NULL,
        .iconScale = 0.0f,
        .iconColor = UI_COLOR_WHITE,
        .text = "Go to Home",
        .textScale = 0.5f,
        .textColor = UI_COLOR_WHITE
    };
    drawRectButton(&goHomeBtn);
}

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
        
        // Exit on START
        if (kDown & KEY_START) break;
        
        // Toggle color picker with Y button
        if (kDown & KEY_Y) {
            if (currentMode == MODE_DRAW) {
                currentMode = MODE_COLOR_PICKER;
                rgbToHsv(currentColor, &currentHue, &currentSaturation, &currentValue);
            } else if (currentMode == MODE_COLOR_PICKER) {
                currentMode = MODE_DRAW;
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

                // Settings button: TODO
            }

            // Render frame
            C3D_FrameBegin(C3D_FRAME_SYNCDRAW);
            C2D_TargetClear(topScreen, C2D_Color32(0x2A, 0x2A, 0x2A, 0xFF));
            C2D_SceneBegin(topScreen);
            renderHomeMenu(bottomScreen);
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
            C2D_TargetClear(topScreen, C2D_Color32(0x2A, 0x2A, 0x2A, 0xFF));
            C2D_SceneBegin(topScreen);
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
            
            // Cycle tool with A button (Brush -> Eraser -> Fill -> Brush)
            if (kDown & KEY_A) {
                currentTool = (currentTool + 1) % 3;
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
                    if (touch.px >= DRAW_MENU_BTN_X && touch.px < DRAW_MENU_BTN_X + DRAW_MENU_BTN_SIZE &&
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
                            
                            floodFill(currentLayerIndex, drawX, drawY, fillColor, fillExpand);
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
                            startStroke();
                            
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
                    bool touchingMenuBtn = (touch.px >= DRAW_MENU_BTN_X && touch.px < DRAW_MENU_BTN_X + DRAW_MENU_BTN_SIZE &&
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
            
        } else if (currentMode == MODE_COLOR_PICKER) {
            // === COLOR PICKER MODE ===
            
            touchPosition touch;
            hidTouchRead(&touch);
            
            if (kHeld & KEY_TOUCH) {
                // Check if touching Saturation-Value box
                if (touch.px >= CP_SV_X && touch.px < CP_SV_X + CP_SV_WIDTH &&
                    touch.py >= CP_SV_Y && touch.py < CP_SV_Y + CP_SV_HEIGHT) {
                    currentSaturation = (float)(touch.px - CP_SV_X) / CP_SV_WIDTH;
                    currentValue = 1.0f - (float)(touch.py - CP_SV_Y) / CP_SV_HEIGHT;
                    
                    if (currentSaturation < 0) currentSaturation = 0;
                    if (currentSaturation > 1) currentSaturation = 1;
                    if (currentValue < 0) currentValue = 0;
                    if (currentValue > 1) currentValue = 1;
                    
                    currentColor = hsvToRgb(currentHue, currentSaturation, currentValue);
                }
                
                // Check if touching Hue bar
                if (touch.px >= CP_HUE_X && touch.px < CP_HUE_X + CP_HUE_WIDTH &&
                    touch.py >= CP_HUE_Y && touch.py < CP_HUE_Y + CP_HUE_HEIGHT) {
                    currentHue = (float)(touch.py - CP_HUE_Y) / CP_HUE_HEIGHT * 360.0f;
                    
                    if (currentHue < 0) currentHue = 0;
                    if (currentHue >= 360) currentHue = 359.9f;
                    
                    currentColor = hsvToRgb(currentHue, currentSaturation, currentValue);
                }
                
                // Check OK button
                if (kDown & KEY_TOUCH) {
                    if (touch.px >= CP_PREVIEW_X && touch.px < CP_PREVIEW_X + CP_PREVIEW_SIZE &&
                        touch.py >= CP_PREVIEW_Y + CP_PREVIEW_SIZE + 20 && touch.py < CP_PREVIEW_Y + CP_PREVIEW_SIZE + 50) {
                        currentMode = MODE_DRAW;
                    }
                }
            }
            
            // B button also returns to draw mode
            if (kDown & KEY_B) {
                currentMode = MODE_DRAW;
            }
            
            // Render frame
            C3D_FrameBegin(C3D_FRAME_SYNCDRAW);
            renderUI(topScreen);
            renderColorPicker(bottomScreen);
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
                    // Fill tool: Expand slider (must match rendering layout)
                    float settingsX = MENU_CONTENT_PADDING;
                    float settingsY = MENU_CONTENT_Y + MENU_CONTENT_PADDING;
                    float settingsWidth = BOTTOM_SCREEN_WIDTH - MENU_CONTENT_PADDING * 2;
                    float knobRadius = 10;
                    float sliderX = settingsX + knobRadius;
                    float sliderY = settingsY;
                    float sliderWidth = settingsWidth - knobRadius * 2;
                    float sliderTrackY = sliderY + 14 + knobRadius + knobRadius;
                    
                    // Reset state when touch released
                    if (!(kHeld & KEY_TOUCH)) {
                        fillExpandSliderActive = false;
                    }
                    
                    // Check if touching expand slider area
                    if (touch.px >= sliderX - 5 && touch.px <= sliderX + sliderWidth + 5 &&
                        touch.py >= sliderTrackY - 15 && touch.py <= sliderTrackY + 15) {
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
                    // TODO: Implement export logic (PNG/BMP)
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
