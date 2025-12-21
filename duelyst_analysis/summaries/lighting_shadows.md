# Lighting and Shadow System

## Overview

Duelyst uses a pseudo-3D isometric lighting system where 2D screen positions are transformed into 3D coordinates for lighting calculations. Understanding this coordinate transformation is critical for porting to other engines.

## Coordinate System

### Cocos2d Convention (Y-up)
- **Screen Y increases upward** (bottom of screen = small Y, top = large Y)
- **Higher screen Y = farther from camera** (background)
- **Lower screen Y = closer to camera** (foreground)

### Key Transformation: Light.js lines 319-322

```javascript
// Screen position is transformed into 3D lighting coordinates
const altitude = Math.sqrt(node.radius) * 6 * scale;
const { y } = this.mvPosition3D;
this.mvPosition3D.y = this.mvPosition3D.z + altitude;  // Y becomes HEIGHT
this.mvPosition3D.z = y;  // Z becomes DEPTH (from screen Y)
```

**After transformation:**
| Axis | Meaning | Source |
|------|---------|--------|
| X | Horizontal position | Screen X (unchanged) |
| Y | Height/altitude | Original Z + altitude offset |
| Z | Depth (camera distance) | Original screen Y |

## Shadow Direction (Flip Logic)

### ShadowVertex.glsl lines 27-30

```glsl
float mv_depth = mv_anchorPosition.z;  // Sprite's depth
float mv_depthDifference = mv_depth - v_mv_lightPosition.z;  // vs light's depth
float flip = mv_depthDifference + u_anchor.y < 0.0 ? -1.0 : 1.0;
```

Where `u_anchor.y` = `CONFIG.DEPTH_OFFSET` = 19.5 pixels.

### Flip Behavior

| Condition | Flip Value | Shadow Direction |
|-----------|------------|------------------|
| Sprite behind light (sprite.z > light.z) | +1 | Downward (toward camera) |
| Sprite in front of light (sprite.z < light.z) | -1 | Upward (away from camera) |

### Cast Matrix

The flip value modifies the shadow projection matrix (ShadowVertex.glsl lines 38-43):

```glsl
mat4 castMatrix = mat4(
    1.0, 0.0, 0.0, 0.0,
    skew, 0.7071067811865475 * flip, -0.7071067811865475 * flip, 0.0,
    0.0, 0.7071067811865475 * flip, 0.7071067811865475 * flip, 0.0,
    0.0, 0.0, 0.0, 1.0
);
```

The 0.7071 value is cos(45°) / sin(45°) for isometric projection.

## Light Positioning

### BattleMap.js Light Offsets

Lights are positioned relative to screen center using `_battlemapOffset`:

```javascript
// BattleMap.js line 277-282
var position = cc.p(winCenterPosition.x, winCenterPosition.y);
var battlemapOffset = light._battlemapOffset;
if (battlemapOffset) {
    position.x += battlemapOffset.x;
    position.y += battlemapOffset.y;
}
light.setPosition(position);
```

### Preset Light Positions

| Map | X Offset | Y Offset | Description |
|-----|----------|----------|-------------|
| BATTLEMAP0 | +width×3.25 | **-height×3.15** | Light BELOW center (foreground) |
| BATTLEMAP1 | +height×3.25 | **+height×3.75** | Light ABOVE center (background) |
| BATTLEMAP2 | +height×3.25 | +height×3.75 | Light above-right |
| BATTLEMAP3 | -height×3.25 | +height×3.75 | Light above-left |

**Note:** In Cocos2d Y-up, negative Y offset = below center = foreground (close to camera).

## Shadow Rendering Pipeline

### 1. Shadow Caster Registration (BaseSprite.js)

```javascript
// Line 445
setupShadowCasting() {
    this._renderCmd.setOcclusionNeedsRebuild();
    this.getFX().addShadowCaster(this);
}
```

### 2. Light Batching (BatchLights.js lines 35-60)

```javascript
// Light position passed to shader via vertex attributes
const mvPosition3D = object.getMVPosition3D();  // After Y/Z swap
const cx = mvPosition3D.x;
const cy = mvPosition3D.y;  // Height
const cz = mvPosition3D.z;  // Depth (original screen Y)
```

### 3. Shadow Drawing (BaseSprite.js lines 1290-1326)

```javascript
proto.drawShadows = function () {
    // Set shadow shader uniforms
    shadowProgram.setUniformLocationWith2f(
        shadowProgram.loc_anchor,
        node._anchorPoint.x * width,
        offset  // shadowOffset, typically 19.5
    );
    // Render with shadow casting lights
    this._batchLighting.renderWithShadowCastingLights();
}
```

## Porting to SDL/Y-Down Coordinates

### The Critical Difference

**SDL convention (Y-down):**
- Screen Y increases downward (top = small Y, bottom = large Y)
- Higher screen Y = closer to camera (foreground)
- Lower screen Y = farther from camera (background)

This is the **opposite** of Cocos2d's isometric depth mapping.

### Depth Calculation Fix

When porting to SDL, the depth difference calculation must be inverted:

```cpp
// Cocos2d/Duelyst (Y-up):
float depth_diff = sprite_depth - light_depth;  // sprite.y - light.y

// SDL (Y-down) - MUST INVERT:
float depth_diff = light_depth - sprite_depth;  // light.y - sprite.y
```

### Why This Works

| Engine | High Screen Y | Isometric Meaning | depth_diff Sign |
|--------|---------------|-------------------|-----------------|
| Cocos2d | Top of screen | Background (far) | sprite - light |
| SDL | Bottom of screen | Foreground (close) | light - sprite (inverted) |

By inverting the subtraction, the depth_diff value has the same semantic meaning (positive = sprite behind light) in both coordinate systems.

## Key Files

| File | Lines | Purpose |
|------|-------|---------|
| `app/view/nodes/fx/Light.js` | 319-322 | Y/Z coordinate swap |
| `app/shaders/ShadowVertex.glsl` | 27-43 | Depth-based flip and cast matrix |
| `app/view/layers/game/BattleMap.js` | 968-1220 | Light position presets |
| `app/view/fx/BatchLights.js` | 35-60 | Light data to shader |
| `app/view/nodes/BaseSprite.js` | 1290-1326 | Shadow rendering call |
| `app/common/config.js` | DEPTH_OFFSET | 19.5 pixel threshold |

## Constants

| Constant | Value | Purpose |
|----------|-------|---------|
| `CONFIG.DEPTH_OFFSET` | 19.5 | Shadow flip threshold (u_anchor.y) |
| `CONFIG.globalScale` | varies | Light radius scaling factor |
| Isometric angle | 45° | cos/sin = 0.7071067811865475 |

## Shadow Quality Settings

From `app/common/config.js`:

```javascript
CONFIG.SHADOW_QUALITY_LOW = 0;
CONFIG.SHADOW_QUALITY_HIGH = 1;
```

Different shader programs are used based on quality:
- `ShadowLowQuality` - Simpler shadow rendering
- `ShadowHighQuality` - Full shadow features
