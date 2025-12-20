# Rarity

## Source Evidence
- Primary class: `RarityFactory` (app/sdk/cards/rarityFactory.coffee)
- Related classes: Cards, CardFactory
- Data shape: Card rarity tiers

## Fields
| Field | Type | Source | Required? |
|-------|------|--------|-----------|
| id | number | rarity identification | yes |
| name | string | display name | yes |
| devName | string | internal name | yes |
| spiritCost | number | crafting cost | yes |
| spiritReward | number | disenchant value | yes |
| spiritCostPrismatic | number | prismatic craft cost | yes |
| spiritRewardPrismatic | number | prismatic disenchant | yes |
| isCraftable | boolean | can be crafted | yes |

## Rarity Tiers
| ID | Name | Spirit Cost | Disenchant | Color |
|----|------|-------------|------------|-------|
| 0 | Basic | 0 | 0 | White |
| 1 | Common | 40 | 10 | White |
| 2 | Rare | 100 | 25 | Blue |
| 3 | Epic | 350 | 100 | Purple |
| 4 | Legendary | 900 | 350 | Orange |
| 5 | Token | 0 | 0 | N/A |
| 6 | Mythron | 900 | 350 | Red |

## Prismatic Values
| Rarity | Craft Cost | Disenchant |
|--------|------------|------------|
| Common | 160 | 40 |
| Rare | 400 | 100 |
| Epic | 1400 | 400 |
| Legendary | 3600 | 1400 |

## Deck Building Rules
| Rarity | Max Copies |
|--------|------------|
| Basic | 3 |
| Common | 3 |
| Rare | 3 |
| Epic | 3 |
| Legendary | 3 |
| Token | N/A (not collectible) |

## Key Methods (RarityFactory)
- `rarityForIdentifier(id)` - Get rarity by ID
- `getAllRarities()` - Get all rarities
- `getIsRarityTypeCraftable(id)` - Check if craftable

## Card Drop Rates (Spirit Orbs)
- Common: ~75%
- Rare: ~20%
- Epic: ~4%
- Legendary: ~1%
- Prismatic chance: ~10% per card

## Description
Rarity determines a card's crafting cost, disenchant value, and relative power level. Higher rarity cards are generally more complex and impactful. Prismatic versions are cosmetic variants with enhanced visuals.
