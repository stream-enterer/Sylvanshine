# System: View / Rendering

**Location:** app/view/

## Purpose
The View system provides the visual representation of the game using Cocos2d-JS, including sprite rendering, animations, visual effects, and scene management.

## Key Components
| Component | File | Purpose |
|-----------|------|---------|
| Scene | Scene.js | Main scene manager |
| Player | Player.js | Player view controller (~88KB) |
| BaseSprite | nodes/BaseSprite.js | Enhanced sprite with effects |
| GlowSprite | nodes/GlowSprite.js | Glowing outline sprites |
| BaseParticleSystem | nodes/BaseParticleSystem.js | Particle effects |
| BaseLabel | nodes/BaseLabel.js | Text rendering |
| BaseLayer | layers/BaseLayer.js | Layer base class |
| GameLayer | layers/game/GameLayer.js | Main game battlefield |

## Data Flow
**Input:** Game state changes, SDK events, user interactions
**Processing:** Node creation → Animation sequencing → Effect compositing → Rendering
**Output:** Visual display, particle effects, UI feedback

## Dependencies
**Requires:** Cocos2d-JS framework, SDK events, Resources
**Used by:** Application, UI system

## Directory Structure
```
view/
├── Scene.js              # Main scene management
├── Player.js             # Player view state
├── actions/              # View action helpers
├── nodes/
│   ├── BaseSprite.js     # Core sprite
│   ├── GlowSprite.js     # Highlighted sprites
│   ├── BaseParticleSystem.js  # Particles
│   ├── cards/            # Card display nodes
│   ├── fx/               # Effect nodes
│   ├── map/              # Board/tile nodes
│   ├── reward/           # Reward displays
│   └── components/       # UI components
├── layers/
│   ├── BaseLayer.js      # Layer foundation
│   ├── game/             # Battle layers
│   ├── arena/            # Gauntlet layers
│   ├── reward/           # Reward screens
│   ├── rift/             # Rift mode layers
│   └── start/            # Menu layers
├── fx/                   # FX utilities
├── helpers/              # View utilities
└── extensions/           # Cocos extensions
```

## Configuration
| Constant | Value | Purpose |
|----------|-------|---------|
| CONFIG.DEPTH_OFFSET | 19.5 | Z-ordering offset |
| CONFIG.ENTITY_XYZ_ROTATION | {x:26, y:0, z:0} | Isometric tilt |
| CONFIG.BLOOM_DEFAULT | 0.7 | Default bloom intensity |
| CONFIG.FADE_FAST_DURATION | 0.2 | Quick fade time |
| CONFIG.FADE_MEDIUM_DURATION | 0.35 | Standard fade time |
| CONFIG.FADE_SLOW_DURATION | 1.0 | Slow fade time |
| CONFIG.ANIMATE_FAST_DURATION | 0.2 | Fast animation |
| CONFIG.ANIMATE_MEDIUM_DURATION | 0.35 | Medium animation |
| CONFIG.ANIMATE_SLOW_DURATION | 1.0 | Slow animation |

## Resources
| Resource Category | Path | Purpose |
|-------------------|------|---------|
| Unit sprites | resources/units/ | 40,285 bytes of animations |
| FX sprites | resources/fx/ | 12,892 bytes of effects |
| Particles | resources/particles/ | 3,753 bytes of particles |
| Maps | resources/maps/ | 1,907 bytes of battlemaps |
| Tiles | resources/tiles/ | 2,239 bytes of tile graphics |

## Node Types
| Node | Purpose |
|------|---------|
| CardNode | Card display with stats |
| EntityNode | Board unit representation |
| SpeechNode | General speech bubbles |
| InstructionNode | Tutorial arrows |
| TileNode | Ground effect display |
| RewardNode | Reward animations |
| LootCrateNode | Crate opening |

## Layer Types
| Layer | Purpose |
|-------|---------|
| GameLayer | Main battlefield |
| BattleMap | Background parallax |
| BattleLog | Action history |
| ChooseCardLayer | Card selection (Gauntlet/Rift) |
| RewardLayer | Victory rewards |
| BoosterPackOpeningLayer | Pack opening animation |

## Visual Effects (FX)
- **240 FX definitions** in instances/fx.tsv
- FX categories:
  - Faction-specific (fx_f1_*, fx_f2_*, etc.)
  - Buff/debuff effects
  - Damage effects
  - Teleport/movement effects
  - Particle systems

## Statistics
- Scene manages entire visual hierarchy
- Player.js is largest view file (88KB)
- 13 node subdirectories
- 13 layer subdirectories
- Supports parallax scrolling, particles, shaders
