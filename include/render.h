#ifndef RENDER_H
#define RENDER_H

#include "types.h"

#include <raylib.h>
#include <raymath.h>

typedef struct {
    float board_origin_x;
    float board_origin_y;
    float zeye;
    Matrix projection;
    Matrix view;
    Matrix model;
    Matrix mvp;
    RenderTexture2D framebuffer;
} RenderState;

void InitRenderState(RenderState* state);
void UnloadRenderState(RenderState* state);

Vector2 TileCenterScreen(RenderState* state, int col, int row);
Vector2 ApplyPerspective(RenderState* state, Vector2 flat_pos);
BoardPos ScreenToBoard(RenderState* state, Vector2 screen_pos);

void BeginGridRender(RenderState* state);
void EndGridRender(RenderState* state);
void DrawGridToScreen(RenderState* state);

void DrawTileQuadColored(RenderState* state, Vector2 center, float half_size, Color color);
void DrawTileQuadTextured(RenderState* state, Texture2D texture, Rectangle src, Vector2 center, float half_size, Color tint);
void DrawTileQuadRotated(Texture2D texture, Rectangle src, Vector2 center, float half_size, float rotation, bool atlas_rotated, Color tint, RenderState* state);
void DrawTexturedQuad2D(Texture2D texture, Rectangle src, Vector2 center, float width, float height, Color tint, bool flip_h);

#endif
