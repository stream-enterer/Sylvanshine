# Phase 7 Bug Fixes

Two bugs to fix in `src/main.cpp`. Execute Issue 6 first (visual), then Issue 5 (gameplay).

---

## Issue 6: Yellow Blob From Wrong Positions

Yellow attack blob invisible for melee units. Currently calculates from unit position only; should calculate from ALL reachable positions.

### Find (line ~1167):
```cpp
        // Calculate attack blob FIRST (needed for seam detection in both renderers)
        const auto& unit = state.units[state.selected_unit_idx];
        auto attack_blob = get_attack_pattern(unit_pos, unit.attack_range);

        // Remove enemy positions (they get reticles)
        std::erase_if(attack_blob, [&](const BoardPos& p) {
            return std::find(state.attackable_tiles.begin(), state.attackable_tiles.end(), p)
                   != state.attackable_tiles.end();
        });

        // Remove tiles overlapping with move blob
        std::erase_if(attack_blob, [&](const BoardPos& p) {
            return std::find(blob_tiles.begin(), blob_tiles.end(), p) != blob_tiles.end();
        });
```

### Replace with:
```cpp
        // Calculate attack blob from ALL move positions
        const auto& unit = state.units[state.selected_unit_idx];
        std::vector<BoardPos> attack_blob;

        for (const auto& from_pos : blob_tiles) {
            auto local_attack = get_attack_pattern(from_pos, unit.attack_range);
            for (const auto& p : local_attack) {
                if (std::find(attack_blob.begin(), attack_blob.end(), p) == attack_blob.end()) {
                    attack_blob.push_back(p);
                }
            }
        }

        // Remove tiles overlapping with move blob
        std::erase_if(attack_blob, [&](const BoardPos& p) {
            return std::find(blob_tiles.begin(), blob_tiles.end(), p) != blob_tiles.end();
        });

        // Remove enemy positions (they get reticles)
        std::erase_if(attack_blob, [&](const BoardPos& p) {
            return std::find(state.attackable_tiles.begin(), state.attackable_tiles.end(), p)
                   != state.attackable_tiles.end();
        });
```

---

## Issue 5: Move Ends Unit's Turn

Single `has_acted` bool blocks reselection after move. Need separate `has_moved`/`has_attacked` so player can move THEN attack.

### Change 5.1 — GameState struct (line 75)

Find:
```cpp
    std::vector<bool> has_acted;
```
Replace:
```cpp
    std::vector<bool> has_moved;
    std::vector<bool> has_attacked;
```

### Change 5.2 — reset_game (lines 327-328)

Find:
```cpp
    state.has_acted.clear();
    state.has_acted.resize(state.units.size(), false);
```
Replace:
```cpp
    state.has_moved.clear();
    state.has_moved.resize(state.units.size(), false);
    state.has_attacked.clear();
    state.has_attacked.resize(state.units.size(), false);
```

### Change 5.3 — all_units_acted (line 355)

Find:
```cpp
            if (i < state.has_acted.size() && !state.has_acted[i]) {
```
Replace:
```cpp
            if (i < state.has_attacked.size() && !state.has_attacked[i]) {
```

### Change 5.4 — find_next_ai_unit (line 478)

Find:
```cpp
        if (i < state.has_acted.size() && state.has_acted[i]) continue;
```
Replace:
```cpp
        if (i < state.has_attacked.size() && state.has_attacked[i]) continue;
```

### Change 5.5 — execute_ai_action (lines 487, 492, 496)

Find:
```cpp
    if (try_ai_attack(state, unit_idx)) {
        state.has_acted[unit_idx] = true;
        return;
    }

    if (try_ai_move(state, unit_idx, config)) {
        state.has_acted[unit_idx] = true;
        return;
    }

    state.has_acted[unit_idx] = true;
```
Replace:
```cpp
    if (try_ai_attack(state, unit_idx)) {
        state.has_attacked[unit_idx] = true;
        return;
    }

    if (try_ai_move(state, unit_idx, config)) {
        state.has_attacked[unit_idx] = true;
        return;
    }

    state.has_attacked[unit_idx] = true;
```

### Change 5.6 — Unit removal sync (lines 610-630)

Find the block starting with:
```cpp
    std::vector<bool> new_has_acted;
```

Replace entire block (through `state.has_acted = std::move(new_has_acted);`) with:
```cpp
    std::vector<bool> new_has_moved;
    std::vector<bool> new_has_attacked;
    for (size_t i = 0; i < state.units.size(); i++) {
        if (!state.units[i].death_complete) {
            new_has_moved.push_back(i < state.has_moved.size() ? state.has_moved[i] : false);
            new_has_attacked.push_back(i < state.has_attacked.size() ? state.has_attacked[i] : false);
        }
    }
    state.has_moved = std::move(new_has_moved);
    state.has_attacked = std::move(new_has_attacked);
```

### Change 5.7 — handle_select_click (line 693)

Find:
```cpp
    if (unit_idx < static_cast<int>(state.has_acted.size()) && state.has_acted[unit_idx]) return;
```
Replace:
```cpp
    if (unit_idx < static_cast<int>(state.has_attacked.size()) && state.has_attacked[unit_idx]) return;
```

### Change 5.8 — handle_move_click (lines 725-733)

Find:
```cpp
    if (unit_idx < static_cast<int>(state.has_acted.size())) {
        state.has_acted[unit_idx] = true;
    }

    state.selected_unit_idx = -1;
    state.reachable_tiles.clear();
    state.attackable_tiles.clear();

    state.units[unit_idx].start_move(config, clicked);
```
Replace:
```cpp
    if (unit_idx < static_cast<int>(state.has_moved.size())) {
        state.has_moved[unit_idx] = true;
    }

    // Keep selection — player can attack after move
    state.reachable_tiles.clear();
    state.movement_path.clear();

    state.units[unit_idx].start_move(config, clicked);
```

### Change 5.9 — handle_attack_click (lines 753-754)

Find:
```cpp
    if (attacker_idx < static_cast<int>(state.has_acted.size())) {
        state.has_acted[attacker_idx] = true;
    }
```
Replace:
```cpp
    if (attacker_idx < static_cast<int>(state.has_attacked.size())) {
        state.has_attacked[attacker_idx] = true;
    }
```

### Change 5.10 — handle_end_turn (line 822)

Find:
```cpp
    state.has_acted.clear();
```
Replace:
```cpp
    state.has_moved.clear();
    state.has_attacked.clear();
```

### Change 5.11 — Update attack range after move (line ~1007)

Find in update() function:
```cpp
    for (auto& unit : state.units) {
        unit.update(dt, config);
    }
```
Replace with:
```cpp
    for (size_t i = 0; i < state.units.size(); i++) {
        bool was_moving = state.units[i].is_moving();
        state.units[i].update(dt, config);

        // Update attackable tiles when selected unit finishes moving
        if (was_moving && !state.units[i].is_moving() &&
            state.selected_unit_idx == static_cast<int>(i)) {
            auto enemy_positions = get_enemy_positions(state, state.selected_unit_idx);
            state.attackable_tiles = get_attackable_tiles(
                state.units[i].board_pos,
                state.units[i].attack_range,
                enemy_positions);
        }
    }
```

### Change 5.12 — Deselect ends unit's turn (line ~771)

Find handle_selected_click:
```cpp
void handle_selected_click(GameState& state, BoardPos clicked, const RenderConfig& config) {
    int clicked_unit = find_unit_at_pos(state, clicked);

    if (clicked_unit >= 0 && state.units[clicked_unit].type != state.units[state.selected_unit_idx].type) {
        handle_attack_click(state, clicked);
    } else {
        handle_move_click(state, clicked, config);
    }
}
```
Replace with:
```cpp
void handle_selected_click(GameState& state, BoardPos clicked, const RenderConfig& config) {
    int clicked_unit = find_unit_at_pos(state, clicked);

    if (clicked_unit >= 0 && state.units[clicked_unit].type != state.units[state.selected_unit_idx].type) {
        handle_attack_click(state, clicked);
        return;
    }

    // Check if clicked tile is reachable
    bool is_reachable = false;
    for (const auto& tile : state.reachable_tiles) {
        if (tile == clicked) { is_reachable = true; break; }
    }

    if (is_reachable) {
        handle_move_click(state, clicked, config);
    } else {
        // Clicked non-reachable tile — deselect
        int idx = state.selected_unit_idx;
        if (idx >= 0 && idx < static_cast<int>(state.has_moved.size()) && state.has_moved[idx]) {
            // Unit moved this turn, end its turn
            if (idx < static_cast<int>(state.has_attacked.size())) {
                state.has_attacked[idx] = true;
            }
        }
        state.selected_unit_idx = -1;
        state.reachable_tiles.clear();
        state.attackable_tiles.clear();
        state.movement_path.clear();
    }
}
```

---

## Build & Test

```bash
./build.fish quick && ./build/tactics
```

### Checklist
- [ ] Yellow ring visible around white movement blob
- [ ] Move unit → stays selected, move blob disappears
- [ ] After move completes → reticles appear on adjacent enemies
- [ ] Attack enemy → selection clears, unit grayed
- [ ] Click empty tile after move → unit deselects, turn ends for that unit
- [ ] Space bar ends turn early
- [ ] AI still works
