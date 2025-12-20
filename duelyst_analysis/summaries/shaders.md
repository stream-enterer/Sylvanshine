# System: Shaders

**Location:** app/shaders/

## Purpose
The shader system provides GPU-accelerated visual effects using WebGL/GLSL. Shaders implement effects like dissolve, bloom, lighting, and distortion that would be too expensive to compute on the CPU.

## Key Components
| Component | File | Purpose |
|-----------|------|---------|
| Fragment Shaders | *.glsl | Pixel-level effects |
| Vertex Shaders | *Vertex.glsl | Geometry transforms |
| Helper Modules | helpers/*.glsl | Shared functions |
| FX Sprites | view/nodes/fx/FX*.js | Shader integration |
| RenderingInjections | view/extensions/ | Shader loading |

## Data Flow
**Input:** Uniform parameters from JavaScript
**Processing:** Vertex → Fragment shader pipeline
**Output:** Rendered pixels with effects

## Shader Categories

### Fragment Shaders (30+)
| Shader | Purpose |
|--------|---------|
| DissolveFragment | Death/despawn effect |
| BloomFragment | Glow accumulation |
| ChromaticFragment | Color aberration |
| DistortionFragment | Wave effects |
| FireRingFragment | Fire animations |
| HighlightFragment | Glow/highlight |
| LightingFragment | Dynamic lighting |

### Vertex Shaders (15+)
| Shader | Purpose |
|--------|---------|
| ChromaticVertex | Aberration positioning |
| DistortionVertex | Wave distortion |
| GlowVertex | Glow geometry |
| LightingVertex | Light calculations |
| HighlightVertex | Highlight geometry |

### Helper Modules (20+)
| Module | Purpose |
|--------|---------|
| Noise2D/Noise3D | Procedural noise |
| Fresnel | Edge glow effect |
| Reflection | Reflective surfaces |
| Refraction | Light bending |

## Common Uniforms
```glsl
uniform float u_time;           // Animation time
uniform vec2 u_resolution;      // Texture size
uniform vec4 u_color;           // Effect color
uniform float u_phase;          // Animation phase (0-1)
uniform float u_frequency;      // Noise frequency
uniform float u_amplitude;      // Effect intensity
uniform float u_threshold;      // Brightness cutoff
```

## Shader Integration (JS)
```javascript
// FXChromaticFlareSprite.js
shaderProgram.setUniformLocationWith1f(loc_time, getTime() * speed);
shaderProgram.setUniformLocationWith1f(loc_frequency, frequency);
shaderProgram.setUniformLocationWith1f(loc_amplitude, amplitude);
```

## Dependencies
**Requires:** WebGL context, Cocos2d shader cache
**Used by:** FX sprites, BaseSprite, lighting system

## Statistics
- **96 shader files** total
- 30+ fragment shaders
- 15+ vertex shaders
- 20+ helper utilities
- Used in all major visual effects
