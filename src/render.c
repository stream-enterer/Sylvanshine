#include "render.h"

#include <math.h>

#include <rlgl.h>

void InitRenderState(RenderState* state) {
    float board_pixel_width = BOARD_COLS * TILE_SIZE;
    float board_pixel_height = BOARD_ROWS * TILE_SIZE;

    state->board_origin_x = (SCREEN_WIDTH - board_pixel_width) * 0.5f + TILE_SIZE * 0.5f + TILE_OFFSET_X;
    state->board_origin_y = (SCREEN_HEIGHT - board_pixel_height) * 0.5f + TILE_SIZE * 0.5f + TILE_OFFSET_Y;
    state->board_center_y = state->board_origin_y + (BOARD_ROWS - 1) * 0.5f * TILE_SIZE;
}

Vector2 TileCenterScreen(RenderState* state, int col, int row) {
    return (Vector2){
        state->board_origin_x + col * TILE_SIZE,
        state->board_origin_y + row * TILE_SIZE
    };
}

Vector2 ApplyTilt(RenderState* state, Vector2 screen_pos) {
    float dy = screen_pos.y - state->board_center_y;
    return (Vector2){
        screen_pos.x,
        state->board_center_y + dy * TILT_COS
    };
}

BoardPos ScreenToBoard(RenderState* state, Vector2 screen_pos) {
    float dy = screen_pos.y - state->board_center_y;
    float untilted_y = state->board_center_y + dy / TILT_COS;

    int col = (int)((screen_pos.x - state->board_origin_x + TILE_SIZE * 0.5f) / TILE_SIZE);
    int row = (int)((untilted_y - state->board_origin_y + TILE_SIZE * 0.5f) / TILE_SIZE);

    return (BoardPos){col, row};
}

void DrawTileQuadColored(RenderState* state, Vector2 center, float half_size, Color color) {
    Vector2 corners[4] = {
        {center.x - half_size, center.y - half_size},
        {center.x + half_size, center.y - half_size},
        {center.x + half_size, center.y + half_size},
        {center.x - half_size, center.y + half_size}
    };

    for (int i = 0; i < 4; i++) {
        corners[i] = ApplyTilt(state, corners[i]);
    }

    rlBegin(RL_QUADS);
    rlColor4ub(color.r, color.g, color.b, color.a);

    rlVertex2f(corners[0].x, corners[0].y);
    rlVertex2f(corners[3].x, corners[3].y);
    rlVertex2f(corners[2].x, corners[2].y);
    rlVertex2f(corners[1].x, corners[1].y);

    rlEnd();
}

void DrawTileQuadTextured(RenderState* state, Texture2D texture, Rectangle src, Vector2 center, float half_size, Color tint) {
    Vector2 corners[4] = {
        {center.x - half_size, center.y - half_size},
        {center.x + half_size, center.y - half_size},
        {center.x + half_size, center.y + half_size},
        {center.x - half_size, center.y + half_size}
    };

    for (int i = 0; i < 4; i++) {
        corners[i] = ApplyTilt(state, corners[i]);
    }

    float tex_left = src.x / texture.width;
    float tex_right = (src.x + src.width) / texture.width;
    float tex_top = src.y / texture.height;
    float tex_bottom = (src.y + src.height) / texture.height;

    rlSetTexture(texture.id);
    rlBegin(RL_QUADS);
    rlColor4ub(tint.r, tint.g, tint.b, tint.a);

    rlTexCoord2f(tex_left, tex_top);
    rlVertex2f(corners[0].x, corners[0].y);

    rlTexCoord2f(tex_left, tex_bottom);
    rlVertex2f(corners[3].x, corners[3].y);

    rlTexCoord2f(tex_right, tex_bottom);
    rlVertex2f(corners[2].x, corners[2].y);

    rlTexCoord2f(tex_right, tex_top);
    rlVertex2f(corners[1].x, corners[1].y);

    rlEnd();
    rlSetTexture(0);
}

void DrawTileQuadRotated(Texture2D texture, Rectangle src, Vector2 center, float half_size, float rotation, bool atlas_rotated, Color tint, RenderState* state) {
    float angle_rad = rotation * DEG2RAD;
    float cos_a = cosf(angle_rad);
    float sin_a = sinf(angle_rad);

    Vector2 local_corners[4] = {
        {-half_size, -half_size},
        { half_size, -half_size},
        { half_size,  half_size},
        {-half_size,  half_size}
    };

    Vector2 corners[4];
    for (int i = 0; i < 4; i++) {
        float rx = local_corners[i].x * cos_a - local_corners[i].y * sin_a;
        float ry = local_corners[i].x * sin_a + local_corners[i].y * cos_a;
        corners[i] = ApplyTilt(state, (Vector2){center.x + rx, center.y + ry});
    }

    float src_w = fabsf(src.width);
    float src_h = fabsf(src.height);

    float tex_left = src.x / texture.width;
    float tex_right = (src.x + src_w) / texture.width;
    float tex_top = src.y / texture.height;
    float tex_bottom = (src.y + src_h) / texture.height;

    rlSetTexture(texture.id);
    rlBegin(RL_QUADS);
    rlColor4ub(tint.r, tint.g, tint.b, tint.a);

    if (atlas_rotated) {
        rlTexCoord2f(tex_left, tex_bottom);
        rlVertex2f(corners[0].x, corners[0].y);

        rlTexCoord2f(tex_right, tex_bottom);
        rlVertex2f(corners[3].x, corners[3].y);

        rlTexCoord2f(tex_right, tex_top);
        rlVertex2f(corners[2].x, corners[2].y);

        rlTexCoord2f(tex_left, tex_top);
        rlVertex2f(corners[1].x, corners[1].y);
    } else {
        rlTexCoord2f(tex_left, tex_top);
        rlVertex2f(corners[0].x, corners[0].y);

        rlTexCoord2f(tex_left, tex_bottom);
        rlVertex2f(corners[3].x, corners[3].y);

        rlTexCoord2f(tex_right, tex_bottom);
        rlVertex2f(corners[2].x, corners[2].y);

        rlTexCoord2f(tex_right, tex_top);
        rlVertex2f(corners[1].x, corners[1].y);
    }

    rlEnd();
    rlSetTexture(0);
}

void DrawTexturedQuad2D(Texture2D texture, Rectangle src, Vector2 center, float width, float height, Color tint, bool flip_h) {
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

    rlSetTexture(texture.id);
    rlBegin(RL_QUADS);
    rlColor4ub(tint.r, tint.g, tint.b, tint.a);

    rlTexCoord2f(tex_left, tex_top);
    rlVertex2f(center.x - hw, center.y - hh);

    rlTexCoord2f(tex_left, tex_bottom);
    rlVertex2f(center.x - hw, center.y + hh);

    rlTexCoord2f(tex_right, tex_bottom);
    rlVertex2f(center.x + hw, center.y + hh);

    rlTexCoord2f(tex_right, tex_top);
    rlVertex2f(center.x + hw, center.y - hh);

    rlEnd();
    rlSetTexture(0);
}
