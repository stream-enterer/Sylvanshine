#include "types.hpp"
#include <cmath>

Vec2 board_to_screen(const RenderConfig& config, BoardPos pos) {
    int ts = config.tile_size();
    float board_origin_x = (config.window_w - BOARD_COLS * ts) * 0.5f + ts * 0.5f + config.tile_offset_x();
    float board_origin_y = (config.window_h - BOARD_ROWS * ts) * 0.5f + ts * 0.5f + config.tile_offset_y();
    
    return {
        pos.x * ts + board_origin_x,
        pos.y * ts + board_origin_y
    };
}

BoardPos screen_to_board(const RenderConfig& config, Vec2 screen) {
    int ts = config.tile_size();
    float board_origin_x = (config.window_w - BOARD_COLS * ts) * 0.5f + ts * 0.5f + config.tile_offset_x();
    float board_origin_y = (config.window_h - BOARD_ROWS * ts) * 0.5f + ts * 0.5f + config.tile_offset_y();
    
    return {
        static_cast<int>(std::floor((screen.x - board_origin_x + ts * 0.5f) / ts)),
        static_cast<int>(std::floor((screen.y - board_origin_y + ts * 0.5f) / ts))
    };
}

float calculate_move_duration(float anim_duration, int tile_count) {
    float base = anim_duration * ENTITY_MOVE_DURATION_MODIFIER;
    float correction = base * ENTITY_MOVE_CORRECTION;
    return base * (tile_count + 1) - correction;
}
