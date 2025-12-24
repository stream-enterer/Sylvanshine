# Out of Scope Features

Boilerplate design documents for features deferred from current implementation phases. Each document captures:

- Duelyst forensic references
- Known behavior and assets
- Dependencies on other systems
- Open questions to resolve

## Grid System v2 Deferrals

| Feature | Prerequisite | Notes |
|---------|--------------|-------|
| [Selection Box Pulse](selection_box_pulse.md) | Phase 2 + UX design | Scale animation 0.85–1.0 |
| [Attack Path Arc](attack_path_arc.md) | Ranged units | Sine-wave projectile trajectory |
| [Move/Attack Seam](move_attack_seam.md) | Phase 5 corners | Boundary sprites between blobs |
| [Duelyst Color Scheme](duelyst_color_scheme.md) | Art direction | Yellow attack vs red attack |
| [Card Play Tiles](card_play_tiles.md) | Card system | Summoning location highlights |

## Adding New Deferrals

When deferring a feature:

1. Create `feature_name.md` in this directory
2. Add Status, Prerequisite, and Duelyst Reference header
3. Document known Duelyst behavior from forensic analysis
4. List dependencies and open questions
5. Update this README table
