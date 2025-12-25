# Unit

## Source Evidence
- Primary class: `Unit` (app/sdk/entities/unit.coffee)
- Related classes: Entity, General (flag-based)
- Data shape: Minions and Generals on the board

## Fields
| Field | Type | Required? |
|-------|------|-----------|
| atk | number | yes |
| hp | number | yes |
| maxHP | number | yes |
| speed | number | no (default: 2) |
| reach | number | no (default: 1) |
| isGeneral | boolean | no |
| exhausted | boolean | no |
| movesRemaining | number | no |
| attacksRemaining | number | no |

Defined in: `app/sdk/entities/unit.coffee`

## Computed Properties
These are derived from modifiers, not stored fields:
- `isRanged` - has ModifierRanged
- `isFlying` - has ModifierFlying
- `hasProvoke` - has ModifierProvoke
- `hasCelerity` - has ModifierTranscendance
- etc.

See `summaries/modifiers.md` for full keyword list.

## Lifecycle Events
- created: PlayCardAction, onApplyToBoard, summon
- destroyed: DieAction, deathrattle triggers
- modified: AttackAction, DamageAction, HealAction, buff/debuff application

## Dependencies
- Requires: Entity, Player, Board, Modifier
- Used by: GameSession, Action, AI/Agents

## Unit Subtypes
- **General**: Units with isGeneral=true (faction leaders)
- **Minion**: Regular units that can be summoned
- **Battle Pet**: Units controlled by AI modifier
- **Structure/Wall**: Units that cannot move (via modifier)
- **Token**: Units spawned by effects (Wraithling, Dervish, etc.)

## Keyword Abilities (via Modifiers)
| Keyword | Modifier Class | Effect |
|---------|---------------|--------|
| Ranged | ModifierRanged | Can attack from distance |
| Flying | ModifierFlying | Ignores movement restrictions |
| Provoke | ModifierProvoke | Enemies must attack this unit |
| Celerity | ModifierTranscendance | Double actions per turn |
| Rush | ModifierFirstBlood | Can act immediately when summoned |
| Blast | ModifierBlastAttack | Attacks in a line |
| Backstab | ModifierBackstab | Bonus damage from behind |
| Frenzy | ModifierFrenzy | Attacks all adjacent enemies |
| Rebirth | ModifierRebirth | Spawns egg on death |
| Grow | ModifierGrow | Gains stats each turn |
| Infiltrate | ModifierInfiltrate | Bonus on enemy side |
| Deathwatch | ModifierDeathWatch | Triggers on any death |
| Dying Wish | ModifierDyingWish | Triggers on own death |
| Opening Gambit | ModifierOpeningGambit | Triggers when played |
| Forcefield | ModifierForcefield | Prevents first damage |

## Description
Unit represents minions and generals that exist on the game board. Units are the primary interactive elements of Duelyst gameplay - they move, attack, and have various abilities through modifiers. Generals are special units that represent the player and determine win/loss conditions.
