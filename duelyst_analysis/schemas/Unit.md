# Unit

## Source Evidence
- Primary class: `Unit` (app/sdk/entities/unit.coffee)
- Related classes: Entity, General (flag-based)
- Data shape: Minions and Generals on the board

## Fields
| Field | Type | Source | Required? |
|-------|------|--------|-----------|
| atk | number | classes.tsv | yes |
| hp | number | classes.tsv | yes |
| maxHP | number | classes.tsv | yes |
| speed | number | classes.tsv | no (default: 2) |
| reach | number | classes.tsv | no (default: 1) |
| isGeneral | boolean | classes.tsv | no |
| exhausted | boolean | classes.tsv | no |
| movesRemaining | number | classes.tsv | no |
| attacksRemaining | number | classes.tsv | no |
| isRanged | boolean | via Modifier | no |
| isFlying | boolean | via Modifier | no |
| hasProvoke | boolean | via Modifier | no |
| hasCelerity | boolean | via Modifier | no |
| hasRush | boolean | via Modifier | no |
| hasBlast | boolean | via Modifier | no |
| hasBackstab | boolean | via Modifier | no |
| hasFrenzy | boolean | via Modifier | no |
| hasRebirth | boolean | via Modifier | no |
| hasGrow | boolean | via Modifier | no |
| hasInfiltrate | boolean | via Modifier | no |

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
