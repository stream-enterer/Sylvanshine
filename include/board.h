#ifndef BOARD_H
#define BOARD_H

#include "render.h"
#include "types.h"

#include <raylib.h>

typedef struct {
    Texture2D background;
    Texture2D tile_move;
} Board;

void InitBoard(Board* board);
void UnloadBoard(Board* board);
void DrawBackground(Board* board);
void DrawTileHighlight(Board* board, RenderState* render, int col, int row, Color color);
void DrawTileSprite(Board* board, RenderState* render, int col, int row, Color tint);
BoardPos WorldToBoard(RenderState* render, Vector3 world_pos);
bool IsValidBoardPos(int col, int row);

#endif
