# Cosmetics

## Source Evidence
- Primary class: `CosmeticsFactory` (app/sdk/cosmetics/cosmeticsFactory.coffee)
- Related classes: CosmeticsLookup, CosmeticsTypeLookup
- Data shape: Visual customization items

## Fields
| Field | Type | Source | Required? |
|-------|------|--------|-----------|
| id | number | cosmetic identification | yes |
| typeId | number | cosmetic category | yes |
| name | string | display name | yes |
| description | string | item description | no |
| rarity | number | rarity tier | yes |
| enabled | boolean | availability | yes |
| purchasable | boolean | can be bought | no |

## Cosmetic Types (from CosmeticsTypeLookup)
| Type | ID | Description |
|------|----|-------------|
| Emote | 1 | In-game emotes |
| CardBack | 2 | Card back designs |
| CardSkin | 3 | Alternate card art |
| ProfileIcon | 4 | Profile pictures |
| BattleMap | 5 | Battle map skins |

## Loot Crate System (from CosmeticsChestTypeLookup)
| Crate Type | Contents |
|------------|----------|
| Bronze | Common cosmetics |
| Silver | Rare cosmetics |
| Gold | Epic cosmetics |
| Boss | Boss-specific items |

## Emote Categories
| Category | Examples |
|----------|----------|
| Greetings | Hello, Good Game |
| Reactions | Happy, Sad, Angry |
| Taunts | Faction-specific |
| Special | Event/promotional |

## Key Methods (CosmeticsFactory)
- `cosmeticsForType(typeId)` - Get cosmetics by type
- `cosmeticForIdentifier(id)` - Get specific cosmetic
- `getCosmeticsTypeForId(id)` - Get type info
- `getIsAvailable(cosmetic)` - Check availability
- `getIsPurchasable(cosmetic)` - Check purchasability

## Acquisition Methods
| Method | Description |
|--------|-------------|
| Loot Crates | Random from crates |
| Achievement | Achievement rewards |
| Purchase | Direct purchase |
| Event | Limited-time events |
| Crafting | Spirit crafting |

## Collection Events
| Event | Trigger |
|-------|---------|
| cosmetics_collection_change | Collection updated |
| cosmetic_chest_collection_change | Crate inventory |
| cosmetic_chest_key_collection_change | Key inventory |

## Description
Cosmetics are visual customization items that don't affect gameplay. They include emotes, card backs, alternate card art (skins), profile icons, and battle maps. Cosmetics are obtained through loot crates, achievements, purchases, and special events.
