#pragma once
#include "types.hpp"
#include <cmath>
#include <array>

constexpr float FOV_DEGREES = 60.0f;
constexpr float BOARD_X_ROTATION = 16.0f;
constexpr float ENTITY_X_ROTATION = 26.0f;
constexpr float DEG_TO_RAD = 3.14159265359f / 180.0f;

struct PerspectiveConfig {
    float zeye;
    float center_x;
    float center_y;
    float rotation_deg;
    
    static PerspectiveConfig for_board(const RenderConfig& config);
    static PerspectiveConfig for_entities(const RenderConfig& config);
};

struct TransformedQuad {
    std::array<Vec2, 4> corners;
};

Vec2 apply_perspective_transform(Vec2 point, float z, const PerspectiveConfig& persp);
TransformedQuad transform_rect_perspective(float x, float y, float w, float h, 
                                           const PerspectiveConfig& persp);
float calculate_zeye(float height);
