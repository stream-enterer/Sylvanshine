# Faction

## Source Evidence
- Primary class: `FactionFactory` (app/sdk/cards/factionsLookup.coffee)
- Related classes: Cards lookup, CardFactory
- Data shape: Game faction/class definitions

## Fields
| Field | Type | Source | Required? |
|-------|------|--------|-----------|
| id | number | faction identification | yes |
| name | string | display name | yes |
| devName | string | internal name | yes |
| generalIds | array | faction general IDs | yes |
| isNeutral | boolean | neutral faction flag | no |
| isPlayable | boolean | playable faction | yes |
| color | object | faction color | yes |

## Faction IDs
| ID | Name | Dev Name | Color Theme |
|----|------|----------|-------------|
| 1 | Lyonar Kingdoms | f1 | Gold/White |
| 2 | Songhai Empire | f2 | Red/Orange |
| 3 | Vetruvian Imperium | f3 | Yellow/Bronze |
| 4 | Abyssian Host | f4 | Purple/Black |
| 5 | Magmar Aspects | f5 | Green |
| 6 | Vanar Kindred | f6 | Blue/Ice |
| 100 | Neutral | neutral | Gray |
| 101 | Boss | boss | Special |

## Faction Generals
| Faction | Primary General | Alternate Generals |
|---------|-----------------|-------------------|
| Lyonar | Argeon Highmayne | Zir'An Sunforge, Brome Warcrest |
| Songhai | Kaleos Xaan | Reva Eventide, Shidai Stormblossom |
| Vetruvian | Zirix Starstrider | Sajj Vetruvian, Ciphyron Ascendant |
| Abyssian | Lilithe Blightchaser | Cassyva Soulreaper, Maehv Skinsolder |
| Magmar | Vaath the Immortal | Starhorn the Seeker, Ragnora the Relentless |
| Vanar | Faie Bloodwing | Kara Winterblade, Ilena Cryobyte |

## Faction Mechanics
| Faction | Signature Mechanic |
|---------|-------------------|
| Lyonar | Heal, Zeal, Provoke |
| Songhai | Backstab, Spells, Ranged |
| Vetruvian | Dervishes, Obelysks, Blast |
| Abyssian | Shadow Creep, Wraithlings, Deathwatch |
| Magmar | Eggs, Grow, Rebirth |
| Vanar | Walls, Infiltrate, Stun |

## Key Methods (FactionFactory)
- `factionForIdentifier(id)` - Get faction by ID
- `getAllFactions()` - Get all factions
- `getAllPlayableFactions()` - Get playable factions
- `factionIdForGeneralId(generalId)` - Get faction from general

## Description
Factions are the playable classes in Duelyst, each with unique generals, cards, and mechanical identity. Players choose a faction when building a deck, which determines available cards and playstyle. Neutral cards can be used by any faction.
