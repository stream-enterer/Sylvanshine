#pragma once
#include <cstdint>
#include <vector>
#include <SDL3/SDL.h>

constexpr int BOARD_COLS = 9;
constexpr int BOARD_ROWS = 5;
constexpr int TILE_SIZE = 95;
constexpr float TILE_OFFSET_X = 0.0f;
constexpr float TILE_OFFSET_Y = 10.0f;

constexpr float ENTITY_MOVE_DURATION_MODIFIER = 1.0f;
constexpr float ENTITY_MOVE_CORRECTION = 0.2f;

struct RenderConfig {
    int window_w;
    int window_h;
    int scale;
    
    int tile_size() const { return TILE_SIZE * scale; }
    float tile_offset_x() const { return TILE_OFFSET_X * scale; }
    float tile_offset_y() const { return TILE_OFFSET_Y * scale; }
};

struct Vec2 {
    float x, y;
    
    Vec2 operator+(Vec2 other) const { return {x + other.x, y + other.y}; }
    Vec2 operator-(Vec2 other) const { return {x - other.x, y - other.y}; }
    Vec2 operator*(float s) const { return {x * s, y * s}; }
    float length() const { return SDL_sqrtf(x * x + y * y); }
};

struct BoardPos {
    int x, y;
    
    bool operator==(BoardPos other) const { return x == other.x && y == other.y; }
    bool operator!=(BoardPos other) const { return !(*this == other); }
    bool is_valid() const {
        return x >= 0 && x < BOARD_COLS && y >= 0 && y < BOARD_ROWS;
    }
};

struct AnimFrame {
    int idx;
    SDL_Rect rect;
};

struct Animation {
    char name[32];
    int fps;
    std::vector<AnimFrame> frames;
    
    float duration() const { return frames.size() / static_cast<float>(fps); }
};

Vec2 board_to_screen(const RenderConfig& config, BoardPos pos);
BoardPos screen_to_board(const RenderConfig& config, Vec2 screen);
float calculate_move_duration(float anim_duration, int tile_count);
