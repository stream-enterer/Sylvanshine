#include "board.h"

void InitBoard(Board* board) {
    board->background = LoadTexture("data/maps/battlemap0_background.png");
    board->origin_x = (SCREEN_WIDTH - BOARD_COLS * TILE_SIZE) * 0.5f + TILE_SIZE * 0.5f + TILE_OFFSET_X;
    board->origin_y = (SCREEN_HEIGHT - BOARD_ROWS * TILE_SIZE) * 0.5f + TILE_SIZE * 0.5f + TILE_OFFSET_Y;
}

void UnloadBoard(Board* board) {
    UnloadTexture(board->background);
}

void DrawBoard(Board* board) {
    float bg_x = (SCREEN_WIDTH - board->background.width) * 0.5f;
    float bg_y = (SCREEN_HEIGHT - board->background.height) * 0.5f;
    DrawTexture(board->background, (int)bg_x, (int)bg_y, WHITE);
}

void DrawTileHighlight(Board* board, int col, int row, Color color) {
    Vector2 pos = BoardToScreen(board, col, row);
    float half_tile = TILE_SIZE * 0.5f;
    DrawRectangle(
        (int)(pos.x - half_tile),
        (int)(pos.y - half_tile),
        TILE_SIZE,
        TILE_SIZE,
        color
    );
}

Vector2 BoardToScreen(Board* board, int col, int row) {
    Vector2 result;
    result.x = col * TILE_SIZE + board->origin_x;
    result.y = row * TILE_SIZE + board->origin_y;
    return result;
}

BoardPos ScreenToBoard(Board* board, float screen_x, float screen_y) {
    BoardPos result;
    result.x = (int)((screen_x - board->origin_x + TILE_SIZE * 0.5f) / TILE_SIZE);
    result.y = (int)((screen_y - board->origin_y + TILE_SIZE * 0.5f) / TILE_SIZE);
    return result;
}

bool IsValidBoardPos(int col, int row) {
    return col >= 0 && col < BOARD_COLS && row >= 0 && row < BOARD_ROWS;
}
