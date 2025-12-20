# System: Data & Resources

**Location:** app/data/, app/resources/, app/common/config.js

## Purpose
The Data & Resources system provides configuration constants, asset definitions, game tips, shop data, and all visual/audio resource references used throughout the game.

## Key Components
| Component | File | Purpose |
|-----------|------|---------|
| config.js | common/config.js | All CONFIG.* constants |
| resources.js | data/resources.js | All RSX.* asset references |
| fx.js | data/fx.js | FX.* visual effect definitions |
| game_tips.js | data/game_tips.js | Loading screen tips |
| shop.json | data/shop.json | Shop product definitions |
| premium_shop.json | data/premium_shop.json | Premium purchases |

## Data Flow
**Input:** Asset files from resources/, configuration values
**Processing:** Lazy loading via PackageManager, caching
**Output:** Loaded textures, sounds, configuration values

## Dependencies
**Requires:** File system, asset pipeline
**Used by:** All game systems, View, Audio

## Directory Structure
```
data/
├── fx.js               # 265KB - FX definitions
├── resources.js        # 1.5MB - All asset paths
├── game_tips.js        # Loading screen text
├── shop.json           # Shop products (34KB)
├── premium_shop.json   # IAP products (2KB)
├── packages_predefined.js  # Asset packages
└── utils/              # Data utilities

resources/
├── arena/              # Gauntlet assets
├── battlelog/          # Battle log UI
├── booster_pack_opening/  # Pack animations
├── boss_battles/       # Boss portraits/assets
├── card_backgrounds/   # Card frame art
├── challenges/         # Challenge assets
├── codex/              # Lore chapter images
├── crests/             # Faction crests
├── emotes/             # Emote animations (26 dirs)
├── fonts/              # Game fonts
├── fx/                 # Visual effects (12,892 bytes)
├── generals/           # General portraits
├── icons/              # UI icons (25,597 bytes)
├── loot_crates/        # Crate graphics
├── maps/               # Battle maps
├── modifiers/          # Modifier icons
├── particles/          # Particle textures
├── profile_icons/      # Player avatars
├── scenes/             # Scene backgrounds
├── sfx/                # Sound effects (26,891 bytes)
├── shop/               # Store UI
├── tiles/              # Tile graphics
├── tutorial/           # Tutorial assets
├── ui/                 # Interface elements
├── units/              # Unit sprites (40,285 bytes)
└── web/                # Web-specific assets
```

## Configuration Categories
| Category | Prefix | Examples |
|----------|--------|----------|
| Animation Timing | CONFIG.ANIMATE_* | FAST_DURATION, SLOW_DURATION |
| Board | CONFIG.BOARD* | BOARDCOL=9, BOARDROW=5 |
| Colors | CONFIG.*_COLOR | AGGRO_COLOR, BUFF_COLOR |
| UI | CONFIG.*_SELECTOR | APP_SELECTOR, GAME_SELECTOR |
| Gameplay | CONFIG.* | CARD_DRAW_PER_TURN, HIGH_DAMAGE |
| Visual | CONFIG.BLOOM_*, CONFIG.GLOW_* | Rendering settings |
| Audio | CONFIG.DEFAULT_*_VOLUME | Sound levels |

## Resource Categories
| Prefix | Content | Size |
|--------|---------|------|
| RSX.battlemap* | Battle backgrounds | 11 maps |
| RSX.crest_* | Faction crests | 6 factions |
| RSX.unit_* | Unit sprites | 664+ units |
| RSX.fx_* | Visual effects | 240 effects |
| RSX.sfx_* | Sound effects | 26,891 bytes |
| RSX.emote_* | Emote animations | 26 sets |
| RSX.icon_* | UI icons | 25,597 bytes |

## FX Categories
| Category | Count | Purpose |
|----------|-------|---------|
| faction1_lyonar | 17 | Lyonar spell effects |
| faction2_songhai | 23 | Songhai effects |
| faction3_vetruvian | 18 | Vetruvian effects |
| faction4_abyssian | 20 | Abyssian effects |
| faction5_magmar | 15 | Magmar effects |
| faction6_vanar | 19 | Vanar effects |
| buff_effect | 4 | Buff visuals |
| damage_effect | 6 | Damage visuals |
| neutral | 50+ | Shared effects |

## Statistics
- **CONFIG.***: 200+ configuration constants
- **RSX.***: 1000+ resource references
- **FX.***: 240 effect definitions
- resources.js: 1.5MB (largest data file)
- fx.js: 265KB of effect templates
- 44 resource subdirectories
- 26 emote variations
- 8 map themes
