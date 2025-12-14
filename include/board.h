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
    bool rotated;
} TileFrame;

typedef enum {
    MERGED_0 = 0,
    MERGED_01,
    MERGED_03,
    MERGED_013,
    MERGED_0123,
    MERGED_0_SEAM,
    MERGED_COUNT
} MergedTileVariant;

typedef enum {
    CORNER_TL = 0,
    CORNER_TR,
    CORNER_BR,
    CORNER_BL
} TileCorner;

typedef struct {
    MergedTileVariant tl;
    MergedTileVariant tr;
    MergedTileVariant br;
    MergedTileVariant bl;
} CornerValues;

typedef struct {
    Texture2D background;
    Texture2D tiles_board;
    TileFrame grid_frame;
    TileFrame merged_large[MERGED_COUNT];
    TileFrame merged_hover[MERGED_COUNT];
} Board;

void InitBoard(Board* board);
void UnloadBoard(Board* board);
void DrawBackground(Board* board);
void DrawBoardFloor(RenderState* render);
void DrawBoardGrid(Board* board, RenderState* render);
bool IsValidBoardPos(int col, int row);

CornerValues GetMergedTileCornerValues(bool move_tiles[BOARD_COLS][BOARD_ROWS], int col, int row,
                                       bool hover_tiles[BOARD_COLS][BOARD_ROWS]);
void DrawMergedTileCorner(Board* board, RenderState* render, Vector2 tile_center,
                          TileFrame frame, TileCorner corner, Color tint);
void DrawMergedMoveTiles(Board* board, RenderState* render,
                         bool move_tiles[BOARD_COLS][BOARD_ROWS],
                         bool hover_tiles[BOARD_COLS][BOARD_ROWS],
                         Color move_color, Color hover_color);

#endif
