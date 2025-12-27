#include "game_logic.hpp"
#include "input.hpp"
#include "asset_manager.hpp"
#include <algorithm>
#include <limits>

int find_unit_at_pos(const GameState& state, BoardPos pos) {
    for (size_t i = 0; i < state.units.size(); i++) {
        if (state.units[i].board_pos == pos && !state.units[i].is_dead()) {
            return static_cast<int>(i);
        }
    }
    return -1;
}

std::vector<BoardPos> get_occupied_positions(const GameState& state, int exclude_idx) {
    std::vector<BoardPos> occupied;
    for (size_t i = 0; i < state.units.size(); i++) {
        if (static_cast<int>(i) != exclude_idx && !state.units[i].is_dead()) {
            if (state.units[i].is_moving()) {
                occupied.push_back(state.units[i].move_target);
            } else {
                occupied.push_back(state.units[i].board_pos);
            }
        }
    }
    return occupied;
}

std::vector<BoardPos> get_enemy_positions(const GameState& state, int unit_idx) {
    std::vector<BoardPos> enemies;
    UnitType unit_type = state.units[unit_idx].type;

    for (size_t i = 0; i < state.units.size(); i++) {
        if (static_cast<int>(i) != unit_idx &&
            state.units[i].type != unit_type &&
            !state.units[i].is_dead()) {
            if (state.units[i].can_act()) {
                enemies.push_back(state.units[i].board_pos);
            }
        }
    }
    return enemies;
}

void clear_selection(GameState& state) {
    if (state.selected_unit_idx >= 0 &&
        state.selected_unit_idx < static_cast<int>(state.units.size())) {
        state.units[state.selected_unit_idx].restore_facing();
    }
    state.selected_unit_idx = -1;
    state.reachable_tiles.clear();
    state.attackable_tiles.clear();
    state.movement_path.clear();
    state.move_blob_opacity = 1.0f;
    state.attack_blob_opacity = 1.0f;
    state.tile_anims.clear();
}

void update_selected_ranges(GameState& state) {
    if (state.selected_unit_idx < 0) return;
    if (state.selected_unit_idx >= static_cast<int>(state.units.size())) {
        state.selected_unit_idx = -1;
        return;
    }

    int idx = state.selected_unit_idx;
    if (state.units[idx].is_dead()) {
        clear_selection(state);
        return;
    }

    if (state.units[idx].is_moving() || state.units[idx].is_attacking()) {
        return;
    }

    BoardPos selected_pos = state.units[idx].board_pos;

    bool can_move = idx >= static_cast<int>(state.has_moved.size()) || !state.has_moved[idx];
    if (can_move) {
        auto occupied = get_occupied_positions(state, idx);
        state.reachable_tiles = get_reachable_tiles(selected_pos, MOVE_RANGE, occupied);
    }

    auto enemy_positions = get_enemy_positions(state, idx);
    state.attackable_tiles = get_attackable_tiles(
        selected_pos,
        state.units[idx].attack_range,
        enemy_positions
    );
}

void update_selected_facing(GameState& state, const RenderConfig& config) {
    if (state.selected_unit_idx < 0) return;
    if (state.selected_unit_idx >= static_cast<int>(state.units.size())) return;
    if (state.units[state.selected_unit_idx].is_moving()) return;

    BoardPos mouse_board = screen_to_board_perspective(config, state.mouse_pos);
    if (!mouse_board.is_valid()) return;

    state.units[state.selected_unit_idx].face_position(mouse_board);
}

void spawn_damage_number(GameState& state, Vec2 pos, int damage, const RenderConfig& config) {
    FloatingText ft;
    ft.pos = pos;
    ft.pos.y -= 30.0f * config.scale;
    ft.value = damage;
    ft.elapsed = 0.0f;
    ft.duration = DAMAGE_NUMBER_DURATION;
    state.floating_texts.push_back(ft);
}

void spawn_fx_at_pos(GameState& state, const std::string& rsx_name, Vec2 pos) {
    FXEntity fx = create_fx(state.fx_cache, rsx_name, pos);
    if (!fx.is_complete()) {
        state.active_fx.push_back(std::move(fx));
    }
}

void spawn_unit_spawn_fx(GameState& state, Vec2 pos) {
    spawn_fx_at_pos(state, "fxSmokeGround", pos);
}

void spawn_unit_death_fx(GameState& state, Vec2 pos) {
    spawn_fx_at_pos(state, "fxExplosionOrangeSmoke", pos);
}

void spawn_attack_fx(GameState& state, Vec2 target_pos) {
    spawn_fx_at_pos(state, "fxClawSlash", target_pos);
    spawn_fx_at_pos(state, "fxImpactOrangeSmall", target_pos);
}

void reset_actions(GameState& state) {
    state.has_moved.clear();
    state.has_moved.resize(state.units.size(), false);
    state.has_attacked.clear();
    state.has_attacked.resize(state.units.size(), false);
}

void start_player_turn(GameState& state) {
    state.turn_phase = TurnPhase::PlayerTurn;
    reset_actions(state);
    SDL_Log("=== PLAYER TURN ===");
}

void start_enemy_turn(GameState& state) {
    state.turn_phase = TurnPhase::EnemyTurn;
    state.ai_current_unit = -1;
    state.ai_action_timer = AI_ACTION_DELAY;
    reset_actions(state);
    clear_selection(state);
    SDL_Log("=== ENEMY TURN ===");
}

void begin_turn_transition(GameState& state, TurnPhase next_phase) {
    state.turn_phase = TurnPhase::TurnTransition;
    state.turn_transition_timer = TURN_TRANSITION_DELAY;
    state.ai_current_unit = (next_phase == TurnPhase::EnemyTurn) ? -1 : -2;
}

bool all_units_acted(const GameState& state, UnitType type) {
    for (size_t i = 0; i < state.units.size(); i++) {
        if (state.units[i].type == type && !state.units[i].is_dead()) {
            if (i < state.has_attacked.size() && !state.has_attacked[i]) {
                return false;
            }
        }
    }
    return true;
}

bool any_units_busy(const GameState& state) {
    for (const auto& unit : state.units) {
        if (unit.is_moving() || unit.is_attacking() || unit.is_dying()) {
            return true;
        }
    }
    return false;
}

bool has_living_units(const GameState& state, UnitType type) {
    for (const auto& unit : state.units) {
        if (unit.type == type && !unit.is_dead()) {
            return true;
        }
    }
    return false;
}

void check_win_lose_condition(GameState& state) {
    if (state.game_phase != GamePhase::Playing) return;
    if (any_units_busy(state)) return;

    bool players_alive = has_living_units(state, UnitType::Player);
    bool enemies_alive = has_living_units(state, UnitType::Enemy);

    if (!enemies_alive) {
        state.game_phase = GamePhase::Victory;
        clear_selection(state);
        SDL_Log("=== VICTORY ===");
    } else if (!players_alive) {
        state.game_phase = GamePhase::Defeat;
        clear_selection(state);
        SDL_Log("=== DEFEAT ===");
    }
}

int find_nearest_enemy(const GameState& state, int unit_idx) {
    const Entity& unit = state.units[unit_idx];
    int nearest_idx = -1;
    int nearest_dist = std::numeric_limits<int>::max();

    for (size_t i = 0; i < state.units.size(); i++) {
        if (static_cast<int>(i) == unit_idx) continue;
        if (state.units[i].type == unit.type) continue;
        if (state.units[i].is_dead()) continue;

        int dx = std::abs(state.units[i].board_pos.x - unit.board_pos.x);
        int dy = std::abs(state.units[i].board_pos.y - unit.board_pos.y);
        int dist = dx + dy;

        if (dist < nearest_dist) {
            nearest_dist = dist;
            nearest_idx = static_cast<int>(i);
        }
    }
    return nearest_idx;
}

BoardPos find_best_move_toward(const GameState& state, int unit_idx, BoardPos target) {
    BoardPos from = state.units[unit_idx].board_pos;
    auto occupied = get_occupied_positions(state, unit_idx);
    auto reachable = get_reachable_tiles(from, MOVE_RANGE, occupied);

    if (reachable.empty()) return from;

    BoardPos best = reachable[0];
    int best_dist = std::numeric_limits<int>::max();

    for (const auto& pos : reachable) {
        int dx = std::abs(pos.x - target.x);
        int dy = std::abs(pos.y - target.y);
        int dist = dx + dy;

        if (dist < best_dist) {
            best_dist = dist;
            best = pos;
        }
    }
    return best;
}

bool try_ai_attack(GameState& state, int unit_idx) {
    Entity& unit = state.units[unit_idx];
    auto player_positions = get_enemy_positions(state, unit_idx);
    auto attackable = get_attackable_tiles(unit.board_pos, unit.attack_range, player_positions);

    if (attackable.empty()) return false;

    int target_idx = find_unit_at_pos(state, attackable[0]);
    if (target_idx < 0) return false;

    unit.face_position(attackable[0]);
    unit.start_attack(target_idx);
    SDL_Log("AI unit %d attacking unit %d", unit_idx, target_idx);
    return true;
}

bool try_ai_move(GameState& state, int unit_idx, const RenderConfig& config) {
    int target_idx = find_nearest_enemy(state, unit_idx);
    if (target_idx < 0) return false;

    BoardPos target_pos = state.units[target_idx].board_pos;
    BoardPos best_move = find_best_move_toward(state, unit_idx, target_pos);

    if (best_move == state.units[unit_idx].board_pos) return false;

    state.units[unit_idx].start_move(config, best_move);
    SDL_Log("AI unit %d moving to (%d, %d)", unit_idx, best_move.x, best_move.y);
    return true;
}

int find_next_ai_unit(const GameState& state) {
    for (size_t i = 0; i < state.units.size(); i++) {
        if (state.units[i].type != UnitType::Enemy) continue;
        if (state.units[i].is_dead()) continue;
        if (i < state.has_attacked.size() && state.has_attacked[i]) continue;
        if (!state.units[i].can_act()) continue;
        return static_cast<int>(i);
    }
    return -1;
}

void execute_ai_action(GameState& state, int unit_idx, const RenderConfig& config) {
    if (try_ai_attack(state, unit_idx)) {
        state.has_attacked[unit_idx] = true;
        return;
    }

    if (try_ai_move(state, unit_idx, config)) {
        state.has_attacked[unit_idx] = true;
        return;
    }

    state.has_attacked[unit_idx] = true;
    SDL_Log("AI unit %d has no valid action", unit_idx);
}

void update_ai(GameState& state, float dt, const RenderConfig& config) {
    if (state.game_phase != GamePhase::Playing) return;
    if (state.turn_phase != TurnPhase::EnemyTurn) return;
    if (any_units_busy(state)) return;

    state.ai_action_timer -= dt;
    if (state.ai_action_timer > 0) return;

    state.ai_action_timer = AI_ACTION_DELAY;

    int next_unit = find_next_ai_unit(state);
    if (next_unit < 0) {
        begin_turn_transition(state, TurnPhase::PlayerTurn);
        return;
    }

    state.ai_current_unit = next_unit;
    execute_ai_action(state, next_unit, config);
}

void update_turn_transition(GameState& state, float dt) {
    if (state.turn_phase != TurnPhase::TurnTransition) return;

    state.turn_transition_timer -= dt;
    if (state.turn_transition_timer > 0) return;

    if (state.ai_current_unit == -1) {
        start_enemy_turn(state);
    } else {
        start_player_turn(state);
    }
}

void process_pending_damage(GameState& state, const RenderConfig& config) {
    for (const auto& pd : state.pending_damage) {
        if (pd.target_idx < 0 || pd.target_idx >= static_cast<int>(state.units.size())) {
            continue;
        }

        Entity& target = state.units[pd.target_idx];
        if (target.is_dead()) continue;

        spawn_damage_number(state, target.screen_pos, pd.damage, config);
        spawn_attack_fx(state, target.screen_pos);

        bool was_alive = target.hp > 0;
        target.take_damage(pd.damage);

        if (was_alive && target.hp <= 0) {
            spawn_unit_death_fx(state, target.screen_pos);
        }
    }
    state.pending_damage.clear();
}

void check_attack_damage(GameState& state) {
    for (size_t i = 0; i < state.units.size(); i++) {
        Entity& unit = state.units[i];

        if (!unit.should_deal_damage()) continue;

        int target_idx = unit.get_target_idx();
        if (target_idx < 0 || target_idx >= static_cast<int>(state.units.size())) continue;

        PendingDamage pd;
        pd.attacker_idx = static_cast<int>(i);
        pd.target_idx = target_idx;
        pd.damage = unit.attack_power;
        state.pending_damage.push_back(pd);

        unit.mark_damage_dealt();
        SDL_Log("Attack from unit %zu dealing %d damage to unit %d", i, pd.damage, target_idx);
    }
}

void update_floating_texts(GameState& state, float dt, const RenderConfig& config) {
    for (auto& ft : state.floating_texts) {
        ft.elapsed += dt;
        ft.pos.y -= DAMAGE_NUMBER_RISE_SPEED * config.scale * dt;
    }

    state.floating_texts.erase(
        std::remove_if(state.floating_texts.begin(), state.floating_texts.end(),
            [](const FloatingText& ft) { return ft.is_expired(); }),
        state.floating_texts.end()
    );
}

void update_active_fx(GameState& state, float dt) {
    for (auto& fx : state.active_fx) {
        fx.update(dt);
    }

    state.active_fx.erase(
        std::remove_if(state.active_fx.begin(), state.active_fx.end(),
            [](const FXEntity& fx) { return fx.is_complete(); }),
        state.active_fx.end()
    );
}

void remove_dead_units(GameState& state) {
    if (state.selected_unit_idx >= 0) {
        if (state.units[state.selected_unit_idx].is_dead()) {
            state.selected_unit_idx = -1;
            state.reachable_tiles.clear();
            state.attackable_tiles.clear();
        }
    }

    int removed_before_selected = 0;
    std::vector<bool> new_has_moved;
    std::vector<bool> new_has_attacked;
    for (size_t i = 0; i < state.units.size(); i++) {
        if (!state.units[i].death_complete) {
            new_has_moved.push_back(i < state.has_moved.size() ? state.has_moved[i] : false);
            new_has_attacked.push_back(i < state.has_attacked.size() ? state.has_attacked[i] : false);
        } else if (static_cast<int>(i) < state.selected_unit_idx) {
            removed_before_selected++;
        }
    }

    state.units.erase(
        std::remove_if(state.units.begin(), state.units.end(),
            [](const Entity& e) { return e.is_dead(); }),
        state.units.end()
    );
    state.has_moved = std::move(new_has_moved);
    state.has_attacked = std::move(new_has_attacked);

    if (state.selected_unit_idx >= 0) {
        state.selected_unit_idx -= removed_before_selected;
    }
}

void update_game(GameState& state, float dt, const RenderConfig& config) {
    check_attack_damage(state);
    process_pending_damage(state, config);

    for (size_t i = 0; i < state.units.size(); i++) {
        state.units[i].update(dt, config);
    }

    update_floating_texts(state, dt, config);
    update_active_fx(state, dt);
    remove_dead_units(state);

    // Update hover state and tile animations
    update_hover_state(state, config);
    update_tile_animations(state, dt);

    // Update target tile pulsing (Duelyst: 0.7s period)
    constexpr float PULSE_PERIOD = 0.7f;
    state.target_pulse_phase += dt / PULSE_PERIOD;
    if (state.target_pulse_phase > 1.0f) state.target_pulse_phase -= 1.0f;

    check_win_lose_condition(state);

    if (state.game_phase != GamePhase::Playing) return;

    update_selected_ranges(state);
    update_turn_transition(state, dt);
    update_ai(state, dt, config);
    check_player_turn_end(state);
}

Entity create_unit(GameState& state, const RenderConfig& config,
                   const char* unit_name, UnitType type, int hp, int atk, BoardPos pos) {
    Entity unit;
    if (!unit.load(unit_name)) {
        SDL_Log("Failed to load unit: %s", unit_name);
        return unit;
    }
    unit.type = type;
    unit.set_stats(hp, atk);
    unit.set_board_position(config, pos);

    UnitTiming timing = AssetManager::instance().get_timing(unit_name);
    unit.set_timing(timing.attack_damage_delay);

    spawn_unit_spawn_fx(state, unit.screen_pos);

    return unit;
}

void reset_game(GameState& state, const RenderConfig& config) {
    state.units.clear();
    state.selected_unit_idx = -1;
    state.reachable_tiles.clear();
    state.attackable_tiles.clear();
    state.floating_texts.clear();
    state.pending_damage.clear();
    state.active_fx.clear();
    state.game_phase = GamePhase::Playing;
    state.turn_phase = TurnPhase::PlayerTurn;
    state.turn_transition_timer = 0.0f;
    state.ai_action_timer = 0.0f;
    state.ai_current_unit = -1;
    state.has_moved.clear();
    state.has_attacked.clear();

    // Reset tile state
    state.hover_pos = {-1, -1};
    state.hover_valid = false;
    state.was_hovering_on_board = false;
    state.movement_path.clear();
    state.move_blob_opacity = 1.0f;
    state.attack_blob_opacity = 1.0f;
    state.tile_anims.clear();

    Entity unit1 = create_unit(state, config, "f1_general", UnitType::Player, 25, 5, {2, 2});
    if (unit1.spritesheet.ptr) {
        state.units.push_back(std::move(unit1));
    }

    Entity unit2 = create_unit(state, config, "f1_general", UnitType::Enemy, 10, 2, {6, 2});
    if (unit2.spritesheet.ptr) {
        state.units.push_back(std::move(unit2));
    }

    Entity unit3 = create_unit(state, config, "f1_general", UnitType::Enemy, 5, 3, {4, 1});
    if (unit3.spritesheet.ptr) {
        state.units.push_back(std::move(unit3));
    }

    reset_actions(state);
    SDL_Log("=== GAME RESET ===");
    SDL_Log("=== PLAYER TURN ===");
}
