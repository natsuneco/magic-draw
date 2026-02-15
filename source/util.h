#pragma once

#include <stdbool.h>
#include <stddef.h>

/**
 * @file util.h
 * @brief Small helper utilities for file and input handling.
 */

/** @brief Round up to the next power of two. */
int nextPowerOf2(int n);

/** @brief Create a directory if it does not exist. */
void ensureDirectoryExists(const char* path);

/**
 * @brief Show software keyboard for text input.
 * @return true when OK is pressed.
 */
bool showKeyboard(const char* hintText, char* outBuf, size_t bufSize);

/**
 * @brief Show numeric keyboard for integer input, returns -1 on cancel.
 */
int showNumericKeyboard(const char* hintText, int currentValue, int minVal, int maxVal);

/** @brief Check if a file exists. */
bool fileExists(const char* path);
