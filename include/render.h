#ifndef RENDER_H
#define RENDER_H

#include "types.h"

#include <raylib.h>

#define TILT_COS 0.9613f
#define TILT_SIN 0.2756f

typedef struct {
    float board_origin_x;
    float board_origin_y;
    float board_center_y;
} RenderState;

void InitRenderState(RenderState* state);
Vector2 TileCenterScreen(RenderState* state, int col, int row);
Vector2 ApplyTilt(RenderState* state, Vector2 screen_pos);
BoardPos ScreenToBoard(RenderState* state, Vector2 screen_pos);

void DrawTileQuadColored(RenderState* state, Vector2 center, float half_size, Color color);
void DrawTileQuadTextured(RenderState* state, Texture2D texture, Rectangle src, Vector2 center, float half_size, Color tint);
void DrawTileQuadRotated(Texture2D texture, Rectangle src, Vector2 center, float half_size, float rotation, bool atlas_rotated, Color tint, RenderState* state);
void DrawTexturedQuad2D(Texture2D texture, Rectangle src, Vector2 center, float width, float height, Color tint, bool flip_h);

#endif
