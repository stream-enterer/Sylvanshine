# Move/Attack Seam Sprites

**Status:** Out of Scope (Grid System v2)
**Prerequisite:** Grid System Phase 5 (Merged Tile Corners)
**Duelyst Reference:** `TileLayer.js:252-265`

---

## Overview

Special corner sprites that render at the boundary where movement range blob meets attack range blob, creating a visible seam line.

## Duelyst Behavior

From forensic analysis:

- Seam detection: when corner has no same-region neighbors but has different-region neighbor
- Uses `tile_merged_large_0_seam.png` sprite variant
- Creates visible boundary between movement (white) and attack (yellow) ranges

## Assets Available

```
dist/resources/tiles/
    tile_merged_large_0_seam.png
    tile_merged_large_0_seam@2x.png
    tile_merged_hover_0_seam.png
    tile_merged_hover_0_seam@2x.png
```

## Sylvanshine Implementation

<!-- TODO: Design after Phase 5 merged corners work -->

## Dependencies

- [ ] Grid System Phase 5 complete (merged tile corners)
- [ ] Both move and attack blobs rendering simultaneously

## Open Questions

- Seam color: inherit from move or attack blob?
- Seam detection algorithm complexity vs. visual payoff?
