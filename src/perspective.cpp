#include "perspective.hpp"

float calculate_zeye(float height) {
    float half_fov_rad = (FOV_DEGREES * 0.5f) * DEG_TO_RAD;
    return (height * 0.5f) / std::tan(half_fov_rad);
}

PerspectiveConfig PerspectiveConfig::for_board(const RenderConfig& config) {
    return {
        calculate_zeye(static_cast<float>(config.window_h)),
        config.window_w * 0.5f,
        config.window_h * 0.5f,
        BOARD_X_ROTATION
    };
}

PerspectiveConfig PerspectiveConfig::for_entities(const RenderConfig& config) {
    return {
        calculate_zeye(static_cast<float>(config.window_h)),
        config.window_w * 0.5f,
        config.window_h * 0.5f,
        ENTITY_X_ROTATION
    };
}

Vec2 apply_perspective_transform(Vec2 point, float z, const PerspectiveConfig& persp) {
    float rel_x = point.x - persp.center_x;
    float rel_y = point.y - persp.center_y;
    
    float angle_rad = persp.rotation_deg * DEG_TO_RAD;
    float cos_a = std::cos(angle_rad);
    float sin_a = std::sin(angle_rad);
    
    float rotated_y = rel_y * cos_a - z * sin_a;
    float rotated_z = rel_y * sin_a + z * cos_a;
    
    float depth = persp.zeye - rotated_z;
    if (depth < 1.0f) depth = 1.0f;
    
    float scale = persp.zeye / depth;
    
    float proj_x = rel_x * scale + persp.center_x;
    float proj_y = rotated_y * scale + persp.center_y;
    
    return {proj_x, proj_y};
}

TransformedQuad transform_rect_perspective(float x, float y, float w, float h,
                                           const PerspectiveConfig& persp) {
    TransformedQuad quad;
    
    quad.corners[0] = apply_perspective_transform({x, y}, 0.0f, persp);
    quad.corners[1] = apply_perspective_transform({x + w, y}, 0.0f, persp);
    quad.corners[2] = apply_perspective_transform({x + w, y + h}, 0.0f, persp);
    quad.corners[3] = apply_perspective_transform({x, y + h}, 0.0f, persp);
    
    return quad;
}
