# Duelyst Asset Loading Guide

This document describes how Sylvanshine loads assets from the open-duelyst/duelyst repository and the structure of that repository.

## Repository Location

Assets are loaded from a locally cloned copy of the Duelyst repository:
- Repository: https://github.com/open-duelyst/duelyst
- Clone it: `git clone https://github.com/open-duelyst/duelyst.git`

## CMake Configuration

Configure the path to your local Duelyst repository clone when building:

```bash
mkdir build && cd build
cmake -DDUELYST_REPO_PATH=/path/to/your/duelyst ..
make
```

The `DUELYST_REPO_PATH` must point to the root of the cloned repository.

## Duelyst Repository Structure

The Duelyst repository has the following relevant structure:

```
duelyst/
├── app/
│   ├── resources/           # Main asset directory
│   │   ├── units/          # Unit sprites and animations
│   │   ├── fx/             # Visual effects
│   │   ├── sfx/            # Sound effects
│   │   ├── music/          # Background music
│   │   ├── tiles/          # Board tiles
│   │   ├── maps/           # Map backgrounds
│   │   ├── icons/          # UI icons
│   │   └── ...
│   ├── shaders/            # GLSL shaders
│   └── ...
└── ...
```

## Asset File Formats

### Units (`app/resources/units/`)

Each unit has two files:
- `{unit_name}.plist` - Sprite frame definitions (Apple Property List XML format)
- `{unit_name}.png` - Spritesheet texture

**Example:** `f1_general.plist` and `f1_general.png`

**Unit Naming Convention:**
| Prefix | Faction |
|--------|---------|
| `f1_` | Lyonar Kingdoms |
| `f2_` | Songhai Empire |
| `f3_` | Vetruvian Imperium |
| `f4_` | Abyssian Host |
| `f5_` | Magmar Aspects |
| `f6_` | Vanar Kindred |
| `boss_` | Boss encounters |
| `neutral_` | Neutral units |

**Animation Types:**
- `idle` - Default standing animation
- `run` / `walk` - Movement animation
- `attack` - Basic attack
- `cast` / `caststart` / `castloop` / `castend` - Spell casting
- `death` - Death animation
- `hit` - Taking damage
- `breathing` - Idle breathing variant

### Visual Effects (`app/resources/fx/`)

Each effect has two files:
- `{fx_name}.plist` - Frame definitions
- `{fx_name}.png` - Spritesheet

**FX Naming Convention:**
- `fx_{name}` - Generic effects (e.g., `fx_clawslash`, `fx_impact`)
- `fx_f{n}_{name}` - Faction-specific (e.g., `fx_f1_divinebond`)
- `fx_f{n}_bbs_{name}` - Bloodborn spells

### Sound Effects (`app/resources/sfx/`)

Format: `.m4a` (MPEG-4 Audio)

**Naming Pattern:**
- `sfx_{unit_name}_{action}.m4a` - Unit sounds
- `sfx_spell_{name}.m4a` - Spell sounds
- `sfx_ui_{element}.m4a` - UI sounds

### Music (`app/resources/music/`)

Format: `.m4a`

Files include:
- `music_battlemap{n}.m4a` - Battle music
- `music_mainmenu.m4a` - Menu music
- `music_collection.m4a` - Collection screen

## Plist File Format

The `.plist` files use Apple's Property List XML format. Key structure:

```xml
<plist version="1.0">
<dict>
    <key>frames</key>
    <dict>
        <key>f1_general_attack_000.png</key>
        <dict>
            <key>frame</key>
            <string>{{404,0},{100,100}}</string>
            <key>offset</key>
            <string>{0,0}</string>
            <key>rotated</key>
            <false/>
            <key>sourceSize</key>
            <string>{100,100}</string>
        </dict>
        <!-- More frames... -->
    </dict>
    <key>metadata</key>
    <dict>
        <key>size</key>
        <string>{1024,1024}</string>
        <key>textureFileName</key>
        <string>f1_general.png</string>
    </dict>
</dict>
</plist>
```

Frame naming pattern: `{unit_name}_{animation}_{frame_number}.png`
- Example: `f1_general_attack_000.png` to `f1_general_attack_022.png`

## Local Data Files

Some data files remain in Sylvanshine's local `data/` directory:

- `data/fx/rsx_mapping.tsv` - Maps RSX effect names to folder names
- `data/fx/manifest.tsv` - FX manifest with animation lists
- `data/timing/unit_timing.tsv` - Attack timing data
- `data/unit_shadow.png` - Shared shadow texture
- `data/shaders/` - Compiled shader files (SPIR-V/MSL)

These files contain game-specific configuration or processed data.

## Code Architecture

### Asset Path Management

All asset paths are managed through `AssetPaths` namespace (`include/asset_paths.hpp`):

```cpp
// Initialize (call once at startup)
AssetPaths::init();

// Get paths to Duelyst assets
AssetPaths::get_unit_spritesheet_path("f1_general");  // -> {repo}/app/resources/units/f1_general.png
AssetPaths::get_unit_plist_path("f1_general");        // -> {repo}/app/resources/units/f1_general.plist
AssetPaths::get_fx_spritesheet_path("fx_clawslash");  // -> {repo}/app/resources/fx/fx_clawslash.png

// Get paths to local data
AssetPaths::get_local_data_path();                    // -> "data"
AssetPaths::get_timing_path("unit_timing.tsv");       // -> "data/timing/unit_timing.tsv"
```

### Plist Parsing

The `plist_parser.hpp` module handles parsing Duelyst's plist format:

```cpp
// Parse a plist file
PlistData plist = parse_plist("/path/to/unit.plist");

// Convert to AnimationSet
AnimationSet anims = plist_to_animations(plist, "f1_general");
```

### Entity Loading

Entities now load directly from the Duelyst repository:

```cpp
Entity unit;
unit.load("f1_general");  // Loads from {DUELYST_REPO_PATH}/app/resources/units/
```

## Troubleshooting

### "Error: DUELYST_REPO_PATH not set"
Set the path when running CMake:
```bash
cmake -DDUELYST_REPO_PATH=/path/to/duelyst ..
```

### "Duelyst resources not found"
Verify the path points to the repository root and contains `app/resources/`.

### "No animations loaded from plist"
- Check that the unit/fx name matches a file in the repository
- Verify the plist file exists and is valid XML
- Check console output for specific parsing errors

## Future Work

- Sound effects loading (currently not implemented)
- Music streaming
- Shader loading from Duelyst repository (currently using local compiled shaders)
- Direct integration with Duelyst's build system
