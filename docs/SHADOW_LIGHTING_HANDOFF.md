# Shadow & Lighting System Handoff

## Summary

Sylvanshine now has a complete Duelyst-authentic shadow and lighting system with:
- Per-sprite penumbra blur (sharp at feet, soft toward head)
- Dynamic shadow geometry (skew/stretch based on light position)
- 10 keyboard-switchable lighting presets extracted from Duelyst battle maps

---

## Quick Reference

### Keyboard Controls
| Key | Preset | Shadow Direction |
|-----|--------|------------------|
| 0 | Sylvanshine Default | Point light (divergent) |
| 1 | Lyonar Highlands | Down-left |
| 2 | Songhai Temple | Left |
| 3 | Vetruvian Dunes | Left (intensity 1.1) |
| 4 | Abyssian Depths | Down-right |
| 5 | Magmar Peaks | Down-right (intensity 0.85) |
| 6 | Vanar Wastes | Down-right (intensity 0.75) |
| 7 | Shimzar Jungle | Right (dimmer light) |
| 8 | Shadow Creep | Right (low altitude) |
| 9 | Redrock Canyon | Left (warm ambient) |

### Other Rendering Toggles
- `M` - Toggle multi-pass rendering
- `B` - Toggle bloom
- `V` - Toggle vignette
- `L` - Toggle dynamic lighting

---

## Architecture

### Files Modified

**Shaders:**
- `shaders/shadow.frag` - Atlas-based shadow fragment shader
- `shaders/shadow_perpass.frag` - Per-sprite FBO shadow shader (primary)
- `shaders/shadow.vert` - Shadow vertex shader

**C++ Core:**
- `src/gpu_renderer.cpp` - `draw_shadow_from_pass()` with dynamic geometry
- `include/gpu_renderer.hpp` - `ShadowUniforms`, `FXConfig`, `PointLight`
- `src/main.cpp` - Lighting presets, keyboard handling

### Shadow Rendering Flow

```
1. Sprite rendered to per-sprite FBO (RenderPass)
2. draw_shadow_from_pass() called with sprite FBO + light
3. CPU calculates:
   - Light altitude: sqrt(radius) * 6 * scale
   - Skew: tan(atan(dx / altitude)) * 0.5
   - Stretch: altitude/depth modifiers (capped at 1.6)
4. Shadow quad vertices transformed with skew/stretch
5. Fragment shader applies:
   - 7x7 box blur (scaled by distance from feet)
   - Intensity falloff (pow curves from Duelyst)
   - Light distance attenuation
```

### Key Formulas (from Duelyst)

**Light altitude:**
```cpp
float altitude = std::sqrt(light->radius) * 6.0f * scale;
```

**Skew (horizontal tilt):**
```cpp
float skew_angle = std::atan2(dx * flip, altitude);
float skew = std::tan(skew_angle) * 0.5f;
```

**Blur gradient:**
```glsl
float occluderDistPctY = min(max(anchorDiff.y, 0.0) / u_size.x, 1.0);
float blurModifier = pow(occluderDistPctY, u_blurShiftModifier);
```

**Intensity falloff:**
```glsl
float intensityFadeX = pow(1.0 - occluderDistPctX, 1.25);
float intensityFadeY = pow(1.0 - occluderDistPctY, 1.5);
```

---

## Duelyst Lighting Analysis

### Per-Map Configuration Pattern

Duelyst uses a **template + tuning** approach:

**Systematic (all maps):**
- Primary light radius: `TILESIZE * 1000` = 95,000 pixels (sun-like)
- Fill light radius: `TILESIZE * 30` (non-shadow-casting)
- Falloff modifier: 2.0
- Intensity modifier: 2.0

**Per-map tuning:**
- Light X position: Left (-3.25) or Right (+3.25) based on faction/theme
- Light Y position: High (3.75) or Medium (1.5-3.15)
- Shadow intensity: 0.75 to 1.1 (artistic mood)
- Light opacity: 255 (launch maps) or 200 (expansion maps)
- Ambient color: Grayscale or warm tint

### Shadow Intensity Values

| Intensity | Maps | Mood |
|-----------|------|------|
| 0.75 | Vanar Wastes | Cold, low contrast |
| 0.80 | Redrock, Vanar | Warm/cold extremes |
| 0.85 | Magmar Peaks | Volcanic |
| 1.00 | Most maps | Baseline |
| 1.05 | BATTLEMAP6 | Slightly dramatic |
| 1.10 | Vetruvian | Harsh desert sun |

**Important:** The FX default of 0.15 is never used at runtime - all maps override it.

---

## Remaining Work

### Immediate (in KANBAN)
- [ ] Sync lighting presets with actual battle map backgrounds

### Future Enhancements
- Per-sprite ambient light color (Duelyst has this for background layers)
- Multiple shadow-casting lights (Duelyst supports this but rarely uses it)
- Dynamic light animation (fade in/out, position changes)

---

## Testing Checklist

1. **Preset switching**: Press 0-9, verify console shows preset name and values
2. **Shadow direction**: Compare presets 1 (down-left) vs 4 (down-right)
3. **Shadow intensity**: Compare preset 6 (0.75, faint) vs 3 (1.1, dark)
4. **Blur gradient**: Shadow should be sharp at unit's feet, soft at head
5. **Light distance**: Move unit away from light center, shadow should fade

---

## Code Locations

| Feature | File | Line/Function |
|---------|------|---------------|
| Lighting presets | `src/main.cpp` | `g_lighting_presets[]`, `apply_lighting_preset()` |
| Dynamic geometry | `src/gpu_renderer.cpp` | `draw_shadow_from_pass()` ~line 1775 |
| Blur calculation | `shaders/shadow_perpass.frag` | Lines 47-48 |
| Intensity falloff | `shaders/shadow_perpass.frag` | Lines 51-53 |
| FX config defaults | `include/gpu_renderer.hpp` | `FXConfig` struct |

---

## References

**Duelyst Source (read-only):**
- `app/shaders/ShadowHighQualityFragment.glsl` - Original blur/intensity formulas
- `app/shaders/ShadowVertex.glsl` - Original skew/stretch/cast matrix
- `app/view/layers/game/BattleMap.js` - Per-map light configurations
- `app/view/fx/FX.js` - FX system defaults and setters
- `app/view/nodes/fx/Light.js` - Light altitude calculation (line 319)
