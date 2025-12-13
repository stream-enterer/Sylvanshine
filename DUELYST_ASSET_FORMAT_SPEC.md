# Duelyst Asset Format Specification

This document describes the plaintext asset formats extracted from Duelyst for use in C/raylib.

---

## Table of Contents

1. [Directory Structure](#1-directory-structure)
2. [Manifest Schemas](#2-manifest-schemas)
3. [Animation Format](#3-animation-format)
4. [FX System](#4-fx-system)
5. [Particle System](#5-particle-system)
6. [Audio](#6-audio)
7. [Shaders](#7-shaders)
8. [Loading Sequences](#8-loading-sequences)

---

## 1. Directory Structure

```
data/
├── units/                    # 694 unit folders
│   ├── manifest.tsv          # Unit registry
│   ├── unit_shadow.png       # Shared shadow sprite (96x48)
│   └── {unit_name}/
│       ├── spritesheet.png   # Packed sprite atlas
│       ├── animations.txt    # Frame definitions
│       └── sounds.txt        # Sound event mappings
│
├── fx/                       # 205 FX atlases
│   ├── manifest.tsv          # FX registry (by PNG name)
│   ├── rsx_mapping.tsv       # RSX name → folder → prefix
│   └── {fx_name}/
│       ├── spritesheet.png   # Packed sprite atlas
│       └── animations.txt    # Frame definitions (keyed by RSX name)
│
├── fx_compositions/          # FX groupings
│   └── compositions.json     # What plays together
│
├── icons/                    # 373 spell/artifact icons
│   ├── manifest.tsv
│   └── {icon_name}/
│       ├── spritesheet.png
│       └── animations.txt
│
├── particles/                # 64 particle systems
│   ├── manifest.tsv
│   ├── textures/             # Particle textures
│   └── {particle_name}.txt   # Emitter parameters
│
├── tiles/                    # 41 board tiles
│   ├── manifest.tsv
│   └── *.png
│
├── maps/                     # 40 backgrounds
│   ├── manifest.tsv
│   └── *.png
│
├── modifiers/                # 15 status icons
│   └── *.png
│
├── shaders/                  # GLSL shaders
│   ├── helpers/              # Include files
│   ├── registry.tsv
│   ├── uniforms.tsv
│   └── *.glsl
│
├── sfx/                      # 522 unit sounds (wav)
├── sfx_ui/                   # 12 UI sounds
├── sfx_announcer/            # 14 voice lines
└── sfx_unused/               # 149 unused sounds
```

---

## 2. Manifest Schemas

### units/manifest.tsv

```
unit	spritesheet	animations	sounds
boss_andromeda	units/boss_andromeda/spritesheet.png	attack,breathing,death,hit,idle,run	apply,attack,attackDamage,death,receiveDamage,walk
```

| Column | Type | Description |
|--------|------|-------------|
| unit | string | Folder name / unit identifier |
| spritesheet | path | Relative path to PNG |
| animations | comma-list | Available animation names |
| sounds | comma-list | Available sound events |

### fx/manifest.tsv

```
folder	spritesheet	animations
fx_bladestorm	fx/fx_bladestorm/spritesheet.png	fxBladestorm
fx_bloodground	fx/fx_bloodground/spritesheet.png	fxBloodGround,fxBloodGround2,fxBloodGround3,fxBloodGround4
```

| Column | Type | Description |
|--------|------|-------------|
| folder | string | Folder name (matches PNG filename) |
| spritesheet | path | Relative path to PNG |
| animations | comma-list | RSX names of animations in this atlas |

### fx/rsx_mapping.tsv

```
rsx_name	folder	prefix
fxSmokeGround	fx_smoke2	fx_smokeground_
fxBloodGround	fx_bloodground	fx_bloodground_
fxBloodGround2	fx_bloodground	fx_bloodground2_
```

| Column | Type | Description |
|--------|------|-------------|
| rsx_name | string | RSX identifier used in compositions |
| folder | string | Folder containing the atlas |
| prefix | string | Frame prefix in animations.txt |

### particles/manifest.tsv

```
name	emitter_type	max_particles	texture
ash	gravity	30	47E234B3-14AB-4F70-BF44-C528803C366B
bird_001	gravity	20.0	bird_001.png
```

| Column | Type | Description |
|--------|------|-------------|
| name | string | Particle system identifier |
| emitter_type | enum | `gravity` or `radial` |
| max_particles | int | Maximum concurrent particles |
| texture | string | Texture filename (may be UUID or .png) |

---

## 3. Animation Format

### animations.txt Structure

Tab-separated: `{name}\t{fps}\t{frame_data}`

```
attack	12	0,404,0,100,100,1,808,0,100,100,2,707,909,100,100,...
breathing	8	0,505,808,100,100,1,505,707,100,100,...
```

### Frame Data Format

Comma-separated quintuplets: `idx,x,y,w,h,idx,x,y,w,h,...`

| Field | Type | Description |
|-------|------|-------------|
| idx | int | Frame index (for ordering) |
| x | int | X position in spritesheet (pixels) |
| y | int | Y position in spritesheet (pixels) |
| w | int | Frame width (pixels) |
| h | int | Frame height (pixels) |

### Parsing Algorithm (C pseudocode)

```c
typedef struct {
    int idx, x, y, w, h;
} Frame;

typedef struct {
    char name[64];
    int fps;
    Frame* frames;
    int frame_count;
} Animation;

Animation parse_animation_line(const char* line) {
    Animation anim = {0};
    
    // Split by tabs
    char* name_end = strchr(line, '\t');
    strncpy(anim.name, line, name_end - line);
    
    char* fps_start = name_end + 1;
    anim.fps = atoi(fps_start);
    
    char* data_start = strchr(fps_start, '\t') + 1;
    
    // Count frames (number of commas / 5 + 1) / 5
    int comma_count = 0;
    for (char* p = data_start; *p; p++) if (*p == ',') comma_count++;
    anim.frame_count = (comma_count + 1) / 5;
    
    anim.frames = malloc(sizeof(Frame) * anim.frame_count);
    
    // Parse quintuplets
    char* p = data_start;
    for (int i = 0; i < anim.frame_count; i++) {
        anim.frames[i].idx = strtol(p, &p, 10); p++; // skip comma
        anim.frames[i].x   = strtol(p, &p, 10); p++;
        anim.frames[i].y   = strtol(p, &p, 10); p++;
        anim.frames[i].w   = strtol(p, &p, 10); p++;
        anim.frames[i].h   = strtol(p, &p, 10); p++;
    }
    
    return anim;
}
```

### Standard Unit Animations

| Name | Typical FPS | Usage |
|------|-------------|-------|
| idle | 8-12 | Standing still (loop) |
| breathing | 8 | Idle with subtle movement (loop) |
| attack | 12 | Attack animation (play once) |
| hit | 12 | Taking damage (play once) |
| death | 12 | Death animation (play once) |
| run | 12 | Movement animation (loop) |
| walk | 12 | Slower movement (loop) |
| cast | 12 | Spell casting (generals only) |
| caststart | 12 | Cast begin |
| castloop | 12 | Cast hold |
| castend | 12 | Cast finish |
| apply | 12 | Spawn/summon animation |

---

## 4. FX System

### FX Composition Resolution

`compositions.json` defines which FX sprites play together:

```json
{
  "Faction1.UnitSpawnFX": {
    "fx_type": "UnitSpawnFX",
    "parent": "Faction1",
    "sprites": [
      "fxTeleportRecallWhite",
      "fx_f1_holyimmolation",
      "fxSmokeGround"
    ],
    "particles": [],
    "sounds": []
  }
}
```

### Resolution Algorithm

```c
// 1. Look up composition by key (e.g., "Faction1.UnitSpawnFX")
// 2. For each sprite RSX name in composition:
//    a. Look up in rsx_mapping.tsv to get folder + prefix
//    b. Load spritesheet from fx/{folder}/spritesheet.png
//    c. Find animation matching RSX name in animations.txt
// 3. Play all sprites simultaneously at spawn position

typedef struct {
    const char* rsx_name;
    const char* folder;
    const char* prefix;
} RSXMapping;

RSXMapping resolve_rsx(const char* rsx_name, RSXMapping* mappings, int count) {
    for (int i = 0; i < count; i++) {
        if (strcmp(mappings[i].rsx_name, rsx_name) == 0) {
            return mappings[i];
        }
    }
    return (RSXMapping){0};
}
```

### Spawn FX by Faction

| Faction | Key | Sprites |
|---------|-----|---------|
| Generic | Factions.UnitSpawnFX | fxTeleportRecallWhite, fx_f1_holyimmolation, fxSmokeGround |
| Lyonar (F1) | Faction1.UnitSpawnFX | (inherits from Factions) |
| Songhai (F2) | Faction2.UnitSpawnFX | fx_f2_teleportsmoke, fx_f2_teleportsmoke, fxSmokeGround |
| Vetruvian (F3) | Faction3.UnitSpawnFX | fxTeleportRecallWhite, fxBladestorm, fxSmokeGround |
| Abyssian (F4) | Faction4.UnitSpawnFX | fx_f2_teleportsmoke, fx_f2_teleportsmoke, fxSmokeGround |
| Magmar (F5) | Faction5.UnitSpawnFX | (check compositions.json) |
| Vanar (F6) | Faction6.UnitSpawnFX | (check compositions.json) |
| Neutral | Neutral.UnitSpawnFX | (check compositions.json) |

### FX Layering

FX sprites are rendered in array order (first = bottom, last = top). All FX play simultaneously, centered on the target position.

---

## 5. Particle System

### Particle Definition Format

Each `particles/{name}.txt` contains tab-separated key-value pairs:

```
maxParticles	30
duration	-1.0
lifespan	8.0
lifespanVariance	6.0
emitterType	gravity
sourcePositionVariancex	557
sourcePositionVariancey	0
speed	0
speedVariance	0
angle	239
angleVariance	27
gravityx	0
gravityy	10
startColorRed	0.99
startColorGreen	0.77
startColorBlue	0.55
startColorAlpha	1.0
finishColorRed	0.91
finishColorGreen	1.0
finishColorBlue	0.91
finishColorAlpha	0.0
startParticleSize	6
finishParticleSize	10
textureFileName	47E234B3-14AB-4F70-BF44-C528803C366B
```

### Key Parameters

| Parameter | Type | Description |
|-----------|------|-------------|
| maxParticles | int | Maximum concurrent particles |
| duration | float | Emitter lifetime (-1 = infinite) |
| lifespan | float | Particle lifetime in seconds |
| lifespanVariance | float | Random variation in lifetime |
| emitterType | string | `gravity` or `radial` |
| sourcePositionVariancex/y | float | Spawn area size |
| speed | float | Initial particle speed |
| speedVariance | float | Random variation in speed |
| angle | float | Emission angle (degrees) |
| angleVariance | float | Random variation in angle |
| gravityx/y | float | Gravity vector |
| startColor* | float | Initial RGBA (0.0-1.0) |
| finishColor* | float | Final RGBA (0.0-1.0) |
| startParticleSize | float | Initial size in pixels |
| finishParticleSize | float | Final size in pixels |
| textureFileName | string | Texture (UUID or filename) |

### Texture Resolution

Textures are in `particles/textures/`. Some use UUIDs as filenames:
- `47E234B3-14AB-4F70-BF44-C528803C366B` → dot texture
- `C502A51C-0B46-4447-A08B-6E6419464ABB` → another dot variant

Map UUIDs to actual files by checking `particles/textures/` contents.

---

## 6. Audio

### Format

All audio has been converted to **WAV** (PCM 16-bit) format, directly compatible with raylib's `LoadSound()`.

### sounds.txt Format

```
apply	sfx/sfx_summonlegendary.wav
attack	sfx/sfx_neutral_sai_attack_swing.wav
attackDamage	sfx/sfx_neutral_sai_attack_impact.wav
death	sfx/sfx_neutral_yun_death.wav
receiveDamage	sfx/sfx_neutral_gro_hit.wav
walk	sfx/sfx_neutral_sai_attack_impact.wav
```

| Event | When to Play |
|-------|--------------|
| apply | Unit spawned |
| attack | Attack animation starts (at attackReleaseDelay) |
| attackDamage | Attack connects with target |
| death | Unit dies |
| receiveDamage | Unit takes damage |
| walk | Each movement step |

### UI Sound Events

Located in `sfx_ui/`:

| File | Usage |
|------|-------|
| sfx_ui_yourturn.wav | Turn start notification |
| select.wav | Selection click |
| notification.wav | Generic notification |
| deploy_circle1.wav | Unit placement |

---

## 7. Shaders

### Shader Registry

`shaders/registry.tsv` lists available shaders and their purposes.

### Key Shaders for Units

| Shader | Purpose |
|--------|---------|
| ShadowLowQualityFragment.glsl | Unit drop shadows |
| ShadowHighQualityFragment.glsl | High-quality shadows |
| DissolveFragment.glsl | Death dissolve effect |
| ChromaticFragment.glsl | Selection/highlight effects |

### Uniforms

`shaders/uniforms.tsv` documents uniform variables.

Shadow shader uniforms:
- `u_size` - Shadow dimensions
- `u_anchor` - Shadow anchor point
- `u_intensity` - Shadow opacity
- `u_blurShiftModifier` - Blur amount
- `u_blurIntensityModifier` - Blur intensity

### Include System

Shaders use `#pragma glslify: require()` for includes:
```glsl
#pragma glslify: getPackedDepth = require(./helpers/DepthPack.glsl)
#pragma glslify: getNoise2D = require(./helpers/Noise2D.glsl)
```

For C/raylib, preprocess these into concatenated shader strings or resolve at load time.

---

## 8. Loading Sequences

### Unit Loading

```c
Unit load_unit(const char* unit_name) {
    Unit u = {0};
    
    char path[256];
    
    // 1. Load spritesheet
    snprintf(path, sizeof(path), "data/units/%s/spritesheet.png", unit_name);
    u.texture = LoadTexture(path);
    
    // 2. Parse animations
    snprintf(path, sizeof(path), "data/units/%s/animations.txt", unit_name);
    u.animations = parse_animations_file(path, &u.anim_count);
    
    // 3. Parse sounds
    snprintf(path, sizeof(path), "data/units/%s/sounds.txt", unit_name);
    u.sounds = parse_sounds_file(path, &u.sound_count);
    
    return u;
}
```

### FX Loading from Composition

```c
FXComposition load_spawn_fx(const char* faction) {
    FXComposition comp = {0};
    
    // 1. Look up composition key
    char key[64];
    snprintf(key, sizeof(key), "%s.UnitSpawnFX", faction);
    
    // 2. Get sprite list from compositions.json
    JSONValue* sprites = json_get(compositions, key, "sprites");
    
    // 3. For each sprite, resolve and load
    comp.sprite_count = json_array_length(sprites);
    comp.sprites = malloc(sizeof(FXSprite) * comp.sprite_count);
    
    for (int i = 0; i < comp.sprite_count; i++) {
        const char* rsx_name = json_array_get_string(sprites, i);
        RSXMapping mapping = resolve_rsx(rsx_name);
        
        char path[256];
        snprintf(path, sizeof(path), "data/fx/%s/spritesheet.png", mapping.folder);
        comp.sprites[i].texture = LoadTexture(path);
        
        snprintf(path, sizeof(path), "data/fx/%s/animations.txt", mapping.folder);
        comp.sprites[i].anim = find_animation_by_name(path, rsx_name);
    }
    
    return comp;
}
```

### Full Scene Setup

```c
void init_game_scene() {
    // 1. Load shared resources
    Texture2D unit_shadow = LoadTexture("data/units/unit_shadow.png");
    Texture2D board_bg = LoadTexture("data/maps/battlemap0_background.png");
    
    // 2. Load tile sprites
    for (int i = 0; i < tile_count; i++) {
        tiles[i] = LoadTexture(tile_manifest[i].path);
    }
    
    // 3. Preload faction spawn FX
    spawn_fx[FACTION_LYONAR] = load_spawn_fx("Faction1");
    spawn_fx[FACTION_SONGHAI] = load_spawn_fx("Faction2");
    // ... etc
    
    // 4. Load UI sounds
    sfx_yourturn = LoadSound("data/sfx_ui/sfx_ui_yourturn.wav");
    sfx_select = LoadSound("data/sfx_ui/select.wav");
}
```

---

## Appendix: Quick Reference

### File Extensions

| Extension | Format | Notes |
|-----------|--------|-------|
| .png | Image | RGBA, various sizes |
| .txt | Text | Tab-separated data |
| .tsv | Text | Tab-separated manifest |
| .json | JSON | Compositions only |
| .wav | Audio | PCM 16-bit, raylib compatible |
| .glsl | Shader | GLSL ES 2.0 compatible |

### Coordinate Systems

| System | Origin | Units |
|--------|--------|-------|
| Board | Bottom-left | Tile indices (0-8, 0-4) |
| Screen | Top-left | Pixels |
| Spritesheet | Top-left | Pixels |

### Animation Timing

```c
float frame_duration = 1.0f / animation.fps;
int current_frame = (int)(elapsed_time / frame_duration) % animation.frame_count;
```

### Unit Faction Prefixes

| Prefix | Faction |
|--------|---------|
| f1_ | Lyonar |
| f2_ | Songhai |
| f3_ | Vetruvian |
| f4_ | Abyssian |
| f5_ | Magmar |
| f6_ | Vanar |
| neutral_ | Neutral |
| boss_ | Boss |
