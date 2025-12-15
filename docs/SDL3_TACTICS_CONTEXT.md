# SDL3 Tactics Game - Context Restoration

## Current Status: Phase 2 Step 1 Complete ✓

Working tactics game with multi-unit selection, collision detection, and RAII-refactored codebase.

## What's Implemented

### Phase 1: Core Movement System ✓
- **Grid rendering**: 9×5 board, 95px tiles, white grid lines, centered at (0, 10) offset
- **Animation system**: Parses `animations.txt`, plays sprite animations with proper FPS
- **Entity rendering**: Sprites centered on tiles with animation playback
- **Movement system**: Click-to-move with Duelyst-accurate timing formula
- **Sprite flipping**: Horizontal flip when moving left (using negative width technique)

### Phase 2 Step 1: Multiple Units ✓
- **Multi-unit support**: GameState holds `std::vector<Entity>`, selection by index
- **Collision detection**: Units cannot move onto occupied tiles
- **Real-time validation**: Checks move targets for units currently moving
- **Independent movement**: Each unit selects and moves independently
- **RAII refactor**: All SDL resources wrapped in RAII types, proper move semantics

### Movement Timing (Exact Duelyst Formula)
```cpp
base_duration = walk_anim_duration × ENTITY_MOVE_DURATION_MODIFIER (1.0)
correction = base_duration × ENTITY_MOVE_CORRECTION (0.2)
total = base_duration × (tile_count + 1) - correction
```

For f1_general (8 frames @ 12 FPS = 0.667s):
- 1 tile: 1.20s
- 2 tiles: 1.87s
- 3 tiles: 2.54s

### Controls
- **Left click unit**: Select and show 3-tile movement range (white highlights)
- **Left click valid tile**: Move to that tile with run animation
- **Click during movement**: Blocked until current animation completes
- **Collision**: Cannot move onto tiles occupied by other units (including their move targets)

## Project Structure

```
project/
├── data/
│   └── units/
│       └── f1_general/
│           ├── spritesheet.png
│           └── animations.txt
├── src/
│   ├── types.hpp              # Core types (Vec2, BoardPos, Animation)
│   ├── types.cpp
│   ├── animation_loader.hpp   # Animation file parser
│   ├── animation_loader.cpp
│   ├── entity.hpp             # Entity/unit class with RAII
│   ├── entity.cpp
│   ├── grid.hpp               # Grid rendering + collision
│   ├── grid.cpp
│   └── main.cpp               # Main loop, GameState, multi-unit logic
├── CMakeLists.txt
└── build.fish
```

## Build Requirements

- **SDL3**: System package (pkg-config: `sdl3`)
- **SDL3_image**: Manual link (no pkg-config file available)
- **C++20**: Required for ranges/concepts

### CMakeLists.txt Configuration
```cmake
find_package(PkgConfig REQUIRED)
pkg_check_modules(SDL3 REQUIRED sdl3)
include_directories(/usr/include/SDL3_image)
target_link_libraries(${PROJECT_NAME} PRIVATE ${SDL3_LIBRARIES} SDL3_image)
```

### Build Commands
```bash
./build.fish build   # or: ./build.fish rebuild
./build.fish run
```

## Critical Implementation Details

### RAII Resource Management
All SDL resources wrapped in RAII types:
```cpp
struct TextureHandle {
    SDL_Texture* ptr = nullptr;
    ~TextureHandle() { if (ptr) SDL_DestroyTexture(ptr); }
    TextureHandle(TextureHandle&& o) noexcept : ptr(o.ptr) { o.ptr = nullptr; }
    // Move assignment, deleted copy...
};

struct Entity {
    TextureHandle spritesheet;  // Automatic cleanup, proper move
    AnimationSet animations;    // Value semantics
};
```

### Multi-Unit Selection
```cpp
struct GameState {
    std::vector<Entity> units;          // Store by value
    int selected_unit_idx = -1;         // -1 = none selected
    std::vector<BoardPos> reachable_tiles;
};
```

### Collision Detection
- `get_occupied_positions(state, exclude_idx)` returns occupied tiles
- For moving units, returns `move_target` not `board_pos`
- Revalidates before move to catch race conditions
- `get_reachable_tiles(from, range, occupied)` filters out occupied

### Animation Frame Storage
```cpp
struct Animation {
    char name[32];
    int fps;
    std::vector<AnimFrame> frames;  // Value semantics, no manual delete
};
```

### Sprite Flipping (SDL3 Workaround)
```cpp
if (flip_x) {
    dst.x = screen_pos.x + src.w * 0.5f;  // Right edge becomes anchor
    dst.w = -src.w;                        // Negative width flips
} else {
    dst.x = screen_pos.x - src.w * 0.5f;  // Left edge anchor (normal)
    dst.w = src.w;
}
SDL_RenderTexture(renderer, spritesheet, &src, &dst);
```

### Animation File Format
Tab-separated: `{name}\t{fps}\t{idx,x,y,w,h,idx,x,y,w,h,...}`

Example:
```
run	12	0,0,707,100,100,1,0,606,100,100,2,0,505,100,100,...
```

Frame data: comma-separated quintuplets (5 values per frame)

## Knowledge Base Files (Project Knowledge)

1. **DUELYST_ASSET_FORMAT_SPEC.md** - Asset formats, manifests, loading sequences
2. **DUELYST_FX_IMPLEMENTATION_GUIDE.md** - FX system architecture (not yet implemented)
3. **DUELYST_FX_KNOWLEDGE.md** - Complete FX pipeline details
4. **VISUAL_FRAMEWORK.md** - Grid layout, transforms, timing constants
5. **ANIMATION_SYSTEM.md** - Complete animation documentation with examples
6. **CPP_CODING_STANDARDS.md** - C++ coding standards (in custom instructions)

## Design Decisions

- **RAII everywhere**: All SDL resources in RAII wrappers, no manual cleanup
- **Value semantics**: Store objects directly, avoid pointers
- **No comments**: Code should be self-documenting
- **Flat structure**: All source in `src/`
- **Negative width flip**: Works around SDL3 flip mode issues
- **Movement range**: Hardcoded 3 tiles (will vary by unit stats later)

## Known Constants

```cpp
BOARD_COLS = 9
BOARD_ROWS = 5
TILE_SIZE = 95
TILE_OFFSET = {0.0f, 10.0f}
ENTITY_MOVE_DURATION_MODIFIER = 1.0f
ENTITY_MOVE_CORRECTION = 0.2f
MOVE_RANGE = 3
```

## Phase 2 Remaining Steps

### Step 2: Combat State Machine (NEXT)
- Extend `EntityState`: add `Attacking`, `TakingDamage`, `Dying`
- Add unit type enum: `Player`, `Enemy`
- Attack range checking (similar to move range)
- Click unit → click enemy → attack action
- Play "attack" animation → apply damage → return to idle
- Test: Attack adjacent enemy, verify state transitions

### Step 3: HP System & Damage Numbers
- Add `int hp, max_hp` to Entity
- HP bars: filled rect above sprite
- Damage numbers: floating text on hit
- Death handling: play death animation → remove from vector
- Test: Damage reduces HP, bars update, numbers float

### Step 4: Basic FX System
- Parse `fx/*/animations.txt` (reuse animation loader)
- New `FXEntity` class: position, animation, auto-destroy
- Spawn FX on unit spawn/death
- Test: Visual feedback on key events

### Step 5: Attack FX & Projectiles
- Parse `fx_compositions/compositions.json`
- Projectile FX: lerp from attacker to target, `impactAtEnd` timing
- Melee vs ranged attack visuals
- Test: Different attack types have correct visuals

### Step 6: Turn System
- Turn phases: `PlayerTurn`, `EnemyTurn`
- Lock input during enemy turn
- Simple AI: random move → random attack
- Test: Full combat with alternating turns

## Common Issues & Solutions

### Issue: Units invisible after move
**Cause**: Raw pointer texture destroyed by move  
**Fix**: Wrap in RAII `TextureHandle` with proper move semantics

### Issue: Units can move onto moving units' destinations
**Cause**: Occupied positions calculated at selection time, not move time  
**Fix**: Revalidate occupied positions before starting move

### Issue: Sprite invisible when flipped
**Cause**: SDL3 flip modes don't work with `SDL_RenderTextureRotated`  
**Fix**: Use negative width in destination rect

### Issue: Animation frames corrupted
**Cause**: Double-free from vector copying raw pointers  
**Fix**: Use `std::vector<AnimFrame>` with value semantics

### Issue: SDL_image functions not found
**Cause**: No pkg-config file for SDL3_image  
**Fix**: Manual include path + link: `/usr/include/SDL3_image` + `-lSDL3_image`

### Issue: Animation not looping
**Cause**: Time accumulates unbounded  
**Fix**: `while (anim_time >= duration) { anim_time -= duration; }`
