# Duelyst Entity Schemas

This directory contains schema documentation for all game entity types discovered in the Duelyst codebase.

## Core SDK Entities

These entities extend `SDKObject` and form the foundation of the game logic:

| Schema | Description |
|--------|-------------|
| [SDKObject](SDKObject.md) | Base class for all SDK entities |
| [Action](Action.md) | Command pattern for game state changes |
| [Card](Card.md) | Base class for all cards |
| [Entity](Entity.md) | Cards that exist on the board |
| [Unit](Unit.md) | Minions and Generals |
| [Tile](Tile.md) | Persistent board effects |
| [Spell](Spell.md) | One-time effect cards |
| [Artifact](Artifact.md) | Equipment for Generals |
| [Modifier](Modifier.md) | Buff/debuff/ability system |
| [PlayerModifier](PlayerModifier.md) | Player-level modifiers |
| [Board](Board.md) | 9x5 game grid |
| [Deck](Deck.md) | Draw pile management |
| [Player](Player.md) | Player state and resources |
| [GameSession](GameSession.md) | Central game state singleton |
| [GameTurn](GameTurn.md) | Turn container |
| [Step](Step.md) | Action execution unit |

## Meta-Game Entities

| Schema | Description |
|--------|-------------|
| [Achievement](Achievement.md) | Meta-game progression goals |
| [Quest](Quest.md) | Daily objectives |
| [Challenge](Challenge.md) | Puzzle/tutorial scenarios |

## Configuration Entities

| Schema | Description |
|--------|-------------|
| [Faction](Faction.md) | Playable classes (Lyonar, Songhai, etc.) |
| [Rarity](Rarity.md) | Card rarity tiers |
| [Race](Race.md) | Unit tribes (Golem, Mech, etc.) |
| [CardSet](CardSet.md) | Expansion sets |
| [BattleMapTemplate](BattleMapTemplate.md) | Battle map configuration |

## Utility Entities

| Schema | Description |
|--------|-------------|
| [Range](Range.md) | Movement/attack range calculation |
| [Validator](Validator.md) | Action validation system |
| [Agent](Agent.md) | AI controller |
| [Cosmetics](Cosmetics.md) | Visual customization items |
| [FX](FX.md) | Visual effects system |

## Entity Relationships

```
                    SDKObject
                        │
    ┌───────────────────┼───────────────────────┐
    │                   │                       │
  Action              Card                   Modifier
    │                   │                       │
    │          ┌────────┼────────┐              │
    │          │        │        │              │
    │       Entity    Spell   Artifact    PlayerModifier
    │          │                                │
    │      ┌───┴───┐                   GameSessionModifier
    │      │       │
    │    Unit    Tile
    │
  [50+ Action types]              [400+ Modifier types]
```

## Data Sources

These schemas were generated from analysis of:
- `duelyst_analysis/semantic/classes.tsv` - Class definitions
- `duelyst_analysis/semantic/inheritance.tsv` - Inheritance chains
- `duelyst_analysis/semantic/data_shapes.tsv` - Field definitions
- `duelyst_analysis/semantic/events.tsv` - Event emitters/listeners
- `duelyst_analysis/semantic/constants.tsv` - Game constants

## Usage

Each schema file contains:
1. **Source Evidence** - Where the entity is defined
2. **Fields** - All properties with types and sources
3. **Lifecycle Events** - Creation, destruction, modification events
4. **Dependencies** - Required and dependent entities
5. **Description** - Detailed explanation of the entity's purpose

## Statistics

- **Total Schemas**: 25
- **Core SDK Entities**: 16
- **Meta-Game Entities**: 3
- **Configuration Entities**: 5
- **Utility Entities**: 5
