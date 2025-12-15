#include "types.hpp"
#include <cmath>

Vec2 board_to_screen(int window_w, int window_h, BoardPos pos) {
    float board_origin_x = (window_w - BOARD_COLS * TILE_SIZE) * 0.5f + TILE_SIZE * 0.5f + TILE_OFFSET_X;
    float board_origin_y = (window_h - BOARD_ROWS * TILE_SIZE) * 0.5f + TILE_SIZE * 0.5f + TILE_OFFSET_Y;
    
    return {
        pos.x * TILE_SIZE + board_origin_x,
        pos.y * TILE_SIZE + board_origin_y
    };
}

BoardPos screen_to_board(int window_w, int window_h, Vec2 screen) {
    float board_origin_x = (window_w - BOARD_COLS * TILE_SIZE) * 0.5f + TILE_SIZE * 0.5f + TILE_OFFSET_X;
    float board_origin_y = (window_h - BOARD_ROWS * TILE_SIZE) * 0.5f + TILE_SIZE * 0.5f + TILE_OFFSET_Y;
    
    return {
        static_cast<int>(std::floor((screen.x - board_origin_x + TILE_SIZE * 0.5f) / TILE_SIZE)),
        static_cast<int>(std::floor((screen.y - board_origin_y + TILE_SIZE * 0.5f) / TILE_SIZE))
    };
}

float calculate_move_duration(float anim_duration, int tile_count) {
    float base = anim_duration * ENTITY_MOVE_DURATION_MODIFIER;
    float correction = base * ENTITY_MOVE_CORRECTION;
    return base * (tile_count + 1) - correction;
}
