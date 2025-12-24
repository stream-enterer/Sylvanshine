# Duelyst Implementation Differences

This document notes intentional differences between Sylvanshine's tile rendering and the original Duelyst implementation.

---

## Path Sprite Rendering Approach

**Duelyst:**
- Path sprites stored as raw content dimensions (e.g., `tile_path_move_straight.png` = 128×12)
- Rendered with uniform scaling: `sprite_scale = CONFIG.TILESIZE / sprite.width`
- Sprite centered in tile at scaled dimensions

**Sylvanshine:**
- Path sprites stored on square canvases (128×128) with content centered in transparent padding
- Rendered by stretching canvas to fill tile quad
- Content scales proportionally within the stretched canvas

**Why equivalent:**
```
Duelyst:     12px content × (95/128) = 8.9px rendered
Sylvanshine: 12px content × (96/128) = 9.0px rendered
```

The padded square canvas approach produces visually identical results while simplifying the rendering code (no per-sprite dimension handling needed).

---

## Target Tile Sprite Choice

**Duelyst:**
- Uses `tile_target.png` for movement hover target (filled diamond/card shape)
- Pulses at 0.85-1.0 scale over 0.7s

**Sylvanshine:**
- Uses `tile_box.png` (bracket corners) for movement hover target
- Same pulsing behavior (0.85-1.0 scale over 0.7s)

**Rationale:**
The bracket corners provide clearer visual feedback for the movement destination, matching the select box shown on the selected unit. The filled `tile_target.png` shape was less visually distinct.

---

## Bloom Post-Processing

**Duelyst:**
- Global bloom applied (CONFIG.BLOOM_DEFAULT = 0.7)
- Causes bright white pixels to "bleed" outward
- Makes path lines and tile edges appear softer/thicker

**Sylvanshine:**
- Bloom infrastructure exists but is not currently active
- Rendering goes directly to swapchain without post-processing
- Results in crisper/sharper tile edges

**Status:** Under consideration. The opacity adjustments (path at 59%, glow at 20%) partially compensate for the lack of bloom.
