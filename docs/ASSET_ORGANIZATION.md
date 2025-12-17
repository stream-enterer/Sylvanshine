# Duelyst Asset Organization Reference

## Directory Structure

```
data/
├── units/                          # 693 unit sprite folders
│   ├── manifest.tsv                # Master unit registry
│   ├── unit_shadow.png             # Shared shadow sprite (96x48)
│   └── {unit_name}/
│       ├── spritesheet.png         # Packed sprite atlas
│       ├── animations.txt          # Frame definitions
│       └── sounds.txt              # Sound event mappings (per-unit)
│
├── fx/                             # 205 visual effect folders
│   ├── manifest.tsv                # FX folder → spritesheet, animations
│   ├── rsx_mapping.tsv             # RSX name → folder → prefix
│   └── {fx_name}/
│       ├── spritesheet.png
│       └── animations.txt
│
├── fx_compositions/                # FX grouping definitions
│   └── compositions.json           # What FX play together per event
│
├── timing/                         # Extracted timing data (NEW)
│   ├── unit_timing.tsv             # Per-unit attackDelay, attackReleaseDelay
│   ├── cosmetic_timing.tsv         # General skin timing overrides
│   ├── config_constants.tsv        # All CONFIG.* timing values
│   ├── sound_events.tsv            # Sound trigger mappings
│   └── VERIFICATION.txt            # Extraction verification report
│
├── sfx/                            # 522 unit sound effects (WAV)
├── sfx_ui/                         # 12 UI sounds
├── sfx_announcer/                  # 14 voice lines
├── sfx_unused/                     # 149 unused sounds
│
├── icons/                          # 373 spell/artifact icons
│   ├── manifest.tsv
│   └── {icon_name}/
│
├── tiles/                          # 41 board tile sprites
│   ├── manifest.tsv
│   └── *.png
│
├── maps/                           # 40 background images
│   ├── manifest.tsv
│   └── *.png
│
├── particles/                      # 64 particle systems
│   ├── manifest.tsv
│   ├── textures/
│   └── {particle_name}.txt
│
├── modifiers/                      # 15 status effect icons
│   └── *.png
│
└── shaders/                        # GLSL shaders
    ├── registry.tsv
    ├── uniforms.tsv
    ├── helpers/
    └── *.glsl
```

---

## Manifest Formats

### units/manifest.tsv
```
unit	spritesheet	animations	sounds
f1_general	units/f1_general/spritesheet.png	attack,breathing,cast,...	apply,attack,attackDamage,death,receiveDamage,walk
```

| Column | Description |
|--------|-------------|
| unit | Folder name (e.g., `f1_general`, `boss_andromeda`) |
| spritesheet | Relative path to PNG |
| animations | Comma-separated animation names available |
| sounds | Comma-separated sound event types available |

### fx/manifest.tsv
```
folder	spritesheet	animations
fx_bladestorm	fx/fx_bladestorm/spritesheet.png	fxBladestorm
fx_bloodground	fx/fx_bloodground/spritesheet.png	fxBloodGround,fxBloodGround2,...
```

### fx/rsx_mapping.tsv
Maps RSX identifiers (used in source code) to actual folders:
```
rsx_name	folder	prefix
fxSmokeGround	fx_smoke2	fx_smokeground_
fxClawSlash	fx_clawslash	fx_clawslash_
```

### timing/unit_timing.tsv (EXTRACTED)
```
unit_folder	card_id	attack_delay	attack_release_delay	source_file
boss_chaosknight	Cards.Boss.Boss4	1.7	0.0	bosses.coffee
f1_general	Cards.Faction1.General	0.0	0.0	faction1.coffee
```

| Column | Description |
|--------|-------------|
| unit_folder | Matches units/manifest.tsv unit column |
| card_id | Original Duelyst card identifier |
| attack_delay | Seconds to pause BEFORE attack animation |
| attack_release_delay | Point WITHIN animation where damage triggers (usually 0.0) |

### timing/config_constants.tsv (EXTRACTED)
```
constant	value	category
CONFIG.ANIMATE_SLOW_DURATION	1.0	animate
CONFIG.FADE_MEDIUM_DURATION	0.35	fade
CONFIG.ACTION_DELAY	0.5	delay
```

---

## Animation Format

### animations.txt
Tab-separated: `{name}\t{fps}\t{frame_data}`

Frame data is comma-separated quintuplets: `idx,x,y,w,h,idx,x,y,w,h,...`

```
attack	12	0,404,0,100,100,1,808,0,100,100,2,707,909,100,100,...
idle	15	0,101,505,100,100,1,101,404,100,100,...
run	12	0,0,707,100,100,1,0,606,100,100,...
```

Standard animations: `idle`, `breathing`, `run`, `walk`, `attack`, `hit`, `death`
General-only: `cast`, `caststart`, `castloop`, `castend`

---

## Sound System

### Per-Unit sounds.txt
```
apply	sfx/sfx_summonlegendary.wav
attack	sfx/sfx_neutral_sai_attack_swing.wav
attackDamage	sfx/sfx_neutral_sai_attack_impact.wav
death	sfx/sfx_neutral_yun_death.wav
receiveDamage	sfx/sfx_neutral_gro_hit.wav
walk	sfx/sfx_neutral_sai_attack_impact.wav
```

| Event | When Triggered |
|-------|----------------|
| apply | Unit spawned on board |
| attack | Attack animation starts |
| attackDamage | Attack connects (at attackReleaseDelay) |
| death | Unit dies |
| receiveDamage | Unit takes damage |
| walk | Each movement step |

### Sound File Naming Convention
Pattern: `sfx_{faction}_{unit}_{event}.wav`

Examples:
- `sfx_f1_general_attack_swing.wav`
- `sfx_neutral_golemsteel_death.wav`
- `sfx_summonlegendary.wav` (shared)

---

## Timing System

### Attack Timeline
```
1. Action queued
2. Wait attackDelay (0.0-1.7s) ← anticipation pause
3. Start attack animation
4. At attackReleaseDelay (usually 0.0):
   - Trigger damage
   - Play attackDamage sound
5. Animation completes
6. Return to idle
```

### Death Timeline
```
1. HP reaches 0
2. Play death animation
3. Start dissolve shader (0.0 → 1.0 over 1.0s)
4. Spawn death particles
5. Destroy unit node
```

### Key Constants (from config_constants.tsv)
| Constant | Value | Use |
|----------|-------|-----|
| ANIMATE_FAST_DURATION | 0.2 | Quick transitions |
| ANIMATE_MEDIUM_DURATION | 0.35 | Standard transitions |
| ANIMATE_SLOW_DURATION | 1.0 | Death dissolve |
| ACTION_DELAY | 0.5 | Between actions |

---

## Sprite Positioning

### shadowOffset (Universal Constant)
All units use `shadowOffset = 19.5` (CONFIG.DEPTH_OFFSET). This is **not** per-unit data.

Source investigation confirmed:
- `UnitSprite.js` sets default `shadowOffset: CONFIG.DEPTH_OFFSET`
- `UnitNode.js` falls back to this default; no per-unit overrides exist in codebase
- No RSX resource files define per-unit shadow offsets

See `GRID_PERSPECTIVE_SPEC.md` for positioning formula and usage.

---

## Cross-Reference Keys

| To find... | Use this key... | In this file... |
|------------|-----------------|-----------------|
| Unit sprite | `unit_folder` | units/manifest.tsv |
| Unit timing | `unit_folder` | timing/unit_timing.tsv |
| Unit sounds | `unit_folder` | timing/sound_events.tsv |
| FX sprite | `rsx_name` | fx/rsx_mapping.tsv → fx/manifest.tsv |
| FX composition | faction + event type | fx_compositions/compositions.json |

---

## Coverage Statistics

| Asset Type | Count | Coverage |
|------------|-------|----------|
| Units in manifest | 693 | 100% |
| Units with timing | 679 | 98% |
| FX folders | 205 | 100% |
| Sound files (sfx/) | 522 | ~60% of references |
| Config constants | 138 | 100% |

---

## Loading Order for Implementation

1. Load `units/manifest.tsv` → get all unit folders
2. Load `timing/unit_timing.tsv` → map unit_folder → timing values
3. Load `timing/config_constants.tsv` → get global timing values
4. For each unit needed:
   - Load `units/{folder}/spritesheet.png`
   - Parse `units/{folder}/animations.txt`
   - Parse `units/{folder}/sounds.txt` (if exists)
5. For FX:
   - Load `fx/rsx_mapping.tsv`
   - Load `fx/manifest.tsv`
   - Load needed FX spritesheets on demand
