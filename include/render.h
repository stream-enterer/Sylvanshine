#ifndef RENDER_H
#define RENDER_H

#include "types.h"

#include <raylib.h>

#define WORLD_SCALE 0.01f
#define CAMERA_DISTANCE 12.0f
#define CAMERA_HEIGHT 8.0f
#define CAMERA_FOVY 45.0f

typedef struct {
    Camera3D camera;
    float board_width;
    float board_height;
    float tile_world_size;
    Vector3 board_center;
} RenderState;

void InitRenderState(RenderState* state);
Vector3 BoardToWorld(RenderState* state, int col, int row);
Vector3 ScreenPosToWorld(RenderState* state, float screen_x, float screen_y);
void DrawTexturedQuad(Texture2D texture, Rectangle src, Vector3 center, float width, float height, float x_rotation, Color tint, bool flip_h);
void DrawColoredQuad(Vector3 center, float width, float height, float x_rotation, Color color);

#endif
