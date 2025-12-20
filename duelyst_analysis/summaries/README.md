# Duelyst System Summaries

This directory contains comprehensive documentation for all major systems in the Duelyst codebase.

## System Index

### Core Game Systems
| System | File | Description |
|--------|------|-------------|
| [Game Session](game_session.md) | gameSession.coffee | Central game state singleton |
| [Actions](actions.md) | actions/ | Command pattern for game changes |
| [Cards](cards.md) | cards/, entities/ | All playable cards |
| [Modifiers](modifiers.md) | modifiers/ | Buffs, abilities, keywords |
| [Board](board.md) | board.coffee | 9x5 game grid |
| [Spells](spells.md) | spells/ | Instant effect cards |

### Visual Systems
| System | File | Description |
|--------|------|-------------|
| [View](view.md) | view/ | Cocos2d rendering |
| [UI](ui.md) | ui/ | Backbone.js interface |
| [Data & Resources](data_resources.md) | data/, resources/ | Assets and config |
| [Audio](audio.md) | audio/, sfx/ | Sound system |

### Meta-Game Systems
| System | File | Description |
|--------|------|-------------|
| [Factions](factions.md) | cards/factions* | 6 playable factions |
| [Progression](progression.md) | achievements/, quests/ | Achievements, quests |
| [Challenges](challenges.md) | challenges/ | Tutorials, puzzles |
| [Cosmetics](cosmetics.md) | cosmetics/ | Visual customization |

### Infrastructure
| System | File | Description |
|--------|------|-------------|
| [Network](network.md) | networkManager.coffee | Multiplayer sync |
| [AI Agents](ai_agents.md) | agents/ | Computer opponents |

## Statistics Summary

### Card System
- **1,116 cards** total
- **664 units** (minions + generals)
- **301 spells**
- **55 artifacts**
- **7 tiles**
- **6 factions** + neutral

### Modifier System
- **717 modifier types**
- **47 player modifiers**
- **18+ keyword abilities**

### Action System
- **64 action types**
- Hierarchical action execution
- Network-synchronized

### Visual System
- **240 FX definitions**
- **449 cosmetics**
- **11+ battle maps**

### Progression System
- **57 achievements**
- **49 quests**
- **51 challenges**

## File Relationships

```
┌─────────────────────────────────────────────────────────────┐
│                      GameSession                             │
│  (Central authority for all game state)                      │
├─────────────────────────────────────────────────────────────┤
│                                                              │
│  ┌─────────┐    ┌─────────┐    ┌─────────┐    ┌─────────┐  │
│  │  Board  │    │ Player  │    │ Player  │    │ Network │  │
│  │  (9x5)  │    │   1     │    │   2     │    │ Manager │  │
│  └────┬────┘    └────┬────┘    └────┬────┘    └─────────┘  │
│       │              │              │                       │
│  ┌────┴────┐    ┌────┴────┐    ┌────┴────┐                 │
│  │ Entities│    │  Deck   │    │  Deck   │                 │
│  │ (Units, │    │  Hand   │    │  Hand   │                 │
│  │  Tiles) │    │ Signat. │    │ Signat. │                 │
│  └────┬────┘    └─────────┘    └─────────┘                 │
│       │                                                     │
│  ┌────┴────────────────────────────────────┐               │
│  │              Modifiers                   │               │
│  │  (Buffs, abilities, keywords - 717)     │               │
│  └──────────────────────────────────────────┘               │
│                                                              │
│  ┌──────────────────────────────────────────┐               │
│  │              Actions                      │               │
│  │  (Commands for state changes - 64)       │               │
│  └──────────────────────────────────────────┘               │
└─────────────────────────────────────────────────────────────┘
                           │
                           ▼
┌─────────────────────────────────────────────────────────────┐
│                    View System                               │
│  ┌─────────┐    ┌─────────┐    ┌─────────┐    ┌─────────┐  │
│  │  Scene  │    │ Layers  │    │  Nodes  │    │   FX    │  │
│  └─────────┘    └─────────┘    └─────────┘    └─────────┘  │
└─────────────────────────────────────────────────────────────┘
                           │
                           ▼
┌─────────────────────────────────────────────────────────────┐
│                    UI System                                 │
│  ┌─────────┐    ┌─────────┐    ┌─────────┐    ┌─────────┐  │
│  │Managers │    │  Views  │    │ Models  │    │Templates│  │
│  └─────────┘    └─────────┘    └─────────┘    └─────────┘  │
└─────────────────────────────────────────────────────────────┘
```

## Data Sources

These summaries were generated from:
- `duelyst_analysis/semantic/` - Code structure analysis
- `duelyst_analysis/instances/` - Entity counts and examples
- `duelyst_analysis/schemas/` - Entity relationships
- `app/` - Source code reading

## Usage

Each summary follows a consistent format:
1. **Purpose** - System's role in the game
2. **Key Components** - Main files and classes
3. **Data Flow** - Input → Processing → Output
4. **Dependencies** - Required and dependent systems
5. **Configuration** - Relevant CONFIG.* constants
6. **Resources** - RSX.* and file assets
7. **Statistics** - Counts from instance data
