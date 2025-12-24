# Grid System Transition Plan (Sprite-Based)

Migration from Sylvanshine's positive-line grid to Duelyst's negative-space tile grid using pre-scaled sprites with baked-in margins.

**Related Documents:**
- [Sylvanshine Grid Spec](sylvanshine_grid_spec.md)
- [Duelyst Grid Spec](duelyst_grid_spec.md)

---

## Executive Summary

| Aspect | Current (Sylvanshine) | Target (Duelyst-style) |
|--------|----------------------|------------------------|
| Grid Method | White lines | Tile gaps (no lines) |
| Floor Gap | 0% (fills cell) | 2.34% per side |
| Floor Opacity | 8% | 7.84% (~8%) |
| Corner Margin | 0% (fills quarter) | 2.34% of full tile (outer edges) |
| Attack Color | Red (255,100,100) | Yellow (#FFD900) |
| Sprite Source | Custom pre-scaled | Duelyst originals, pre-scaled with margins |

---

## Why Sprite-Based, Not Programmatic

### The Scaling Problem

Duelyst uses runtime scaling:
```
128px sprite × 0.742 scale = 95px rendered
3px margin × 0.742 = 2.23px rendered (fractional, anti-aliased)
```

Sylvanshine uses integer pre-scaling:
```
48px (1x) or 96px (2x) — no runtime scaling
Margins must be INTEGER pixels to avoid sub-pixel artifacts
```

### Programmatic Margin Issues

1. **Rounding errors** — `48 × 0.0234375 = 1.125px` requires floor/ceil decisions
2. **Margin calculations per frame** — adds complexity to hot render path
3. **Misaligned sprites** — existing sprites have no margins baked in

### Sprite-Based Advantages

1. **Integer margins baked into assets** — no runtime math
2. **Alignment guaranteed** — margins are pixel-perfect by construction
3. **Simpler rendering** — just draw sprite at tile position
4. **Uses Duelyst's proven artwork** — exact visual parity

---

## Ground Truth: Duelyst Sprites

### Source Sprites (dist/resources/tiles/)

| Sprite | Dimensions | Visible Content | Margin |
|--------|------------|-----------------|--------|
| `tile_board.png` | 128×128 | 122×122 at +3+3 | 3px |
| `tile_merged_large_0.png` | 64×64 | 61×61 at +3+3 | 3px outer |
| `tile_merged_large_0123.png` | 64×64 | 62×62 at +1+1 | 1px (connects) |
| `tile_hover.png` | 128×128 | 122×122 at +3+3 | 3px |
| `tile_path_move_*.png` | 128×128 | varies | varies |

### Critical Ratio

```
Floor gap ratio:  3 / 128 = 0.0234375 (2.34%)
Corner margin:    3 / 64  = 0.046875  (4.69% of half = 2.34% of full tile)
                  ✓ ALIGNED
```

---

## Sylvanshine Scale Mapping

### Current Build Pipeline

**Source:** `build_assets.py:529-596`

```python
tile_size = 48 * scale      # 48 (1x) or 96 (2x)
quarter_size = tile_size // 2  # 24 (1x) or 48 (2x)

# Corners: 64×64 → quarter_size
resize_image(src, dst, quarter_size, quarter_size)

# Floor/hover: 128×128 → tile_size
resize_image(src, dst, tile_size, tile_size)
```

### Margin Calculations for Integer Pixels

To match Duelyst's 2.34% gap ratio with integer pixels:

| Scale | Tile Size | Ideal Margin | Integer Margin | Actual Ratio |
|-------|-----------|--------------|----------------|--------------|
| 1x | 48px | 1.125px | **1px** | 2.08% |
| 2x | 96px | 2.25px | **2px** | 2.08% |

For corners (half-tile):

| Scale | Corner Size | Ideal Margin | Integer Margin | Actual Ratio |
|-------|-------------|--------------|----------------|--------------|
| 1x | 24px | 1.125px | **1px** | 4.17% of half = 2.08% of tile |
| 2x | 48px | 2.25px | **2px** | 4.17% of half = 2.08% of tile |

**Result:** 2.08% ratio (vs Duelyst's 2.34%) — visually indistinguishable, pixel-perfect.

---

## Implementation Plan

### Phase 1: Sprite Generation Pipeline

Modify `build_assets.py` to generate sprites with baked-in margins.

**File:** `build_assets.py`

#### 1.1 Add Helper Functions (after line 526, after `resize_image`)

```python
def resize_with_symmetric_margin(src_path: Path, dst_path: Path,
                                  target_size: int, margin: int) -> None:
    """
    Resize sprite with symmetric margin on all sides.
    Used for floor and hover tiles.

    Args:
        src_path: Source image (e.g., tile_board.png at 128×128)
        dst_path: Destination image
        target_size: Output dimensions (e.g., 48 or 96)
        margin: Transparent margin per side (e.g., 1 or 2)
    """
    content_size = target_size - (margin * 2)

    img = Image.open(src_path).convert('RGBA')
    # Crop to visible content (remove source's transparent margins)
    bbox = img.getbbox()
    if bbox:
        img = img.crop(bbox)
    img = img.resize((content_size, content_size), Image.LANCZOS)

    # Create output with margin on all sides
    output = Image.new('RGBA', (target_size, target_size), (0, 0, 0, 0))
    output.paste(img, (margin, margin))
    output.save(dst_path)


def resize_with_asymmetric_margin(src_path: Path, dst_path: Path,
                                   target_size: int, margin: int) -> None:
    """
    Resize sprite with asymmetric margin (TL orientation: margin on left/top only).
    Used for corner sprites. Rotation during rendering places margin on correct edges.

    Args:
        src_path: Source image (e.g., tile_merged_large_0.png at 64×64)
        dst_path: Destination image
        target_size: Output dimensions (e.g., 24 or 48)
        margin: Margin on left and top edges only (e.g., 1 or 2)
    """
    # Content extends to right and bottom edges (no margin there)
    content_size = target_size - margin

    img = Image.open(src_path).convert('RGBA')
    # Crop to visible content
    bbox = img.getbbox()
    if bbox:
        img = img.crop(bbox)
    img = img.resize((content_size, content_size), Image.LANCZOS)

    # Place at (margin, margin) — content extends to right/bottom edges
    output = Image.new('RGBA', (target_size, target_size), (0, 0, 0, 0))
    output.paste(img, (margin, margin))
    output.save(dst_path)
```

#### 1.2 Modify `generate_scaled_tiles` (lines 529-602)

Replace the existing function with:

```python
def generate_scaled_tiles(
    source_dir: Path,
    dist_dir: Path,
    force: bool = False,
    verbose: bool = False,
    output_files: set[Path] | None = None
) -> int:
    """Generate tile sprites for each scale level. Returns count of tiles generated."""
    src_dir = source_dir / 'resources' / 'tiles'
    generated = 0

    for scale in [1, 2]:
        dst_dir = dist_dir / 'resources' / 'tiles' / f's{scale}'
        dst_dir.mkdir(parents=True, exist_ok=True)

        tile_size = 48 * scale      # 48 or 96
        quarter_size = tile_size // 2  # 24 or 48
        margin = scale              # 1px (1x) or 2px (2x)

        # === Corner sprites (64×64 source → quarter_size with asymmetric margin) ===
        corner_map = {
            'tile_merged_large_0.png': 'corner_0.png',
            'tile_merged_large_01.png': 'corner_01.png',
            'tile_merged_large_03.png': 'corner_03.png',
            'tile_merged_large_013.png': 'corner_013.png',
            'tile_merged_large_0123.png': 'corner_0123.png',
            'tile_merged_large_0_seam.png': 'corner_0_seam.png',  # NEW: seam sprite
        }
        for src_name, dst_name in corner_map.items():
            src = src_dir / src_name
            dst = dst_dir / dst_name
            if src.exists():
                if output_files is not None:
                    output_files.add(dst)
                if force or needs_regeneration(src, dst):
                    if '_0123' in src_name:
                        # Filled corner: no margin (connects on all sides)
                        resize_image(src, dst, quarter_size, quarter_size)
                    else:
                        # Outer corners: asymmetric margin (TL orientation)
                        resize_with_asymmetric_margin(src, dst, quarter_size, margin)
                    generated += 1

        # === Floor and hover tiles (128×128 source → tile_size with symmetric margin) ===
        for src_name, dst_name in [('tile_board.png', 'floor.png'),
                                    ('tile_hover.png', 'hover.png')]:
            src = src_dir / src_name
            dst = dst_dir / dst_name
            if src.exists():
                if output_files is not None:
                    output_files.add(dst)
                if force or needs_regeneration(src, dst):
                    resize_with_symmetric_margin(src, dst, tile_size, margin)
                    generated += 1

        # === Select box (80×80 source → tile_size, NO margin — intentionally larger) ===
        src = src_dir / 'tile_box.png'
        dst = dst_dir / 'select_box.png'
        if src.exists():
            if output_files is not None:
                output_files.add(dst)
            if force or needs_regeneration(src, dst):
                resize_image(src, dst, tile_size, tile_size)
                generated += 1

        # === Path sprites (128×128 → tile_size, NO margin — cross gaps intentionally) ===
        path_map = {
            'tile_path_move_start.png': 'path_start.png',
            'tile_path_move_straight.png': 'path_straight.png',
            'tile_path_move_straight_from_start.png': 'path_straight_from_start.png',
            'tile_path_move_corner.png': 'path_corner.png',
            'tile_path_move_corner_from_start.png': 'path_corner_from_start.png',
            'tile_path_move_end.png': 'path_end.png',
            'tile_path_move_end_from_start.png': 'path_end_from_start.png',
        }
        for src_name, dst_name in path_map.items():
            src = src_dir / src_name
            dst = dst_dir / dst_name
            if src.exists():
                if output_files is not None:
                    output_files.add(dst)
                if force or needs_regeneration(src, dst):
                    resize_image(src, dst, tile_size, tile_size)
                    generated += 1

        if verbose:
            print(f"  Generated scale {scale} tiles in {dst_dir}")

    return generated
```

#### 1.3 Integration Summary

| Location | Change |
|----------|--------|
| After line 526 | Add `resize_with_symmetric_margin()` |
| After line 526 | Add `resize_with_asymmetric_margin()` |
| Lines 529-602 | Replace `generate_scaled_tiles()` with new version |
| Line 553 | Add `'tile_merged_large_0_seam.png': 'corner_0_seam.png'` to corner_map |
| Lines 567-577 | Change floor/hover to use `resize_with_symmetric_margin()` |
| Line 562 | Change corners to use `resize_with_asymmetric_margin()` (except _0123) |

### Phase 2: Corner Margin Logic

Corner sprites need **asymmetric margins** — margin only on outer edges, none on inner edges.

#### 2.1 Ground Truth from Duelyst

Duelyst's `tile_merged_large_0.png` (TL orientation):
```
File: 64×64
Visible: 61×61 at +3+3
Left margin: 3px, Top margin: 3px
Right margin: 0px, Bottom margin: 0px
```

**Critical insight:** Margins are ONLY on outer edges. Inner edges extend to the quarter-tile boundary so corners meet seamlessly at tile center.

#### 2.2 Corner Edge Analysis

```
Tile divided into 4 corners:
┌─────┬─────┐
│ TL  │ TR  │   TL = corner 0
├─────┼─────┤   TR = corner 1
│ BL  │ BR  │   BR = corner 2
└─────┴─────┘   BL = corner 3

Margin placement (outer edges only):
  TL: margin on LEFT and TOP      (right/bottom extend to center)
  TR: margin on RIGHT and TOP     (left/bottom extend to center)
  BR: margin on RIGHT and BOTTOM  (left/top extend to center)
  BL: margin on LEFT and BOTTOM   (right/top extend to center)
```

#### 2.3 Why Symmetric Margins + Offset FAILS

With symmetric 1px margin on all sides:
- TL visible content: (1,1) to (23,23)
- TR visible content: (25,1) to (47,23)
- **Gap at x=24!** (between 23 and 25)

Offsetting the sprite moves ALL edges, creating gaps on the opposite side.

**Only asymmetric margins work.**

#### 2.4 Sprite Generation for Corners (CORRECTED)

```python
def generate_corner_with_asymmetric_margin(src: Path, dst: Path,
                                            target_size: int, margin: int,
                                            corner: int) -> None:
    """
    Generate corner sprite with margin only on outer edges.

    corner: 0=TL, 1=TR, 2=BR, 3=BL
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

    # Crop source to visible content
    bbox = img.getbbox()
    if bbox:
        img = img.crop(bbox)

    # Resize to content dimensions
    img = img.resize((content_w, content_h), Image.LANCZOS)

    # Place on canvas with correct margin positioning
    output = Image.new('RGBA', (target_size, target_size), (0, 0, 0, 0))
    output.paste(img, (margin_left, margin_top))
    output.save(dst)
```

#### 2.5 Modified Tile Generation

```python
def generate_scaled_tiles(...):
    for scale in [1, 2]:
        tile_size = 48 * scale
        quarter_size = tile_size // 2
        margin = scale  # 1px (1x) or 2px (2x)

        # Generate 4 variants of each corner type
        for src_name, dst_base in corner_map.items():
            src = src_dir / src_name

            if '_0123' in src_name:
                # Filled corner: minimal uniform margin (connects on all sides)
                resize_with_margin(src, dst_dir / dst_base, quarter_size, 0)
            else:
                # Outer corners: need 4 variants with different margin placement
                # But Sylvanshine uses rotation, so we generate ONE sprite
                # with TL margin placement and rotate during rendering
                # OR we generate 4 separate sprites

                # Option 1: Single sprite, render handles rotation
                # (current approach — requires asymmetric source)
                generate_corner_with_asymmetric_margin(
                    src, dst_dir / dst_base, quarter_size, margin, corner=0
                )
```

**Note:** Since Sylvanshine's `render_corner_quad_rotated()` rotates the sprite based on corner position, and the source sprites have TL-oriented margins, the rotation automatically places margins on the correct outer edges.

### Phase 3: Rendering Changes

#### 3.1 Disable Grid Lines

**File:** `src/main.cpp`

```cpp
// Before (around line 1141):
state.grid_renderer.render(config);

// After: Remove or comment out
// Grid lines disabled — using tile gaps
```

#### 3.2 Floor Rendering (Unchanged)

Floor tiles already render at tile positions. With margins baked into sprites, gaps appear automatically.

#### 3.3 Corner Rendering (Unchanged)

**No offset needed.** With asymmetric margins baked into sprites:

1. Corner sprites are generated with TL-oriented margins (margin on left/top)
2. `render_corner_quad_rotated()` rotates the sprite 0°/90°/180°/270° based on corner position
3. Rotation automatically places margins on correct outer edges:
   - TL (0°): margin stays on left/top ✓
   - TR (90°): margin rotates to right/top ✓
   - BR (180°): margin rotates to right/bottom ✓
   - BL (270°): margin rotates to left/bottom ✓

**Current rendering code works as-is:**
```cpp
// No changes needed
float base_x = pos.x * ts + ((corner == 1 || corner == 2) ? half : 0);
float base_y = pos.y * ts + ((corner == 2 || corner == 3) ? half : 0);
// Rotation: TL=0°, TR=90°, BR=180°, BL=270°
float rotation_deg = corner * 90.0f;
```

### Phase 4: Color Updates

**File:** `include/grid_renderer.hpp`

```cpp
namespace TileColor {
    // Movement: Duelyst uses #F0F0F0 (slightly off-white)
    constexpr SDL_FColor MOVE_CURRENT = {0.941f, 0.941f, 0.941f, 200.0f/255.0f};

    // Attack: Duelyst's signature yellow #FFD900
    constexpr SDL_FColor ATTACK_CURRENT = {1.0f, 0.851f, 0.0f, 200.0f/255.0f};
}
```

---

## Edge Cases

### E1: Rounding at 1x Scale

At 48px tiles with 1px margin (symmetric, all sides):
- Visible content: 46×46 pixels

At 24px corners with 1px asymmetric margin (TL orientation):
- Visible content: 23×23 pixels (24 - 1 on left, 24 - 0 on right)
- Content spans from (1,1) to (24,24) — meets at tile center ✓

**Verdict:** No sub-pixel issues at 1x.

### E2: Rounding at 2x Scale

At 96px tiles with 2px margin (symmetric, all sides):
- Visible content: 92×92 pixels

At 48px corners with 2px asymmetric margin (TL orientation):
- Visible content: 46×46 pixels (48 - 2 on left, 48 - 0 on right)
- Content spans from (2,2) to (48,48) — meets at tile center ✓

**Verdict:** No sub-pixel issues at 2x.

### E3: Corner Sprite Variants

Duelyst has 6 corner variants: `_0`, `_01`, `_03`, `_013`, `_0123`, `_0_seam`

Sylvanshine currently has 5: `corner_0`, `corner_01`, `corner_03`, `corner_013`, `corner_0123`

**Missing:** `_0_seam` — used for boundaries between move and attack blobs.

**Resolution:** Add seam sprite generation:
```python
corner_map = {
    ...
    'tile_merged_large_0_seam.png': 'corner_0_seam.png',
}
```

### E4: Hover Sprite Alignment

`tile_hover.png` has same margin structure as `tile_board.png`. When scaled with matching margins, hover highlight aligns exactly with floor tiles.

### E5: Path Sprites Overlapping Gaps

Path sprites (`path_start.png`, etc.) extend to full tile boundaries — they intentionally cross the gap to create visual continuity.

**No change needed** — current behavior is correct.

### E6: Select Box Sprite

`tile_box.png` is 80×80 (not 128×128). Current scaling produces select boxes slightly larger than tiles.

**Resolution:** Apply same margin logic, or keep as overlay that intentionally extends beyond tile.

---

## Sprite Inventory

### Required Output (per scale)

| Sprite | Size (1x) | Size (2x) | Margin | Source |
|--------|-----------|-----------|--------|--------|
| `floor.png` | 48×48 | 96×96 | 1px / 2px | `tile_board.png` |
| `hover.png` | 48×48 | 96×96 | 1px / 2px | `tile_hover.png` |
| `corner_0.png` | 24×24 | 48×48 | 1px / 2px | `tile_merged_large_0.png` |
| `corner_01.png` | 24×24 | 48×48 | 1px / 2px | `tile_merged_large_01.png` |
| `corner_03.png` | 24×24 | 48×48 | 1px / 2px | `tile_merged_large_03.png` |
| `corner_013.png` | 24×24 | 48×48 | 1px / 2px | `tile_merged_large_013.png` |
| `corner_0123.png` | 24×24 | 48×48 | 0px | `tile_merged_large_0123.png` |
| `corner_0_seam.png` | 24×24 | 48×48 | 1px / 2px | `tile_merged_large_0_seam.png` |
| `path_*.png` | 48×48 | 96×96 | 0px | `tile_path_move_*.png` |
| `select_box.png` | 48×48 | 96×96 | 0px | `tile_box.png` |

### Visible Content After Margins

| Scale | Floor (symmetric) | Corner (asymmetric) | Corner (filled) |
|-------|-------------------|---------------------|-----------------|
| 1x | 46×46 | 23×23 | 24×24 |
| 2x | 92×92 | 46×46 | 48×48 |

**Note:** Corner sprites have asymmetric margins (outer edges only), so visible content is `size - margin` not `size - 2*margin`.

---

## Verification Checklist

### Alignment Tests
- [ ] Gap visible between adjacent floor tiles
- [ ] Corner sprite edges align with floor tile edges
- [ ] Highlights do NOT extend into gap area
- [ ] Full corner (`_0123`) fills quarter without gap

### Scale Tests
- [ ] 1x scale: gaps visible, pixel-perfect
- [ ] 2x scale: gaps visible, pixel-perfect
- [ ] Proportions identical at both scales

### Visual Tests
- [ ] Movement blob has smooth rounded corners
- [ ] Attack blob renders in yellow (#FFD900)
- [ ] Seam visible between adjacent move/attack blobs
- [ ] Hover highlight aligns with floor edges
- [ ] No grid lines visible

### Regression Tests
- [ ] Path rendering unchanged
- [ ] Select box positioning unchanged
- [ ] Entity rendering unaffected

---

## Implementation Order

1. **Sprite Pipeline** — Modify `build_assets.py`:
   - Add `resize_with_margin()` for floor/hover (symmetric margins)
   - Add `generate_corner_with_asymmetric_margin()` for corners
2. **Add Seam Sprite** — Include `corner_0_seam.png` in corner_map
3. **Regenerate Assets** — `./build.fish assets`
4. **Disable Grid Lines** — Comment out `render()` call in main.cpp
5. **Update Colors** — Change attack color to yellow in grid_renderer.hpp
6. **Test Alignment** — Visual inspection at 1x and 2x scales

**Note:** No rendering code changes needed — asymmetric margins + rotation handles alignment automatically.

---

## Rollback Plan

### Quick Rollback (< 1 minute)
1. Re-enable grid line rendering in main.cpp
2. Gaps remain but are less noticeable with lines

### Full Rollback
1. Revert `build_assets.py` margin changes
2. Regenerate assets: `./build.fish assets`
3. Uncomment grid line rendering
4. Revert color changes

---

## Ground Truth Reference

```
DUELYST (128px base, 0.742x runtime scale):
  Floor: 128×128, 3px symmetric margin, 122×122 visible
  Corner: 64×64, 3px ASYMMETRIC margin (outer edges only), 61×61 visible
  Gap ratio: 3/128 = 2.34%

SYLVANSHINE 1x (48px, no runtime scale):
  Floor: 48×48, 1px symmetric margin, 46×46 visible
  Corner: 24×24, 1px ASYMMETRIC margin (outer edges only), 23×23 visible
  Gap ratio: 1/48 = 2.08%

SYLVANSHINE 2x (96px, no runtime scale):
  Floor: 96×96, 2px symmetric margin, 92×92 visible
  Corner: 48×48, 2px ASYMMETRIC margin (outer edges only), 46×46 visible
  Gap ratio: 2/96 = 2.08%

ALIGNMENT:
  Floor outer edge at: margin pixels inset
  Corner outer edge at: margin pixels inset (matches floor)
  Corner inner edge at: tile center (no margin, corners meet)
  ✓ ALIGNED
```
