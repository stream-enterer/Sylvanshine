# Grid Perspective Rendering Specification

## Overview

SDL3 perspective grid rendering using CPU-side transforms. No shaders or framebuffers.

**Key insight:** SDL_RenderGeometry uses affine (linear) texture interpolation, not perspective-correct. Textured quads "fold" when transformed to trapezoids. Solution: render each grid line and tile highlight as individually transformed primitives.

---

## Constants

```cpp
// Board geometry
constexpr int BOARD_COLS = 9;
constexpr int BOARD_ROWS = 5;
constexpr int TILE_SIZE = 95;  // Base pixels at 1x scale

// Perspective
constexpr float FOV_DEGREES = 60.0f;
constexpr float BOARD_X_ROTATION = 16.0f;   // Positive = top recedes
constexpr float ENTITY_X_ROTATION = 26.0f;  // Sprites tilt more (future)
constexpr float DEG_TO_RAD = 3.14159265359f / 180.0f;

// Scaling
// scale=1: 720p (1280×720), tile=95px, board=855×475
// scale=2: 1080p (1920×1080), tile=190px, board=1710×950
```

---

## Core Math

### Eye Distance (zeye)
```cpp
float zeye = (window_h / 2.0f) / tan(FOV_DEGREES * 0.5f * DEG_TO_RAD);
// At 720p: zeye = 360 / tan(30°) ≈ 623.5
// At 1080p: zeye = 540 / tan(30°) ≈ 935.3
```

### Perspective Transform
```cpp
Vec2 apply_perspective(Vec2 point, float center_x, float center_y, float zeye, float rotation_deg) {
    float rel_x = point.x - center_x;
    float rel_y = point.y - center_y;
    
    float angle_rad = rotation_deg * DEG_TO_RAD;
    float cos_a = cos(angle_rad);
    float sin_a = sin(angle_rad);
    
    float rotated_y = rel_y * cos_a;
    float rotated_z = rel_y * sin_a;
    
    float depth = zeye - rotated_z;
    float scale = zeye / depth;
    
    return {
        rel_x * scale + center_x,
        rotated_y * scale + center_y
    };
}
```

### Board Origin (Analytical Centering)

The flat board origin must be chosen so the perspective-transformed board is vertically centered.

**Problem:** Perspective is non-linear. Simple offset doesn't work.

**Solution:** Solve for flat origin `a` (relative to screen center) where transformed top and bottom are equidistant from center:

```cpp
float board_origin_y() const {
    float zeye = (window_h / 2.0f) / tan(FOV * 0.5f * DEG_TO_RAD);
    float center_y = window_h * 0.5f;
    float H = board_height();  // BOARD_ROWS * tile_size
    
    float s = sin(BOARD_X_ROTATION * DEG_TO_RAD);
    
    // Quadratic solution: perspective(a) + perspective(a+H) = 0
    float discriminant = zeye * zeye + s * s * H * H;
    float a = (zeye - s * H - sqrt(discriminant)) / (2.0f * s);
    
    return a + center_y;
}

float board_origin_x() const {
    return (window_w - board_width()) * 0.5f;
}
```

---

## Rendering Pipeline

### Grid Lines
Transform each line endpoint, draw with SDL_RenderLine:
```cpp
for (int x = 0; x <= BOARD_COLS; x++) {
    Vec2 top = transform(origin_x + x * tile_size, origin_y);
    Vec2 bottom = transform(origin_x + x * tile_size, origin_y + board_height);
    SDL_RenderLine(renderer, top.x, top.y, bottom.x, bottom.y);
}
// Same for horizontal lines
```

### Tile Highlights
Transform 4 corners per tile, draw with SDL_RenderGeometry:
```cpp
void render_highlight(BoardPos pos, SDL_Color color) {
    float x = origin_x + pos.x * tile_size;
    float y = origin_y + pos.y * tile_size;
    
    Vec2 tl = transform(x, y);
    Vec2 tr = transform(x + tile_size, y);
    Vec2 br = transform(x + tile_size, y + tile_size);
    Vec2 bl = transform(x, y + tile_size);
    
    SDL_Vertex vertices[4] = {tl, tr, br, bl};  // with color
    int indices[6] = {0, 1, 2, 0, 2, 3};
    SDL_RenderGeometry(renderer, nullptr, vertices, 4, indices, 6);
}
```

### Entity Positioning
Entities use `board_to_screen_perspective()`:
```cpp
Vec2 board_to_screen_perspective(BoardPos pos) {
    Vec2 flat = {
        board_origin_x() + (pos.x + 0.5f) * tile_size,
        board_origin_y() + (pos.y + 0.5f) * tile_size
    };
    return apply_perspective(flat, center_x, center_y, zeye, BOARD_X_ROTATION);
}
```

### Mouse Input (Inverse Transform)
```cpp
Vec2 inverse_perspective(Vec2 screen_point) {
    float proj_x = screen_point.x - center_x;
    float proj_y = screen_point.y - center_y;
    
    float cos_a = cos(rotation), sin_a = sin(rotation);
    
    float denom = zeye * cos_a + proj_y * sin_a;
    float rel_y = (proj_y * zeye) / denom;
    float depth = zeye - rel_y * sin_a;
    float rel_x = proj_x * depth / zeye;
    
    return {rel_x + center_x, rel_y + center_y};
}

BoardPos screen_to_board_perspective(Vec2 screen) {
    Vec2 flat = inverse_perspective(screen);
    return {
        floor((flat.x - origin_x) / tile_size),
        floor((flat.y - origin_y) / tile_size)
    };
}
```

---

## File Structure

```
include/
  perspective.hpp   - PerspectiveConfig, apply_perspective_transform()
  grid_renderer.hpp - GridRenderer struct
  types.hpp         - RenderConfig with board_origin_y()

src/
  perspective.cpp   - Transform implementations
  grid_renderer.cpp - Grid/highlight rendering
  types.cpp         - Coordinate conversions, analytical centering
```

---

## Validation

| Check | Pass Condition |
|-------|----------------|
| Tiles wider than tall | Top edge < bottom edge width |
| Board is trapezoid | Vertical lines converge toward top |
| Centered vertically | Top margin = bottom margin (±2px) |
| Works at all scales | Test 720p (scale=1) and 1080p (scale=2) |
| Mouse clicks land correctly | Click tile center → selects that tile |

**Wrong rotation sign:** Tiles taller than wide, board inverted.

---

## Not Implemented (Future)

- **Entity -26° rotation:** Sprites currently use board rotation. Should use steeper angle to "stand up" on tilted tiles.
- **Shadow system:** Static blob shadows at ground position.
- **Sprite positioning formula:** `sprite_y = ground_y + sprite_height - (shadowOffset * 2.0)`
- **Z-ordering:** Sort by board_pos.y descending (far first).

---

## Common Bugs

| Symptom | Cause | Fix |
|---------|-------|-----|
| Diagonal fold in grid | Affine texture interpolation | Don't use framebuffer; render lines directly |
| Board clipped at bottom | Origin calculated for flat, not transformed board | Use analytical centering formula |
| Board not centered | Adding offset after centering calculation | Remove stray offsets from board_origin_y() |
| Tiles taller than wide | Wrong rotation sign | Use positive 16°, not negative |
| Mouse clicks offset | Inverse transform incorrect | Check denom calculation in inverse_perspective |
