# Duelyst Timing System - Complete Specification

This document specifies the complete timing system for Duelyst, including attack timing, death sequences, movement, and all related constants.

---

## Table of Contents

1. [Attack System](#1-attack-system)
2. [Death System](#2-death-system)
3. [Movement System](#3-movement-system)
4. [Animation Frame Timing](#4-animation-frame-timing)
5. [Config Constants](#5-config-constants)
6. [Sound Event Timing](#6-sound-event-timing)
7. [Data File Formats](#7-data-file-formats)
8. [Implementation Guide](#8-implementation-guide)

---

## 1. Attack System

### Timeline

```
┌─────────────────────────────────────────────────────────────────────────┐
│                         ATTACK SEQUENCE                                  │
├─────────────────────────────────────────────────────────────────────────┤
│                                                                          │
│  [Action Queued]                                                         │
│       │                                                                  │
│       ▼                                                                  │
│  ┌─────────────────────────────────────────────┐                        │
│  │  PRE-ATTACK DELAY (attackDelay)             │                        │
│  │  Default: 0.0s                              │                        │
│  │  Range: 0.0 - 1.5s                          │                        │
│  │  Purpose: Anticipation pause before swing   │                        │
│  └─────────────────────────────────────────────┘                        │
│       │                                                                  │
│       ▼                                                                  │
│  [Attack Animation Starts]                                               │
│  [Attack Swing Sound Plays]                                              │
│       │                                                                  │
│       ▼                                                                  │
│  ┌─────────────────────────────────────────────┐                        │
│  │  RELEASE POINT (attackReleaseDelay)         │  ◄── DAMAGE HERE       │
│  │  Default: 0.0s                              │  ◄── Impact Sound      │
│  │  Range: 0.0 - 0.2s                          │  ◄── Impact FX         │
│  │  Purpose: Point in anim where damage hits   │                        │
│  └─────────────────────────────────────────────┘                        │
│       │                                                                  │
│       ▼                                                                  │
│  [Animation Continues to End]                                            │
│       │                                                                  │
│       ▼                                                                  │
│  [Return to Idle/Breathing State]                                        │
│                                                                          │
└─────────────────────────────────────────────────────────────────────────┘
```

### Key Insight: Damage at Animation START

**95%+ of units have attackReleaseDelay: 0.0**

This means damage applies the instant the attack animation begins, NOT at the midpoint. The animation is purely visual feedback after the damage has already been applied.

### Source Code References

**UnitNode.js (attack sound timing):**
```javascript
const releaseDelay = (this.getAnimResource() && this.getAnimResource().attackReleaseDelay) || 0.0;
```

**GameLayer.js (pre-attack delay):**
```javascript
const attackDelay = (_currentActionSourceNode.getAnimResource().attackDelay || 0.0) 
                    * CONFIG.ENTITY_ATTACK_DURATION_MODIFIER;
```

### Timing Values Distribution

**attackDelay (pre-animation pause):**

| Range | Typical Units | Feel |
|-------|--------------|------|
| 0.0s | Default (most minions) | Instant |
| 0.2-0.3s | Fast attackers (Songhai) | Quick |
| 0.4-0.5s | Standard units | Normal |
| 0.6-0.9s | Heavy hitters | Weighty |
| 1.0-1.25s | Generals, big units | Dramatic |
| 1.5s | Slow generals | Very dramatic |

**attackReleaseDelay (damage point in animation):**

| Value | Usage |
|-------|-------|
| 0.0s | 95%+ of all units (damage at anim start) |
| 0.2s | ~4 Songhai generals (multi-hit feel) |

### Data File Format

`timing/unit_timing.tsv`:
```
unit_folder	card_id	attack_delay	attack_release_delay	source_file
neutral_golem_steel	Cards.Neutral.GolemMetallurgist	0.5	0.0	neutral.coffee
f2_general_tier2	Cards.Faction2.General	0.5	0.2	faction2.coffee
```

---

## 2. Death System

### Timeline

```
┌─────────────────────────────────────────────────────────────────────────┐
│                         DEATH SEQUENCE                                   │
├─────────────────────────────────────────────────────────────────────────┤
│                                                                          │
│  [HP reaches 0]                                                          │
│       │                                                                  │
│       ▼                                                                  │
│  ┌─────────────────────────────────────────────┐                        │
│  │  1. TERMINATE SUPPORT NODES                 │                        │
│  │     Duration: CONFIG.FADE_MEDIUM_DURATION   │                        │
│  │     (0.35s)                                 │                        │
│  └─────────────────────────────────────────────┘                        │
│       │                                                                  │
│       ▼                                                                  │
│  ┌─────────────────────────────────────────────┐                        │
│  │  2. PLAY DEATH ANIMATION                    │                        │
│  │     Duration: From animation data           │                        │
│  │     (~0.5 - 1.0s typical)                   │                        │
│  │     Holds on final frame                    │                        │
│  └─────────────────────────────────────────────┘                        │
│       │                                                                  │
│       ▼                                                                  │
│  ┌─────────────────────────────────────────────┐                        │
│  │  3. START DISSOLVE                          │                        │
│  │     - Apply dissolve shader                 │                        │
│  │     - Spawn death particles                 │                        │
│  │       (ptcl_unit_dissolve)                  │                        │
│  │     - Destroy child nodes                   │                        │
│  └─────────────────────────────────────────────┘                        │
│       │                                                                  │
│       ▼                                                                  │
│  ┌─────────────────────────────────────────────┐                        │
│  │  4. DISSOLVE TWEEN                          │                        │
│  │     Duration: CONFIG.FADE_SLOW_DURATION     │                        │
│  │     (1.0s)                                  │                        │
│  │     Tween dissolve uniform: 0.0 → 1.0       │                        │
│  └─────────────────────────────────────────────┘                        │
│       │                                                                  │
│       ▼                                                                  │
│  ┌─────────────────────────────────────────────┐                        │
│  │  5. DESTROY                                 │                        │
│  │     Remove unit from scene                  │                        │
│  └─────────────────────────────────────────────┘                        │
│                                                                          │
│  TOTAL DURATION: death_anim + 1.0s (dissolve)                           │
│  TYPICAL: 1.5 - 2.0 seconds                                             │
│                                                                          │
└─────────────────────────────────────────────────────────────────────────┘
```

### Dissolve Shader

**File:** `DissolveFragment.glsl` (or inline in EntitySprite)

**Uniforms:**
- `u_dissolve` - Dissolve progress (0.0 = solid, 1.0 = gone)
- Standard sprite uniforms

**Effect:** Pixels fade out based on noise pattern, creating disintegration effect.

### Death Particles

**System:** `ptcl_unit_dissolve.plist`

**Behavior:**
- Emits from unit center
- Affected by wind
- Duration: 0.1s emission
- Particles drift and fade

### Source Code Reference

**UnitNode.js showDeathState():**
```javascript
this.terminateSupportNodes(CONFIG.FADE_MEDIUM_DURATION);

let sequenceSteps = [];
const animAction = this.getAnimationActionFromAnimResource('death');
if (animAction != null) {
  sequenceSteps.push(animAction);
}

const dissolveDuration = CONFIG.FADE_SLOW_DURATION;  // 1.0s
sequenceSteps = sequenceSteps.concat(
  cc.callFunc(function () {
    this.addDissolveVisualTag();
    // spawn particles...
    const particleSystem = BaseParticleSystem.create({
      plistFile: RSX.ptcl_unit_dissolve.plist,
      affectedByWind: true,
      duration: 0.1,
    });
    this.getScene().getGameLayer().addNode(particleSystem);
  }, this),
  cc.actionTween(dissolveDuration, TweenTypes.DISSOLVE, 0.0, 1.0),
  cc.callFunc(function () {
    this.destroy();
  }, this),
);
```

---

## 3. Movement System

### Timeline

```
┌─────────────────────────────────────────────────────────────────────────┐
│                        MOVEMENT SEQUENCE                                 │
├─────────────────────────────────────────────────────────────────────────┤
│                                                                          │
│  [Move Command]                                                          │
│       │                                                                  │
│       ▼                                                                  │
│  [Start Run Animation]                                                   │
│  [Play Walk Sound (per step)]                                            │
│       │                                                                  │
│       ▼                                                                  │
│  ┌─────────────────────────────────────────────┐                        │
│  │  INTERPOLATE POSITION                       │                        │
│  │                                             │                        │
│  │  Duration Formula:                          │                        │
│  │  base = anim_duration * MOVE_MODIFIER (1.0) │                        │
│  │  correction = base * MOVE_CORRECTION (0.2)  │                        │
│  │  total = base * (tiles + 1) - correction    │                        │
│  │                                             │                        │
│  │  Example (8 frame run @ 12fps = 0.667s):    │                        │
│  │    1 tile: 0.667 * 2 - 0.133 = 1.20s        │                        │
│  │    2 tiles: 0.667 * 3 - 0.133 = 1.87s       │                        │
│  │    3 tiles: 0.667 * 4 - 0.133 = 2.54s       │                        │
│  └─────────────────────────────────────────────┘                        │
│       │                                                                  │
│       ▼                                                                  │
│  [Arrive at Destination]                                                 │
│  [Return to Idle/Breathing]                                              │
│                                                                          │
└─────────────────────────────────────────────────────────────────────────┘
```

### Movement Constants

| Constant | Value | Purpose |
|----------|-------|---------|
| ENTITY_MOVE_DURATION_MODIFIER | 1.0 | Speed multiplier |
| ENTITY_MOVE_MODIFIER_MAX | 1.0 | Max speed cap |
| ENTITY_MOVE_MODIFIER_MIN | 0.75 | Min speed cap |
| ENTITY_MOVE_CORRECTION | 0.2 | Timing adjustment |

### Flying Units

Flying units use a fixed arc duration regardless of distance:
- `ENTITY_MOVE_FLYING_FIXED_TILE_COUNT = 3.0`
- Always animates as if moving 3 tiles

---

## 4. Animation Frame Timing

### Frame Delay Values

From `rsx_timing.tsv`:

| Animation Type | frameDelay | Effective FPS |
|----------------|------------|---------------|
| Standard (attack, run, etc.) | 0.08s | 12.5 FPS |
| Breathing/Idle | 0.12s | 8.3 FPS |
| Fast FX | 0.04s | 25 FPS |

### Duration Calculation

```
animation_duration = frame_count * frameDelay
```

Or if FPS specified:
```
animation_duration = frame_count / fps
```

---

## 5. Config Constants

### All Timing-Related Constants

| Constant | Value | Category | Purpose |
|----------|-------|----------|---------|
| ANIMATE_FAST_DURATION | 0.2 | animate | Quick transitions |
| ANIMATE_MEDIUM_DURATION | 0.35 | animate | Standard transitions |
| ANIMATE_SLOW_DURATION | 1.0 | animate | Slow transitions |
| FADE_FAST_DURATION | 0.2 | fade | = ANIMATE_FAST |
| FADE_MEDIUM_DURATION | 0.35 | fade | = ANIMATE_MEDIUM |
| FADE_SLOW_DURATION | 1.0 | fade | = ANIMATE_SLOW, death dissolve |
| ENTITY_ATTACK_DURATION_MODIFIER | 1.0 | attack | Attack speed multiplier |
| ENTITY_MOVE_DURATION_MODIFIER | 1.0 | movement | Move speed multiplier |
| ENTITY_MOVE_CORRECTION | 0.2 | movement | Timing adjustment |
| MOVE_FAST_DURATION | 0.15 | movement | Quick repositions |
| MOVE_MEDIUM_DURATION | 0.35 | movement | = ANIMATE_MEDIUM |
| MOVE_SLOW_DURATION | 1.0 | movement | = ANIMATE_SLOW |

---

## 6. Sound Event Timing

### Attack Sounds

| Event | Trigger Point | Sound Type |
|-------|--------------|------------|
| attack | Attack animation start | Swing/whoosh |
| attackDamage | attackReleaseDelay point | Impact |

### Other Sound Events

| Event | Trigger Point |
|-------|--------------|
| apply | Unit spawn complete |
| walk | Each movement step |
| receiveDamage | When taking damage |
| death | Death animation start |

### Data File Format

`timing/sound_events.tsv`:
```
unit_folder	event_type	sound_file
neutral_golem_steel	attack	sfx_neutral_bluetipscorpion_attack_swing
neutral_golem_steel	attackDamage	sfx_neutral_bluetipscorpion_attack_impact
neutral_golem_steel	death	sfx_neutral_bluetipscorpion_death
```

---

## 7. Data File Formats

### unit_timing.tsv

```
unit_folder    card_id                        attack_delay  attack_release_delay  source_file
neutral_sai    Cards.Neutral.SaberspineTiger  0.4           0.0                   neutral.coffee
f1_general     Cards.Faction1.General         0.5           0.0                   faction1.coffee
```

### cosmetic_timing.tsv

For general skins that override base timing:

```
cosmetic_id        unit_folder           attack_delay  attack_release_delay  source_file
f1GeneralTier2     f1_general_tier2      0.5           0.0                   cosmeticsFactory.coffee
f2GeneralDogehai   f2_general_dogehai    0.5           0.2                   cosmeticsFactory.coffee
```

### config_constants.tsv

```
constant                           value  category
CONFIG.FADE_SLOW_DURATION          1.0    fade
CONFIG.ENTITY_ATTACK_DURATION_MODIFIER  1.0    attack
```

---

## 8. Implementation Guide

### Attack Implementation

```cpp
void Entity::start_attack(int target_idx) {
    target_entity_idx = target_idx;
    attack_elapsed = 0.0f;
    
    // Get timing from unit data (default 0.0 if not specified)
    attack_pre_delay = unit_data.attack_delay;  // pause before animation
    attack_release_delay = unit_data.attack_release_delay;  // damage point
    
    const Animation* attack_anim = animations.find("attack");
    attack_duration = attack_pre_delay + attack_anim->duration();
    
    state = EntityState::PreAttack;  // or skip if attack_pre_delay == 0
    // Play attack swing sound at animation start (not at release)
}

void Entity::update_attack(float dt) {
    attack_elapsed += dt;
    
    // Pre-attack delay phase
    if (attack_elapsed < attack_pre_delay) {
        return;  // still in anticipation pause
    }
    
    float anim_elapsed = attack_elapsed - attack_pre_delay;
    
    // Damage trigger point
    if (!damage_applied && anim_elapsed >= attack_release_delay) {
        apply_damage_to_target();
        play_impact_sound();
        spawn_impact_fx();
        damage_applied = true;
    }
    
    // Animation complete
    if (attack_elapsed >= attack_duration) {
        state = EntityState::Idle;
        play_animation("idle");
    }
}
```

### Death Implementation

```cpp
void Entity::start_death() {
    state = EntityState::Dying;
    death_elapsed = 0.0f;
    
    const Animation* death_anim = animations.find("death");
    death_anim_duration = death_anim ? death_anim->duration() : 0.5f;
    dissolve_duration = 1.0f;  // CONFIG.FADE_SLOW_DURATION
    
    play_animation("death");
    play_death_sound();
}

void Entity::update_death(float dt) {
    death_elapsed += dt;
    
    if (death_elapsed < death_anim_duration) {
        // Still in death animation
        return;
    }
    
    float dissolve_elapsed = death_elapsed - death_anim_duration;
    
    if (dissolve_elapsed < dissolve_duration) {
        // Dissolve phase - reduce opacity or apply shader
        float dissolve_progress = dissolve_elapsed / dissolve_duration;
        opacity = 1.0f - dissolve_progress;
        // Or: set dissolve shader uniform
    } else {
        // Fully dissolved, mark for removal
        death_complete = true;
    }
}
```

### Default Values

When unit has no timing data:
```cpp
const float DEFAULT_ATTACK_DELAY = 0.0f;
const float DEFAULT_ATTACK_RELEASE_DELAY = 0.0f;
const float DISSOLVE_DURATION = 1.0f;
```

---

## Appendix: Verification Checklist

Before implementation, verify:

- [ ] `unit_timing.tsv` exists and has entries
- [ ] `config_constants.tsv` has all required constants
- [ ] Death animation exists for all units
- [ ] Sound files exist for all events
- [ ] Dissolve shader implemented (or opacity fade fallback)
- [ ] Attack timing triggers damage at correct point (NOT 50%)

---

## Source Files Reference

| File | Contains |
|------|----------|
| `app/view/nodes/cards/UnitNode.js` | showAttackState, showDeathState |
| `app/view/layers/game/GameLayer.js` | Attack delay calculation |
| `app/common/config.js` | All CONFIG constants |
| `app/sdk/cards/factory/**/*.coffee` | Unit timing definitions |
| `app/sdk/cosmetics/cosmeticsFactory.coffee` | General skin timing |
