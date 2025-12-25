# System: Modifiers

**Location:** app/sdk/modifiers/, app/sdk/playerModifiers/, app/sdk/gameSessionModifiers/

## Purpose
The Modifier system implements all card abilities, buffs, debuffs, and keywords through a flexible pattern that can intercept actions, grant stats, and trigger effects.

## Key Components
| Component | File | Purpose |
|-----------|------|---------|
| Modifier | modifiers/modifier.coffee | Base class for all modifiers |
| PlayerModifier | playerModifiers/playerModifier.coffee | Player-level effects |
| GameSessionModifier | gameSessionModifiers/gameSessionModifier.coffee | Global game effects |
| ModifierKeyword* | modifiers/modifierKeyword*.coffee | Keyword abilities |
| ModifierWatch* | modifiers/modifierWatch*.coffee | Trigger-based abilities |
| ModifierAura* | modifiers/modifierAura*.coffee | Area effect modifiers |

## Data Flow
**Input:** Action events, game state changes, card applications
**Processing:** Event interception → Condition checking → Effect execution
**Output:** Modified actions, triggered sub-actions, stat changes

## Dependencies
**Requires:** GameSession, Card system, Action system
**Used by:** All cards, game rules, visual effects

## Modifier Categories
| Category | Pattern | Examples |
|----------|---------|----------|
| Keywords | ModifierKeyword* | Airdrop, Backstab, Blast, Celerity |
| Watches | Modifier*Watch | DeathWatch, SummonWatch, DamageWatch |
| Auras | ModifierAura* | Adjacent buffs, global effects |
| Stat Buffs | ModifierBuff* | +ATK, +HP, stat modifications |
| Opening Gambits | ModifierOpeningGambit* | On-play effects |
| Dying Wishes | ModifierDyingWish* | On-death effects |

## Configuration
| Constant | Value | Purpose |
|----------|-------|---------|
| CONFIG.CARD_MODIFIER_ACTIVE_OPACITY | 255.0 | Active modifier display |
| CONFIG.CARD_MODIFIER_INACTIVE_OPACITY | 100.0 | Inactive modifier display |
| CONFIG.CARD_MODIFIER_PADDING_* | 8.0/5.0 | Modifier icon spacing |

## Resources
| Resource | Purpose |
|----------|---------|
| resources/modifiers/ | Modifier icons |
| FloatingShield | Forcefield visual |
| ForceField | Shield effect |
| fxBuffSimpleGold | Buff glow effect |

## Keywords (is_keyword=TRUE)
| Keyword | Description |
|---------|-------------|
| Airdrop | Can be summoned anywhere |
| Backstab | Bonus damage from behind |
| Blast | Attacks hit entire row |
| Celerity | Can move/attack twice |
| Deathwatch | Triggers on any death |
| Dispel | Remove all modifiers |
| Dying Wish | Triggers on own death |
| Flying | Move anywhere on board |
| Frenzy | Attacks all adjacent enemies |
| Grow | Stats increase each turn |
| Infiltrate | Bonus on enemy side |
| Opening Gambit | Triggers on play |
| Provoke | Enemies must attack this |
| Ranged | Attack from any distance |
| Rebirth | Resummon as egg on death |
| Rush | No summoning sickness |
| Structure | Cannot move or attack |
| Zeal | Bonus near own general |

## TSV Data

See `instances/modifiers.tsv` for all modifier definitions.

### Column Classification
| Column | Values | Meaning |
|--------|--------|---------|
| `is_keyword` | TRUE/FALSE | Keywords shown on cards (Provoke, Flying, etc.) |
| `is_watch` | TRUE/FALSE | Event-triggered abilities (DeathWatch, SummonWatch, etc.) |
| `stack_type` | e.g. `maxStacks=1` | Stacking behavior and limits |
| `triggers` | e.g. `onBeforeAction` | When the modifier activates |

## Event Hooks
| Hook | Purpose |
|------|---------|
| onValidateAction | Prevent/allow actions |
| onBeforeAction | Modify actions pre-execution |
| onAfterAction | React to completed actions |
| onStartTurn | Turn start triggers |
| onEndTurn | Turn end triggers |
| onApplyToCard | Applied to entity |
| onRemoveFromCard | Removed from entity |
| onActivated | Modifier activates |
| onDeactivated | Modifier deactivates |

## Watch Types
| Watch | Trigger |
|-------|---------|
| DeathWatch | Any unit dies |
| SummonWatch | Unit summoned |
| DamageWatch | Unit takes damage |
| HealWatch | Unit heals |
| SpellWatch | Spell cast |
| AttackWatch | Attack made |
| DrawCardWatch | Card drawn |
| ReplaceWatch | Card replaced |
