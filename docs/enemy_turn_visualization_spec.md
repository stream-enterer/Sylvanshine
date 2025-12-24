# Enemy Turn Visualization Specification

**Status:** Sylvanshine Original Feature (not from Duelyst)

> Duelyst's bot games have **zero visual feedback** during AI turns. Actions just "happen" without any preview or indication. This document specifies a novel system for Sylvanshine to provide enemy turn visualization.

---

## 1. Why This Feature

### 1.1 The Gap in Duelyst

**Source:** `networkManager.coffee:258-262`

```coffeescript
broadcastGameEvent: (eventData) ->
  # don't broadcast anything if running locally
  if gameSession.getIsRunningAsAuthoritative()
    return false
```

In single-player/bot games:
- No `network_game_hover` events
- No `network_game_select` events
- No visual feedback during AI evaluation
- Actions execute instantly without preview

### 1.2 Why Sylvanshine Should Add This

| Benefit | Description |
|---------|-------------|
| **Readability** | Player can follow enemy decisions |
| **Learning** | Helps player understand AI priorities |
| **Polish** | Adds tactical game "weight" to enemy turns |
| **Tension** | Brief pauses before attacks build anticipation |

---

## 2. Duelyst Assets Available

These assets exist but are unused in bot games:

### 2.1 Tile Sprites (reusable)

| Asset | File | Usage |
|-------|------|-------|
| Movement blob | `tile_merged_*.png` | Enemy movement range |
| Attack blob | `tile_large.png` | Enemy attack range |
| Selection box | `tile_box.png` | Selected enemy unit |
| Target reticle | `tile_attack.png` | Attack target |
| Path arrows | `tile_path_*.png` | Movement/attack path |
| Glow tile | `tile_glow.png` | Movement destination |

### 2.2 Colors (from config.js)

| Constant | Hex | Usage |
|----------|-----|-------|
| `SELECT_OPPONENT_COLOR` | #D22846 | Enemy selection box |
| `MOVE_OPPONENT_COLOR` | #F0F0F0 | Enemy movement blob |
| `AGGRO_OPPONENT_COLOR` | #D22846 | Enemy attack blob |
| `AGGRO_OPPONENT_ALT_COLOR` | #82192D | Enemy path/target |

### 2.3 UI Elements

| Asset | File | Usage |
|-------|------|-------|
| Enemy turn banner | `notification_enemy_turn` | Turn start indicator |
| Selection glow | Entity shader | Highlight selected enemy |

---

## 3. Proposed System

### 3.1 Turn Structure

```
┌─────────────────────────────────────────────────────────────────┐
│                      ENEMY TURN FLOW                            │
├─────────────────────────────────────────────────────────────────┤
│                                                                 │
│  1. TURN START                                                  │
│     └─ Show "Enemy Turn" banner                                 │
│     └─ Brief pause (0.5s)                                       │
│                                                                 │
│  2. FOR EACH ENEMY (in AI priority order):                      │
│     ┌─ SELECTION PHASE (0.3s)                                   │
│     │  └─ Highlight current enemy (selection box)               │
│     │  └─ Hide ownership indicator (like player hover)          │
│     │                                                           │
│     ├─ EVALUATION PHASE (0.2-0.5s, optional)                    │
│     │  └─ Show movement range (dimmed)                          │
│     │  └─ Show attack range (dimmed)                            │
│     │  └─ Pulse potential targets                               │
│     │                                                           │
│     ├─ DECISION PHASE (0.2s)                                    │
│     │  └─ Show chosen movement destination (bright)             │
│     │  └─ Show path to destination                              │
│     │  └─ Show attack target (if any)                           │
│     │                                                           │
│     ├─ EXECUTE MOVE (animation duration)                        │
│     │  └─ Movement animation plays                              │
│     │  └─ Path fades out                                        │
│     │                                                           │
│     └─ EXECUTE ATTACK (animation duration)                      │
│        └─ Attack animation plays                                │
│        └─ Damage numbers appear                                 │
│                                                                 │
│  3. TURN END                                                    │
│     └─ Brief pause (0.3s)                                       │
│     └─ Transition to player turn                                │
│                                                                 │
└─────────────────────────────────────────────────────────────────┘
```

### 3.2 Timing Configuration

```cpp
struct EnemyTurnConfig {
    // Phase durations (seconds)
    float turn_start_delay = 0.5f;
    float selection_duration = 0.3f;
    float evaluation_duration = 0.3f;  // 0 to disable
    float decision_duration = 0.2f;
    float turn_end_delay = 0.3f;

    // Feature toggles
    bool show_evaluation_phase = true;
    bool show_movement_preview = true;
    bool show_attack_preview = true;
    bool show_path_lines = true;

    // Speed multiplier (applied from settings)
    float speed_multiplier = 1.0f;
};
```

### 3.3 Visual States

```cpp
enum class EnemyTurnPhase {
    Idle,           // No visualization
    TurnStart,      // Banner showing
    Selecting,      // Enemy highlighted
    Evaluating,     // Ranges shown (dimmed)
    Deciding,       // Chosen action shown (bright)
    Moving,         // Movement executing
    Attacking,      // Attack executing
    TurnEnd         // Transitioning out
};
```

---

## 4. Implementation

### 4.1 Integration Point

**Current flow:**
```cpp
void process_enemy_turn() {
    for (auto& enemy : enemies) {
        auto action = ai.choose_action(enemy);
        execute_action(action);  // Immediate
    }
}
```

**With visualization:**
```cpp
void process_enemy_turn() {
    show_turn_banner();
    wait(config.turn_start_delay);

    for (auto& enemy : enemies) {
        // Selection phase
        show_selection_box(enemy);
        wait(config.selection_duration);

        auto action = ai.choose_action(enemy);

        // Preview phase
        if (config.show_movement_preview && action.has_move) {
            show_movement_preview(enemy, action.move_dest);
        }
        if (config.show_attack_preview && action.has_attack) {
            show_attack_preview(enemy, action.attack_target);
        }
        wait(config.decision_duration);

        // Execute with animations
        execute_action_with_animations(action);

        hide_all_previews();
    }

    wait(config.turn_end_delay);
}
```

### 4.2 Reusing Existing Renderer

The `grid_renderer` already has the necessary functions:

| Function | Current Use | Enemy Turn Use |
|----------|-------------|----------------|
| `render_enemy_indicator()` | Hover feedback | Selection box (adapt) |
| `render_movement_highlight()` | Player movement | Enemy movement (red color) |
| `render_attack_highlight()` | Player attack | Enemy attack (already red) |
| `render_path()` | Player path | Enemy path (red color) |

### 4.3 Color Adaptation

```cpp
// Add to grid_renderer
void render_enemy_turn_movement(const GridConfig& config,
                                 const std::vector<Vec2i>& tiles) {
    render_movement_highlight(config, tiles, MOVE_OPPONENT_COLOR);
}

void render_enemy_turn_attack(const GridConfig& config,
                               const std::vector<Vec2i>& tiles) {
    render_attack_highlight(config, tiles, AGGRO_OPPONENT_COLOR);
}

void render_enemy_selection(const GridConfig& config, Vec2i pos) {
    render_selection_box(config, pos, SELECT_OPPONENT_COLOR);
}
```

---

## 5. Settings Integration

### 5.1 User Options

```
┌─────────────────────────────────────────┐
│           ENEMY TURN SETTINGS           │
├─────────────────────────────────────────┤
│ Enemy Turn Visualization: [ON] / OFF    │
│                                         │
│ Show Movement Preview:    [ON] / OFF    │
│ Show Attack Preview:      [ON] / OFF    │
│ Show Path Lines:          [ON] / OFF    │
│                                         │
│ Enemy Animation Speed:    [1x] 2x 3x    │
└─────────────────────────────────────────┘
```

### 5.2 Speed Modes

| Mode | Delay Multiplier | Description |
|------|------------------|-------------|
| Normal (1x) | 1.0 | Full visualization |
| Fast (2x) | 0.5 | Half delays |
| Turbo (3x) | 0.33 | Minimal delays |
| Instant | 0.0 | No delays (Duelyst-like) |

---

## 6. Comparison: Before/After

### 6.1 Without Feature (Current Duelyst Behavior)

```
Enemy Turn Starts
  → Enemy 1 instantly moves
  → Enemy 1 instantly attacks
  → Enemy 2 instantly moves
  → Enemy 2 instantly attacks
Player Turn Starts
```

### 6.2 With Feature (Sylvanshine)

```
Enemy Turn Starts
  → "Enemy Turn" banner (0.5s)
  → Enemy 1 highlighted with red selection box
  → Enemy 1's movement range shown (dimmed red)
  → Enemy 1's attack targets pulsing
  → Path appears to chosen destination
  → Enemy 1 moves (animation)
  → Attack line to target
  → Enemy 1 attacks (animation)
  → Repeat for Enemy 2...
Player Turn Starts
```

---

## 7. Future Extensions

### 7.1 Threat Indicators

Show which player units are in danger:
- Highlight tiles enemies can reach + attack
- Dim player units that are safe
- Pulse player units at risk

### 7.2 AI Intent Preview

Before enemy turn executes, show:
- "This enemy will attack X"
- "This enemy will move toward Y"

### 7.3 Combat Forecast

Like Fire Emblem:
- Show predicted damage
- Show counter-attack damage
- HP bars preview

---

## 8. Implementation Priority

| Phase | Features | Effort |
|-------|----------|--------|
| **1** | Turn banner, selection box, basic delays | Low |
| **2** | Movement/attack preview blobs | Medium |
| **3** | Path lines, evaluation phase | Medium |
| **4** | Settings integration, speed modes | Low |
| **5** | Threat indicators (future) | High |
