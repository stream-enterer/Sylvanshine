# System: Particles

**Location:** app/resources/particles/, app/data/fx.js

## Purpose
The particle system provides emitter-based visual effects for explosions, trails, ambient effects, and magical abilities. It uses Cocos2d's particle engine with plist-based configuration.

## Key Components
| Component | File | Purpose |
|-----------|------|---------|
| Particle Plists | resources/particles/*.plist | Emitter configs |
| FX Definitions | data/fx.js | High-level FX with particles |
| FXParticleSystemSprite | view/nodes/fx/ | Particle rendering |
| FX System | view/fx/FX.js | Effect composition |

## Data Flow
**Input:** FX trigger from game action
**Processing:** Load plist → Configure emitter → Spawn particles
**Output:** Animated particle effect

## Particle Configuration (Plist)
```xml
<dict>
  <key>maxParticles</key>
  <integer>100</integer>
  <key>particleLifespan</key>
  <real>0.5</real>
  <key>speed</key>
  <real>200</real>
  <key>angle</key>
  <real>0</real>
  <key>angleVariance</key>
  <real>360</real>
</dict>
```

## Emitter Types
| Type | Value | Behavior |
|------|-------|----------|
| Gravity | 0 | Linear motion with gravity |
| Radial | 1 | Spiral/circular motion |

## Key Parameters
| Parameter | Range | Purpose |
|-----------|-------|---------|
| maxParticles | 30-200 | Particle limit |
| particleLifespan | 0.25-8s | Individual lifetime |
| speed | 0-200 | Initial velocity |
| angle/Variance | 0-360° | Emission direction |
| radialAcceleration | -400 to 0 | Inward pull |
| tangentialAcceleration | -300 to 300 | Spin |

## Particle Categories
| Category | Examples | Characteristics |
|----------|----------|-----------------|
| Explosions | fireexplosion, explosion | High speed, short life |
| Ambient | manaswirl, ash | Low speed, long life |
| Trails | projectile_trail | Follow source position |
| Magic | teleport_ball, sparkle | Radial motion, colorful |
| UI | card_fade, zodiac_appear | Designed for menus |

## Blend Modes
| Value | OpenGL | Effect |
|-------|--------|--------|
| 1 | GL_ONE | Additive (glow) |
| 770 | GL_SRC_ALPHA | Standard transparency |
| 771 | GL_ONE_MINUS_SRC_ALPHA | Normal blend |

## FX.js Integration
```javascript
{
  type: 'Particles',
  plistFile: RSX.ptcl_projectile_trail.plist,
  positionType: 'PARTICLE_TYPE_FOLLOW',
  duration: 0.5,
  scale: 0.8
}
```

## Dependencies
**Requires:** Cocos2d particle engine, texture atlas
**Used by:** FX system, combat effects, spell visuals

## Statistics
- **63 particle systems** in resources/particles/
- Lifespans: 0.25s to 8s
- Most use additive blending
- Integrated with 240+ FX definitions
