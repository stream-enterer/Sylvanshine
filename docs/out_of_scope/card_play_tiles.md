# Card Play Tiles

**Status:** Out of Scope (Grid System v2)
**Prerequisite:** Card/summoning system
**Duelyst Reference:** `Player.js`, `TileCardSprite`, `TileSpawnSprite`

---

## Overview

Tile highlights for summoning units from cards. Shows valid placement locations with distinct visual style from movement/attack tiles.

## Duelyst Behavior

From forensic analysis:

- `TileCardSprite` — Card play location highlights
- `TileSpawnSprite` — Unit spawn locations
- Colors:
  - Player card: `#FFD900` (yellow)
  - Opponent card: `#D22846` (red)
- Z-order: `cardTileZOrder: 6` (above attack, below select)

## Assets Available

```
dist/resources/tiles/
    tile_card.png       — Card play highlight
    tile_spawn.png      — Spawn location highlight
```

## Sylvanshine Implementation

<!-- TODO: Design when card system exists -->

## Dependencies

- [ ] Card system designed
- [ ] Unit summoning mechanic
- [ ] Grid System Phase 2+ complete

## Open Questions

- Summoning range calculation (adjacent to general? anywhere?)
- Visual distinction from movement tiles
- Animation on valid placement hover?
