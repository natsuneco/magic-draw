#pragma once

#include "app_state.h"

/**
 * @file history.h
 * @brief Undo/redo history management.
 */

void initHistory(void);
void exitHistory(void);
void pushHistory(void);
bool canUndo(void);
bool canRedo(void);
void undo(void);
void redo(void);
