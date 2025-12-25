# System: Factions

**Location:** app/sdk/cards/factionsLookup.coffee, app/sdk/cards/factionFactory.coffee

## Purpose
The Faction system defines the six playable factions plus neutral, each with unique generals, cards, visual identity, and playstyle themes.

## Key Components
| Component | File | Purpose |
|-----------|------|---------|
| Factions | cards/factionsLookup.coffee | Faction ID constants |
| FactionFactory | cards/factionFactory.coffee | Faction data access |
| CardFactory_*_Faction* | cards/factory/*/faction*.coffee | Faction card definitions |

## Data Flow
**Input:** Deck building, matchmaking, card factory
**Processing:** Faction filtering → Card pool → Visual theming
**Output:** Faction-restricted cards, themed visuals

## Dependencies
**Requires:** Card system, Resources
**Used by:** Deck building, matchmaking, card factory

## Faction Definitions
| ID | Name | Dev Name | Theme |
|----|------|----------|-------|
| 1 | Lyonar Kingdoms | lyonar | Holy warriors, healing, provoke |
| 2 | Songhai Empire | songhai | Assassins, backstab, movement |
| 3 | Vetruvian Imperium | vetruvian | Desert, dervishes, artifacts |
| 4 | Abyssian Host | abyssian | Demons, shadow creep, sacrifice |
| 5 | Magmar Aspects | magmar | Dinosaurs, grow, self-damage |
| 6 | Vanar Kindred | vanar | Ice, infiltrate, walls |
| 100 | Neutral | neutral | Shared card pool |
| 200 | Tutorial | tutorial | Training opponents |
| 300 | Boss | bosses | Boss encounters |

## Faction Colors
| Faction | White Gradient | Black Gradient |
|---------|----------------|----------------|
| Lyonar | rgb(250,200,80) | rgb(40,33,4) |
| Songhai | rgb(254,80,100) | rgb(70,5,1) |
| Vetruvian | rgb(250,160,0) | rgb(39,33,21) |
| Abyssian | rgb(247,151,254) | rgb(45,50,167) |
| Magmar | rgb(0,252,250) | rgb(0,62,66) |
| Vanar | rgb(185,208,226) | rgb(9,12,55) |

## Generals Per Faction
Each playable faction has 3 generals:
| Faction | General 1 | General 2 | General 3 |
|---------|-----------|-----------|-----------|
| Lyonar | Argeon Highmayne | Ziran Sunforge | Brome Warcrest |
| Songhai | Kaleos Xaan | Reva Eventide | Shidai Stormblossom |
| Vetruvian | Zirix Starstrider | Sajj Bloodtear | Ciphyron Ascendant |
| Abyssian | Lilithe Blightchaser | Cassyva Soulreaper | Maehv Skinsolder |
| Magmar | Vaath the Immortal | Starhorn the Seeker | Ragnora the Relentless |
| Vanar | Kara Winterblade | Faie Bloodwing | Ilena Cryobyte |

## Configuration
| Constant | Purpose |
|----------|---------|
| FX.Factions.Faction1 | Lyonar effects |
| FX.Factions.Faction2 | Songhai effects |
| FX.Factions.Faction3 | Vetruvian effects |
| FX.Factions.Faction4 | Abyssian effects |
| FX.Factions.Faction5 | Magmar effects |
| FX.Factions.Faction6 | Vanar effects |
| FX.Factions.Neutral | Shared effects |

## Resources Per Faction
| Resource | Pattern | Purpose |
|----------|---------|---------|
| Crest | RSX.crest_f{n} | Faction emblem |
| FX | fx_f{n}_* | Spell effects |
| SFX | sfx_f{n}_* | Sound effects |
| Units | resources/units/f{n}_* | Unit sprites |

## Starter Decks
Each faction has a predefined starter deck with 40 cards total (including general).

## Faction Keywords
| Faction | Signature Mechanics |
|---------|---------------------|
| Lyonar | Zeal, Provoke, Healing |
| Songhai | Backstab, Ranged, Movement |
| Vetruvian | Blast, Dervish, Obelysks |
| Abyssian | Deathwatch, Dying Wish, Creep |
| Magmar | Grow, Rebirth, Self-damage |
| Vanar | Infiltrate, Walls, Stunned |

## TSV Data
See `instances/factions.tsv` for faction definitions and `instances/cards.tsv` for card counts by faction.
