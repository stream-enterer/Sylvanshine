#include "render.h"

#include <math.h>

#include <rlgl.h>

static float CalculateZeye(float screen_height, float fov_degrees) {
    float fov_radians = fov_degrees * DEG2RAD;
    return (screen_height / 2.0f) / tanf(fov_radians / 2.0f);
}

static Matrix CreatePerspectiveMatrix(float fov_deg, float aspect, float near_plane, float far_plane) {
    float fov_rad = fov_deg * DEG2RAD;
    float f = 1.0f / tanf(fov_rad / 2.0f);

    Matrix m = { 0 };
    m.m0 = f / aspect;
    m.m5 = f;
    m.m10 = (far_plane + near_plane) / (near_plane - far_plane);
    m.m11 = -1.0f;
    m.m14 = (2.0f * far_plane * near_plane) / (near_plane - far_plane);
    m.m15 = 0.0f;

    return m;
}

static Matrix CreateLookAtMatrix(float screen_width, float screen_height, float zeye) {
    Vector3 eye = {
        screen_width / 2.0f,
        screen_height / 2.0f,
        zeye
    };

    Vector3 target = {
        screen_width / 2.0f,
        screen_height / 2.0f,
        0.0f
    };

    Vector3 up = { 0.0f, 1.0f, 0.0f };

    return MatrixLookAt(eye, target, up);
}

static Matrix CreateXYZRotationMatrix(float pitch_deg, float yaw_deg, float roll_deg) {
    float pitch = pitch_deg * DEG2RAD;
    float yaw = yaw_deg * DEG2RAD;
    float roll = roll_deg * DEG2RAD;

    Matrix rot_x = MatrixRotateX(pitch);
    Matrix rot_y = MatrixRotateY(yaw);
    Matrix rot_z = MatrixRotateZ(roll);

    Matrix result = MatrixMultiply(rot_z, rot_x);
    result = MatrixMultiply(result, rot_y);

    return result;
}

static Matrix CreateAnchoredRotation(float width, float height, Matrix rotation) {
    float anchor_x = width / 2.0f;
    float anchor_y = height / 2.0f;

    Matrix translate_to = MatrixTranslate(anchor_x, anchor_y, 0.0f);
    Matrix translate_back = MatrixTranslate(-anchor_x, -anchor_y, 0.0f);

    Matrix result = MatrixMultiply(translate_back, rotation);
    result = MatrixMultiply(result, translate_to);

    return result;
}

void InitRenderState(RenderState* state) {
    float board_pixel_width = BOARD_COLS * TILE_SIZE;
    float board_pixel_height = BOARD_ROWS * TILE_SIZE;

    state->board_origin_x = (SCREEN_WIDTH - board_pixel_width) * 0.5f + TILE_SIZE * 0.5f + TILE_OFFSET_X;
    state->board_origin_y = (SCREEN_HEIGHT - board_pixel_height) * 0.5f + TILE_SIZE * 0.5f + TILE_OFFSET_Y;

    state->zeye = CalculateZeye(SCREEN_HEIGHT, PERSPECTIVE_FOV);
    float aspect = (float)SCREEN_WIDTH / (float)SCREEN_HEIGHT;
    float far_plane = state->zeye * 2.0f;

    state->projection = CreatePerspectiveMatrix(PERSPECTIVE_FOV, aspect, NEAR_PLANE, far_plane);
    state->view = CreateLookAtMatrix(SCREEN_WIDTH, SCREEN_HEIGHT, state->zeye);

    Matrix xyz_rotation = CreateXYZRotationMatrix(XYZ_ROTATION_X, XYZ_ROTATION_Y, XYZ_ROTATION_Z);
    state->model = CreateAnchoredRotation(SCREEN_WIDTH, SCREEN_HEIGHT, xyz_rotation);

    Matrix mv = MatrixMultiply(state->model, state->view);
    state->mvp = MatrixMultiply(mv, state->projection);

    state->framebuffer = LoadRenderTexture(SCREEN_WIDTH, SCREEN_HEIGHT);
}

void UnloadRenderState(RenderState* state) {
    UnloadRenderTexture(state->framebuffer);
}

Vector2 TileCenterScreen(RenderState* state, int col, int row) {
    return (Vector2){
        state->board_origin_x + col * TILE_SIZE,
        state->board_origin_y + row * TILE_SIZE
    };
}

Vector2 ApplyPerspective(RenderState* state, Vector2 flat_pos) {
    Vector4 pos = { flat_pos.x, flat_pos.y, 0.0f, 1.0f };

    Vector4 transformed;
    transformed.x = state->mvp.m0 * pos.x + state->mvp.m4 * pos.y + state->mvp.m8 * pos.z + state->mvp.m12 * pos.w;
    transformed.y = state->mvp.m1 * pos.x + state->mvp.m5 * pos.y + state->mvp.m9 * pos.z + state->mvp.m13 * pos.w;
    transformed.z = state->mvp.m2 * pos.x + state->mvp.m6 * pos.y + state->mvp.m10 * pos.z + state->mvp.m14 * pos.w;
    transformed.w = state->mvp.m3 * pos.x + state->mvp.m7 * pos.y + state->mvp.m11 * pos.z + state->mvp.m15 * pos.w;

    if (transformed.w != 0.0f) {
        transformed.x /= transformed.w;
        transformed.y /= transformed.w;
    }

    Vector2 screen_pos;
    screen_pos.x = (transformed.x + 1.0f) * 0.5f * SCREEN_WIDTH;
    screen_pos.y = (1.0f - transformed.y) * 0.5f * SCREEN_HEIGHT;

    return screen_pos;
}

static Vector2 InversePerspective(RenderState* state, Vector2 screen_pos) {
    float ndc_x = (screen_pos.x / SCREEN_WIDTH) * 2.0f - 1.0f;
    float ndc_y = 1.0f - (screen_pos.y / SCREEN_HEIGHT) * 2.0f;

    Matrix inv_mvp = MatrixInvert(state->mvp);

    Vector4 ndc_near = { ndc_x, ndc_y, -1.0f, 1.0f };
    Vector4 ndc_far = { ndc_x, ndc_y, 1.0f, 1.0f };

    Vector4 world_near;
    world_near.x = inv_mvp.m0 * ndc_near.x + inv_mvp.m4 * ndc_near.y + inv_mvp.m8 * ndc_near.z + inv_mvp.m12 * ndc_near.w;
    world_near.y = inv_mvp.m1 * ndc_near.x + inv_mvp.m5 * ndc_near.y + inv_mvp.m9 * ndc_near.z + inv_mvp.m13 * ndc_near.w;
    world_near.z = inv_mvp.m2 * ndc_near.x + inv_mvp.m6 * ndc_near.y + inv_mvp.m10 * ndc_near.z + inv_mvp.m14 * ndc_near.w;
    world_near.w = inv_mvp.m3 * ndc_near.x + inv_mvp.m7 * ndc_near.y + inv_mvp.m11 * ndc_near.z + inv_mvp.m15 * ndc_near.w;

    Vector4 world_far;
    world_far.x = inv_mvp.m0 * ndc_far.x + inv_mvp.m4 * ndc_far.y + inv_mvp.m8 * ndc_far.z + inv_mvp.m12 * ndc_far.w;
    world_far.y = inv_mvp.m1 * ndc_far.x + inv_mvp.m5 * ndc_far.y + inv_mvp.m9 * ndc_far.z + inv_mvp.m13 * ndc_far.w;
    world_far.z = inv_mvp.m2 * ndc_far.x + inv_mvp.m6 * ndc_far.y + inv_mvp.m10 * ndc_far.z + inv_mvp.m14 * ndc_far.w;
    world_far.w = inv_mvp.m3 * ndc_far.x + inv_mvp.m7 * ndc_far.y + inv_mvp.m11 * ndc_far.z + inv_mvp.m15 * ndc_far.w;

    if (world_near.w != 0.0f) {
        world_near.x /= world_near.w;
        world_near.y /= world_near.w;
        world_near.z /= world_near.w;
    }

    if (world_far.w != 0.0f) {
        world_far.x /= world_far.w;
        world_far.y /= world_far.w;
        world_far.z /= world_far.w;
    }

    Vector3 ray_origin = { world_near.x, world_near.y, world_near.z };
    Vector3 ray_dir = {
        world_far.x - world_near.x,
        world_far.y - world_near.y,
        world_far.z - world_near.z
    };

    if (fabsf(ray_dir.z) < 0.0001f) {
        return (Vector2){ -1.0f, -1.0f };
    }

    float t = -ray_origin.z / ray_dir.z;
    Vector2 flat_pos;
    flat_pos.x = ray_origin.x + t * ray_dir.x;
    flat_pos.y = ray_origin.y + t * ray_dir.y;

    return flat_pos;
}

BoardPos ScreenToBoard(RenderState* state, Vector2 screen_pos) {
    Vector2 flat_pos = InversePerspective(state, screen_pos);

    int col = (int)((flat_pos.x - state->board_origin_x + TILE_SIZE * 0.5f) / TILE_SIZE);
    int row = (int)((flat_pos.y - state->board_origin_y + TILE_SIZE * 0.5f) / TILE_SIZE);

    return (BoardPos){col, row};
}

void BeginGridRender(RenderState* state) {
    BeginTextureMode(state->framebuffer);
    ClearBackground(BLANK);
}

void EndGridRender(RenderState* state) {
    (void)state;
    EndTextureMode();
}

void DrawGridToScreen(RenderState* state) {
    rlDrawRenderBatchActive();

    rlMatrixMode(RL_PROJECTION);
    rlPushMatrix();
    rlLoadIdentity();

    float proj[16];
    proj[0] = state->projection.m0;
    proj[1] = state->projection.m1;
    proj[2] = state->projection.m2;
    proj[3] = state->projection.m3;
    proj[4] = state->projection.m4;
    proj[5] = state->projection.m5;
    proj[6] = state->projection.m6;
    proj[7] = state->projection.m7;
    proj[8] = state->projection.m8;
    proj[9] = state->projection.m9;
    proj[10] = state->projection.m10;
    proj[11] = state->projection.m11;
    proj[12] = state->projection.m12;
    proj[13] = state->projection.m13;
    proj[14] = state->projection.m14;
    proj[15] = state->projection.m15;
    rlMultMatrixf(proj);

    rlMatrixMode(RL_MODELVIEW);
    rlPushMatrix();
    rlLoadIdentity();

    Matrix mv = MatrixMultiply(state->model, state->view);
    float modelview[16];
    modelview[0] = mv.m0;
    modelview[1] = mv.m1;
    modelview[2] = mv.m2;
    modelview[3] = mv.m3;
    modelview[4] = mv.m4;
    modelview[5] = mv.m5;
    modelview[6] = mv.m6;
    modelview[7] = mv.m7;
    modelview[8] = mv.m8;
    modelview[9] = mv.m9;
    modelview[10] = mv.m10;
    modelview[11] = mv.m11;
    modelview[12] = mv.m12;
    modelview[13] = mv.m13;
    modelview[14] = mv.m14;
    modelview[15] = mv.m15;
    rlMultMatrixf(modelview);

    Texture2D tex = state->framebuffer.texture;

    rlSetTexture(tex.id);
    rlBegin(RL_QUADS);

    rlColor4ub(255, 255, 255, 255);

    rlTexCoord2f(0.0f, 1.0f);
    rlVertex3f(0.0f, 0.0f, 0.0f);

    rlTexCoord2f(1.0f, 1.0f);
    rlVertex3f(SCREEN_WIDTH, 0.0f, 0.0f);

    rlTexCoord2f(1.0f, 0.0f);
    rlVertex3f(SCREEN_WIDTH, SCREEN_HEIGHT, 0.0f);

    rlTexCoord2f(0.0f, 0.0f);
    rlVertex3f(0.0f, SCREEN_HEIGHT, 0.0f);

    rlEnd();
    rlSetTexture(0);

    rlMatrixMode(RL_MODELVIEW);
    rlPopMatrix();
    rlMatrixMode(RL_PROJECTION);
    rlPopMatrix();
}

void DrawTileQuadColored(RenderState* state, Vector2 center, float half_size, Color color) {
    (void)state;

    Vector2 corners[4] = {
        {center.x - half_size, center.y - half_size},
        {center.x + half_size, center.y - half_size},
        {center.x + half_size, center.y + half_size},
        {center.x - half_size, center.y + half_size}
    };

    rlBegin(RL_QUADS);
    rlColor4ub(color.r, color.g, color.b, color.a);

    rlVertex2f(corners[0].x, corners[0].y);
    rlVertex2f(corners[3].x, corners[3].y);
    rlVertex2f(corners[2].x, corners[2].y);
    rlVertex2f(corners[1].x, corners[1].y);

    rlEnd();
}

void DrawTileQuadTextured(RenderState* state, Texture2D texture, Rectangle src, Vector2 center, float half_size, Color tint) {
    (void)state;

    Vector2 corners[4] = {
        {center.x - half_size, center.y - half_size},
        {center.x + half_size, center.y - half_size},
        {center.x + half_size, center.y + half_size},
        {center.x - half_size, center.y + half_size}
    };

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
    (void)state;

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
        corners[i] = (Vector2){center.x + rx, center.y + ry};
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
