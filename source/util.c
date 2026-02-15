#include "util.h"

#include <3ds.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

int nextPowerOf2(int n) {
    if (n <= 0) return 1;
    n--;
    n |= n >> 1;
    n |= n >> 2;
    n |= n >> 4;
    n |= n >> 8;
    n |= n >> 16;
    return n + 1;
}

void ensureDirectoryExists(const char* path) {
    struct stat st = {0};
    if (stat(path, &st) == -1) {
        mkdir(path, 0777);
    }
}

bool showKeyboard(const char* hintText, char* outBuf, size_t bufSize) {
    SwkbdState swkbd;
    swkbdInit(&swkbd, SWKBD_TYPE_NORMAL, 2, (int)bufSize - 1);
    swkbdSetHintText(&swkbd, hintText);
    swkbdSetButton(&swkbd, SWKBD_BUTTON_LEFT, "Cancel", false);
    swkbdSetButton(&swkbd, SWKBD_BUTTON_RIGHT, "OK", true);
    swkbdSetValidation(&swkbd, SWKBD_NOTEMPTY_NOTBLANK, 0, 0);
    swkbdSetFeatures(&swkbd, SWKBD_PREDICTIVE_INPUT);

    SwkbdButton button = swkbdInputText(&swkbd, outBuf, (u32)bufSize);
    return (button == SWKBD_BUTTON_RIGHT);
}

int showNumericKeyboard(const char* hintText, int currentValue, int minVal, int maxVal) {
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

bool fileExists(const char* path) {
    struct stat st;
    return (stat(path, &st) == 0);
}
