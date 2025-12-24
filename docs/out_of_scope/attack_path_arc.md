# Attack Path Arc Animation

**Status:** Out of Scope (Grid System v2)
**Prerequisite:** Ranged unit implementation
**Duelyst Reference:** `AttackPathSprite.js:78-115`, `Player.js:showDirectPath()`

---

## Overview

Animated projectile arc for ranged attack targeting. Shows trajectory from attacker to target with sine-wave motion and fade in/out at endpoints.

## Duelyst Behavior

From forensic analysis (`duelyst_analysis/summaries/grid_rendering.md`):

```
PATH_MOVE_DURATION = 1.5s      — Arc cycle time
PATH_FADE_DISTANCE = 40px      — Fade in/out distance
PATH_ARC_DISTANCE = 47.5px     — Arc height
```

- Speed modifier for short distances: `clamp((TILE_SIZE * 2) / distance, 1.0, 1.5)`
- Arc motion: `sin(arc_pct * PI)`
- Multiple sprites in array for multi-projectile effects

## Sylvanshine Implementation

<!-- TODO: Design when ranged units exist -->

## Dependencies

- [ ] Ranged unit type implemented
- [ ] Attack targeting system
- [ ] Grid System Phase 4 complete

## Open Questions

- Single sprite or particle system?
- How to handle multi-target attacks?
- Integration with FX system?
