# Sylvanshine KANBAN

## Done

### Tile Highlighting (2024-12-24)
- [x] Attack reticles render yellow (#FFD900) instead of white
- [x] Yellow blob disabled after movement (matches Duelyst `CONFIG.SHOW_MERGED_MOVE_ATTACK_TILES = false`)
- [x] Verified: player tile white is correct, yellow reticles only on currently-attackable enemies

---

## In Progress

(none)

---

## Backlog

### Investigation: Enemy Behavior Preview
**Priority:** High

Duelyst shows enemy intent before they act. Investigate:

1. **Red crosshair on enemy move target**
   - Does it animate/glide across tiles to destination?
   - Timing relative to AI decision vs actual move execution
   - Sprite used (`tile_attack.png`? `tile_opponent.png`?)

2. **Hover on enemy unit shows red move blob**
   - Color: `CONFIG.MOVE_OPPONENT_COLOR` vs `CONFIG.AGGRO_OPPONENT_COLOR`
   - Does it show movement range, attack range, or both?
   - Is attack range shown differently (separate color/layer)?

3. **AI timing integration**
   - When does enemy "telegraph" their move?
   - How long is preview shown before execution?
   - Is there a CONFIG value for this delay?

**Files to check:**
- `app/view/Player.js` — opponent tile handling
- `app/common/config.js` — timing/color constants
- `duelyst_analysis/flows/` — AI action flow

---

### Investigation: Bloom/Glow Shader on UI Elements
**Priority:** Medium

Mouse hover in Duelyst appears to leave a longer visual trail than Sylvanshine. Investigate:

1. **Is there a bloom/glow shader on hover tiles?**
   - Check `TileMapHoverSprite`, `TileGlowSprite`
   - Look for post-processing effects in shader pipeline

2. **Trail/persistence effect**
   - Is this shader-based or sprite animation?
   - Fade duration on hover tile removal

3. **Other UI elements with bloom**
   - Selection box pulsing
   - Path arrow glow
   - Attack reticle effects

**Files to check:**
- `app/view/nodes/map/TileGlowSprite.js`
- `app/view/nodes/map/TileMapHoverSprite.js`
- `duelyst_analysis/summaries/shaders.md`

---

### Investigation: Diagonal Blob Tile Connections
**Priority:** Low

Does Duelyst's merged tile blob system connect diagonally adjacent tiles?

1. **Corner sprite selection**
   - When two blob tiles share only a diagonal (not edge), do they connect?
   - Check `TileLayer.js` corner pattern logic

2. **Visual inspection needed**
   - Screenshot Duelyst with L-shaped or diagonal movement patterns
   - Compare to Sylvanshine rendering

3. **Seam sprite usage**
   - Are `_seam` variants used for diagonal-only adjacency?
   - Or do diagonals create visual gaps?

**Files to check:**
- `app/view/layers/game/TileLayer.js:213-269` — corner pattern algorithm
- `duelyst_analysis/flows/tile_interaction_flow.md` §7.4

---

## Reference

### Duelyst Color Constants
| Constant | Hex | Usage |
|----------|-----|-------|
| `CONFIG.MOVE_COLOR` | #F0F0F0 | Player movement blob |
| `CONFIG.MOVE_OPPONENT_COLOR` | #D22846 | Enemy movement blob |
| `CONFIG.AGGRO_COLOR` | #FFD900 | Player attack (yellow) |
| `CONFIG.AGGRO_OPPONENT_COLOR` | #D22846 | Enemy attack (red) |
