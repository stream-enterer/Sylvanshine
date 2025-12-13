#include "board.h"

void InitBoard(Board* board) {
    board->background = LoadTexture("data/maps/battlemap0_background.png");
    board->tile_move = LoadTexture("data/tiles/tile_mana.png");
    board->tiles_board = LoadTexture("data/tiles/tiles_board.png");

    board->grid_frame = (TileFrame){2, 126, 122, 122};
    board->hover_frame = (TileFrame){126, 2, 122, 122};
}

void UnloadBoard(Board* board) {
    UnloadTexture(board->background);
    UnloadTexture(board->tile_move);
    UnloadTexture(board->tiles_board);
}

void DrawBackground(Board* board) {
    float scale_x = (float)SCREEN_WIDTH / board->background.width;
    float scale_y = (float)SCREEN_HEIGHT / board->background.height;
    float scale = scale_x > scale_y ? scale_x : scale_y;

    float w = board->background.width * scale;
    float h = board->background.height * scale;
    float x = (SCREEN_WIDTH - w) * 0.5f;
    float y = (SCREEN_HEIGHT - h) * 0.5f;

    Rectangle src = {0, 0, (float)board->background.width, (float)board->background.height};
    Rectangle dest = {x, y, w, h};
    DrawTexturePro(board->background, src, dest, (Vector2){0, 0}, 0, WHITE);
}

void DrawTileHighlight(Board* board, RenderState* render, int col, int row, Color color) {
    (void)board;

    Vector3 pos = BoardToWorld(render, col, row);
    pos.y = 0.002f;

    DrawFloorQuad(pos, 0.95f, 0.95f, color);
}

void DrawTileSprite(Board* board, RenderState* render, int col, int row, Color tint) {
    Vector3 pos = BoardToWorld(render, col, row);
    pos.y = 0.002f;

    Rectangle src = {0, 0, (float)board->tile_move.width, (float)board->tile_move.height};

    DrawFloorTexturedQuad(board->tile_move, src, pos, 1.0f, 1.0f, tint);
}

void DrawTileFromSheet(Board* board, RenderState* render, int col, int row, TileFrame frame, Color tint) {
    Vector3 pos = BoardToWorld(render, col, row);
    pos.y = 0.001f;

    Rectangle src = {(float)frame.x, (float)frame.y, (float)frame.w, (float)frame.h};

    DrawFloorTexturedQuad(board->tiles_board, src, pos, 1.0f, 1.0f, tint);
}

void DrawBoardGrid(Board* board, RenderState* render) {
    Color grid_tint = {GRID_COLOR_R, GRID_COLOR_G, GRID_COLOR_B, GRID_COLOR_A};

    for (int col = 0; col < BOARD_COLS; col++) {
        for (int row = 0; row < BOARD_ROWS; row++) {
            DrawTileFromSheet(board, render, col, row, board->grid_frame, grid_tint);
        }
    }
}

BoardPos WorldToBoard(RenderState* render, Vector3 world_pos) {
    (void)render;

    BoardPos result;
    result.x = (int)world_pos.x;
    result.y = (int)world_pos.z;

    return result;
}

bool IsValidBoardPos(int col, int row) {
    return col >= 0 && col < BOARD_COLS && row >= 0 && row < BOARD_ROWS;
}
