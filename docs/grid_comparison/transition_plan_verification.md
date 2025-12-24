# Transition Plan Verification Report

Exhaustive verification of the sprite-based grid transition plan against all interconnected systems.

---

## Verification Status

| System | Status | Issues |
|--------|--------|--------|
| Perspective Transform | PASS | No impact |
| Shadow Rendering | PASS | No impact |
| Mouse Hit Testing | PASS | No impact |
| Entity Positioning | PASS | No impact |
| Z-Order/Depth Sorting | PASS | No impact |
| Floor Tile Rendering | PASS | Correct approach |
| Corner Sprite Margins | **FAIL** | Fundamental flaw in symmetric margin approach |
| Select Box Sizing | WARN | Source sprite different size than documented |
| Path Sprites | PASS | Correct (no margins) |
| Seam Sprites | PASS | Correctly identified as missing |

---

## Critical Issue: Corner Margin Approach is Flawed

### The Problem

The plan proposes:
1. Generate corner sprites with SYMMETRIC margins (1px all sides at 1x)
2. Apply render-time OFFSET to compensate

This approach **does not work**.

### Why Symmetric + Offset Fails

At 1x scale (48px tile, 24px corners, 1px uniform margin):

**Floor visible content:** (1,1) to (47,47) — 46×46 pixels

**TL corner with symmetric 1px margin:**
- Sprite at (0,0): visible content at (1,1) to (23,23) — 22×22 pixels

**Problem:**
- Floor left edge at x=1, corner left edge at x=1 ✓
- Floor at x=24 (half tile), corner RIGHT edge at x=23 ✗
- **1px gap at inner edge!**

**If we offset by (+1, +1):**
- Sprite at (1,1): visible content at (2,2) to (24,24)
- Floor left edge at x=1, corner left edge at x=2 ✗
- **1px gap at outer edge now!**

### Ground Truth from Duelyst

Checking `tile_merged_large_0.png`:
```
File: 64×64
Visible: 61×61 at +3+3
```

This means:
- Left margin: 3px
- Top margin: 3px
- Right margin: 64 - 3 - 61 = **0px**
- Bottom margin: 64 - 3 - 61 = **0px**

**Duelyst uses ASYMMETRIC margins — margin only on outer edges!**

### Correct Approach

**Option A (from plan) is correct, not Option B:**

Generate 4 rotations of each corner type with asymmetric margin placement:

| Corner | Outer Edges | Margin Placement |
|--------|-------------|------------------|
| TL (0°) | Left, Top | Left: 1px, Top: 1px, Right: 0px, Bottom: 0px |
| TR (90°) | Right, Top | Left: 0px, Top: 1px, Right: 1px, Bottom: 0px |
| BR (180°) | Right, Bottom | Left: 0px, Top: 0px, Right: 1px, Bottom: 1px |
| BL (270°) | Left, Bottom | Left: 1px, Top: 0px, Right: 0px, Bottom: 1px |

---

## Corrected Sprite Generation

### For Corner Sprites

```python
def resize_corner_with_asymmetric_margin(src: Path, dst: Path,
                                          target_size: int, margin: int,
                                          corner: int) -> None:
    """
    Generate corner sprite with asymmetric margin.

    corner: 0=TL, 1=TR, 2=BR, 3=BL
    Margin applied only to OUTER edges.
    """
    from PIL import Image

    # Determine which edges get margins
    margin_left   = margin if corner in [0, 3] else 0  # TL, BL
    margin_right  = margin if corner in [1, 2] else 0  # TR, BR
    margin_top    = margin if corner in [0, 1] else 0  # TL, TR
    margin_bottom = margin if corner in [2, 3] else 0  # BR, BL

    content_w = target_size - margin_left - margin_right
    content_h = target_size - margin_top - margin_bottom

    img = Image.open(src).convert('RGBA')
    bbox = img.getbbox()
    if bbox:
        img = img.crop(bbox)
    img = img.resize((content_w, content_h), Image.LANCZOS)

    output = Image.new('RGBA', (target_size, target_size), (0, 0, 0, 0))
    output.paste(img, (margin_left, margin_top))
    output.save(dst)
```

### Alternative: Use Duelyst's Rotation

Since corner sprites are rotated 90° for each corner position, we can generate ONE sprite with asymmetric margin and rotate it:

```python
def generate_corner_variants(src: Path, dst_dir: Path,
                              target_size: int, margin: int) -> None:
    """Generate all 4 corner rotations from single source."""
    from PIL import Image

    # Create base with TL margins (left and top)
    content_size = target_size - margin  # only 1 margin applies per edge

    img = Image.open(src).convert('RGBA')
    bbox = img.getbbox()
    if bbox:
        img = img.crop(bbox)
    img = img.resize((content_size, content_size), Image.LANCZOS)

    # TL: margin on left/top
    tl = Image.new('RGBA', (target_size, target_size), (0, 0, 0, 0))
    tl.paste(img, (margin, margin))
    # Note: Duelyst rotates sprites, but Sylvanshine uses corner index
    # So we need 4 separate sprites, not rotations

    # For Sylvanshine, all corners use same sprite rotated via render code
    # But the MARGIN must be in the correct position for each rotation
    # This means we need 4 different sprites OR render-time quad adjustment
```

### Cleanest Solution: Render-Time Quad Expansion

Instead of offsetting the sprite position, **expand the quad on inner edges**:

```cpp
void GridRenderer::render_corner_quad_rotated(const RenderConfig& config,
                                               BoardPos pos, int corner,
                                               const GPUTextureHandle& texture,
                                               SDL_FColor tint) {
    int ts = config.tile_size();
    int half = ts / 2;
    float margin = static_cast<float>(config.scale);  // 1px at 1x, 2px at 2x

    // Base position for this corner's quarter
    float base_x = pos.x * ts + ((corner == 1 || corner == 2) ? half : 0);
    float base_y = pos.y * ts + ((corner == 2 || corner == 3) ? half : 0);

    // Determine which edges are OUTER (need margin) vs INNER (extend to center)
    bool outer_left   = (corner == 0 || corner == 3);
    bool outer_right  = (corner == 1 || corner == 2);
    bool outer_top    = (corner == 0 || corner == 1);
    bool outer_bottom = (corner == 2 || corner == 3);

    // Compute quad corners with asymmetric insets
    float x0 = base_x + (outer_left ? margin : 0);
    float y0 = base_y + (outer_top ? margin : 0);
    float x1 = base_x + half - (outer_right ? margin : 0);
    float y1 = base_y + half - (outer_bottom ? margin : 0);

    // Compute visible content size in sprite (for UV mapping)
    float content_w = half - margin;  // sprite has 1 margin, content is rest
    float content_h = half - margin;

    // UV coords: sample only the visible content, skip margin
    // This requires knowing which edge has the margin in the sprite
    // If sprite has TL margin: u0 = margin/sprite_size, v0 = margin/sprite_size
    // ... complex UV calculation per corner rotation

    // Actually, easier approach: generate sprites WITH correct margin placement
}
```

---

## Verification: Unaffected Systems

### 1. Perspective Transform — PASS

**File:** `src/types.cpp`, `src/perspective.cpp`

The perspective transform operates on **tile positions** (multiples of tile_size), not on visible content. The transform pipeline:

```
BoardPos (x, y)
    ↓
Tile coords: (x * tile_size, y * tile_size)
    ↓
apply_perspective_transform()
    ↓
Screen coords
```

Floor tile margins are internal to the sprite — the tile's logical position is unchanged.

**No impact from this transition.**

### 2. Shadow Rendering — PASS

**Files:** `src/entity.cpp`, `src/gpu_renderer.cpp`

Shadow rendering uses:
- `SHADOW_OFFSET = 19.5f` for depth calculations
- Entity `screen_pos` (tile center, perspective-transformed)
- SDF atlas for shadow shape

Floor tile gaps do not affect entity positioning or shadow direction.

**No impact from this transition.**

### 3. Mouse Hit Testing — PASS

**File:** `src/types.cpp:107-114`

```cpp
BoardPos screen_to_board_perspective(const RenderConfig& config, Vec2 screen) {
    // Inverse perspective → flat coords → tile index
    return screen_to_board(config, flat);
}
```

Hit testing uses the full tile cell, not the visible content. Clicks in the gap area still register as the tile they're geometrically within.

**This matches Duelyst behavior** — their `transformScreenToBoardIndex` also uses `TILESIZE` without considering margins.

**No impact from this transition.**

### 4. Entity Positioning — PASS

**File:** `src/entity.cpp:115-118`

```cpp
void Entity::set_board_position(const RenderConfig& config, BoardPos pos) {
    board_pos = pos;
    screen_pos = board_to_screen_perspective(config, pos);
}
```

Entities are positioned at **tile center** (via `+tile_size*0.5` offset in `board_to_screen`). The visible floor content has a margin, but the entity anchor is at the cell center, not the visible content center.

**This matches Duelyst** — entities are at tile center, not inset.

**No impact from this transition.**

### 5. Z-Order/Depth Sorting — PASS

**File:** `src/main.cpp:1126-1133`

```cpp
std::vector<size_t> get_render_order(const GameState& state) {
    // Sort by screen_pos.y (lower Y = render later = on top)
}
```

Z-ordering uses entity `screen_pos.y`, which is tile center. Floor tile margins don't affect entity depth.

**No impact from this transition.**

---

## Verification: Affected Systems

### 6. Floor Tile Rendering — PASS (with clarification)

**File:** `src/grid_renderer.cpp:78-98`

Current implementation draws floor sprite at tile corners:
```cpp
Vec2 tl = transform_board_point(config, tx, ty);
Vec2 tr = transform_board_point(config, tx + ts, ty);
// ... draws sprite filling entire cell
```

**With margins baked into sprites:**
- Sprite still drawn at (tx, ty) to (tx+ts, ty+ts)
- But visible content is inset by margin
- Gaps appear automatically between adjacent tiles

**Correct behavior — no code change needed for floor.**

### 7. Corner Sprite Margins — FAIL

As detailed above, the symmetric margin approach is flawed.

**Required fix:** Either:
1. Generate asymmetric margin sprites (margin only on outer edges), OR
2. Expand quad at render time to fill to center on inner edges

### 8. Select Box Sizing — WARN

**Issue:** Plan states `tile_box.png` is 80×80, but current `select_box.png` is 48×48.

**Ground truth:**
```
tile_box.png: 80×80 (Duelyst source)
select_box.png: 48×48 (Sylvanshine s1)
```

The current build pipeline scales 80×80 → 48×48, which is **0.6× scale** (not 0.375× like other 128px sprites).

**At 95px Duelyst tiles:** 80px box = 84% of tile size (intentionally larger for visibility)
**At 48px Sylvanshine tiles:** 48px box = 100% of tile size (current) or 40px = 84% (Duelyst ratio)

**Recommendation:** Document this intentional difference or adjust to match Duelyst's 84% ratio.

### 9. Path Sprites — PASS

Plan correctly identifies that path sprites should have NO margins. They intentionally cross gaps for visual continuity.

### 10. Seam Sprites — PASS

Plan correctly identifies missing `corner_0_seam.png` and proposes adding it.

---

## Not-Yet-Implemented Features: Impact Assessment

### Future: Card Play Targeting

When implementing card targeting (spawn tiles, spell targets):
- `TileSpawnSprite`, `TileSpellSprite` in Duelyst use same margin system
- If we add these, they should use matching margins

### Future: Attackable Target Highlighting

Duelyst uses `tile_large.png` (128×128) for attackable targets, same margin as floor.
- Current plan doesn't include this
- Add to sprite inventory if implementing

### Future: Mana Spring Tiles (SDK.Tile)

Game object tiles (Mana Springs, etc.) are separate from highlight tiles.
- Rendered as entities, not grid overlays
- No impact from grid transition

### Future: Enemy Ownership Indicator

Duelyst shows red tile under enemy units (`TileMapGridSprite`):
- Uses same margin system as floor tiles
- If implementing, needs matching margins

### Future: Blob Dimming on Hover

Duelyst dims movement blob to 50% when hovering valid target:
- Implementation is opacity change, not geometry
- No impact from margin system

---

## Sanity Check: Fudged Numbers

### Claimed: "2.08% ratio (vs Duelyst's 2.34%) — visually indistinguishable"

**Verification:**
- Duelyst: 3/128 = 0.0234375 = 2.34%
- Sylvanshine 1x: 1/48 = 0.0208333 = 2.08%
- Difference: 0.26 percentage points

**At 48px tile:**
- Duelyst gap would be: 48 × 0.0234375 = 1.125px
- Sylvanshine gap: 1px
- Difference: 0.125px (12.5% smaller gap)

**Verdict:** Acceptable. Sub-pixel difference, not noticeable.

### Claimed: "46 is even, divides cleanly for corner quarters (23×23 each)"

**Verification:**
- 46/2 = 23 ✓
- Each quarter-tile visible content would be 23×23 ✓

**But this assumes symmetric margins, which we've shown is wrong.**

With asymmetric margins:
- TL corner: 23×23 visible (margin on left/top)
- Visible spans from (1,1) to (24,24) ✓ (meets at tile center)

**Verdict:** Correct math, but the margin approach needs fixing.

### Claimed: `tile_board.png` 128×128, visible 122×122 at +3+3

**Verification via ImageMagick:**
```
$ identify tile_board.png
tile_board.png PNG 128x128 128x128+0+0 8-bit sRGB
```

**Checking bounding box requires pixel inspection.** The atlas plist confirms:
```
tile_board.png: Atlas rect 2,2,122,122, sourceSize 128×128
```

**Verdict:** CORRECT — visible is 122×122 in 128×128 frame. 3px margin each side.

### Claimed: Corner sprites 64×64

**Verification:**
```
$ identify tile_merged_large_0.png
tile_merged_large_0.png PNG 64x64 64x64+0+0
```

**Verdict:** CORRECT.

---

## Corrected Implementation Plan

### Phase 1: Sprite Generation (REVISED)

```python
def generate_corner_with_asymmetric_margin(src: Path, dst: Path,
                                            target_size: int, margin: int,
                                            corner_rotation: int) -> None:
    """
    Generate corner sprite with margin only on outer edges.

    corner_rotation: 0=TL, 90=TR, 180=BR, 270=BL (degrees)
    """
    from PIL import Image

    img = Image.open(src).convert('RGBA')

    # Crop to visible content (remove source margins)
    bbox = img.getbbox()
    if bbox:
        img = img.crop(bbox)

    # Duelyst sprites are oriented for TL (0°) with margin on left/top
    # We need to rotate the SOURCE to get correct margin placement
    if corner_rotation != 0:
        img = img.rotate(-corner_rotation, expand=False)

    # Now resize: visible content fills (target_size - margin) on outer edges
    content_size = target_size - margin
    img = img.resize((content_size, content_size), Image.LANCZOS)

    # Paste at position that puts margin on correct edges for TL
    output = Image.new('RGBA', (target_size, target_size), (0, 0, 0, 0))
    output.paste(img, (margin, margin))  # TL has margin on left/top

    # Rotate back to final orientation
    if corner_rotation != 0:
        output = output.rotate(corner_rotation, expand=False)

    output.save(dst)
```

### Phase 2: Rendering (NO OFFSET NEEDED)

With asymmetric margins baked into sprites, corner rendering remains unchanged:

```cpp
// No offset needed — sprites have correct margin placement
float base_x = pos.x * ts + ((corner == 1 || corner == 2) ? half : 0);
float base_y = pos.y * ts + ((corner == 2 || corner == 3) ? half : 0);
// Draw sprite at base position, full quarter-tile size
```

### Phase 3-4: Unchanged

Disable grid lines and update colors as originally planned.

---

## Updated Verification Checklist

### Critical Fixes Required
- [ ] Change corner sprite generation to asymmetric margins
- [ ] Remove render-time offset logic (not needed with correct sprites)
- [ ] Verify asymmetric margins via visual inspection

### Original Checklist (Still Valid)
- [ ] Gap visible between adjacent floor tiles
- [ ] Corner sprite edges align with floor tile edges
- [ ] Highlights do NOT extend into gap area
- [ ] Full corner (`_0123`) fills quarter without gap
- [ ] 1x and 2x scales: gaps visible, pixel-perfect
- [ ] Movement blob has smooth rounded corners
- [ ] Attack blob renders in yellow
- [ ] Path rendering unchanged
- [ ] Entity rendering unaffected

---

## Summary

| Finding | Severity | Resolution |
|---------|----------|------------|
| Symmetric margin approach is wrong | **CRITICAL** | Use asymmetric margins |
| Render-time offset doesn't fix alignment | **CRITICAL** | Remove offset, fix sprites |
| Select box sizing undocumented | LOW | Document or adjust ratio |
| Unaffected systems verified | PASS | No changes needed |
| Ground truth numbers verified | PASS | Calculations correct |
