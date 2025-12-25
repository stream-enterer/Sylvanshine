# Modifier

## Source Evidence
- Primary class: `Modifier` (app/sdk/modifiers/modifier.coffee)
- Related classes: 400+ modifier subclasses
- Data shape: Buff/debuff/ability system

## Fields
| Field | Type | Required? | Description |
|-------|------|-----------|-------------|
| type | string | yes | Modifier class identifier |
| cardIndex | number | yes | Index of card this modifier is attached to |
| attributeBuffs | object | no | Stat modifications {atk, hp, etc.} |
| durationEndTurn | number | no | Turn when modifier expires (end of turn) |
| durationStartTurn | number | no | Turn when modifier expires (start of turn) |
| isAura | boolean | no | Whether this is an aura effect |
| isInherent | boolean | no | Built-in ability (part of card definition) |
| isAdditionalInherent | boolean | no | Card text ability (shown in UI) |
| stackType | string | no | SINGLE, DUPLICATE, or UNIQUE |
| maxStacks | number | no | Maximum stack count |
| appliedName | string | no | Display name when applied |
| appliedDescription | string | no | Tooltip text |

## Lifecycle Events
- created: ApplyModifierAction, onApply
- destroyed: RemoveModifierAction, onRemove, modifier_remove_aura
- modified: modifier_active_change, modifier_add_aura

## Dependencies
- Requires: Card/Entity (host), GameSession
- Used by: Entity, Player, Spell, Action

## Modifier Categories
```
Modifier (SDKObject)
├── PlayerModifier - Affects entire player
│   └── GameSessionModifier - Affects game state
├── ModifierKeyword - Keyword abilities
│   ├── ModifierRanged
│   ├── ModifierFlying
│   ├── ModifierProvoke
│   ├── ModifierTranscendance (Celerity)
│   ├── ModifierFirstBlood (Rush)
│   └── ...
├── ModifierWatch - Trigger modifiers
│   ├── ModifierDeathWatch
│   ├── ModifierDyingWish
│   ├── ModifierOpeningGambit
│   ├── ModifierSummonWatch
│   ├── ModifierSpellWatch
│   └── ...
├── ModifierBuff - Stat modifiers
│   ├── ModifierBuffSelf
│   ├── ModifierGrow
│   └── ...
├── ModifierAura - Area-of-effect
│   └── ModifierAuraType variants
├── ModifierArtifact - Equipment effects
└── ModifierToken - Token unit markers
```

## Stack Types
| Type | Behavior |
|------|----------|
| SINGLE | Only one instance allowed |
| DUPLICATE | Multiple instances stack |
| UNIQUE | Unique per source |

## Watch Types (Trigger Conditions)
| Watch Type | Trigger Condition |
|------------|-------------------|
| DeathWatch | Any unit dies |
| DyingWish | This unit dies |
| OpeningGambit | When played from hand |
| SummonWatch | When a unit is summoned |
| SpellWatch | When a spell is cast |
| AttackWatch | When an attack occurs |
| DamageWatch | When damage is dealt |
| HealWatch | When healing occurs |
| EndTurnWatch | At end of turn |
| StartTurnWatch | At start of turn |
| ReplaceWatch | When a card is replaced |

## Key Methods
- `onApply` - Called when modifier is applied
- `onRemove` - Called when modifier is removed
- `onAction` - Respond to game actions
- `onEvent` - Respond to game events
- `getBuffedAttribute` - Get modified stat value
- `getIsActive` - Check if modifier is active

## Description
Modifiers are the primary system for card abilities, buffs, debuffs, and triggered effects in Duelyst. They attach to cards (usually entities) and can modify stats, grant abilities, respond to game events, or create persistent effects. The modifier system is highly extensible with 400+ specific implementations.
