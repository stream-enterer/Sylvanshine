# Duelyst Grid Perspective & Sprite System

## Visual Settings Architecture

Three independent player-adjustable factors:

| Setting | Controls | Independent? |
|---------|----------|--------------|
| Screen Resolution | Window/display size | Yes |
| UI Scale | Menus, HUD, cards in hand | Yes |
| Game Scale | Units AND grid tiles together | **Tied** |

**Why Unit + Grid are tied:** Sprites must match tile geometry for correct positioning, overlap, and attack animations.

### Game Scale Options

| Scale | Tile Size | Board Size | Minimum Resolution |
|-------|-----------|------------|-------------------|
| 1x | 95px | 855×475 | 720p (1280×720) |
| 2x | 190px | 1710×950 | ~1080p (1920×1080) |
| 3x | 285px | 2565×1425 | 1440p (2560×1440) |
| 4x | 380px | 3420×1900 | 4K (3840×2160) |

### Runtime Flow
```cpp
int max_game_scale = min(window_w / 855, window_h / 475);
int game_scale = clamp(player_choice, 1, max_game_scale);
float tile_size = 95.0f * game_scale;
// UI scale is separate, doesn't affect board
```

### TODO
- [ ] Zoom (camera distance adjustment, affects apparent size without changing pixel scale)
- [ ] Fractional scaling (1.5x etc - accept blur, or render at higher scale and downsample)

---

## Constants

```cpp
// Base values (1x scale)
constexpr int BOARD_COLS = 9;
constexpr int BOARD_ROWS = 5;
constexpr float BASE_TILE_SIZE = 95.0f;
constexpr float TILE_OFFSET_X = 0.0f;
constexpr float TILE_OFFSET_Y = 10.0f;

// Perspective
constexpr float FOV_DEGREES = 60.0f;
constexpr float BOARD_X_ROTATION = -16.0f;   // tiles (NEGATIVE)
constexpr float ENTITY_X_ROTATION = -26.0f;  // sprites tilt more

// Sprite positioning
constexpr float DEFAULT_SHADOW_OFFSET = 19.5f;  // CONFIG.DEPTH_OFFSET
```

---

## Rotation Angles

| Layer | Rotation | Purpose |
|-------|----------|---------|
| Board/tiles | -16° | Base perspective tilt |
| Entity sprites | -26° | Sprites "stand up" against foreshortened tiles |

**Why different:** Entities at -26° appear more upright on the -16° tilted board, creating proper "standing" illusion.

**Negative sign:** Tilts top away from camera (smaller), bottom toward camera (larger). Wrong sign = tiles taller than wide.

---

## Coordinate Transforms

### Board → Screen (flat, pre-perspective)
```cpp
float tile_size = BASE_TILE_SIZE * game_scale;
float board_w = BOARD_COLS * tile_size;
float board_h = BOARD_ROWS * tile_size;

float origin_x = (window_w - board_w) / 2.0f + tile_size / 2.0f + TILE_OFFSET_X * game_scale;
float origin_y = (window_h - board_h) / 2.0f + tile_size / 2.0f + TILE_OFFSET_Y * game_scale;

Vec2 board_to_screen(int col, int row) {
    return { col * tile_size + origin_x, row * tile_size + origin_y };
}
```

### Screen → Perspective
Transform through MVP matrix. Use -16° for tiles, -26° for sprites.

---

## Sprite Positioning System

### Core Concept
- **Ground position** = tile center (where shadow renders)
- **Sprite position** = lifted above ground based on sprite height and shadowOffset
- **Sprite anchor** = center of sprite

### shadowOffset Meaning
Distance from sprite's **bottom edge** to the "feet" position. All units use default 19.5px (universal constant, no per-unit overrides).

**Critical:** shadowOffset is multiplied by game_scale:
```cpp
float shadow_offset = DEFAULT_SHADOW_OFFSET * game_scale;
```

### Positioning Formula (from UnitNode.js)
```cpp
Vec2 ground_pos = board_to_screen(col, row);  // tile center

// Shadow at ground
draw_shadow(ground_pos);

// Sprite lifted up
// Formula: sprite_y = ground_y + sprite_height - (shadowOffset * 2.0)
float sprite_y = ground_pos.y + sprite_height - (shadow_offset * 2.0f);
draw_sprite({ground_pos.x, sprite_y});
```

### centerOffset Adjustment
The node's logical center is also shifted up by shadowOffset:
```cpp
// Node center offset (for hit detection, targeting, etc.)
center_offset.y += shadow_offset;
```

### Worked Example
Sprite height = 100px, shadowOffset = 19.5px, game_scale = 1:
```
shadow_offset = 19.5 * 1 = 19.5

sprite_center_y = ground_y + sprite_height - (shadow_offset * 2.0)
                = ground_y + 100 - 39
                = ground_y + 61

sprite_bottom = sprite_center_y - (sprite_height / 2)
              = ground_y + 61 - 50
              = ground_y + 11

feet_position = sprite_bottom + shadow_offset
              = ground_y + 11 + 19.5
              = ground_y + 30.5
```

The sprite is drawn centered at `ground_y + 61`. Its visual "feet" (19.5px up from sprite bottom) land at `ground_y + 30.5`, which after perspective transform aligns with the tile surface.

---

## Shadow System

Duelyst uses **two independent shadow systems**:

### 1. Static Blob Shadow
Simple pre-rendered ellipse (`unit_shadow.png`, 96×48) placed at ground position.

```cpp
// UnitNode.js behavior
shadow_sprite = load("unit_shadow.png");
shadow_sprite.set_opacity(200);  // semi-transparent
shadow_sprite.set_position(ground_pos);
shadow_sprite.z_order = -9999;   // always behind entity
```

- Used by all units
- Fixed shape regardless of sprite
- Fades in during spawn (`opacity 0 → 200` over `FADE_MEDIUM_DURATION`)

### 2. Dynamic Projected Shadows
Shader-based system that projects sprite silhouettes based on light positions.

**How it works:**
1. Sprites with `castsShadows: true` register as shadow casters
2. Vertex shader (`ShadowVertex.glsl`) transforms sprite geometry:
   - Calculates skew based on light-to-sprite angle
   - Applies 45° cast matrix to "lay down" the shadow
   - Stretches based on light altitude
3. Fragment shader samples sprite's **alpha channel** (silhouette)
4. Applies distance-based blur (farther from feet = blurrier)
5. Fades by distance from light source

**Quality levels:**
- `ShadowHighQuality`: 7×7 box blur (49 texture samples)
- `ShadowLowQuality`: 3×3 box blur (9 texture samples)

**Key uniforms:**
| Uniform | Purpose |
|---------|---------|
| `u_size` | Sprite dimensions |
| `u_anchor` | Foot position (shadowOffset from bottom) |
| `u_intensity` | Shadow darkness |
| `u_blurShiftModifier` | Blur increase with distance |
| `u_blurIntensityModifier` | Overall blur amount |

**Vertex attributes:**
| Attribute | Purpose |
|-----------|---------|
| `a_originRadius` | Light position (xyz) + radius (w) |

### Implementation Priority
For MVP, static blob shadow is sufficient. Dynamic shadows are a visual polish feature requiring:
- Light system implementation
- Per-sprite shadow caster registration
- Additional render passes

---

## Z-Ordering

Sprites with higher board Y (farther back) render **first** → proper occlusion.

```cpp
// Sort entities by board_pos.y descending before rendering
// Or use z-buffer with: z = -board_pos.y

// Shadow always behind entity
shadow_z_order = -9999;
```

---

## Camera Setup

```cpp
float zeye = (window_h / 2.0f) / tanf(FOV_DEGREES * 0.5f * DEG_TO_RAD);

vec3 eye = { window_w/2.0f, window_h/2.0f, zeye };
vec3 center = { window_w/2.0f, window_h/2.0f, 0.0f };
vec3 up = { 0.0f, 1.0f, 0.0f };

mat4 view = lookAt(eye, center, up);
```

---

## Rendering Pipeline

### Pass 1: Tiles to framebuffer (flat 2D)
- Orthographic projection
- Each tile at board_to_screen() position
- Result: flat grid texture

### Pass 2: Framebuffer to screen (perspective)
- Apply MVP with -16° X rotation around screen center
- Draw as textured quad via SDL_RenderGeometry or shader

### Pass 3: Sprites (perspective)
- Position each sprite using shadowOffset formula
- Apply MVP with -26° X rotation
- Render back-to-front (high Y first)
- Static shadows render at ground position, behind sprites

### Pass 4: Dynamic Shadows (optional, advanced)
- For each light with `castsShadows: true`:
  - For each sprite with `castsShadows: true`:
    - Render shadow geometry using shadow shaders
    - Accumulate into shadow buffer
- Composite shadow buffer onto scene

---

## Validation Checklist

- [ ] Tiles wider than tall (parallelograms)
- [ ] Board is trapezoid (wider at bottom)
- [ ] Sprites appear to "stand" on tiles
- [ ] Near sprites (low Y) occlude far sprites (high Y)
- [ ] Static shadows stay at tile centers
- [ ] No gaps between tiles
- [ ] Sprites align correctly at all game_scale values

**Wrong rotation sign symptom:** Tiles appear taller than wide.
