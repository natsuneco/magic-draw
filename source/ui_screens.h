#pragma once

#include <citro2d.h>

/**
 * @file ui_screens.h
 * @brief UI screen rendering and icon initialization.
 */

void initIcons(void);
void exitIcons(void);
void renderUI(C3D_RenderTarget* target);
void renderCanvas(C3D_RenderTarget* target, bool showOverlay);
void renderMenu(C3D_RenderTarget* target);
void renderHomeTop(C3D_RenderTarget* target);
void renderHomeMenu(C3D_RenderTarget* target);
void renderSettingsMenu(C3D_RenderTarget* target);
void renderSettingsTop(C3D_RenderTarget* target);
void renderOpenMenu(C3D_RenderTarget* target);
void renderNewProjectMenu(C3D_RenderTarget* target);
void renderSaveMenu(C3D_RenderTarget* target);
