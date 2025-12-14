# Duelyst Visual Framework Reference

## Board Grid

| Constant | Value | Notes |
|----------|-------|-------|
| BOARDCOL | 9 | columns (x) |
| BOARDROW | 5 | rows (y) |
| TILESIZE | 95 | pixels per tile |
| TILEOFFSET_X | 0.0 | board horizontal nudge |
| TILEOFFSET_Y | 10.0 | board vertical nudge |
| BOARDCENTER | {4, 2} | floor(col/2), floor(row/2) |

Board pixel dimensions: 855 x 475 (9×95, 5×95)

## Coordinate Transforms

Reference window: 1280×720 (all layout math assumes this)
```c
// Board origin (bottom-left of tile 0,0) in screen space
float board_origin_x = (screen_width - BOARDCOL * TILESIZE) * 0.5f + TILESIZE * 0.5f + TILEOFFSET_X;
float board_origin_y = (screen_height - BOARDROW * TILESIZE) * 0.5f + TILESIZE * 0.5f + TILEOFFSET_Y;

// Board position to screen position
float screen_x = board_x * TILESIZE + board_origin_x;
float screen_y = board_y * TILESIZE + board_origin_y;

// Screen position to board index
int board_x = floor((screen_x - board_origin_x + TILESIZE * 0.5f) / TILESIZE);
int board_y = floor((screen_y - board_origin_y + TILESIZE * 0.5f) / TILESIZE);
```

## 3D Rotation (Pseudo-Perspective)

| Layer | X Rotation | Notes |
|-------|------------|-------|
| Board tiles | 16° | XYZ_ROTATION |
| Entities | 26° | ENTITY_XYZ_ROTATION |

Board is tilted back, entities tilt slightly more. Both rotate around X axis only.

## Unit Shadow

File: `units/unit_shadow.png` (96×48 RGBA)
- Drawn at ground position (entity's board tile center)
- Opacity: 200/255
- Z-order: -9999 (behind everything)

## Unit Sprite Positioning

Units have a `shadowOffset` in their sprite data that shifts the visual center upward from the ground position. The shadow stays at ground level while the sprite floats above it.
```c
// Ground position is the tile center
Vector2 ground_pos = board_to_screen(board_x, board_y);

// Shadow draws at ground
draw_shadow(ground_pos);

// Entity sprite center is offset upward
float sprite_y = ground_pos.y + content_height - shadow_offset * 2.0f;
```

## Movement Animation

| Constant | Value |
|----------|-------|
| ENTITY_MOVE_DURATION_MODIFIER | 1.0 |
| ENTITY_MOVE_MODIFIER_MAX | 1.0 |
| ENTITY_MOVE_MODIFIER_MIN | 0.75 |
| ENTITY_MOVE_CORRECTION | 0.2 |
| ENTITY_MOVE_FLYING_FIXED_TILE_COUNT | 3.0 |

Flying units move with fixed arc over 3 tiles regardless of distance.

## Spawn FX by Faction

From `fx_compositions/compositions.json`:

| Faction | Sprites |
|---------|---------|
| Default | fxTeleportRecallWhite, fx_f1_holyimmolation, fxSmokeGround |
| Faction2 | fx_f2_teleportsmoke ×2, fxSmokeGround |
| Faction3 | fxTeleportRecallWhite, fxBladestorm, fxSmokeGround |
| Faction4 | fx_f2_teleportsmoke ×2, fxSmokeGround |
| Faction5 | fxTeleportRecallWhite, (see json) |
| Faction6 | (see json) |

Spawn sequence:
1. Play `apply` sound from unit's sounds.txt
2. Show spawn FX sprites at entity position
3. If unit has `apply` animation, play it
4. Fade shadow from 0 to 200 opacity over FADE_MEDIUM_DURATION

## Animation Format

From `units/*/animations.txt`:
```
<name> <fps> <frame_data>

frame_data = index,sheet_x,sheet_y,width,height[,...]
```

Standard animations: attack, breathing, cast, castend, castloop, caststart, death, hit, idle, run

Optional: apply (spawn animation)

## FX Sprite Mapping

From `fx/rsx_mapping.tsv`:
```
rsx_name → folder → animation_prefix
```

Example: `BloodExplosionBig → fx_blood_explosion → fx_bloodbig_`

Each fx folder contains:
- `spritesheet.png`
- `animations.txt` (same format as units)

## Tile Highlight Opacity

| State | Opacity |
|-------|---------|
| TILE_SELECT_OPACITY | 200 |
| TILE_HOVER_OPACITY | 200 |
| TILE_DIM_OPACITY | 127 |
| TILE_FAINT_OPACITY | 75 |
| PATH_TILE_ACTIVE_OPACITY | 150 |
| PATH_TILE_DIM_OPACITY | 100 |

Colors:
- Player tiles: white (255, 255, 255)
- Opponent tiles: red (255, 100, 100)

## UI Layout Anchors

All relative to 1280×720 reference, scaled proportionally.

Player frames positioned outside board bounds:
- P1: board_left - 200px, board_top + 122px
- P2: board_right + 200px, board_top area

Cards in hand: below board center

## Key Timing Constants

| Constant | Value | Use |
|----------|-------|-----|
| FADE_MEDIUM_DURATION | (check config) | Shadow fade-in |
| PATH_ARC_DISTANCE | TILESIZE × 0.5 | Movement path curve |
