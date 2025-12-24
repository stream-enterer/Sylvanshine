# Sylvanshine Grid System Specification

Technical specification of the current Sylvanshine grid rendering implementation.

**Source Files:**
- `src/grid_renderer.cpp` — Grid rendering logic
- `include/grid_renderer.hpp` — GridRenderer class definition
- `src/main.cpp:1136-1169` — Render call order

---

## 1. Grid Visualization Method

### 1.1 Primary Approach: Positive Grid Lines

Sylvanshine renders the grid using **white lines** drawn between tile boundaries. This is a "positive grid" approach where the grid structure is explicitly drawn.

**Implementation:** `grid_renderer.cpp:58-76`

```cpp
void GridRenderer::render(const RenderConfig& config) {
    SDL_FColor white = {1.0f, 1.0f, 1.0f, 1.0f};

    // Vertical lines
    for (int x = 0; x <= BOARD_COLS; x++) {
        Vec2 top = transform_board_point(config, bx, 0);
        Vec2 bottom = transform_board_point(config, bx, BOARD_ROWS * ts);
        g_gpu.draw_line(top, bottom, white);
    }

    // Horizontal lines
    for (int y = 0; y <= BOARD_ROWS; y++) {
        Vec2 left = transform_board_point(config, 0, by);
        Vec2 right = transform_board_point(config, BOARD_COLS * ts, by);
        g_gpu.draw_line(left, right, white);
    }
}
```

### 1.2 Secondary Layer: Floor Tiles

Semi-transparent dark floor tiles are rendered beneath the grid lines.

**Implementation:** `grid_renderer.cpp:78-98`

```cpp
void GridRenderer::render_floor_grid(const RenderConfig& config) {
    SDL_FColor floor_tint = {0.0f, 0.0f, 0.0f, 0.08f};  // 8% opacity black

    for (int y = 0; y < BOARD_ROWS; y++) {
        for (int x = 0; x < BOARD_COLS; x++) {
            // Draw floor_tile sprite at each grid position
            g_gpu.draw_sprite_transformed(floor_tile, src, tl, tr, br, bl, floor_tint.a);
        }
    }
}
```

---

## 2. Render Order

**Source:** `src/main.cpp:1136-1169`

```
1. Floor grid (semi-transparent tiles)     render_floor_grid()
2. Grid lines (white)                      render()
3. Movement blob (merged corner tiles)     render_move_range_alpha()
4. Attack blob (merged corner tiles)       render_attack_range()
5. Selection box at unit                   render_select_box()
6. Movement path                           render_path()
7. Selection box at destination            render_select_box()
8. Hover highlight                         render_hover()
──────────────────────────────────────────
9. Entity shadows (Y-sorted)
10. Entity sprites (Y-sorted)
```

---

## 3. Visual Characteristics

### 3.1 Grid Lines

| Property | Value |
|----------|-------|
| Color | White (#FFFFFF) |
| Opacity | 100% |
| Line Width | 1px (hardware line) |
| Perspective | 16° X-rotation applied |
| Coverage | 10 vertical + 6 horizontal lines |

### 3.2 Floor Tiles

| Property | Value |
|----------|-------|
| Sprite | `dist/resources/tiles/s{1,2}/floor.png` |
| Tint | Black (0, 0, 0) |
| Opacity | 8% |
| Size | **Fills entire tile cell (no gaps)** |
| Gap | **None** |

**Ground Truth Measurements:**

| Scale | Sprite File | Dimensions | Visible Content | Margin |
|-------|-------------|------------|-----------------|--------|
| 1x | s1/floor.png | 48×48 | 48×48 | 0px |
| 2x | s2/floor.png | 96×96 | 96×96 | 0px |

### 3.3 Visual Result

- White grid lines form a clearly visible lattice
- Floor tiles provide subtle darkening behind the grid
- **Tiles fill the entire cell with no gaps between them**
- Grid structure is defined by **positive lines**, not negative space

---

## 4. Movement/Attack Blobs

### 4.1 Merged Corner System

Uses quarter-tile sprites to render blob boundaries with smooth corners.

**Corner Textures:**
- `corner_0.png` — Solo corner (no neighbors)
- `corner_01.png` — One edge neighbor
- `corner_03.png` — Other edge neighbor
- `corner_013.png` — Both edges, no diagonal
- `corner_0123.png` — All neighbors (full fill)

**Implementation:** `grid_renderer.cpp:382-415`

Each tile is divided into 4 quarters. For each quarter, neighbor presence determines which sprite variant to use.

### 4.2 Corner Sprite Dimensions (Ground Truth)

| Scale | Sprite | File Size | Visible Content | Margin |
|-------|--------|-----------|-----------------|--------|
| 1x | corner_0.png | 24×24 | 24×24 at +0+0 | **0px** |
| 1x | corner_0123.png | 24×24 | 24×24 at +0+0 | **0px** |
| 2x | corner_0.png | 48×48 | 48×48 at +0+0 | **0px** |
| 2x | corner_0123.png | 48×48 | 46×46 at +1+1 | 1px (inconsistent) |

**Key Finding:** Corner sprites have **no margin** — they fill the entire quarter-cell.

### 4.3 Corner Rendering Code

**Source:** `grid_renderer.cpp:417-454`

```cpp
void GridRenderer::render_corner_quad_rotated(...) {
    int ts = config.tile_size();
    int half = ts / 2;

    // No inset applied — fills entire quarter-cell
    float h = half * 0.5f;

    Vec2 tl_b = rotate(-h, -h);
    Vec2 tr_b = rotate(h, -h);
    // ... draws full quarter-cell quad
}
```

**No programmatic inset** is applied. Corners extend to cell boundaries.

### 4.4 Colors

**Source:** `include/grid_renderer.hpp:32-44`

| Highlight Type | Color | Opacity |
|----------------|-------|---------|
| Movement (MOVE_CURRENT) | White (#FFFFFF) | 78% (200/255) |
| Attack (ATTACK_CURRENT) | Light Red (255, 100, 100) | 78% |
| Path | White | 59% (150/255) |
| Hover | White | 78% |

**Note:** Attack color differs from Duelyst (which uses yellow #FFD900 for friendly attacks).

---

## 5. Movement Path Rendering

### 5.1 Path Sprites

| Segment | Sprite |
|---------|--------|
| Start | `path_start.png` |
| Straight | `path_straight.png` |
| Straight from start | `path_straight_from_start.png` |
| Corner | `path_corner.png` |
| Corner from start | `path_corner_from_start.png` |
| End | `path_end.png` |
| End from start | `path_end_from_start.png` |

### 5.2 Rotation Logic

**Source:** `grid_renderer.cpp:367-376`

```cpp
float path_segment_rotation(BoardPos from, BoardPos to) {
    if (dx > 0) return 0.0f;    // Right
    if (dx < 0) return 180.0f;  // Left
    if (dy > 0) return 90.0f;   // Down
    return 270.0f;              // Up
}
```

### 5.3 Corner Detection

Uses cross product of direction vectors to determine corner flip.

**Source:** `grid_renderer.cpp:345-365`

---

## 6. Selection and Hover

### 6.1 Selection Box

**Sprite:** `select_box.png` — Bracket corners forming a square outline

**Usage:** Rendered at:
- Selected unit position
- Path destination position

### 6.2 Hover Tile

**Sprite:** `hover.png` — Full tile highlight

**Implementation:** `grid_renderer.cpp:305-323`

Falls back to solid quad if sprite not available.

---

## 7. Coordinate System

### 7.1 Constants

| Constant | Value | Source |
|----------|-------|--------|
| BOARD_COLS | 9 | types.hpp |
| BOARD_ROWS | 5 | types.hpp |
| TILE_SIZE | 48px base | types.hpp |
| Board X-Rotation | 16° | perspective.hpp |
| Entity X-Rotation | 26° | perspective.hpp |

### 7.2 Transform Pipeline

```
Board Position (x, y)
    ↓
Tile Coordinates (x * tile_size, y * tile_size)
    ↓
transform_board_point() — applies 16° perspective
    ↓
Screen Coordinates
```

---

## 8. Asset Organization

**Scale-specific assets:** `dist/resources/tiles/s{1,2}/`

| Scale | Folder | Tile Size |
|-------|--------|-----------|
| 1x | s1/ | 48px |
| 2x | s2/ | 96px |

**Assets per scale:**
- `floor.png` (48×48 / 96×96, no margin)
- `hover.png`
- `select_box.png`
- `corner_*.png` (5 variants, no margin)
- `path_*.png` (7 variants)

---

## 9. Alignment Analysis

### 9.1 Current State

| Component | Margin | Fills To |
|-----------|--------|----------|
| Floor tiles | 0px | Cell boundary |
| Corner sprites | 0px | Cell boundary |
| Grid lines | N/A | Cell boundary |

**Result:** All components align at cell boundaries. Grid lines provide visual separation.

### 9.2 Comparison with Duelyst

| Aspect | Sylvanshine | Duelyst |
|--------|-------------|---------|
| Grid Method | White lines (positive) | Tile gaps (negative) |
| Floor Margin | 0px (fills cell) | 3px in 128px sprite (2.34%) |
| Corner Margin | 0px (fills quarter) | 3px in 64px sprite (4.69% outer) |
| Floor Opacity | 8% | 20% |
| Attack Color | Red (255,100,100) | Yellow (#FFD900) |
| Tile Size | 48px base | 95px |

### 9.3 Alignment Implications

Both floor and corners extend to cell boundaries, so they are **aligned with each other**.

However, if floor gaps were added without modifying corners:
- Floor edges would be inset by 2.34%
- Corner sprites would still extend to cell boundary
- **Highlights would overlap into the gap area**

---

## 10. Limitations

1. **No Tile Gaps:** Floor tiles fill entire cell, unlike Duelyst's gapped approach
2. **No Corner Margins:** Corner sprites extend to cell boundary, unlike Duelyst
3. **Aliasing:** Hardware lines can have aliasing artifacts at certain angles
4. **Line Width:** Fixed at 1px, cannot vary
5. **Color Divergence:** Attack tiles use red instead of Duelyst's signature yellow
