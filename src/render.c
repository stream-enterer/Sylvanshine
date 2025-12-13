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

    if (fabsf(ray.direction.y) < 0.0001f) {
        return (Vector3){0, 0, 0};
    }

    float t = -ray.position.y / ray.direction.y;

    Vector3 result;
    result.x = ray.position.x + ray.direction.x * t;
    result.y = 0.0f;
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

void DrawFloorQuad(Vector3 center, float width, float depth, Color color) {
    float hw = width * 0.5f;
    float hd = depth * 0.5f;

    rlBegin(RL_QUADS);
    rlColor4ub(color.r, color.g, color.b, color.a);

    rlVertex3f(center.x - hw, center.y, center.z - hd);
    rlVertex3f(center.x - hw, center.y, center.z + hd);
    rlVertex3f(center.x + hw, center.y, center.z + hd);
    rlVertex3f(center.x + hw, center.y, center.z - hd);

    rlEnd();
}

void DrawFloorTexturedQuad(Texture2D texture, Rectangle src, Vector3 center, float width, float depth, Color tint) {
    rlSetTexture(texture.id);

    float hw = width * 0.5f;
    float hd = depth * 0.5f;

    float tex_left = src.x / (float)texture.width;
    float tex_right = (src.x + src.width) / (float)texture.width;
    float tex_top = src.y / (float)texture.height;
    float tex_bottom = (src.y + src.height) / (float)texture.height;

    rlBegin(RL_QUADS);
    rlColor4ub(tint.r, tint.g, tint.b, tint.a);

    rlTexCoord2f(tex_left, tex_top);
    rlVertex3f(center.x - hw, center.y, center.z - hd);

    rlTexCoord2f(tex_left, tex_bottom);
    rlVertex3f(center.x - hw, center.y, center.z + hd);

    rlTexCoord2f(tex_right, tex_bottom);
    rlVertex3f(center.x + hw, center.y, center.z + hd);

    rlTexCoord2f(tex_right, tex_top);
    rlVertex3f(center.x + hw, center.y, center.z - hd);

    rlEnd();
    rlSetTexture(0);
}

void DrawFloorTexturedQuadRotated(Texture2D texture, Rectangle src, Vector3 center, float width, float depth, float y_rotation, bool atlas_rotated, Color tint) {
    rlSetTexture(texture.id);

    float hw = width * 0.5f;
    float hd = depth * 0.5f;

    float angle_rad = y_rotation * DEG2RAD;
    float cos_a = cosf(angle_rad);
    float sin_a = sinf(angle_rad);

    float tex_left = src.x / (float)texture.width;
    float tex_right = (src.x + fabsf(src.width)) / (float)texture.width;
    float tex_top = src.y / (float)texture.height;
    float tex_bottom = (src.y + fabsf(src.height)) / (float)texture.height;

    float corners[4][2] = {
        {-hw, -hd},
        {-hw,  hd},
        { hw,  hd},
        { hw, -hd}
    };

    float rotated[4][2];
    for (int i = 0; i < 4; i++) {
        rotated[i][0] = corners[i][0] * cos_a - corners[i][1] * sin_a;
        rotated[i][1] = corners[i][0] * sin_a + corners[i][1] * cos_a;
    }

    rlBegin(RL_QUADS);
    rlColor4ub(tint.r, tint.g, tint.b, tint.a);

    if (atlas_rotated) {
        rlTexCoord2f(tex_left, tex_bottom);
        rlVertex3f(center.x + rotated[0][0], center.y, center.z + rotated[0][1]);

        rlTexCoord2f(tex_right, tex_bottom);
        rlVertex3f(center.x + rotated[1][0], center.y, center.z + rotated[1][1]);

        rlTexCoord2f(tex_right, tex_top);
        rlVertex3f(center.x + rotated[2][0], center.y, center.z + rotated[2][1]);

        rlTexCoord2f(tex_left, tex_top);
        rlVertex3f(center.x + rotated[3][0], center.y, center.z + rotated[3][1]);
    } else {
        rlTexCoord2f(tex_left, tex_top);
        rlVertex3f(center.x + rotated[0][0], center.y, center.z + rotated[0][1]);

        rlTexCoord2f(tex_left, tex_bottom);
        rlVertex3f(center.x + rotated[1][0], center.y, center.z + rotated[1][1]);

        rlTexCoord2f(tex_right, tex_bottom);
        rlVertex3f(center.x + rotated[2][0], center.y, center.z + rotated[2][1]);

        rlTexCoord2f(tex_right, tex_top);
        rlVertex3f(center.x + rotated[3][0], center.y, center.z + rotated[3][1]);
    }

    rlEnd();
    rlSetTexture(0);
}
