# Shadow Implementation Comparison: Duelyst vs Sylvanshine

## Purpose
This document provides a factual comparison of shadow rendering between Duelyst's original implementation and Sylvanshine's current implementation, without assuming which approach is correct.

---

## 1. Uniform Default Values

| Parameter | Duelyst | Sylvanshine | Notes |
|-----------|---------|-------------|-------|
| `u_intensity` | **0.15** | **0.5** | Sylvanshine uses higher value |
| `u_blurShiftModifier` | 1.0 | 1.0 | Same |
| `u_blurIntensityModifier` | 3.0 | 3.0 | Same |
| `shadowOffset` / `SHADOW_OFFSET` | 19.5 | 19.5 | Same |

**Source:**
- Duelyst: `app/view/fx/FX.js:35` → `shadowIntensity: 0.15`
- Sylvanshine: `include/gpu_renderer.hpp:156` → `shadow_intensity = 0.5f`

---

## 2. Blur Calculation

### Duelyst (ShadowHighQualityFragment.glsl:31-32)
```glsl
float blurX = (1.0/u_size.x * u_blurIntensityModifier) * occluderDistPctBlurModifier;
float blurY = (1.0/u_size.y * u_blurIntensityModifier) * occluderDistPctBlurModifier;
```

### Sylvanshine (shadow_perpass.frag:49-51)
```glsl
float scaleCompensation = max(u_renderScale, 0.25);
float blurX = (u_blurIntensityModifier / u_size.x / scaleCompensation) * occluderDistPctBlurModifier;
float blurY = (u_blurIntensityModifier / u_size.y / scaleCompensation) * occluderDistPctBlurModifier;
```

**Difference:** Sylvanshine divides by `scaleCompensation` (render scale). Duelyst does not.

**Context:** Duelyst rendered at a fixed internal resolution and scaled the final output. Sylvanshine renders sprites at variable scales directly, hence the compensation.

---

## 3. Texture Sampling

### Duelyst
```glsl
alpha += texture2D(CC_Texture0, v_texCoord + vec2(xn3, yn3)).a * boxWeight;
```
- Raw texture sampling
- No explicit UV clamping in shader
- Relies on GPU texture wrap mode (likely CLAMP_TO_EDGE)
- When sampling outside sprite bounds, gets edge pixels

### Sylvanshine
```glsl
float sampleAlpha(vec2 uv) {
    vec2 clampedUV = clamp(uv, vec2(0.0), vec2(1.0));
    return texture(u_texture, clampedUV).a;
}
```
- Explicit UV clamping to [0,1] range
- When blur samples outside sprite, clamps to edge UV
- This may produce slightly different edge behavior

**Potential Impact:** Edge clamping in shader vs GPU texture mode may produce subtle differences in how the blur looks at sprite edges.

---

## 4. Final Alpha Calculation

### Duelyst (ShadowHighQualityFragment.glsl:121)
```glsl
gl_FragColor = vec4(0.0, 0.0, 0.0, min(1.0, lightDistPctInv * alpha * intensity * v_fragmentColor.a));
```

### Sylvanshine (shadow_perpass.frag:70)
```glsl
fragColor = vec4(0.0, 0.0, 0.0, min(1.0, u_lightDistPctInv * alpha * intensity * u_opacity));
```

**Same formula structure.** The multipliers are:
- `lightDistPctInv` / `u_lightDistPctInv` - light distance attenuation
- `alpha` - blur result
- `intensity` - computed intensity with falloff
- `v_fragmentColor.a` / `u_opacity` - external opacity control

**Difference in source:**
- Duelyst: `v_fragmentColor.a` comes from vertex attribute `a_color.a` (light color alpha)
- Sylvanshine: `u_opacity` comes from uniform (entity opacity, typically ~0.78)

---

## 5. Light Distance Attenuation

### Duelyst (ShadowHighQualityFragment.glsl:17-21)
```glsl
vec3 lightDiff = v_mv_lightPosition - v_mv_castedAnchorPosition;
float lightDist = length(lightDiff);
float lightDistPct = pow(lightDist / v_mv_lightRadius, 2.0);
float lightDistPctInv = 1.0 - lightDistPct;
```

### Sylvanshine (gpu_renderer.cpp:1780-1794)
```cpp
float lightDistPctInv = 1.0f;
if (active_light) {
    float dx = feet_pos.x - active_light->x;
    float dy = feet_pos.y - active_light->y;
    float lightDist = std::sqrt(dx * dx + dy * dy);
    float lightDistPct = std::pow(lightDist / active_light->radius, 2.0f);
    lightDistPctInv = std::max(0.0f, 1.0f - lightDistPct);
    lightDistPctInv *= active_light->a * active_light->intensity;
}
```

**Differences:**
1. Duelyst calculates in 3D modelview space; Sylvanshine in 2D screen space
2. Sylvanshine adds `max(0.0f, ...)` clamp (Duelyst doesn't)
3. Sylvanshine multiplies by `active_light->a * active_light->intensity`

---

## 6. Vertex Shader / Shadow Geometry

### Duelyst (ShadowVertex.glsl)
- Complex 45° cast matrix with dynamic skew based on light position
- Calculates stretch based on altitude and skew modifiers
- Handles shadow flip based on depth difference
- Outputs 3D casted anchor position for fragment shader

### Sylvanshine (shadow.vert + gpu_renderer.cpp)
- Simplified: Pre-calculates shadow quad on CPU with fixed 0.7 squash
- No dynamic skew based on light position
- No shadow flip logic
- Passes 2D local position for blur calculation

**This is a significant architectural difference.** Duelyst's shadows react dynamically to light position; Sylvanshine's are fixed projection.

---

## 7. Rendering Context

### Duelyst
- Per-sprite FBO with UV 0-1 covering entire sprite
- Texture sampler likely set to CLAMP_TO_EDGE by Cocos2d
- Always has a scene light for shadow calculations

### Sylvanshine
- Per-sprite FBO with UV 0-1 (same approach)
- Explicit UV clamping in shader
- May or may not have a scene light (defaults to full intensity if none)

---

## 8. Summary of Potential Differences

| Aspect | Possible Effect |
|--------|-----------------|
| Intensity 0.5 vs 0.15 | Shadows ~3.3x darker in Sylvanshine |
| Scale compensation | Makes blur consistent across scales (Sylvanshine-specific) |
| UV clamping in shader | May affect edge blur appearance |
| No dynamic shadow skew | Shadows don't tilt toward/away from light |
| No stretch modifier | Shadow length is fixed vs dynamic in Duelyst |
| Light alpha multiplication | Additional dimming factor in Sylvanshine |

---

## 9. Questions for Resolution

1. **Should intensity remain at 0.5 or revert to 0.15?**
   - 0.15 is Duelyst authentic
   - 0.5 was chosen to compensate for "faint" shadows

2. **Should scale compensation be kept?**
   - Duelyst didn't have it
   - But Duelyst had fixed internal resolution

3. **Should UV clamping be changed?**
   - Current: Shader-side clamp to [0,1]
   - Alternative: Rely on GPU sampler CLAMP_TO_EDGE

4. **Should dynamic shadow geometry be implemented?**
   - Duelyst calculated skew/stretch per-vertex based on light
   - Sylvanshine uses fixed projection

---

## 10. Files Referenced

**Duelyst:**
- `app/shaders/ShadowHighQualityFragment.glsl`
- `app/shaders/ShadowVertex.glsl`
- `app/view/fx/FX.js`
- `app/common/config.js`

**Sylvanshine:**
- `shaders/shadow_perpass.frag`
- `shaders/shadow.vert`
- `src/gpu_renderer.cpp`
- `include/gpu_renderer.hpp`
