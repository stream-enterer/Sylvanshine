#include "types.hpp"
#include <cmath>

constexpr float FOV_DEGREES = 60.0f;
constexpr float BOARD_X_ROTATION = 16.0f;
constexpr float DEG_TO_RAD = 3.14159265359f / 180.0f;

static float calculate_zeye(float height) {
    float half_fov_rad = (FOV_DEGREES * 0.5f) * DEG_TO_RAD;
    return (height * 0.5f) / std::tan(half_fov_rad);
}

float RenderConfig::board_origin_y() const {
    float zeye = calculate_zeye(static_cast<float>(window_h));
    float center_y = window_h * 0.5f;
    float H = board_height();
    
    float angle_rad = BOARD_X_ROTATION * DEG_TO_RAD;
    float s = std::sin(angle_rad);
    
    float discriminant = zeye * zeye + s * s * H * H;
    float a = (zeye - s * H - std::sqrt(discriminant)) / (2.0f * s);
    
    return a + center_y;
}

static Vec2 apply_perspective(Vec2 point, float center_x, float center_y, float zeye, float rotation_deg) {
    float rel_x = point.x - center_x;
    float rel_y = point.y - center_y;
    
    float angle_rad = rotation_deg * DEG_TO_RAD;
    float cos_a = std::cos(angle_rad);
    float sin_a = std::sin(angle_rad);
    
    float rotated_y = rel_y * cos_a;
    float rotated_z = rel_y * sin_a;
    
    float depth = zeye - rotated_z;
    if (depth < 1.0f) depth = 1.0f;
    
    float scale = zeye / depth;
    
    return {
        rel_x * scale + center_x,
        rotated_y * scale + center_y
    };
}

static Vec2 inverse_perspective(Vec2 screen_point, float center_x, float center_y, float zeye, float rotation_deg) {
    float proj_x = screen_point.x - center_x;
    float proj_y = screen_point.y - center_y;
    
    float angle_rad = rotation_deg * DEG_TO_RAD;
    float cos_a = std::cos(angle_rad);
    float sin_a = std::sin(angle_rad);
    
    float a = cos_a;
    float b = -sin_a;
    
    float denom = zeye * a - proj_y * b;
    if (std::abs(denom) < 0.001f) {
        return {screen_point.x, screen_point.y};
    }
    
    float rel_y = (proj_y * zeye) / denom;
    float depth = zeye - rel_y * sin_a;
    float rel_x = proj_x * depth / zeye;
    
    return {
        rel_x + center_x,
        rel_y + center_y
    };
}

Vec2 board_to_screen(const RenderConfig& config, BoardPos pos) {
    int ts = config.tile_size();
    float origin_x = config.board_origin_x() + ts * 0.5f;
    float origin_y = config.board_origin_y() + ts * 0.5f;
    
    return {
        pos.x * ts + origin_x,
        pos.y * ts + origin_y
    };
}

Vec2 board_to_screen_perspective(const RenderConfig& config, BoardPos pos) {
    Vec2 flat = board_to_screen(config, pos);
    
    float center_x = config.window_w * 0.5f;
    float center_y = config.window_h * 0.5f;
    float zeye = calculate_zeye(static_cast<float>(config.window_h));
    
    return apply_perspective(flat, center_x, center_y, zeye, BOARD_X_ROTATION);
}

BoardPos screen_to_board(const RenderConfig& config, Vec2 screen) {
    int ts = config.tile_size();
    float origin_x = config.board_origin_x() + ts * 0.5f;
    float origin_y = config.board_origin_y() + ts * 0.5f;
    
    return {
        static_cast<int>(std::floor((screen.x - origin_x + ts * 0.5f) / ts)),
        static_cast<int>(std::floor((screen.y - origin_y + ts * 0.5f) / ts))
    };
}

BoardPos screen_to_board_perspective(const RenderConfig& config, Vec2 screen) {
    float center_x = config.window_w * 0.5f;
    float center_y = config.window_h * 0.5f;
    float zeye = calculate_zeye(static_cast<float>(config.window_h));
    
    Vec2 flat = inverse_perspective(screen, center_x, center_y, zeye, BOARD_X_ROTATION);
    return screen_to_board(config, flat);
}

float calculate_move_duration(float anim_duration, int tile_count) {
    float base = anim_duration * ENTITY_MOVE_DURATION_MODIFIER;
    float correction = base * ENTITY_MOVE_CORRECTION;
    return base * (tile_count + 1) - correction;
}
