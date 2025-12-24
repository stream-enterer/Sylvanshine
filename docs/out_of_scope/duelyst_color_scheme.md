# Duelyst Color Scheme

**Status:** Out of Scope (Grid System v2)
**Prerequisite:** Art direction decision
**Duelyst Reference:** `config.js:824-843`

---

## Overview

Switch tile highlight colors from current Sylvanshine scheme (white move, red attack) to original Duelyst scheme (gray move, yellow attack).

## Color Comparison

| Tile Type | Sylvanshine Current | Duelyst Original |
|-----------|---------------------|------------------|
| Move | `#FFFFFF` (white) | `#F0F0F0` (light gray) |
| Move Alt | — | `#FFFFFF` (white) |
| Attack/Aggro | `#FF6464` (red) | `#FFD900` (yellow) |
| Aggro Alt | — | `#FFFFFF` (white) |
| Aggro Opponent | — | `#D22846` (red) |
| Select | — | `#FFFFFF` (white) |
| Select Opponent | — | `#D22846` (red) |
| Hover | `#FFFFFF` (white) | `#FFFFFF` (white) |
| Card Player | — | `#FFD900` (yellow) |
| Card Opponent | — | `#D22846` (red) |

## Impact Assessment

- **Visual identity change:** Significant — attack tiles change from red to yellow
- **User expectations:** Red typically means "danger/attack" in games
- **Colorblind accessibility:** Yellow may be more visible than red for some users

## Sylvanshine Implementation

Constants already defined in `grid_renderer.hpp`:
```cpp
namespace TileColor {
    // Duelyst colors (for future use)
    constexpr SDL_FColor MOVE_DUELYST   = {0.941f, 0.941f, 0.941f, 1.0f};
    constexpr SDL_FColor AGGRO_DUELYST  = {1.0f, 0.851f, 0.0f, 1.0f};
}
```

## Dependencies

- [ ] Art direction decision made
- [ ] User testing feedback on color preferences

## Open Questions

- Keep red for "danger" or use yellow for Duelyst authenticity?
- Add user preference toggle?
- Different colors for player vs opponent tiles?
