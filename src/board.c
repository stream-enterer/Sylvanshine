#include "board.h"

void InitBoard(Board* board) {
    board->background = LoadTexture("data/maps/battlemap0_background.png");
    board->tile_move = LoadTexture("data/tiles/tile_mana.png");
}

void UnloadBoard(Board* board) {
    UnloadTexture(board->background);
    UnloadTexture(board->tile_move);
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
    (void)render;

    Vector3 pos = BoardToWorld(render, col, row);
    pos.y = 0.002f;

    float tile_size = 0.95f;
    DrawColoredQuad(pos, tile_size, tile_size, BOARD_XYZ_ROTATION, color);
}

void DrawTileSprite(Board* board, RenderState* render, int col, int row, Color tint) {
    Vector3 pos = BoardToWorld(render, col, row);
    pos.y = 0.002f;

    Rectangle src = {0, 0, (float)board->tile_move.width, (float)board->tile_move.height};

    DrawTexturedQuad(board->tile_move, src, pos, 1.0f, 1.0f, BOARD_XYZ_ROTATION, tint, false);
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
