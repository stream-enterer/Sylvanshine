# Animation System Documentation

## Overview

This document explains how the Duelyst animation system works in our C++/SDL3 implementation, using the extracted plaintext asset format.

---

## File Organization

```
data/
└── units/
    └── {unit_name}/
        ├── spritesheet.png      # Single packed texture atlas
        └── animations.txt       # Frame definitions (tab-separated)
```

### Example: f1_general
- **Spritesheet**: 909×1010 pixels, RGBA PNG
- **Animations**: 8 named animations (attack, breathing, cast, etc.)
- **Frame size**: All frames are 100×100 pixels for this unit

---

## Animation File Format

### Structure
```
{name}<TAB>{fps}<TAB>{frame_data}
```

Each line defines one animation. Fields are tab-separated.

### Fields

| Field | Type | Description |
|-------|------|-------------|
| name | string | Animation identifier (e.g., "idle", "run", "attack") |
| fps | integer | Frames per second (playback speed) |
| frame_data | comma-separated | Frame rectangles as quintuplets |

### Frame Data Format

Frame data is a comma-separated list of 5-integer groups:
```
idx,x,y,w,h,idx,x,y,w,h,idx,x,y,w,h,...
```

Each quintuplet defines one frame:
- **idx**: Frame index (for ordering, usually sequential 0,1,2...)
- **x**: X coordinate in spritesheet (pixels)
- **y**: Y coordinate in spritesheet (pixels)
- **w**: Frame width (pixels)
- **h**: Frame height (pixels)

### Real Example: f1_general run animation
```
run	12	0,0,707,100,100,1,0,606,100,100,2,0,505,100,100,3,0,404,100,100,4,0,303,100,100,5,0,202,100,100,6,0,101,100,100,7,0,0,100,100
```

Breakdown:
- **Name**: "run"
- **FPS**: 12 frames per second
- **Frames**: 8 frames total
  - Frame 0: (0, 707) → 100×100
  - Frame 1: (0, 606) → 100×100
  - Frame 2: (0, 505) → 100×100
  - ...
  - Frame 7: (0, 0) → 100×100

All frames are vertically stacked in the leftmost column of the spritesheet.

---

## Parsing Algorithm

### Step 1: Split line by tabs
```cpp
std::getline(file, line)
tab1 = line.find('\t')
tab2 = line.find('\t', tab1 + 1)

name = line.substr(0, tab1)
fps_str = line.substr(tab1 + 1, tab2 - tab1 - 1)
frame_data = line.substr(tab2 + 1)
```

### Step 2: Parse frame data
```cpp
// Split by commas
values = split(frame_data, ',')

// Calculate frame count
frame_count = values.size() / 5

// Extract frames
for i in 0..frame_count:
    base = i * 5
    frames[i].idx = values[base]
    frames[i].rect.x = values[base + 1]
    frames[i].rect.y = values[base + 2]
    frames[i].rect.w = values[base + 3]
    frames[i].rect.h = values[base + 4]
```

### Step 3: Store in AnimationSet
```cpp
struct AnimFrame {
    int idx;
    SDL_Rect rect;  // {x, y, w, h}
};

struct Animation {
    char name[32];
    int fps;
    AnimFrame* frames;
    int frame_count;
};
```

---

## Runtime Playback

### Animation Timing

```cpp
// Calculate duration
float duration = frame_count / static_cast<float>(fps);

// Update animation time
anim_time += dt;  // dt = delta time in seconds

// Calculate current frame
int frame_idx = static_cast<int>(anim_time * fps) % frame_count;

// Loop when reaching end
if (anim_time >= duration) {
    anim_time -= duration;  // or reset to 0
}
```

### Example Calculation (f1_general run)
- Frame count: 8
- FPS: 12
- Duration: 8 / 12 = **0.667 seconds**
- Frame duration: 1 / 12 = **0.083 seconds per frame**

Timeline:
```
t=0.000s → frame 0
t=0.083s → frame 1
t=0.167s → frame 2
t=0.250s → frame 3
t=0.333s → frame 4
t=0.417s → frame 5
t=0.500s → frame 6
t=0.583s → frame 7
t=0.667s → loop to frame 0
```

### Rendering a Frame

```cpp
// Get current frame
const AnimFrame& frame = anim.frames[frame_idx];
const SDL_Rect& src = frame.rect;

// Render centered on entity position
SDL_FRect dst = {
    entity_x - src.w * 0.5f,  // center horizontally
    entity_y - src.h * 0.5f,  // center vertically
    src.w,
    src.h
};

SDL_RenderTexture(renderer, spritesheet, &src, &dst);
```

---

## Standard Unit Animations

Based on extracted data, units typically have these animations:

| Name | Typical FPS | Loop? | Usage |
|------|-------------|-------|-------|
| idle | 12-15 | Yes | Standing still |
| breathing | 8 | Yes | Subtle idle movement |
| run | 12 | Yes | Movement (our implementation) |
| walk | 12 | Yes | Slower movement (fallback) |
| attack | 12 | No | Attack animation |
| hit | 12 | No | Taking damage |
| death | 12 | No | Death sequence |
| cast | 12 | No | Spell casting (generals) |
| castloop | 12 | Yes | Channeling spell |
| castend | 12 | No | Finish casting |
| apply | 12 | No | Spawn/summon |

### Animation State Machine

```
[Idle/Breathing]
    ↓ click destination
[Run] → move to tile
    ↓ arrive
[Idle/Breathing]
    ↓ click enemy
[Attack] → deal damage
    ↓ animation complete
[Idle/Breathing]
```

---

## Movement Integration

### Duelyst Movement Formula

Movement duration is tied to the walk/run animation:

```cpp
float base_duration = anim_duration * ENTITY_MOVE_DURATION_MODIFIER;  // 1.0
float correction = base_duration * ENTITY_MOVE_CORRECTION;  // 0.2
float total_duration = base_duration * (tile_count + 1) - correction;
```

### Example: f1_general moving 3 tiles

Animation: run at 8 frames, 12 FPS = 0.667s
```
base = 0.667 × 1.0 = 0.667s
correction = 0.667 × 0.2 = 0.133s
total = 0.667 × (3 + 1) - 0.133
      = 2.668 - 0.133
      = 2.535 seconds
```

Speed: 3 tiles / 2.535s ≈ **1.18 tiles/second**

### Why (tile_count + 1)?

The animation plays through `tile_count + 1` full cycles during movement:
- Moving 1 tile: plays animation ~1.8 times
- Moving 2 tiles: plays animation ~2.8 times
- Moving 3 tiles: plays animation ~3.8 times

This creates natural acceleration/deceleration.

### Movement Implementation

```cpp
void Entity::start_move(int window_w, int window_h, BoardPos target) {
    // Calculate tile distance
    int dx = abs(target.x - board_pos.x);
    int dy = abs(target.y - board_pos.y);
    int tile_count = dx + dy;  // Manhattan distance
    
    // Get run animation
    const Animation* run_anim = animations.find("run");
    
    // Calculate movement duration
    move_duration = calculate_move_duration(run_anim->duration(), tile_count);
    
    // Flip sprite if moving left
    flip_x = (target.x < board_pos.x);
    
    // Start playing run animation
    play_animation("run");
    state = EntityState::Moving;
}
```

---

## Animation Variations by Unit

Different units have different animation specs:

### f1_general (Lyonar general)
- Run: 8 frames @ 12 FPS = 0.667s
- Attack: 23 frames @ 12 FPS = 1.917s
- Cast: 12 frames @ 12 FPS = 1.000s

### f2_general (Songhai general)
- Run: 8 frames @ 14 FPS = 0.571s (faster!)
- Attack: 24 frames @ 14 FPS = 1.714s

### neutral_sai (minion)
- Run: 8 frames @ 12 FPS = 0.667s
- Attack: 18 frames @ 12 FPS = 1.500s

Units with higher FPS play animations faster (Songhai is quick and agile).

---

## Implementation Checklist

### Loader (animation_loader.cpp)
- [x] Parse tab-separated format
- [x] Split comma-separated frame data
- [x] Create AnimFrame structs with rect
- [x] Calculate frame_count from values.size() / 5
- [x] Store fps for timing calculations

### Entity (entity.cpp)
- [x] Load spritesheet texture
- [x] Load animations from file
- [x] Track current animation and time
- [x] Update animation time with delta
- [x] Calculate current frame index
- [x] Loop animations correctly
- [x] Render current frame centered

### Movement (entity.cpp)
- [x] Use run/walk animation duration
- [x] Apply Duelyst timing formula
- [x] Interpolate position over duration
- [x] Play run animation during move
- [x] Return to idle on arrival
- [x] Flip sprite when moving left

---

## Performance Notes

### Memory Layout
Each unit stores:
- 1 texture (spritesheet) - shared GPU memory
- N animations (typically 8-12)
- Frame data is small (5 ints × frames per animation)

f1_general example:
- Spritesheet: ~900KB GPU memory
- Animations: ~100 bytes CPU memory

### Rendering Cost
- 1 texture bind per entity
- 1 draw call per entity
- Rect copy to set source region
- Negligible overhead

### Optimization Opportunities
- Sprite batching: group entities using same spritesheet
- Animation data sharing: units of same type share animation structs
- Texture atlasing: multiple units in one texture (not in our format)

---

## Common Pitfalls

### Wrong Frame Count
```cpp
// WRONG: Counts commas, not values
frame_count = count(frame_data, ',')

// CORRECT: Counts values then divides by 5
frame_count = (count(frame_data, ',') + 1) / 5
```

### Animation Not Looping
```cpp
// WRONG: time keeps growing unbounded
anim_time += dt;

// CORRECT: wrap around at duration
anim_time += dt;
if (anim_time >= duration) {
    anim_time -= duration;
}
```

### Frame Index Out of Bounds
```cpp
// WRONG: can exceed frame_count-1
int frame_idx = anim_time * fps;

// CORRECT: modulo to stay in range
int frame_idx = static_cast<int>(anim_time * fps) % frame_count;
```

### Forgetting to Flip Sprite
```cpp
// Entities should flip horizontally when moving left
flip_x = (target.x < current.x);

SDL_RenderTextureFlipped(
    renderer, texture, &src, &dst, 0, nullptr,
    flip_x ? SDL_FLIP_HORIZONTAL : SDL_FLIP_NONE
);
```

---

## Future Enhancements

### Attack Animation Timing
Read `attackDelay` and `attackReleaseDelay` from unit data to sync:
- Play attack animation
- Wait for `attackReleaseDelay` (weapon swing point)
- Spawn projectile or damage FX
- Wait for animation to complete

### Animation Events
Trigger events at specific frames:
- Frame 5: play weapon swoosh sound
- Frame 8: spawn impact FX
- Frame 12: return to idle

### Animation Blending
Smooth transitions between animations:
- Crossfade over 0.1s when switching
- Prevents jarring pops

### Sprite Sheets with Multiple Units
Load multiple unit types into one texture:
- Reduce texture binds
- Enable sprite batching
- More complex packing algorithm needed

---

## Reference: Full f1_general animations.txt

```
attack	12	0,404,0,100,100,1,808,0,100,100,2,707,909,100,100,3,707,808,100,100,4,707,707,100,100,5,707,606,100,100,6,707,505,100,100,7,707,404,100,100,8,707,303,100,100,9,707,202,100,100,10,707,101,100,100,11,707,0,100,100,12,606,909,100,100,13,606,808,100,100,14,606,707,100,100,15,606,606,100,100,16,606,505,100,100,17,606,404,100,100,18,606,303,100,100,19,606,202,100,100,20,606,101,100,100,21,606,0,100,100,22,505,909,100,100
breathing	8	0,505,808,100,100,1,505,707,100,100,2,404,0,100,100,3,404,0,100,100,4,404,0,100,100,5,404,0,100,100,6,505,606,100,100,7,505,505,100,100,8,505,404,100,100,9,505,303,100,100,10,505,202,100,100,11,505,101,100,100
cast	12	0,505,0,100,100,1,404,909,100,100,2,404,808,100,100,3,404,707,100,100,4,404,606,100,100,5,404,505,100,100,6,404,404,100,100,7,404,404,100,100,8,404,303,100,100,9,404,202,100,100,10,404,101,100,100,11,404,0,100,100
castend	12	0,808,101,100,100,1,303,909,100,100,2,303,808,100,100
castloop	12	0,303,707,100,100,1,303,606,100,100,2,303,505,100,100,3,303,404,100,100
caststart	12	0,303,303,100,100,1,303,202,100,100,2,303,101,100,100,3,303,0,100,100,4,202,909,100,100,5,202,808,100,100,6,202,707,100,100,7,202,606,100,100
death	12	0,202,505,100,100,1,202,404,100,100,2,202,303,100,100,3,202,202,100,100,4,202,101,100,100,5,202,0,100,100,6,101,909,100,100,7,101,808,100,100,8,101,707,100,100,9,101,606,100,100
idle	15	0,101,505,100,100,1,101,404,100,100,2,101,303,100,100,3,101,202,100,100,4,101,101,100,100,5,101,0,100,100,6,0,909,100,100,7,0,808,100,100
run	12	0,0,707,100,100,1,0,606,100,100,2,0,505,100,100,3,0,404,100,100,4,0,303,100,100,5,0,202,100,100,6,0,101,100,100,7,0,0,100,100
```

**Statistics:**
- Total animations: 9
- Total frames: 96
- Spritesheet dimensions: 909×1010
- Frame size: 100×100 (consistent)
- FPS range: 8-15

---

## Summary

The animation system is straightforward:
1. **Parse** tab-separated text files into structs
2. **Store** frame rectangles and timing info
3. **Update** animation time each frame
4. **Calculate** current frame index with modulo
5. **Render** the appropriate sprite rect

The key insight is that **animation duration drives movement timing**, not the other way around. This creates natural-feeling movement that stays in sync with the sprite's walk cycle.
