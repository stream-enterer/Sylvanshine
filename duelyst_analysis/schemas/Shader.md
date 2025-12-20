# Shader

## Source Evidence
- Primary location: `app/shaders/` (76+ GLSL files)
- Loading: `app/view/extensions/RenderingInjections.js`
- Configuration: FX sprite classes in `app/view/nodes/fx/`

## Fields
| Field | Type | Source | Required? |
|-------|------|--------|-----------|
| name | string | File basename | yes |
| file | string | Relative path | yes |
| type | enum | vertex/fragment/helper | yes |
| uniforms | string[] | Uniform declarations | no |
| attributes | string[] | Attribute declarations | no |
| defines | string[] | Preprocessor defines | no |

## Shader Types
| Type | Purpose | Examples |
|------|---------|----------|
| vertex | Transform vertices | ChromaticVertex, LightingVertex |
| fragment | Pixel coloring | DissolveFragment, BloomFragment |
| helper | Shared functions | Noise2D, Fresnel, Reflection |

## Common Uniforms
| Uniform | Type | Purpose |
|---------|------|---------|
| u_time | float | Animation time |
| u_resolution | vec2 | Screen/texture resolution |
| u_color | vec3/vec4 | Effect color |
| u_phase | float | Animation phase (0-1) |
| u_frequency | float | Noise/wave frequency |
| u_amplitude | float | Effect intensity |
| u_threshold | float | Brightness threshold |
| u_intensity | float | Glow/highlight intensity |

## Shader Categories
| Category | Count | Purpose |
|----------|-------|---------|
| FX Effects | 15+ | Visual effects (chromatic, dissolve, distortion) |
| Lighting | 4 | Dynamic lighting and shadows |
| Post-processing | 5 | Bloom, blur, screen effects |
| Helpers | 20+ | Noise, fresnel, reflection utilities |

## Key Shaders
| Shader | Purpose |
|--------|---------|
| DissolveFragment | Death/despawn effect |
| ChromaticFragment | Color aberration |
| BloomFragment | Glow accumulation |
| LightingFragment | Dynamic lighting |
| DistortionFragment | Wave distortion |
| FireRingFragment | Fire ring animations |

## Dependencies
- Requires: WebGL context, Cocos2d shader cache
- Used by: FX sprites, BaseSprite, lighting system

## Description
Shaders implement GPU-based visual effects using GLSL. The system includes vertex shaders for geometry transformation, fragment shaders for pixel manipulation, and helper modules for shared functions like noise generation. Shaders are loaded via the Cocos2d shader cache system.

## Statistics
- **96 shader files** extracted
- 30+ fragment shaders
- 15+ vertex shaders
- 20+ helper utilities
