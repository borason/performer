#pragma once

#include "core/gfx/Canvas.h"

class SongPainter {
public:
    static void drawArrowDown(Canvas &canvas, int x, int y, int w);
    static void drawArrowUp(Canvas &canvas, int x, int y, int w);
    static void drawProgress(Canvas &canvas, int x, int y, int w, int h, float progress);
};
