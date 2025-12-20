# Particle

## Source Evidence
- Primary definitions: `app/resources/particles/` (63 plist files)
- FX integration: `app/data/fx.js`
- View system: `app/view/nodes/fx/FXParticleSystemSprite.js`

## Fields
| Field | Type | Source | Required? |
|-------|------|--------|-----------|
| name | string | Plist filename | yes |
| maxParticles | number | Maximum particle count | yes |
| particleLifespan | number | Lifetime in seconds | yes |
| particleLifespanVariance | number | Lifetime variance | no |
| startParticleSize | number | Initial size in pixels | yes |
| finishParticleSize | number | Final size on death | no |
| speed | number | Initial velocity | yes |
| speedVariance | number | Speed variance | no |
| angle | number | Emission angle (degrees) | yes |
| angleVariance | number | Angle variance | no |
| emitterType | number | 0=gravity, 1=radial | yes |
| duration | number | Emitter duration (-1=infinite) | yes |

## Motion Fields
| Field | Type | Purpose |
|-------|------|---------|
| gravityx/gravityy | number | Gravity forces |
| radialAcceleration | number | Inward/outward acceleration |
| tangentialAcceleration | number | Circular acceleration |
| rotatePerSecond | number | Rotation rate |

## Color Fields
| Field | Type | Purpose |
|-------|------|---------|
| startColorRed/Green/Blue/Alpha | number | Initial color (0-1) |
| finishColorRed/Green/Blue/Alpha | number | Final color (0-1) |
| startColorVariance* | number | Color randomness |
| finishColorVariance* | number | Death color randomness |

## Blend Modes
| Value | OpenGL Constant | Purpose |
|-------|-----------------|---------|
| 1 | GL_ONE | Additive blending |
| 770 | GL_SRC_ALPHA | Standard alpha |
| 771 | GL_ONE_MINUS_SRC_ALPHA | Transparency |

## Particle Categories
| Category | Examples | Typical Settings |
|----------|----------|------------------|
| Explosions | fireexplosion, explosion | High speed, short life |
| Ambient | manaswirl, ash | Low speed, long life |
| Trails | projectile_trail | Follow source |
| Magic | teleport_ball, sparkle | Radial, colorful |

## Dependencies
- Requires: Cocos2d particle system, texture atlas
- Used by: FX system, unit abilities, spell effects

## Description
Particle systems define emitter-based visual effects using the Cocos2d particle engine. Configuration is stored in plist (XML) format with parameters for emission, motion, color, and rendering. The system supports both gravity-based and radial particle motion patterns.

## Statistics
- **63 particle systems** extracted
- Max particles: 30-200 typical
- Lifespans: 0.25s to 8s
- Most use additive blending (GL_ONE)
