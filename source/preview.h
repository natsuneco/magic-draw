#pragma once

#include <stdbool.h>

#include "app_state.h"

/**
 * @file preview.h
 * @brief Project browser list and preview rendering data.
 */

void scanProjectFiles(void);
void freeOpenPreview(void);
bool loadProjectPreview(const char* projectName);
