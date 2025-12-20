# System: Cosmetics

**Location:** app/sdk/cosmetics/

## Purpose
The Cosmetics system manages visual customization items including emotes, card backs, profile icons, card skins, and battle maps that don't affect gameplay.

## Key Components
| Component | File | Purpose |
|-----------|------|---------|
| CosmeticsFactory | cosmetics/cosmeticsFactory.coffee | Cosmetic lookup |
| CosmeticsTypeLookup | cosmetics/cosmeticsTypeLookup.coffee | Type definitions |
| CosmeticsChestTypeLookup | cosmetics/cosmeticsChestTypeLookup.coffee | Chest types |
| Emotes | cosmetics/emotes*.coffee | Emote definitions |
| CardBacks | cosmetics/cardBacks*.coffee | Card back skins |
| ProfileIcons | cosmetics/profileIcons.coffee | Avatar icons |
| CardSkins | cosmetics/cardSkins*.coffee | Unit skins |
| BattleMaps | cosmetics/battleMaps*.coffee | Map skins |
| Scenes | cosmetics/scenes.coffee | Menu backgrounds |

## Data Flow
**Input:** Purchase, achievement unlock, crate opening
**Processing:** Inventory update → Selection → Display
**Output:** Visual customization applied

## Dependencies
**Requires:** Inventory system, Resources
**Used by:** UI, View system, Game display

## Cosmetic Types
| Type | Description | Examples |
|------|-------------|----------|
| Emotes | In-game expressions | Taunt, laugh, cry |
| CardBacks | Deck card backs | Faction themed |
| ProfileIcons | Player avatars | General portraits |
| CardSkins | Alternate unit art | Premium skins |
| BattleMaps | Battle backgrounds | Themed maps |
| Scenes | Menu backgrounds | Seasonal |

## Configuration
| Constant | Value | Purpose |
|----------|-------|---------|
| CONFIG.EMOTE_DELAY | 5 | Cooldown between emotes |
| CONFIG.EMOTE_DURATION | 3 | Emote display time |

## Resources
| Resource | Size | Purpose |
|----------|------|---------|
| resources/emotes/ | 26 dirs | Emote animations |
| resources/profile_icons/ | 7,498 bytes | Avatar images |
| resources/maps/ | 1,907 bytes | Battle backgrounds |
| resources/card_backgrounds/ | 4,219 bytes | Card frames |

## Emote Types
Each faction has themed emotes:
| Category | Examples |
|----------|----------|
| Greetings | Hello, Goodbye |
| Reactions | Thanks, Sorry |
| Taunts | Taunt, Frustration |
| Emotions | Happy, Sad, Angry |
| Special | Kiss, Sleep |

## Card Backs
| Type | Acquisition |
|------|-------------|
| Default | Free |
| Faction | Faction leveling |
| Seasonal | Event rewards |
| Promotional | Special offers |
| Achievement | Unlock requirements |

## Loot Crates
| Crate Type | Contents |
|------------|----------|
| Common | Basic cosmetics |
| Rare | Rare cosmetics |
| Epic | Epic cosmetics |
| Legendary | Legendary cosmetics |
| Boss | Boss crate rewards |

## Statistics
- **449 cosmetics** in instances/cosmetics.tsv
- Cosmetic breakdown:
  - Emotes: 100+
  - Card Backs: 50+
  - Profile Icons: 200+
  - Card Skins: 50+
  - Battle Maps: 11+
  - Scenes: 10+
- 26 emote animation sets
- Faction-specific cosmetics
- Seasonal/event cosmetics
