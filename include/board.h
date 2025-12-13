#ifndef BOARD_H
#define BOARD_H

#include "types.h"

#include <raylib.h>

typedef struct {
    Texture2D background;
    float origin_x;
    float origin_y;
} Board;

void InitBoard(Board* board);
void UnloadBoard(Board* board);
void DrawBoard(Board* board);
void DrawTileHighlight(Board* board, int col, int row, Color color);
Vector2 BoardToScreen(Board* board, int col, int row);
BoardPos ScreenToBoard(Board* board, float screen_x, float screen_y);
bool IsValidBoardPos(int col, int row);

#endif
