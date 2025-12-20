# Spell

## Source Evidence
- Primary class: `Spell` (app/sdk/spells/spell.coffee)
- Related classes: 200+ spell subclasses
- Data shape: One-time effect cards

## Fields
| Field | Type | Source | Required? |
|-------|------|--------|-----------|
| canTargetGeneral | boolean | spell config | no |
| targetType | string | spell config | no |
| targetModifiersContextObjects | array | spell config | no |
| radius | number | spell config | no |
| followups | array | spell config | no |
| drawCardsPostPlay | number | spell config | no |
| requiresFollowup | boolean | spell config | no |
| applyEffectPositions | array | runtime | no |
| spellType | string | spell classification | no |

## Lifecycle Events
- created: play_card_start
- destroyed: play_card_stop (immediate effect)
- modified: modify_action_for_execution

## Dependencies
- Requires: Card, Player, Board, Target Entity/Position
- Used by: GameSession, PlayCardAction, Modifier

## Spell Categories (from inheritance)
```
Spell (Card)
├── SpellDamage - Direct damage spells
│   ├── SpellCrimsonCoil
│   ├── SpellFrostburn
│   └── ... damage variants
├── SpellHeal - Healing spells
│   ├── SpellHealToFull
│   └── SpellHealYourGeneral
├── SpellApplyModifiers - Buff/debuff spells
│   ├── SpellApplyModifiersToGeneral
│   └── ... modifier applications
├── SpellSpawnEntity - Summon spells
│   ├── SpellShadowspawn
│   ├── SpellMindSteal
│   └── ... spawn variants
├── SpellKillTarget - Removal spells
│   ├── SpellMartyrdom
│   ├── SpellDecimate
│   └── ... kill variants
├── SpellTeleport - Movement spells
│   ├── SpellJuxtaposition
│   └── SpellMistWalking
├── SpellBounce - Return to hand spells
│   ├── SpellBounceToActionbar
│   └── variants
├── SpellIntensify - Scaling spells
│   ├── SpellIntensifyDealDamage
│   └── SpellIntensifyHealMyGeneral
├── SpellSilence - Dispel effects
│   └── SpellChromaticCold
└── SpellFollowup - Multi-step spells
    ├── SpellFollowupTeleport
    └── SpellFollowupDamage
```

## Targeting Types
| Type | Description |
|------|-------------|
| FRIENDLY | Only allied entities |
| ENEMY | Only enemy entities |
| MINION | Non-general units |
| GENERAL | Only generals |
| ALL | Any valid target |
| SELF | Caster only |
| EMPTY | Empty board spaces |
| OCCUPIED | Spaces with entities |

## Key Methods
- `_findApplyEffectPositions` - Determine valid targets
- `onApplyEffectToBoardTile` - Execute spell effect
- `onApplyOneEffectToBoard` - Single target effect
- `_postFilterPlayPositions` - Validate positions

## Description
Spells are one-shot effect cards that are played and immediately resolve. Unlike Entities, Spells don't persist on the board but instead trigger effects on targets or board positions. Many spells have followup effects that require additional targeting.
