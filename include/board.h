#ifndef BOARD_H
#define BOARD_H

#include "render.h"
#include "types.h"

#include <raylib.h>

typedef struct {
    int x;
    int y;
    int w;
    int h;
} TileFrame;

typedef struct {
    Texture2D background;
    Texture2D tile_move;
    Texture2D tiles_board;
    TileFrame grid_frame;
    TileFrame hover_frame;
} Board;

void InitBoard(Board* board);
void UnloadBoard(Board* board);
void DrawBackground(Board* board);
void DrawBoardGrid(Board* board, RenderState* render);
void DrawTileHighlight(Board* board, RenderState* render, int col, int row, Color color);
void DrawTileSprite(Board* board, RenderState* render, int col, int row, Color tint);
void DrawTileFromSheet(Board* board, RenderState* render, int col, int row, TileFrame frame, Color tint);
BoardPos WorldToBoard(RenderState* render, Vector3 world_pos);
bool IsValidBoardPos(int col, int row);

#endif
