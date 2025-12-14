#ifndef TYPES_HPP
#define TYPES_HPP

#include <glm/glm.hpp>

#include <cstdint>
#include <string>
#include <vector>

struct AnimationFrame {
    int index;
    int x;
    int y;
    int width;
    int height;
};

struct Animation {
    std::string name;
    int fps;
    std::vector<AnimationFrame> frames;
};

struct SoundMapping {
    std::string event_name;
    std::string path;
};

enum class UnitState {
    SPAWNING,
    IDLE,
    MOVING,
    ATTACKING,
    HIT,
    DYING,
    DEAD
};

enum class TileHighlight {
    NONE,
    MOVE,
    ATTACK,
    SELECTED
};

struct BoardPosition {
    int x;
    int y;
};

inline bool operator==(const BoardPosition& a, const BoardPosition& b) {
    return a.x == b.x && a.y == b.y;
}

inline bool operator!=(const BoardPosition& a, const BoardPosition& b) {
    return !(a == b);
}

constexpr int BOARD_COLS = 9;
constexpr int BOARD_ROWS = 5;
constexpr int TILE_SIZE = 95;
constexpr float TILE_OFFSET_X = 0.0f;
constexpr float TILE_OFFSET_Y = 10.0f;
constexpr int BOARD_CENTER_X = 4;
constexpr int BOARD_CENTER_Y = 2;

constexpr float BOARD_X_ROTATION = 16.0f;
constexpr float ENTITY_X_ROTATION = 26.0f;

constexpr int WINDOW_WIDTH = 1280;
constexpr int WINDOW_HEIGHT = 720;

constexpr uint8_t TILE_SELECT_OPACITY = 200;
constexpr uint8_t TILE_HOVER_OPACITY = 200;
constexpr uint8_t TILE_DIM_OPACITY = 127;

constexpr float DEFAULT_FRAME_DELAY = 0.08f;
constexpr float SHADOW_FADE_DURATION = 0.3f;
constexpr int DEFAULT_SHADOW_OFFSET = 40;
constexpr uint8_t SHADOW_OPACITY = 200;

#endif
