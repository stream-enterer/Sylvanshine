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

## Usage

Each schema file documents:
1. **Source Evidence** - Where the entity is defined in source code
2. **Fields** - Properties with types and whether required
3. **Lifecycle Events** - Creation, destruction, modification triggers
4. **Dependencies** - Required and dependent entities
5. **Description** - Entity's purpose and behavior

## Cross-References

Schemas reference TSV data in `instances/`:
- `cards.tsv` - All card definitions with faction, type, stats
- `units.tsv` - Unit stats, abilities (pipe-separated), sprites
- `modifiers.tsv` - Modifier types with is_keyword, is_watch flags
- `actions.tsv` - Action type hierarchy and execution order
