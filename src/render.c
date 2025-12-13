#include "render.h"

#include <math.h>

#include <rlgl.h>

void InitRenderState(RenderState* state) {
    state->board_width = (float)BOARD_COLS;
    state->board_height = (float)BOARD_ROWS;
    state->board_center = (Vector3){BOARD_COLS * 0.5f, 0.0f, BOARD_ROWS * 0.5f};

    state->camera.position = (Vector3){BOARD_COLS * 0.5f, 8.0f, BOARD_ROWS * 0.5f + 10.0f};
    state->camera.target = (Vector3){BOARD_COLS * 0.5f, 0.0f, BOARD_ROWS * 0.5f};
    state->camera.up = (Vector3){0.0f, 1.0f, 0.0f};
    state->camera.fovy = CAMERA_FOVY;
    state->camera.projection = CAMERA_ORTHOGRAPHIC;
}

Vector3 BoardToWorld(RenderState* state, int col, int row) {
    (void)state;

    Vector3 pos;
    pos.x = col + 0.5f;
    pos.y = 0.0f;
    pos.z = row + 0.5f;

    return pos;
}

Vector3 ScreenPosToWorld(RenderState* state, float screen_x, float screen_y) {
    Ray ray = GetMouseRay((Vector2){screen_x, screen_y}, state->camera);

    float board_angle_rad = BOARD_XYZ_ROTATION * DEG2RAD;
    Vector3 plane_normal = {0.0f, cosf(board_angle_rad), -sinf(board_angle_rad)};
    float plane_d = 0.0f;

    float denom = plane_normal.x * ray.direction.x +
                  plane_normal.y * ray.direction.y +
                  plane_normal.z * ray.direction.z;

    if (fabsf(denom) < 0.0001f) {
        return (Vector3){0, 0, 0};
    }

    float t = -(plane_normal.x * ray.position.x +
                plane_normal.y * ray.position.y +
                plane_normal.z * ray.position.z + plane_d) / denom;

    Vector3 result;
    result.x = ray.position.x + ray.direction.x * t;
    result.y = ray.position.y + ray.direction.y * t;
    result.z = ray.position.z + ray.direction.z * t;

    return result;
}

void DrawTexturedQuad(Texture2D texture, Rectangle src, Vector3 center, float width, float height, float x_rotation, Color tint, bool flip_h) {
    rlSetTexture(texture.id);

    float angle_rad = x_rotation * DEG2RAD;
    float cos_a = cosf(angle_rad);
    float sin_a = sinf(angle_rad);

    float hw = width * 0.5f;
    float hh = height * 0.5f;

    float tex_left = src.x / texture.width;
    float tex_right = (src.x + fabsf(src.width)) / texture.width;
    float tex_top = src.y / texture.height;
    float tex_bottom = (src.y + fabsf(src.height)) / texture.height;

    if (flip_h) {
        float tmp = tex_left;
        tex_left = tex_right;
        tex_right = tmp;
    }

    float top_y = hh * cos_a;
    float top_z = -hh * sin_a;
    float bot_y = -hh * cos_a;
    float bot_z = hh * sin_a;

    rlBegin(RL_QUADS);
    rlColor4ub(tint.r, tint.g, tint.b, tint.a);

    rlTexCoord2f(tex_left, tex_top);
    rlVertex3f(center.x - hw, center.y + top_y, center.z + top_z);

    rlTexCoord2f(tex_left, tex_bottom);
    rlVertex3f(center.x - hw, center.y + bot_y, center.z + bot_z);

    rlTexCoord2f(tex_right, tex_bottom);
    rlVertex3f(center.x + hw, center.y + bot_y, center.z + bot_z);

    rlTexCoord2f(tex_right, tex_top);
    rlVertex3f(center.x + hw, center.y + top_y, center.z + top_z);

    rlEnd();
    rlSetTexture(0);
}

void DrawColoredQuad(Vector3 center, float width, float height, float x_rotation, Color color) {
    float angle_rad = x_rotation * DEG2RAD;
    float cos_a = cosf(angle_rad);
    float sin_a = sinf(angle_rad);

    float hw = width * 0.5f;
    float hh = height * 0.5f;

    float top_y = hh * cos_a;
    float top_z = -hh * sin_a;
    float bot_y = -hh * cos_a;
    float bot_z = hh * sin_a;

    rlBegin(RL_QUADS);
    rlColor4ub(color.r, color.g, color.b, color.a);

    rlVertex3f(center.x - hw, center.y + top_y, center.z + top_z);
    rlVertex3f(center.x - hw, center.y + bot_y, center.z + bot_z);
    rlVertex3f(center.x + hw, center.y + bot_y, center.z + bot_z);
    rlVertex3f(center.x + hw, center.y + top_y, center.z + top_z);

    rlEnd();
}
