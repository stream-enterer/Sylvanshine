# Duelyst Asset Format Specification - Timing Addendum

This addendum extends `DUELYST_ASSET_FORMAT_SPEC.md` with timing-related data formats.

---

## 9. Timing Data

### Directory Structure

```
data/
├── timing/
│   ├── unit_timing.tsv         # Per-unit attack timing
│   ├── cosmetic_timing.tsv     # General skin overrides
│   ├── config_constants.tsv    # Global timing constants
│   ├── sound_events.tsv        # Sound trigger mappings
│   ├── VERIFICATION.txt        # Extraction verification report
│   └── ASSET_VERIFICATION.txt  # Shader/particle verification
```

### timing/unit_timing.tsv

Per-unit attack timing values extracted from card factories.

```
unit_folder	card_id	attack_delay	attack_release_delay	source_file
neutral_sai	Cards.Neutral.SaberspineTiger	0.4	0.0	neutral.coffee
neutral_golem_steel	Cards.Neutral.GolemMetallurgist	0.5	0.0	neutral.coffee
f1_silverguard_knight	Cards.Faction1.SilverguardKnight	0.3	0.0	faction1.coffee
```

| Column | Type | Description |
|--------|------|-------------|
| unit_folder | string | Unit identifier (matches units/manifest.tsv) |
| card_id | string | Card identifier from source |
| attack_delay | float | Pre-animation pause in seconds |
| attack_release_delay | float | Damage point within animation |
| source_file | string | Source factory file |

**Default Values:**
- If unit not in file: `attack_delay = 0.0`, `attack_release_delay = 0.0`

### timing/cosmetic_timing.tsv

General skin timing overrides from cosmeticsFactory.coffee.

```
cosmetic_id	unit_folder	attack_delay	attack_release_delay	source_file
f1GeneralTier2	f1_general_tier2	0.5	0.0	cosmeticsFactory.coffee
f2GeneralDogehai	f2_general_dogehai	0.5	0.2	cosmeticsFactory.coffee
```

| Column | Type | Description |
|--------|------|-------------|
| cosmetic_id | string | Cosmetic/skin identifier |
| unit_folder | string | Associated unit folder |
| attack_delay | float | Pre-animation pause |
| attack_release_delay | float | Damage point |
| source_file | string | Always cosmeticsFactory.coffee |

### timing/config_constants.tsv

Global timing constants from config.js.

```
constant	value	category
CONFIG.FADE_SLOW_DURATION	1.0	fade
CONFIG.FADE_MEDIUM_DURATION	0.35	fade
CONFIG.FADE_FAST_DURATION	0.2	fade
CONFIG.ANIMATE_SLOW_DURATION	1.0	animate
CONFIG.ANIMATE_MEDIUM_DURATION	0.35	animate
CONFIG.ANIMATE_FAST_DURATION	0.2	animate
CONFIG.ENTITY_ATTACK_DURATION_MODIFIER	1.0	attack
CONFIG.ENTITY_MOVE_DURATION_MODIFIER	1.0	movement
CONFIG.ENTITY_MOVE_CORRECTION	0.2	movement
```

| Column | Type | Description |
|--------|------|-------------|
| constant | string | Full constant name with CONFIG prefix |
| value | string | Value (may reference other constants) |
| category | string | fade, animate, attack, movement, duration, delay |

### timing/sound_events.tsv

Sound event mappings per unit.

```
unit_folder	event_type	sound_file
neutral_sai	apply	sfx_unit_deploy_1
neutral_sai	attack	sfx_neutral_sai_attack_swing
neutral_sai	attackDamage	sfx_neutral_sai_attack_impact
neutral_sai	receiveDamage	sfx_neutral_sai_hit
neutral_sai	death	sfx_neutral_sai_death
```

| Column | Type | Description |
|--------|------|-------------|
| unit_folder | string | Unit identifier |
| event_type | string | apply, walk, attack, attackDamage, receiveDamage, death |
| sound_file | string | Sound resource name (without extension) |

**Event Types:**
| Event | When Triggered |
|-------|---------------|
| apply | Unit spawns on board |
| walk | Each movement step |
| attack | Attack animation starts |
| attackDamage | Damage point (at attackReleaseDelay) |
| receiveDamage | Unit takes damage |
| death | Death animation starts |

---

## 10. Attack Timing System

### Flow

```
1. Action queued
2. Wait attackDelay seconds (pre-animation pause)
3. Start attack animation
4. Play attack sound
5. At attackReleaseDelay into animation:
   - Apply damage
   - Play attackDamage sound
   - Spawn impact FX
6. Animation completes
7. Return to idle
```

### Key Values

| Property | Default | Range | Notes |
|----------|---------|-------|-------|
| attackDelay | 0.0 | 0.0 - 1.5 | Anticipation before animation |
| attackReleaseDelay | 0.0 | 0.0 - 0.2 | Damage point in animation |

**Critical:** 95%+ of units have `attackReleaseDelay: 0.0` - damage at animation START.

### C Implementation

```c
typedef struct {
    float attack_delay;         // Pre-animation pause (default 0.0)
    float attack_release_delay; // Damage point (default 0.0)
} UnitTiming;

UnitTiming load_unit_timing(const char* unit_folder) {
    // Look up in unit_timing.tsv, return defaults if not found
    UnitTiming timing = {0.0f, 0.0f};
    // ... parse TSV ...
    return timing;
}

void update_attack(Entity* e, float dt) {
    e->attack_elapsed += dt;
    
    // Pre-attack delay
    if (e->attack_elapsed < e->timing.attack_delay) {
        return;
    }
    
    float anim_elapsed = e->attack_elapsed - e->timing.attack_delay;
    
    // Damage point
    if (!e->damage_applied && anim_elapsed >= e->timing.attack_release_delay) {
        apply_damage(e->target);
        play_sound(e, "attackDamage");
        e->damage_applied = true;
    }
    
    // Check animation complete
    // ...
}
```

---

## 11. Death Timing System

### Flow

```
1. HP reaches 0
2. Play death animation (duration from animation data)
3. Start dissolve effect:
   - Apply dissolve shader OR fade opacity
   - Emit death particles (ptcl_unit_dissolve)
4. Tween dissolve 0.0 → 1.0 over 1.0s (CONFIG.FADE_SLOW_DURATION)
5. Remove unit from scene
```

### Constants

| Constant | Value | Purpose |
|----------|-------|---------|
| DISSOLVE_DURATION | 1.0s | Death fade/dissolve time |
| Death animation | varies | From animation data |
| Total death time | ~1.5-2.0s | Animation + dissolve |

### Death Particles

**File:** `particles/ptcl_unit_dissolve.plist`

Spawned when dissolve begins, emits briefly then fades.

### C Implementation

```c
#define DISSOLVE_DURATION 1.0f

void update_death(Entity* e, float dt, RenderConfig* cfg) {
    e->death_elapsed += dt;
    
    float death_anim_duration = e->death_anim->duration;
    
    if (e->death_elapsed < death_anim_duration) {
        // Still in death animation
        update_animation(e, dt);
        return;
    }
    
    // Dissolve phase
    float dissolve_elapsed = e->death_elapsed - death_anim_duration;
    
    if (dissolve_elapsed < DISSOLVE_DURATION) {
        float progress = dissolve_elapsed / DISSOLVE_DURATION;
        e->opacity = 1.0f - progress;
        // OR: set dissolve shader uniform
    } else {
        e->death_complete = true;
    }
}
```

---

## 12. Movement Timing System

### Formula

```
base_duration = walk_animation_duration * ENTITY_MOVE_DURATION_MODIFIER
correction = base_duration * ENTITY_MOVE_CORRECTION
total_duration = base_duration * (tile_count + 1) - correction
```

### Constants

| Constant | Value |
|----------|-------|
| ENTITY_MOVE_DURATION_MODIFIER | 1.0 |
| ENTITY_MOVE_CORRECTION | 0.2 |
| ENTITY_MOVE_MODIFIER_MAX | 1.0 |
| ENTITY_MOVE_MODIFIER_MIN | 0.75 |

### Example

Walk animation: 8 frames @ 12 FPS = 0.667s

| Tiles | Duration |
|-------|----------|
| 1 | 0.667 × 2 - 0.133 = 1.20s |
| 2 | 0.667 × 3 - 0.133 = 1.87s |
| 3 | 0.667 × 4 - 0.133 = 2.54s |

---

## 13. Extraction Scripts

### extract_timing.py

Extracts all timing data from Duelyst source:

```bash
python extract_timing.py <duelyst_source> <our_data_dir> -o timing/
```

**Outputs:**
- `unit_timing.tsv`
- `cosmetic_timing.tsv`
- `config_constants.tsv`
- `sound_events.tsv`
- `VERIFICATION.txt`

### verify_timing_assets.py

Verifies shader, particle, and sound dependencies:

```bash
python verify_timing_assets.py <duelyst_source> <our_data_dir>
```

**Outputs:**
- `ASSET_VERIFICATION.txt`

---

## 14. Cross-Reference with Existing Data

### Relationship to units/manifest.tsv

`unit_timing.tsv` uses the same `unit_folder` identifiers as `units/manifest.tsv`.

Units in manifest but not in timing → use defaults (0.0, 0.0)

### Relationship to fx_compositions

Attack FX (UnitAttackedFX, UnitDamagedFX) are triggered at:
- `UnitAttackedFX`: Attack animation start
- `UnitDamagedFX`: attackReleaseDelay point (when damage applies)

Death FX (UnitDiedFX) triggered at HP=0, before death animation.

---

## Summary: Loading Sequence

```c
void load_game_data(const char* data_dir) {
    // Existing
    load_unit_manifest(data_dir);
    load_fx_manifest(data_dir);
    load_animations(data_dir);
    
    // New timing data
    load_timing_constants(data_dir);  // config_constants.tsv
    load_unit_timing(data_dir);       // unit_timing.tsv
    load_sound_events(data_dir);      // sound_events.tsv
}

UnitTiming get_unit_timing(const char* unit_folder) {
    // Look up in loaded timing data
    // Return defaults if not found
    return (UnitTiming){0.0f, 0.0f};
}
```
