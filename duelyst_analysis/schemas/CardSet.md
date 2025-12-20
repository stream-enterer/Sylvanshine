# CardSet

## Source Evidence
- Primary class: `CardSetFactory` (app/sdk/cards/cardSetFactory.coffee)
- Related classes: CardSet lookup, CardFactory
- Data shape: Card expansion/set definitions

## Fields
| Field | Type | Source | Required? |
|-------|------|--------|-----------|
| id | number | set identification | yes |
| name | string | display name | yes |
| devName | string | internal name | yes |
| releaseDate | date | launch date | no |
| isEnabled | boolean | availability | yes |

## Card Sets (from CardSet lookup)
| ID | Name | Dev Name | Description |
|----|------|----------|-------------|
| 0 | Gauntlet Special | gauntlet | Gauntlet-only cards |
| 1 | Core | core | Base game cards |
| 2 | Shimzar | shimzar | Rise of the Bloodborn |
| 3 | Bloodborn | bloodborn | Denizens of Shim'Zar |
| 4 | Unity | unity | Ancient Bonds |
| 5 | First Watch | firstWatch | Unearthed Prophecy |
| 6 | Wartech | wartech | Immortal Vanguard |
| 7 | Combined Unlockables | combinedUnlockables | Combined content |
| 8 | Coreshatter | coreshatter | Trials of Mythron |

## Key Methods (CardSetFactory)
- `cardSetForIdentifier(id)` - Get set by ID
- `cardSetForDevName(name)` - Get set by dev name
- `getAllCardSets()` - Get all sets
- `getEnabledCardSets()` - Get available sets

## Set Release Order
1. Core (Launch)
2. Rise of the Bloodborn
3. Denizens of Shim'Zar
4. Ancient Bonds
5. Unearthed Prophecy
6. Immortal Vanguard
7. Trials of Mythron

## Format Implications
| Format | Included Sets |
|--------|--------------|
| Standard | Recent sets |
| Legacy | All sets |
| Gauntlet | Core + selected |

## Collection Integration
- Cards organized by set in collection
- Spirit orbs contain set-specific cards
- Set completion tracking

## Description
CardSet defines expansion sets that group related cards together. Each set has a theme, release date, and determines which spirit orbs contain those cards. Sets affect format legality and collection organization.
