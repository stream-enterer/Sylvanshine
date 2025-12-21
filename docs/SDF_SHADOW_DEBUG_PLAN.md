# SDF Shadow Systematic Debug Plan

## Status: RESOLVED

The original problem statement was incorrect. The issue was **shadow position** (above vs below sprite), not silhouette orientation.

## Actual Problem

Shadow was rendering on the **wrong side** of the sprite relative to light position. This was caused by a **Y-axis coordinate system mismatch** between Duelyst (Cocos2d Y-up) and Sylvanshine (SDL Y-down).

## Root Cause (from Duelyst Archaeology)

Duelyst's `Light.js:319-322` swaps screen Y into depth Z:
```javascript
this.mvPosition3D.z = y;  // Original screen Y becomes DEPTH
```

In Cocos2d Y-up: high screen Y = far from camera (background)
In SDL Y-down: high screen Y = close to camera (foreground)

The depth calculation `sprite.y - light.y` gives the OPPOSITE sign in SDL vs Cocos2d.

## Solution Applied

**Inverted the depth calculation** in both shadow functions:

```cpp
// Before (wrong - Cocos2d convention):
float depth_diff = feet_pos.y - active_light->y;

// After (correct - SDL convention):
float depth_diff = active_light->y - feet_pos.y;  // INVERTED
```

This makes Sylvanshine's depth semantics match Duelyst's isometric shadow projection.

---

## Original Debug Plan (for reference)

## Why Previous Attempts Failed

We were making single-variable tweaks (swapping UV coords) without understanding the full coordinate transformation pipeline. Each change affected multiple things (UV sampling AND shader fade logic), causing unexpected interactions.

## Structured Approach

### Phase 1: Map the Complete Coordinate Pipeline

Before changing anything, document EVERY coordinate transformation from source to screen:

```
SDF Atlas Image File
    ↓ (PIL/numpy loads image)
build_assets.py generates SDF
    ↓ (row 0 = top of image, V=0)
SDL_image loads PNG
    ↓ (does it flip? check SDL GPU texture orientation)
GPU Texture Memory
    ↓ (V=0 is where?)
Vertex shader receives UV
    ↓ (passthrough)
Fragment shader samples texture
    ↓ (texture() function)
Final pixel color
```

**Action**: Add a single debug sprite that's asymmetric (e.g., arrow pointing up) to verify each stage.

### Phase 2: Isolate and Test Each Component

#### Test 1: Verify SDF Atlas Orientation
```bash
# View actual SDF file
python3 -c "from PIL import Image; img=Image.open('dist/resources/units/f1_001_sdf.png'); img.show()"
```
Document: Is head at top (row 0) or bottom of each frame?

#### Test 2: Verify Texture Loading Orientation
Temporarily modify `sdf_shadow.frag` to output raw V coordinate as color:
```glsl
fragColor = vec4(v_texCoord.y, 0.0, 0.0, 1.0);  // Red = V value
```
- If top of shadow quad is RED (V≈1): texture V is flipped from image
- If top of shadow quad is BLACK (V≈0): texture V matches image

#### Test 3: Verify Geometry Orientation
Temporarily render shadow without SDF, just solid color with gradient:
```glsl
fragColor = vec4(0.0, 0.0, 0.0, 1.0 - v_texCoord.y);  // Gradient top to bottom
```
Verify: Does the gradient go from dark (near sprite) to light (far from sprite)?

#### Test 4: Verify Local Coords for Fade
Output local Y as color:
```glsl
fragColor = vec4(v_localPos.y / u_spriteSize.y, 0.0, 0.0, 1.0);
```
- Near edge should be RED (localPos.y = spriteSize.y = feet)
- Far edge should be BLACK (localPos.y = 0 = head)

### Phase 3: Document Actual vs Expected at Each Stage

Create a truth table:

| Stage | Variable | Expected Value (near edge) | Expected Value (far edge) | Actual |
|-------|----------|---------------------------|--------------------------|--------|
| Screen Y | y_near, y_far | smaller Y (top) | larger Y (bottom) | ? |
| NDC Y | vertices[0].y | +1 (top) | -1 (bottom) | ? |
| Texture V | v_texCoord.y | feet V coord | head V coord | ? |
| Local Y | v_localPos.y | sprite_h (feet) | 0 (head) | ? |
| SDF sample | texture().r | feet SDF | head SDF | ? |

### Phase 4: Identify the Single Point of Failure

Once the table is filled, there will be ONE row where Expected ≠ Actual. That's the real bug.

Possible failure points:
1. **SDF atlas is upside-down** → fix in `build_assets.py`
2. **SDL GPU flips texture V** → compensate in UV calculation
3. **NDC Y conversion wrong** → fix `to_ndc_y()`
4. **Local coords miscalculated** → fix vertex attribute assignment

### Phase 5: Single Targeted Fix

Only after identifying the exact failure point, make ONE change that addresses it at the source, not downstream compensation.

## Constraints to Formalize

The shadow system must satisfy these rules:

1. **Anchor rule**: The shadow anchor (where shadow meets ground at sprite feet) MUST align with `feet_pos` on screen
2. **Orientation rule**: The part of the SDF representing sprite FEET must appear at the shadow anchor
3. **Fade rule**: Shadow opacity MUST be maximum at anchor, fading toward projection
4. **Direction rule**: Shadow MUST extend AWAY from the light source

## Implementation Checklist

- [ ] Run Test 1: Document SDF atlas orientation in image file
- [ ] Run Test 2: Verify texture V orientation after GPU load
- [ ] Run Test 3: Verify geometry orientation independent of SDF
- [ ] Run Test 4: Verify local coords match expected fade pattern
- [ ] Fill in truth table with actual values
- [ ] Identify single failure point
- [ ] Implement targeted fix
- [ ] Verify all 4 constraints are satisfied

## Code Locations for Each Test

| Test | File | Modification |
|------|------|--------------|
| Test 1 | terminal | Python one-liner to view image |
| Test 2 | `shaders/sdf_shadow.frag` | Temporary fragColor override |
| Test 3 | `shaders/sdf_shadow.frag` | Temporary gradient output |
| Test 4 | `shaders/sdf_shadow.frag` | Temporary localPos visualization |

After each test, REVERT the shader change before proceeding.
