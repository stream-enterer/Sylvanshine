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
│  ┌─────────────────────────────────────────────────────────────────┐    │
│  │  ATTACK ANIMATION STARTS IMMEDIATELY                            │    │
│  │  - Attacker begins attack animation                             │    │
│  │  - Attack swing sound plays at attackReleaseDelay (usually 0.0) │    │
│  └─────────────────────────────────────────────────────────────────┘    │
│       │                                                                  │
│       ▼                                                                  │
│  ┌─────────────────────────────────────────────────────────────────┐    │
│  │  DAMAGE POINT (attackDelay from animation start)                │    │
│  │  Default: 0.5s                                                  │    │
│  │  Range: 0.0 - 1.7s                                              │    │
│  │  ─────────────────────────────────────────────                  │    │
│  │  At this point:                                                 │    │
│  │    ◄── Target takes damage                                      │    │
│  │    ◄── Target plays hit reaction                                │    │
│  │    ◄── Impact FX spawns on target                               │    │
│  │    ◄── Impact sound plays                                       │    │
│  └─────────────────────────────────────────────────────────────────┘    │
│       │                                                                  │
│       ▼                                                                  │
│  [Attack Animation Continues to End]                                     │
│       │                                                                  │
│       ▼                                                                  │
│  [Return to Idle/Breathing State]                                        │
│                                                                          │
└─────────────────────────────────────────────────────────────────────────┘
```

### Critical Insight: Animation Starts Immediately

**The attack animation starts the instant the attack action is triggered.** There is NO pre-attack windup pause.

The `attackDelay` value controls how long INTO the animation before the target takes damage and reacts. This creates the visual effect of the weapon swing connecting with the target at the right moment.

### Source Code Evidence

**GameLayer.js (lines 3379-3418):**
```javascript
// Source animation scheduled FIRST at current actionDelay
sequenceSteps.push(cc.delayTime(actionDelay));
sequenceSteps.push(cc.callFunc(() => {
    _showActionForSource(action, ...);  // Attack animation STARTS here
}));

// THEN attackDelay is added to actionDelay
const attackDelay = (animResource.attackDelay || 0.0) * CONFIG.ENTITY_ATTACK_DURATION_MODIFIER;
actionDelay += attackDelay;  // Added AFTER source animation scheduled

// Target reaction uses the UPDATED actionDelay
const targetReactionDelay = actionDelay + impactDelay;
sequenceSteps.push(cc.delayTime(targetReactionDelay));
sequenceSteps.push(cc.callFunc(() => {
    _showActionForTarget(action, ...);  // Target takes damage HERE
}));
```

**Key insight:** The source animation is scheduled BEFORE attackDelay is added. The target reaction is scheduled AFTER. This means attackDelay is the gap between animation start and damage.

### Timing Values

**attackDelay (time from animation start to damage):**

| Value | Typical Units | Visual Feel |
|-------|--------------|-------------|
| 0.0s | Fast minions | Instant hit |
| 0.3-0.5s | Standard units (f1_general) | Normal swing |
| 0.6-0.9s | Heavy hitters | Weighty impact |
| 1.0-1.5s | Bosses, big units | Dramatic windup |
| 1.7s | Chaos Knight boss | Maximum drama |

**attackReleaseDelay (time from animation start to swing sound):**

| Value | Usage |
|-------|-------|
| 0.0s | 95%+ of all units (sound at anim start) |
| 0.2s | ~4 Songhai generals (delayed sound for multi-hit feel) |

### Example: f1_general Attack

```
Attack animation: 23 frames @ 12 FPS = 1.917s duration
attackDelay: 0.5s (from timing data)

Timeline:
T=0.0s   Attack animation STARTS, swing sound plays
T=0.5s   Target takes damage, impact FX, hit reaction
T=1.917s Attack animation ENDS, return to idle
```

### Data File Format

`timing/unit_timing.tsv`:
```
unit_folder	card_id	attack_delay	attack_release_delay	source_file
f1_general	Cards.Faction1.General	0.5	0.0	faction1.coffee
neutral_golem_steel	Cards.Neutral.GolemMetallurgist	0.5	0.0	neutral.coffee
boss_chaosknight	Cards.Boss.Boss4	1.7	0.0	bosses.coffee
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
│  │    1 tile: 0.667 × 2 - 0.133 = 1.20s        │                        │
│  │    2 tiles: 0.667 × 3 - 0.133 = 1.87s       │                        │
│  │    3 tiles: 0.667 × 4 - 0.133 = 2.54s       │                        │
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
| attack | Animation start + attackReleaseDelay | Swing/whoosh |
| attackDamage | Animation start + attackDelay | Impact |

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

**Column definitions:**
- `attack_delay`: Time from animation START to when target takes damage
- `attack_release_delay`: Time from animation START to when swing sound plays

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
    attack_damage_dealt = false;
    
    // attack_damage_delay loaded from unit_timing.tsv (default 0.5)
    // This is when damage occurs, measured from animation START
    
    const Animation* attack_anim = animations.find("attack");
    attack_duration = attack_anim->duration();
    
    // Animation starts IMMEDIATELY - no pre-attack pause
    state = EntityState::Attacking;
    play_animation("attack");
}

void Entity::update_attack(float dt) {
    attack_elapsed += dt;
    
    // Damage trigger point (attack_damage_delay into animation)
    if (!attack_damage_dealt && attack_elapsed >= attack_damage_delay) {
        // Damage is dealt via callback/event system
        attack_damage_dealt = true;
    }
    
    // Animation complete
    if (attack_elapsed >= attack_duration) {
        state = EntityState::Idle;
        play_animation("idle");
    }
}

bool Entity::should_deal_damage() const {
    return state == EntityState::Attacking 
        && !attack_damage_dealt 
        && attack_elapsed >= attack_damage_delay;
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
const float DEFAULT_ATTACK_DAMAGE_DELAY = 0.5f;  // Reasonable default
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
- [ ] Attack animation starts IMMEDIATELY (no pre-attack windup)
- [ ] Damage is dealt at `attack_delay` seconds INTO the animation

---

## Appendix: Common Mistakes

### WRONG: Pre-attack windup state
```cpp
// DON'T DO THIS
state = EntityState::AttackWindup;  // NO SUCH STATE EXISTS
wait(attack_delay);
play_animation("attack");
deal_damage();  // Damage at animation START is wrong
```

### CORRECT: Animation-first, delayed damage
```cpp
// DO THIS
play_animation("attack");  // Starts IMMEDIATELY
state = EntityState::Attacking;
// ... later, when elapsed >= attack_damage_delay ...
deal_damage();  // Damage during animation
```

---

## Source Files Reference

| File | Contains |
|------|----------|
| `app/view/nodes/cards/UnitNode.js` | showAttackState, showDeathState |
| `app/view/layers/game/GameLayer.js` | Attack sequencing (lines 3379-3418) |
| `app/common/config.js` | All CONFIG constants |
| `app/sdk/cards/factory/**/*.coffee` | Unit timing definitions |
| `app/sdk/cosmetics/cosmeticsFactory.coffee` | General skin timing |
