#include "app_state.h"

// Canvas settings (using bottom screen size for now)
int canvasWidth = BOTTOM_SCREEN_WIDTH;
int canvasHeight = BOTTOM_SCREEN_HEIGHT;

// Texture size (power-of-2, computed from canvas size)
int texWidth = 512;
int texHeight = 256;

// Available brushes
const BrushDef brushDefs[] = {
    {"Antialias Pen", BRUSH_ANTIALIAS},
    {"G-Pen", BRUSH_GPEN},
    {"Pixel Pen", BRUSH_PIXEL},
    {"Airbrush", BRUSH_AIRBRUSH}
};
const size_t brushDefsCount = sizeof(brushDefs) / sizeof(brushDefs[0]);

AppMode currentMode = MODE_HOME;
ToolType currentTool = TOOL_BRUSH;
MenuTab currentMenuTab = TAB_TOOL;

// HSV color state (for color picker)
float currentHue = 0.0f;        // 0-360
float currentSaturation = 1.0f; // 0-1
float currentValue = 0.0f;      // 0-1

// Layer system
Layer layers[MAX_LAYERS];
int currentLayerIndex = 0;
int numLayers = MAX_LAYERS;

// Composite buffer (result of merging all layers)
u32* compositeBuffer = NULL;
C3D_Tex canvasTex;
Tex3DS_SubTexture canvasSubTex;
C2D_Image canvasImage;

// Current drawing state
u32 currentColor = 0x000000FF;  // Black (RGBA format: 0xRRGGBBAA)
int brushSizesByType[BRUSH_TYPE_COUNT] = {2, 2, 2, 2};
u8 brushAlpha = 255;  // Brush opacity (0-255)
bool isDrawing = false;
float lastCanvasX = 0.0f;
float lastCanvasY = 0.0f;
bool brushSizeSliderActive = false;  // For brush size preview on top screen

// Brush selection state
int currentBrushType = 0;  // Index into brushDefs
float brushListScrollY = 0;  // Scroll position for brush list
float brushListLastTouchY = 0;  // For scroll tracking
bool brushListDragging = false;  // Whether dragging the list

// Fill tool settings
int fillExpand = 0;  // Fill expansion in pixels (0-10)
bool fillExpandSliderActive = false;  // For slider tracking

// Icon sprites
C2D_SpriteSheet iconSpriteSheet;
C2D_SpriteSheet bannerSpriteSheet;
C2D_SpriteSheet menuButtonBgSpriteSheet;
C2D_Sprite brushIconSprite;
C2D_Sprite eraserIconSprite;
C2D_Sprite bucketIconSprite;
C2D_Sprite wrenchIconSprite;
C2D_Sprite closeIconSprite;
C2D_Sprite plusIconSprite;
C2D_Sprite minusIconSprite;
C2D_Sprite eyeIconSprite;
C2D_Sprite clearIconSprite;
C2D_Sprite upArrowIconSprite;
C2D_Sprite downArrowIconSprite;
C2D_Sprite mergeIconSprite;
C2D_Sprite zoomInIconSprite;
C2D_Sprite zoomOutIconSprite;
C2D_Sprite undoIconSprite;
C2D_Sprite redoIconSprite;
C2D_Sprite saveIconSprite;
C2D_Sprite saveAsIconSprite;
C2D_Sprite exportIconSprite;
C2D_Sprite palettePlusIconSprite;
C2D_Sprite paletteMinusIconSprite;
C2D_Sprite clippingIconSprite;
C2D_Sprite alphaLockIconSprite;
C2D_Sprite pencilIconSprite;
C2D_Sprite crossArrowIconSprite;
C2D_Sprite checkIconSprite;
C2D_Sprite folderIconSprite;
C2D_Sprite settingsIconSprite;
C2D_Sprite newFileIconSprite;
C2D_Sprite backArrowIconSprite;
C2D_Sprite bannerSprite;
C2D_Sprite menuButtonBgSprite;

// Screen targets (for dialog rendering)
C3D_RenderTarget* g_topScreen = NULL;
C3D_RenderTarget* g_bottomScreen = NULL;

// FPS counter
u64 lastFrameTime = 0;
float currentFPS = 0.0f;

// Canvas update optimization
bool canvasNeedsUpdate = true;  // Dirty flag for canvas
int updateFrameCounter = 0;     // Frame counter for update throttling

// Zoom and pan state
float canvasZoom = 1.0f;
float canvasPanX = 0.0f;
float canvasPanY = 0.0f;
bool isPanning = false;
touchPosition panStartTouch;

// Text rendering
C2D_TextBuf g_textBuf = NULL;

// Current project state
char currentProjectName[PROJECT_NAME_MAX] = "";
bool projectHasName = false;
bool projectHasUnsavedChanges = false;

// Open project browser state
char openProjectNames[OPEN_MAX_PROJECTS][PROJECT_NAME_MAX];
int openProjectCount = 0;
int openSelectedIndex = -1;  // -1 = none selected
float openListScrollY = 0;
int openListLastTouchX = 0;
float openListLastTouchY = 0;
bool openListDragging = false;

// Preview texture for open browser
C3D_Tex openPreviewTex;
Tex3DS_SubTexture openPreviewSubTex;
C2D_Image openPreviewImage;
bool openPreviewValid = false;
int openPreviewWidth = 0;
int openPreviewHeight = 0;

// New project creation state
char newProjectName[PROJECT_NAME_MAX] = "";
int newProjectWidth = BOTTOM_SCREEN_WIDTH;
int newProjectHeight = BOTTOM_SCREEN_HEIGHT;

// Color palette settings
u32 paletteColors[PALETTE_MAX_COLORS];
bool paletteUsed[PALETTE_MAX_COLORS];
bool paletteDeleteMode = false;  // false = add mode, true = delete mode

int getCurrentBrushSize(void) {
    return brushSizesByType[currentBrushType];
}

void setCurrentBrushSize(int size) {
    brushSizesByType[currentBrushType] = size;
}

void initPalette(void) {
    for (int i = 0; i < PALETTE_MAX_COLORS; i++) {
        paletteColors[i] = 0x00000000;
        paletteUsed[i] = false;
    }

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
