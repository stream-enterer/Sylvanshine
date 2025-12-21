# SDF Shadow Orientation - Handoff Document

## Status: FIXED - Coordinate system inversion corrected

## Problem History

### Original Issue (RESOLVED)
SDF shadows were rendering on the **wrong vertical side** of the sprite (above vs below) for all lighting presets. This was caused by a Y-axis coordinate system mismatch between Duelyst (Cocos2d Y-up) and Sylvanshine (SDL Y-down).

### Root Cause: Y-Axis Inversion

**Duelyst's coordinate flow (from archaeology of original source):**

1. **Light.js:319-322** - Critical Y/Z swap:
```javascript
const { y } = this.mvPosition3D;
this.mvPosition3D.y = this.mvPosition3D.z + altitude;
this.mvPosition3D.z = y;  // Original screen Y becomes DEPTH
```

2. **Cocos2d Y-up convention:**
   - High screen Y (top of screen) = far from camera (background)
   - Low screen Y (bottom of screen) = close to camera (foreground)

3. **ShadowVertex.glsl:27-30** - Depth comparison for flip:
```glsl
float mv_depthDifference = mv_depth - v_mv_lightPosition.z;
float flip = mv_depthDifference + u_anchor.y < 0.0 ? -1.0 : 1.0;
```

**SDL Y-down convention (inverted from Cocos2d):**
- High screen Y (bottom of screen) = close to camera (foreground)
- Low screen Y (top of screen) = far from camera (background)

The original Sylvanshine code used `depth_diff = sprite.y - light.y`, which gives the OPPOSITE sign in SDL coords compared to what Duelyst expects.

### Fix Applied

Changed the depth calculation in both shadow functions to invert the subtraction order:

**gpu_renderer.cpp:1853** (`draw_shadow_from_pass`):
```cpp
// Before (wrong):
float depth_diff = sprite_depth - light_depth;

// After (correct):
float depth_diff = light_depth - sprite_depth;  // INVERTED for SDL coords
```

**gpu_renderer.cpp:2122** (`draw_sdf_shadow`):
```cpp
// Before (wrong):
float depth_diff = feet_pos.y - active_light->y;

// After (correct):
float depth_diff = active_light->y - feet_pos.y;  // INVERTED for SDL coords
```

## Archaeological Evidence

### BattleMap.js Light Positions

Duelyst's battle map light offsets (in Cocos2d Y-up coordinates, relative to screen center):

| Map | X Offset | Y Offset | Description |
|-----|----------|----------|-------------|
| BATTLEMAP0 | +width*3.25 | **-height*3.15** | Light BELOW center (close to camera) |
| BATTLEMAP1 | +height*3.25 | **+height*3.75** | Light ABOVE center (far from camera) |
| BATTLEMAP2 | +height*3.25 | +height*3.75 | Light above |
| BATTLEMAP3 | -height*3.25 | +height*3.75 | Light above-left |

Note: Negative Y offset in Cocos2d = below center = foreground = close to camera.

### Light.js Coordinate Transform

```javascript
// Lines 319-322: Y and Z are swapped for 3D lighting
const altitude = Math.sqrt(node.radius) * 6 * scale;
const { y } = this.mvPosition3D;
this.mvPosition3D.y = this.mvPosition3D.z + altitude;  // Y = height
this.mvPosition3D.z = y;  // Z = original screen Y (depth)
```

### ShadowVertex.glsl Flip Logic

```glsl
// Lines 27-30: Depth-based flip for shadow direction
float mv_depth = mv_anchorPosition.z;  // Sprite's depth
float mv_depthDifference = mv_depth - v_mv_lightPosition.z;  // Light's depth
float flip = mv_depthDifference + u_anchor.y < 0.0 ? -1.0 : 1.0;
```

Where `u_anchor.y` = `CONFIG.DEPTH_OFFSET` = 19.5 pixels.

### BatchLights.js - Light Position to Shader

```javascript
// Lines 37-40: mvPosition3D (with Y/Z swap) is passed to shader
const mvPosition3D = object.getMVPosition3D();
const cx = mvPosition3D.x;
const cy = mvPosition3D.y;  // Height (altitude)
const cz = mvPosition3D.z;  // Depth (original screen Y)
```

## Key Insight

The critical insight is that **Duelyst treats screen Y position as isometric depth** through the Y/Z swap in Light.js. Higher screen Y (in Cocos2d Y-up) means farther from camera.

In SDL Y-down, this relationship is inverted: higher screen Y means closer to camera. Therefore, the depth calculation must be inverted to produce the same shadow behavior.

## Verification

After applying the fix, shadows should:
- Cast **downward** when light is at top of screen (background, far from camera)
- Cast **upward** when light is at bottom of screen (foreground, close to camera)

This matches Duelyst's isometric shadow projection behavior across all lighting presets.

## Architecture

### Files
- `src/gpu_renderer.cpp`: `draw_sdf_shadow()` and `draw_shadow_from_pass()` - shadow geometry
- `shaders/sdf_shadow.frag`: Fragment shader - SDF sampling and alpha calculation
- `shaders/sdf_shadow.vert`: Vertex shader - passthrough
- `build_assets.py`: `generate_sdf_atlas()` - creates SDF textures from sprites

### Coordinate Systems

**Screen coords**: Y increases downward (SDL standard)
**NDC**: Y increases upward (-1 bottom, +1 top)
**Texture V**: Typically 0 at top, 1 at bottom (OpenGL standard)

### Duelyst Reference Files

| File | Key Lines | Purpose |
|------|-----------|---------|
| `app/view/nodes/fx/Light.js` | 319-322 | Y/Z swap for depth |
| `app/view/layers/game/BattleMap.js` | 968-1220 | Light position presets |
| `app/shaders/ShadowVertex.glsl` | 27-30 | Depth-based flip logic |
| `app/view/fx/BatchLights.js` | 35-60 | Light data to shader |
| `app/view/nodes/BaseSprite.js` | 1290-1326 | Shadow rendering call |
