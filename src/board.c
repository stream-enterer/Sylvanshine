#include "board.h"

#include <stddef.h>

void InitBoard(Board* board) {
    board->background = LoadTexture("data/maps/battlemap0_background.png");
    board->tile_move = LoadTexture("data/tiles/tile_mana.png");
    board->tiles_board = LoadTexture("data/tiles/tiles_board.png");

    board->grid_frame = (TileFrame){2, 126, 122, 122, false};
    board->hover_frame = (TileFrame){126, 2, 122, 122, false};

    board->merged_large[MERGED_0] = (TileFrame){532, 128, 61, 61, false};
    board->merged_large[MERGED_01] = (TileFrame){472, 30, 64, 61, false};
    board->merged_large[MERGED_03] = (TileFrame){388, 159, 61, 64, true};
    board->merged_large[MERGED_013] = (TileFrame){466, 93, 64, 64, false};
    board->merged_large[MERGED_0123] = (TileFrame){400, 93, 64, 64, false};
    board->merged_large[MERGED_0_SEAM] = (TileFrame){517, 191, 61, 61, false};

    board->merged_hover[MERGED_0] = (TileFrame){454, 159, 61, 61, false};
    board->merged_hover[MERGED_01] = (TileFrame){406, 30, 64, 61, false};
    board->merged_hover[MERGED_03] = (TileFrame){538, 2, 61, 64, true};
    board->merged_hover[MERGED_013] = (TileFrame){322, 140, 64, 64, false};
    board->merged_hover[MERGED_0123] = (TileFrame){334, 74, 64, 64, false};
    board->merged_hover[MERGED_0_SEAM] = (TileFrame){538, 65, 61, 61, false};
}

void UnloadBoard(Board* board) {
    UnloadTexture(board->background);
    UnloadTexture(board->tile_move);
    UnloadTexture(board->tiles_board);
}

void DrawBackground(Board* board) {
    float ref_width = 1280.0f;
    float ref_height = 720.0f;

    float scale = (float)SCREEN_WIDTH / ref_width;
    if ((float)SCREEN_HEIGHT / ref_height < scale) {
        scale = (float)SCREEN_HEIGHT / ref_height;
    }

    float w = board->background.width * scale;
    float h = board->background.height * scale;

    float x = (SCREEN_WIDTH - w) * 0.5f;
    float y = 0.0f;

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

static bool IsTileInMap(bool tiles[BOARD_COLS][BOARD_ROWS], int col, int row) {
    if (!IsValidBoardPos(col, row)) return false;
    return tiles[col][row];
}

static MergedTileVariant GetCornerVariant(bool has_cardinal1, bool has_cardinal2, bool has_diagonal,
                                          bool is_seam) {
    if (is_seam) {
        return MERGED_0_SEAM;
    }

    if (has_cardinal1 && has_cardinal2 && has_diagonal) {
        return MERGED_0123;
    }
    if (has_cardinal1 && has_cardinal2) {
        return MERGED_013;
    }
    if (has_cardinal1 && has_diagonal) {
        return MERGED_01;
    }
    if (has_cardinal2 && has_diagonal) {
        return MERGED_03;
    }
    return MERGED_0;
}

CornerValues GetMergedTileCornerValues(bool move_tiles[BOARD_COLS][BOARD_ROWS], int col, int row,
                                       bool hover_tiles[BOARD_COLS][BOARD_ROWS]) {
    CornerValues values = {MERGED_0, MERGED_0, MERGED_0, MERGED_0};

    bool has_l = IsTileInMap(move_tiles, col - 1, row);
    bool has_r = IsTileInMap(move_tiles, col + 1, row);
    bool has_t = IsTileInMap(move_tiles, col, row - 1);
    bool has_b = IsTileInMap(move_tiles, col, row + 1);
    bool has_tl = IsTileInMap(move_tiles, col - 1, row - 1);
    bool has_tr = IsTileInMap(move_tiles, col + 1, row - 1);
    bool has_bl = IsTileInMap(move_tiles, col - 1, row + 1);
    bool has_br = IsTileInMap(move_tiles, col + 1, row + 1);

    bool seam_l = hover_tiles && IsTileInMap(hover_tiles, col - 1, row);
    bool seam_r = hover_tiles && IsTileInMap(hover_tiles, col + 1, row);
    bool seam_t = hover_tiles && IsTileInMap(hover_tiles, col, row - 1);
    bool seam_b = hover_tiles && IsTileInMap(hover_tiles, col, row + 1);
    bool seam_tl = hover_tiles && IsTileInMap(hover_tiles, col - 1, row - 1);
    bool seam_tr = hover_tiles && IsTileInMap(hover_tiles, col + 1, row - 1);
    bool seam_bl = hover_tiles && IsTileInMap(hover_tiles, col - 1, row + 1);
    bool seam_br = hover_tiles && IsTileInMap(hover_tiles, col + 1, row + 1);

    bool tl_seam = seam_l || seam_t || seam_tl;
    bool tr_seam = seam_r || seam_t || seam_tr;
    bool br_seam = seam_r || seam_b || seam_br;
    bool bl_seam = seam_l || seam_b || seam_bl;

    values.tl = GetCornerVariant(has_l, has_t, has_tl, tl_seam);
    values.tr = GetCornerVariant(has_r, has_t, has_tr, tr_seam);
    values.br = GetCornerVariant(has_r, has_b, has_br, br_seam);
    values.bl = GetCornerVariant(has_l, has_b, has_bl, bl_seam);

    return values;
}

void DrawMergedTileCorner(Board* board, RenderState* render, int col, int row,
                          TileFrame frame, TileCorner corner, Color tint) {
    Vector3 pos = BoardToWorld(render, col, row);
    pos.y = 0.002f;

    float quarter_size = 0.25f;
    float half_size = 0.5f;

    float offset_x = 0.0f;
    float offset_z = 0.0f;
    float rotation = 0.0f;

    switch (corner) {
        case CORNER_TL:
            offset_x = -quarter_size;
            offset_z = -quarter_size;
            rotation = 0.0f;
            break;
        case CORNER_TR:
            offset_x = quarter_size;
            offset_z = -quarter_size;
            rotation = 90.0f;
            break;
        case CORNER_BR:
            offset_x = quarter_size;
            offset_z = quarter_size;
            rotation = 180.0f;
            break;
        case CORNER_BL:
            offset_x = -quarter_size;
            offset_z = quarter_size;
            rotation = 270.0f;
            break;
    }

    pos.x += offset_x;
    pos.z += offset_z;

    float src_w = (float)frame.w;
    float src_h = (float)frame.h;
    if (frame.rotated) {
        float temp = src_w;
        src_w = src_h;
        src_h = temp;
    }

    Rectangle src = {(float)frame.x, (float)frame.y, src_w, src_h};

    DrawFloorTexturedQuadRotated(board->tiles_board, src, pos, half_size, half_size,
                                 rotation, frame.rotated, tint);
}

void DrawMergedMoveTiles(Board* board, RenderState* render,
                         bool move_tiles[BOARD_COLS][BOARD_ROWS],
                         bool hover_tiles[BOARD_COLS][BOARD_ROWS],
                         Color move_color, Color hover_color) {
    for (int col = 0; col < BOARD_COLS; col++) {
        for (int row = 0; row < BOARD_ROWS; row++) {
            bool is_move = move_tiles[col][row];
            bool is_hover = hover_tiles && hover_tiles[col][row];

            if (!is_move && !is_hover) continue;

            TileFrame* frames;
            Color tint;
            bool hover_as_alt = false;

            if (is_hover) {
                frames = board->merged_hover;
                tint = hover_color;
                hover_as_alt = is_move;
            } else {
                frames = board->merged_large;
                tint = move_color;
            }

            CornerValues corners = GetMergedTileCornerValues(
                is_hover ? hover_tiles : move_tiles,
                col, row,
                hover_as_alt ? move_tiles : NULL
            );

            DrawMergedTileCorner(board, render, col, row, frames[corners.tl], CORNER_TL, tint);
            DrawMergedTileCorner(board, render, col, row, frames[corners.tr], CORNER_TR, tint);
            DrawMergedTileCorner(board, render, col, row, frames[corners.br], CORNER_BR, tint);
            DrawMergedTileCorner(board, render, col, row, frames[corners.bl], CORNER_BL, tint);
        }
    }
}
