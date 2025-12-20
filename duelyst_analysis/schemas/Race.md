# Race

## Source Evidence
- Primary class: `RaceFactory` (app/sdk/cards/raceFactory.coffee)
- Related classes: Cards, Modifier tribal effects
- Data shape: Unit tribes/types

## Fields
| Field | Type | Source | Required? |
|-------|------|--------|-----------|
| id | number | race identification | yes |
| name | string | display name | yes |
| devName | string | internal name | yes |

## Race/Tribe IDs
| ID | Name | Description |
|----|------|-------------|
| 0 | None | No tribe |
| 1 | Golem | Neutral/faction golems |
| 2 | Mech | Mech synergy units |
| 3 | Arcanyst | Spell synergy units |
| 4 | Dervish | Vetruvian tokens |
| 5 | Vespyr | Vanar vespyrs |
| 6 | Structure | Obelysks, walls |
| 7 | Pet | Battle pets |
| 8 | Wraithling | Abyssian tokens |
| 9 | General | Player generals |
| 10 | Warmaster | Wartech units |

## Tribal Synergies
| Race | Key Synergy Cards |
|------|------------------|
| Golem | Golem Metallurgist, Golem Vanquisher |
| Mech | Mechaz0r, Chassis of Mechaz0r |
| Arcanyst | Owlbeast Sage, Trinity Oath |
| Dervish | Dunecaster, Fireblaze Obelysk |
| Vespyr | Voice of the Wind, Glacial Elemental |
| Structure | Soulburn Obelysk, Bonechill Barrier |
| Pet | Battle Panddo, Gro |
| Wraithling | Bloodmoon Priestess, Wraithling Fury |

## Key Methods (RaceFactory)
- `raceForIdentifier(id)` - Get race by ID
- `getAllRaces()` - Get all races

## Modifier Interactions
| Modifier | Effect |
|----------|--------|
| ModifierSummonWatchByRace | Trigger on race summon |
| ModifierOpeningGambitByRaceId | Target race on play |
| ModifierBuffByRace | Buff specific race |

## Description
Race (or Tribe) is a unit classification that enables synergy effects. Units can belong to one or more races, and many cards have effects that specifically target or benefit units of certain races. Tribal synergy is a key deck-building consideration.
