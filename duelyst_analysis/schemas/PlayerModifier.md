# PlayerModifier

## Source Evidence
- Primary class: `PlayerModifier` (app/sdk/playerModifiers/playerModifier.coffee)
- Related classes: Modifier, GameSessionModifier
- Data shape: Player-level buff/effect system

## Fields
| Field | Type | Source | Required? |
|-------|------|--------|-----------|
| type | string | modifier type | yes |
| playerId | string | affected player | yes |
| modifiersContextObjects | array | sub-modifiers | no |
| manaModifier | number | mana cost change | no |
| cardDrawModifier | number | draw change | no |

## Lifecycle Events
- created: ApplyModifierAction to player
- destroyed: RemoveModifierAction, duration expires
- modified: Effect triggers, stacking

## Dependencies
- Requires: Modifier, Player, GameSession
- Used by: Spell effects, Card abilities

## PlayerModifier Types
```
PlayerModifier (Modifier)
├── PlayerModifierManaModifier - Modify spell costs
│   ├── PlayerModifierManaModifierNextCard
│   ├── PlayerModifierManaModifierOncePerTurn
│   └── PlayerModifierManaModifierSingleUse
├── PlayerModifierCardDrawModifier - Modify card draw
├── PlayerModifierBattlePetManager - Control battle pets
├── PlayerModifierSummonWatch - Track summons
│   ├── PlayerModifierSummonWatchApplyModifiers
│   └── PlayerModifierSummonWatchFromActionBar
├── PlayerModifierSpellWatch - Track spells
│   ├── PlayerModifierSpellWatchHollowVortex
│   └── PlayerModifierSpellWatchSpawnNeutralEntity
├── PlayerModifierEmblem - Quest emblem effects
│   ├── PlayerModifierEmblemEndTurnWatch
│   ├── PlayerModifierEmblemSummonWatch
│   └── PlayerModifierEmblemGainMinionOrLoseControlWatch
├── PlayerModifierChangeSignatureCard - Modify BBS
├── PlayerModifierCannotReplace - Disable replace
├── PlayerModifierTeamAlwaysBackstabbed - Vulnerability
└── GameSessionModifier - Global game effects
    └── GameSessionModifierFestiveSpirit - Event modifier
```

## Common Effects
| Effect | PlayerModifier |
|--------|----------------|
| Reduce spell costs | PlayerModifierManaModifier |
| Extra card draw | PlayerModifierCardDrawModifier |
| Track summons | PlayerModifierSummonWatch |
| Track spells | PlayerModifierSpellWatch |
| Change BBS | PlayerModifierChangeSignatureCard |
| Block replace | PlayerModifierCannotReplace |

## Key Methods
- `getPlayerId()` - Get affected player
- `getManaModifier()` - Get mana cost change
- `getCardDrawModifier()` - Get draw change
- `onAction(action)` - Respond to actions
- `onSummonWatch(action)` - Summon triggers
- `onSpellWatch(action)` - Spell triggers

## vs Entity Modifiers
| Aspect | PlayerModifier | Modifier |
|--------|---------------|----------|
| Target | Player | Entity |
| Scope | All player cards | Single card |
| Duration | Turn/permanent | Varies |
| Examples | Mana reduction | Stat buffs |

## Description
PlayerModifier extends the Modifier system to affect player-wide states rather than individual entities. They modify mana costs, card draw, track player actions, and implement quest effects (emblems). PlayerModifiers are key to many spell and ability effects.
