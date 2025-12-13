#ifndef RENDER_H
#define RENDER_H

#include "types.h"

#include <raylib.h>

#define CAMERA_FOVY 7.0f

typedef struct {
    Camera3D camera;
    float board_width;
    float board_height;
    Vector3 board_center;
} RenderState;

void InitRenderState(RenderState* state);
Vector3 BoardToWorld(RenderState* state, int col, int row);
Vector3 ScreenPosToWorld(RenderState* state, float screen_x, float screen_y);
void DrawTexturedQuad(Texture2D texture, Rectangle src, Vector3 center, float width, float height, float x_rotation, Color tint, bool flip_h);
void DrawColoredQuad(Vector3 center, float width, float height, float x_rotation, Color color);
void DrawFloorQuad(Vector3 center, float width, float depth, Color color);
void DrawFloorTexturedQuad(Texture2D texture, Rectangle src, Vector3 center, float width, float depth, Color tint);
void DrawFloorTexturedQuadRotated(Texture2D texture, Rectangle src, Vector3 center, float width, float depth, float y_rotation, bool atlas_rotated, Color tint);

#endif
