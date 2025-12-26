# SDF Techniques for Sprite-Based Tiles: Analysis

## Question

Would PristineGrid-style derivative-based anti-aliasing improve edge rendering of sprite-based tiles and highlight sprites in Sylvanshine?

---

## Short Answer: No, Not Directly

The PristineGrid technique **cannot** be applied to pre-rendered sprite textures in the same way.

| Aspect | Procedural Grid | Sprite Textures |
|--------|-----------------|-----------------|
| Edge definition | Mathematical (computed in shader) | Baked into pixels |
| Shader knows edge location | Yes — computes distance to grid line | No — only sees RGBA samples |
| Derivatives useful for | Computing exact AA width | Texture filtering (different problem) |

The `ddx`/`ddy` trick works because the shader **knows the shape mathematically**. For sprites, the shader only sees pixel values — it doesn't know where edges "should" be.

---

## Current Renderer State

```cpp
// gpu_renderer.cpp:78-79
sampler_info.min_filter = SDL_GPU_FILTER_NEAREST;
sampler_info.mag_filter = SDL_GPU_FILTER_NEAREST;
```

Nearest-neighbor filtering preserves pixel-art crispness but causes jagged edges when sprites are transformed (rotated, scaled, perspective-warped).

---

## Options That WOULD Improve Sprite Edges

### Option 1: SDF Textures (Best Quality, Most Work)

Convert highlight sprites to signed distance field textures. Store distance-to-edge instead of color. Then PristineGrid-style AA works perfectly.

**How it works**:
- Each pixel stores distance to nearest edge (negative inside, positive outside)
- Shader computes `smoothstep` based on screen-space derivatives
- Result: resolution-independent, perfect edges at any transform

**Candidates for SDF conversion** (simple geometric shapes):
- `floor.png`, `hover.png` — rounded rectangles
- `corner_*.png` — quarter-circles with varying neighbor states
- `glow.png`, `target.png` — circular/ring shapes
- `path_*.png` — path segment shapes

**Benefit**: Resolution-independent, perfect edges at any transform.

**Cost**: Requires new asset pipeline + shader changes. SDF textures typically single-channel (grayscale), color applied in shader.

---

### Option 2: Procedural SDF Rendering (No Textures)

Render highlights as procedural SDFs directly in shader — no textures at all.

**Example shader code**:
```glsl
// Rounded rectangle SDF
float sdRoundedBox(vec2 p, vec2 size, float radius) {
    vec2 q = abs(p) - size + radius;
    return length(max(q, 0.0)) + min(max(q.x, q.y), 0.0) - radius;
}

// Anti-aliased edge
float alpha = 1.0 - smoothstep(-aa_width, aa_width, sdf);
```

**Mapping current tiles to SDFs**:

| Current Texture | Procedural SDF |
|-----------------|----------------|
| `floor.png` | `sdRoundedBox()` with small radius |
| `hover.png` | `sdRoundedBox()` with glow falloff |
| `corner_0.png` | Quarter of `sdRoundedBox()` |
| `corner_01.png` | `sdRoundedBox()` minus one corner |
| `corner_0123.png` | Full `sdRoundedBox()` |
| `glow.png` | `sdCircle()` with soft falloff |
| `target.png` | `sdCircle()` ring (abs of circle SDF) |
| `path_*.png` | Capsule/box SDFs with rotation |

**Benefit**:
- Perfect quality at any resolution/transform
- Smaller memory footprint (no textures)
- Easier to animate (just change parameters)
- Natural support for the corner-merging system (already computes topology)

**Cost**:
- Rewrite tile rendering system
- More shader complexity
- Some shapes (path corners) need careful SDF construction

---

### Option 3: Linear Filtering + Premultiplied Alpha (Quick Win)

Switch to linear filtering for highlight sprites:

```cpp
sampler_info.min_filter = SDL_GPU_FILTER_LINEAR;
sampler_info.mag_filter = SDL_GPU_FILTER_LINEAR;
```

Requires premultiplied alpha in source textures to avoid dark halos at edges.

**Benefit**: Simple change, immediate improvement for transformed sprites.

**Cost**:
- Slightly softer appearance (may not suit pixel-art aesthetic)
- Requires re-exporting textures with premultiplied alpha
- Still resolution-dependent

---

### Option 4: MSAA (Hardware Anti-Aliasing)

Enable multi-sample anti-aliasing at render target level.

**Benefit**: Improves all polygon edges including sprite quads.

**Cost**:
- Performance overhead (2x-8x sample count)
- Doesn't help texture sampling artifacts (only geometry edges)
- May interfere with post-processing effects

---

### Option 5: Hybrid Approach

Keep textures for complex artwork, use procedural SDF for simple geometric highlights.

**Texture-based** (keep as-is):
- Entity sprites (complex artwork)
- Floor tiles with patterns
- UI elements with fine detail

**Procedural SDF** (convert):
- Move range highlights (rounded rects)
- Attack range highlights (rounded rects)
- Hover indicator
- Selection box
- Path segments
- Glow effects

---

## Recommendation

For **highlights specifically**, procedural SDF rendering (Option 2) is ideal because:

1. **Simple shapes**: Highlights are rounded rectangles, circles, and corners — trivial SDFs
2. **Already computed topology**: The corner-merging system (`get_corner_neighbors()`) computes neighbor state, which maps directly to SDF shape selection
3. **Perfect under perspective**: SDF + derivative AA produces crisp edges regardless of transform
4. **No asset management**: No need to regenerate textures at different scales

**Implementation priority**:
1. Start with move/attack range blobs (most visible, simple rounded rects)
2. Add hover and selection indicators
3. Convert path rendering
4. Keep floor tiles as textures (they work fine)

---

## Reference: PristineGrid Applicability

From `PristineGrid.shader` analysis:

The technique works for **any** rasterized rendering where:
- Shape is defined mathematically (not baked into texture)
- Fragment shader has access to `dFdx`/`dFdy` derivatives
- UV coordinates are interpolated across the primitive

This includes Sylvanshine's pseudo-3D perspective — the derivatives automatically capture the perspective distortion and adjust AA width accordingly.

---

## Files Referenced

- `gpu_renderer.cpp:78-79` — Current sampler configuration
- `grid_renderer.cpp` — Tile highlight rendering system
- `PristineGrid.shader` — Reference implementation of derivative-based AA
- `docs/sdf_manual/analytical-antialiasing-manual.md` — SDF primitive library
