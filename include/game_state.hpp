#pragma once
#include "types.hpp"
#include "entity.hpp"
#include "grid_renderer.hpp"
#include "fx.hpp"
#include <vector>

// Game constants
constexpr int MOVE_RANGE = 3;
constexpr float TURN_TRANSITION_DELAY = 0.5f;
constexpr float AI_ACTION_DELAY = 0.4f;
constexpr float DAMAGE_NUMBER_DURATION = 1.0f;
constexpr float DAMAGE_NUMBER_RISE_SPEED = 50.0f;

enum class GamePhase {
    Playing,
    Victory,
    Defeat
};

enum class TurnPhase {
    PlayerTurn,
    EnemyTurn,
    TurnTransition
};

struct FloatingText {
    Vec2 pos;
    int value;
    float elapsed;
    float duration;

    bool is_expired() const { return elapsed >= duration; }
    float get_alpha() const {
        float t = elapsed / duration;
        if (t < 0.2f) return t / 0.2f;
        if (t > 0.7f) return 1.0f - (t - 0.7f) / 0.3f;
        return 1.0f;
    }
};

struct PendingDamage {
    int attacker_idx;
    int target_idx;
    int damage;
};

struct GameState {
    std::vector<Entity> units;
    int selected_unit_idx = -1;
    std::vector<BoardPos> reachable_tiles;
    std::vector<BoardPos> attackable_tiles;
    Vec2 mouse_pos = {0.0f, 0.0f};
    std::vector<FloatingText> floating_texts;
    std::vector<PendingDamage> pending_damage;
    FXCache fx_cache;
    std::vector<FXEntity> active_fx;
    GridRenderer grid_renderer;

    GamePhase game_phase = GamePhase::Playing;
    TurnPhase turn_phase = TurnPhase::PlayerTurn;
    float turn_transition_timer = 0.0f;
    float ai_action_timer = 0.0f;
    int ai_current_unit = -1;
    std::vector<bool> has_moved;
    std::vector<bool> has_attacked;

    // Hover state for tile system
    BoardPos hover_pos{-1, -1};
    bool hover_valid = false;
    bool was_hovering_on_board = false;  // For instant transition detection

    // Computed path to hover target
    std::vector<BoardPos> movement_path;

    // Blob opacity for dimming during hover
    float move_blob_opacity = 1.0f;
    float attack_blob_opacity = 1.0f;

    // Active fade animations
    std::vector<TileFadeAnim> tile_anims;

    // Pulsing animation for target tiles (Duelyst: 0.7s period, 0.85-1.0 scale)
    float target_pulse_phase = 0.0f;
};
