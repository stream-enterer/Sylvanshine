# Duelyst FX System - Implementation Guide

## Quick Reference: Key Concepts

**FX Definition** → data object with sprite/light/particle properties
**FX Identifier** → path string like "FX.Cards.Neutral.GhostLynx.UnitSpawnFX"
**FXType** → filter key (UnitSpawnFX, UnitAttackedFX, etc)
**FXSprite** → visual node created from FX definition
**impactAtEnd** → delays impact FX until animation completes (projectiles)
**emitFX** → child FX spawned during parent lifetime
**impactFX** → child FX spawned on impact

## Visual Event Flow

### Unit Spawn
```
1. ApplyCardToBoardAction
2. Filter: card.getFXResource() + Faction.fxResource → UnitSpawnFX
3. Create: Teleport effect + faction particles + light
4. Play: Fade in + spawn FX + unit appear animation
5. Duration: Math.max(teleport, particles, light)
```

### Unit Attack
```
1. AttackAction
2. Source filters:
   - attacker.getFXResource() → UnitAttackedFX, UnitPrimaryAttackedFX
3. Target filters:
   - union(attacker, target).getFXResource() → UnitDamagedFX
4. Sequence:
   a. sourceNode.showAttackState()
   b. sourceNode.showFX(sourceFX) ← attack swoosh, projectile
   c. [attack delay]
   d. targetNode.showAttackedState()
   e. targetNode.showFX(targetFX) ← impact, blood, damage numbers
5. Duration: attackAnim + projectileTravel + impactFX
```

### Spell Cast
```
1. PlayCardFromHandAction
2. Filters:
   - SpellCastFX (at caster)
   - SpellAppliedFX (at each target)
3. If spell damages: SpellDamagedFX
4. If spell heals: SpellHealedFX
5. Sequence:
   a. Card play animation
   b. SpellCastFX at general
   c. For each target: SpellAppliedFX + damage/heal reaction
```

## FX Positioning Modes

```javascript
// DEFAULT: spawn at target
{
  spriteIdentifier: "explosion"
  // appears at targetBoardPosition
}

// Source to target (projectiles)
{
  type: "Projectile",
  spriteIdentifier: "fireball",
  sourceToTarget: true,
  impactAtEnd: true
  // travels from sourceBoardPosition → targetBoardPosition
}

// Target to source (reverse beam)
{
  type: "Beam",
  targetToSource: true
  // draws from targetBoardPosition → sourceBoardPosition
}

// At source only
{
  spriteIdentifier: "charge_up",
  source: true
  // appears at sourceBoardPosition
}
```

## Timing Patterns

### Parallel FX (simultaneous)
```javascript
showDuration = 0.0
showDuration = Math.max(showDuration, sparksFX.getShowDelay())
showDuration = Math.max(showDuration, smokeFX.getShowDelay())
showDuration = Math.max(showDuration, lightFX.getShowDelay())
// All play at once, total = longest duration
```

### Sequential FX (one after another)
```javascript
showDuration = 0.0
showDuration += chargeUpFX.getShowDelay()
showDuration += beamFX.getShowDelay()
showDuration += impactFX.getShowDelay()
// Total = sum of all durations
```

### Projectile + Impact
```javascript
{
  type: "Projectile",
  spriteIdentifier: "arrow",
  sourceToTarget: true,
  impactAtEnd: true,        // wait until arrow reaches target
  impactFX: {
    spriteIdentifier: "hit_spark",
    emitFX: {
      type: "Light",
      radius: 100,
      duration: 0.0         // inherits hit_spark baseDuration
    }
  }
}

// Timing:
// arrow.getShowDelay() = arrow.baseDuration + impactFX.getShowDelay()
//                      = 0.8 + (0.3 + light duration)
//                      = 0.8 + (0.3 + 0.3) = 1.4s total
```

## Faction Visual Identity

Each faction has distinct spawn FX colors:

```javascript
Lyonar:    { teleport: white, light: yellow (255,200,100) }
Songhai:   { smoke: red, light: red (255,100,100) }
Vetruvian: { bladestorm: yellow, light: yellow (255,255,0) }
Abyssian:  { smoke: purple, light: purple (150,50,255) }
Magmar:    { lightning: green, light: green (127,255,127) }
Vanar:     { water: blue, light: cyan (0,255,255) }
Neutral:   { teleport: blue, light: white (255,255,255) }
```

## Critical FX Properties

### Must-implement for basic FX:
```javascript
{
  spriteIdentifier: "fx_name",  // RSX resource
  offset: {x, y},               // position offset
  color: {r, g, b},             // tint
  duration: 0.0,                // extra lifetime
  fadeInDuration: 0.0,
  fadeOutDuration: 0.0
}
```

### For projectiles:
```javascript
{
  type: "Projectile",
  sourceToTarget: true,
  impactAtEnd: true,
  impactFX: {...}
}
```

### For continuous effects:
```javascript
{
  looping: true,                // loop animation
  duration: -1,                 // infinite
  xyzRotationPerSecond: {x,y,z} // rotation
}
```

### For lights:
```javascript
{
  type: "Light",
  radius: 270,
  intensity: 9,
  color: {r, g, b},
  duration: 1,
  castsShadows: false
}
```

## Implementation Checklist

### Phase 1: Basic FX
- [ ] FX definition data structure (fx.js)
- [ ] FXType enum
- [ ] DATA.dataForIdentifier path resolver
- [ ] DATA.dataForIdentifiersWithFilter
- [ ] FXSprite base class
  - [ ] offset, color, opacity
  - [ ] duration, fadeIn, fadeOut
  - [ ] getBaseDuration(), getLifeDuration(), getShowDelay()
- [ ] NodeFactory.createFX basic version

### Phase 2: Spawning
- [ ] Faction spawn FX definitions
- [ ] Unit spawn sequence
  - [ ] Teleport/smoke effects
  - [ ] Faction-colored lights
  - [ ] Fade in timing

### Phase 3: Combat
- [ ] Attack state animations
- [ ] Attack FX (swooshes, projectiles)
- [ ] Damage FX (impacts, blood)
- [ ] FXProjectileSprite
  - [ ] sourceToTarget movement
  - [ ] impactAtEnd logic
  - [ ] impactFX spawning

### Phase 4: Advanced
- [ ] emitFX system
  - [ ] Child FX spawning
  - [ ] Duration inheritance
  - [ ] Timing calculation
- [ ] Light rendering
- [ ] Particle systems
- [ ] Continuous FX (artifacts, modifiers)
- [ ] 3D rotation effects

## Common Patterns

### Basic explosion at target
```javascript
{
  spriteIdentifier: RSX.fxExplosion.name,
  offset: {x: 0, y: -20}
}
```

### Projectile with impact
```javascript
{
  type: "Projectile",
  spriteIdentifier: RSX.fxArrow.name,
  sourceToTarget: true,
  impactAtEnd: true,
  impactFX: {
    spriteIdentifier: RSX.fxHitSpark.name
  }
}
```

### Effect with light
```javascript
{
  spriteIdentifier: RSX.fxFireball.name,
  emitFX: {
    type: "Light",
    offset: {x: 0, y: -40},
    radius: 200,
    intensity: 9,
    color: {r: 255, g: 100, b: 0}
  }
}
```

### Continuous modifier glow
```javascript
{
  spriteIdentifier: RSX.fxBuffGlow.name,
  looping: true,
  pulseScaleMin: 0.9,
  pulseScaleMax: 1.1,
  xyzRotationPerSecond: {x: 0, y: 0, z: 45}
}
```

## Performance Notes

- FX identifiers searched **backwards** (highest priority first)
- Each FXType found **only once** (prevents duplicates)
- Results cached in `_cache` and `_filterCache`
- Max FX limit enforced (can bypass with `noLimit: true`)
- Duration calculations use Math.max for parallel, += for sequential

## Debug Tips

1. Check FX identifier resolution:
   ```javascript
   DATA.dataForIdentifier("FX.Cards.Neutral.GhostLynx.UnitSpawnFX")
   ```

2. Verify filter priority:
   ```javascript
   // Processes backwards - last identifier has priority
   dataForIdentifiersWithFilter(
     ["FX.Factions.Faction1", "FX.Cards.Custom"],
     "UnitSpawnFX"
   )
   // Will use FX.Cards.Custom.UnitSpawnFX if exists,
   // else FX.Factions.Faction1.UnitSpawnFX
   ```

3. Trace timing:
   ```javascript
   sprite.getBaseDuration()      // animation length
   sprite.getLifeDuration()      // + emitFX time
   sprite.getShowDelay()         // + impactFX time
   sprite.getImpactDelay()       // when to spawn impactFX
   ```

4. Check position modes:
   - Default: both at target
   - sourceToTarget: travels
   - source: both at source
   - targetToSource: reverse travel
