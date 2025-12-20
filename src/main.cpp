#include <SDL3/SDL.h>
#include <SDL3_image/SDL_image.h>
#include "types.hpp"
#include "entity.hpp"
#include "grid_renderer.hpp"
#include "fx.hpp"
#include "timing_loader.hpp"
#include "asset_manager.hpp"
#include "sdl_handles.hpp"
#include <vector>
#include <cstring>
#include <algorithm>
#include <cmath>
#include <limits>
#include <numeric>

constexpr int MOVE_RANGE = 3;
constexpr float TURN_TRANSITION_DELAY = 0.5f;
constexpr float AI_ACTION_DELAY = 0.4f;

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

constexpr float DAMAGE_NUMBER_DURATION = 1.0f;
constexpr float DAMAGE_NUMBER_RISE_SPEED = 50.0f;

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
    TimingData timing_data;
    GridRenderer grid_renderer;
    
    GamePhase game_phase = GamePhase::Playing;
    TurnPhase turn_phase = TurnPhase::PlayerTurn;
    float turn_transition_timer = 0.0f;
    float ai_action_timer = 0.0f;
    int ai_current_unit = -1;
    std::vector<bool> has_acted;
};

void print_help() {
    SDL_Log("Usage: tactics [OPTIONS]");
    SDL_Log("");
    SDL_Log("OPTIONS:");
    SDL_Log("  -f, --fullscreen    Launch in 1920x1080 fullscreen with 2x sprite scaling");
    SDL_Log("  -h, --help          Show this help message and exit");
    SDL_Log("");
    SDL_Log("CONTROLS:");
    SDL_Log("  Left click unit     Select unit and show movement range");
    SDL_Log("  Left click tile     Move selected unit to that tile");
    SDL_Log("  Left click enemy    Attack enemy in range");
    SDL_Log("  Right click         Deselect unit");
    SDL_Log("  Space               End turn early");
    SDL_Log("  R                   Restart (after game over)");
}

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
}

void update_selected_ranges(GameState& state) {
    if (state.selected_unit_idx < 0) return;
    if (state.selected_unit_idx >= static_cast<int>(state.units.size())) {
        state.selected_unit_idx = -1;
        return;
    }
    
    if (state.units[state.selected_unit_idx].is_dead()) {
        clear_selection(state);
        return;
    }
    
    BoardPos selected_pos = state.units[state.selected_unit_idx].board_pos;
    
    auto occupied = get_occupied_positions(state, state.selected_unit_idx);
    state.reachable_tiles = get_reachable_tiles(selected_pos, MOVE_RANGE, occupied);
    
    auto enemy_positions = get_enemy_positions(state, state.selected_unit_idx);
    state.attackable_tiles = get_attackable_tiles(
        selected_pos,
        state.units[state.selected_unit_idx].attack_range,
        enemy_positions
    );
}

void update_selected_facing(GameState& state, const RenderConfig& config) {
    if (state.selected_unit_idx < 0) return;
    if (state.selected_unit_idx >= static_cast<int>(state.units.size())) return;
    
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
    state.has_acted.clear();
    state.has_acted.resize(state.units.size(), false);
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
            if (i < state.has_acted.size() && !state.has_acted[i]) {
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
        if (i < state.has_acted.size() && state.has_acted[i]) continue;
        if (!state.units[i].can_act()) continue;
        return static_cast<int>(i);
    }
    return -1;
}

void execute_ai_action(GameState& state, int unit_idx, const RenderConfig& config) {
    if (try_ai_attack(state, unit_idx)) {
        state.has_acted[unit_idx] = true;
        return;
    }
    
    if (try_ai_move(state, unit_idx, config)) {
        state.has_acted[unit_idx] = true;
        return;
    }
    
    state.has_acted[unit_idx] = true;
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
    std::vector<bool> new_has_acted;
    
    for (size_t i = 0; i < state.units.size(); i++) {
        if (!state.units[i].is_dead()) {
            if (i < state.has_acted.size()) {
                new_has_acted.push_back(state.has_acted[i]);
            } else {
                new_has_acted.push_back(false);
            }
        } else if (static_cast<int>(i) < state.selected_unit_idx) {
            removed_before_selected++;
        }
    }
    
    state.units.erase(
        std::remove_if(state.units.begin(), state.units.end(),
            [](const Entity& e) { return e.is_dead(); }),
        state.units.end()
    );
    
    state.has_acted = std::move(new_has_acted);
    
    if (state.selected_unit_idx >= 0) {
        state.selected_unit_idx -= removed_before_selected;
    }
}

RenderConfig parse_args(int argc, char* argv[]) {
    bool fullscreen = false;
    
    for (int i = 1; i < argc; i++) {
        if (std::strcmp(argv[i], "-h") == 0 || std::strcmp(argv[i], "--help") == 0) {
            print_help();
            exit(0);
        } else if (std::strcmp(argv[i], "-f") == 0 || std::strcmp(argv[i], "--fullscreen") == 0) {
            fullscreen = true;
        } else {
            SDL_Log("Unknown option: %s", argv[i]);
            print_help();
            exit(1);
        }
    }
    
    if (fullscreen) {
        return {1920, 1080, 2};
    } else {
        return {1280, 720, 1};
    }
}

bool init(const RenderConfig& config, WindowHandle& window) {
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        SDL_Log("SDL_Init failed: %s", SDL_GetError());
        return false;
    }

    Uint32 window_flags = 0;
    if (config.scale == 2) {
        window_flags = SDL_WINDOW_FULLSCREEN;
    }

    SDL_Window* raw_window = SDL_CreateWindow("Duelyst Tactics",
        config.window_w, config.window_h, window_flags);

    if (!raw_window) {
        SDL_Log("Window creation failed: %s", SDL_GetError());
        SDL_Quit();
        return false;
    }

    window = WindowHandle(raw_window);

    return true;
}

void handle_select_click(GameState& state, BoardPos clicked) {
    if (state.turn_phase != TurnPhase::PlayerTurn) return;
    
    int unit_idx = find_unit_at_pos(state, clicked);
    if (unit_idx < 0) return;
    
    if (state.units[unit_idx].type != UnitType::Player) return;
    if (!state.units[unit_idx].can_act()) return;
    if (unit_idx < static_cast<int>(state.has_acted.size()) && state.has_acted[unit_idx]) return;
    
    if (state.selected_unit_idx >= 0 && state.selected_unit_idx != unit_idx) {
        state.units[state.selected_unit_idx].restore_facing();
    }
    
    state.selected_unit_idx = unit_idx;
    state.units[unit_idx].store_facing();
    update_selected_ranges(state);
    
    SDL_Log("Unit %d selected at (%d, %d)", unit_idx, clicked.x, clicked.y);
}

void handle_move_click(GameState& state, BoardPos clicked, const RenderConfig& config) {
    bool is_reachable = false;
    for (const auto& tile : state.reachable_tiles) {
        if (tile == clicked) {
            is_reachable = true;
            break;
        }
    }
    
    if (!is_reachable) return;
    
    auto occupied = get_occupied_positions(state, state.selected_unit_idx);
    for (const auto& occ : occupied) {
        if (clicked == occ) return;
    }
    
    int unit_idx = state.selected_unit_idx;
    SDL_Log("Moving unit %d to (%d, %d)", unit_idx, clicked.x, clicked.y);
    
    if (unit_idx < static_cast<int>(state.has_acted.size())) {
        state.has_acted[unit_idx] = true;
    }
    
    state.selected_unit_idx = -1;
    state.reachable_tiles.clear();
    state.attackable_tiles.clear();
    
    state.units[unit_idx].start_move(config, clicked);
}

void handle_attack_click(GameState& state, BoardPos clicked) {
    bool is_attackable = false;
    for (const auto& tile : state.attackable_tiles) {
        if (tile == clicked) {
            is_attackable = true;
            break;
        }
    }
    
    if (!is_attackable) return;
    
    int target_idx = find_unit_at_pos(state, clicked);
    if (target_idx < 0) return;
    
    int attacker_idx = state.selected_unit_idx;
    BoardPos target_pos = state.units[target_idx].board_pos;
    
    if (attacker_idx < static_cast<int>(state.has_acted.size())) {
        state.has_acted[attacker_idx] = true;
    }
    
    state.selected_unit_idx = -1;
    state.reachable_tiles.clear();
    state.attackable_tiles.clear();
    
    state.units[attacker_idx].face_position(target_pos);
    state.units[attacker_idx].start_attack(target_idx);
}

void handle_selected_click(GameState& state, BoardPos clicked, const RenderConfig& config) {
    int clicked_unit = find_unit_at_pos(state, clicked);
    
    if (clicked_unit >= 0 && state.units[clicked_unit].type != state.units[state.selected_unit_idx].type) {
        handle_attack_click(state, clicked);
    } else {
        handle_move_click(state, clicked, config);
    }
}

void handle_click(GameState& state, Vec2 mouse, const RenderConfig& config) {
    if (state.game_phase != GamePhase::Playing) return;
    if (state.turn_phase != TurnPhase::PlayerTurn) return;
    
    BoardPos clicked = screen_to_board_perspective(config, mouse);
    if (!clicked.is_valid()) return;
    
    if (state.selected_unit_idx >= 0 && state.units[state.selected_unit_idx].is_moving()) {
        return;
    }
    
    if (state.selected_unit_idx >= 0 && state.units[state.selected_unit_idx].is_attacking()) {
        return;
    }
    
    if (state.selected_unit_idx == -1) {
        handle_select_click(state, clicked);
    } else {
        handle_selected_click(state, clicked, config);
    }
}

void handle_end_turn(GameState& state) {
    if (state.game_phase != GamePhase::Playing) return;
    if (state.turn_phase != TurnPhase::PlayerTurn) return;
    if (any_units_busy(state)) return;
    
    SDL_Log("Player ended turn early");
    begin_turn_transition(state, TurnPhase::EnemyTurn);
}

Entity create_unit(GameState& state, const RenderConfig& config,
                   const char* unit_name, UnitType type, int hp, int atk, BoardPos pos);

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
    state.has_acted.clear();

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

void handle_events(bool& running, GameState& state, const RenderConfig& config) {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        if (event.type == SDL_EVENT_QUIT) {
            running = false;
        }
        else if (event.type == SDL_EVENT_KEY_DOWN) {
            if (event.key.key == SDLK_SPACE) {
                handle_end_turn(state);
            }
            else if (event.key.key == SDLK_R) {
                if (state.game_phase != GamePhase::Playing) {
                    reset_game(state, config);
                }
            }
            else if (event.key.key == SDLK_ESCAPE) {
                running = false;
            }
        }
        else if (event.type == SDL_EVENT_MOUSE_BUTTON_DOWN) {
            if (event.button.button == SDL_BUTTON_LEFT) {
                Vec2 mouse{static_cast<float>(event.button.x), static_cast<float>(event.button.y)};
                if (state.game_phase != GamePhase::Playing) {
                    reset_game(state, config);
                } else {
                    handle_click(state, mouse, config);
                }
            }
            else if (event.button.button == SDL_BUTTON_RIGHT) {
                clear_selection(state);
            }
        }
        else if (event.type == SDL_EVENT_MOUSE_MOTION) {
            state.mouse_pos = {static_cast<float>(event.motion.x), static_cast<float>(event.motion.y)};
            if (state.game_phase == GamePhase::Playing && state.turn_phase == TurnPhase::PlayerTurn) {
                update_selected_facing(state, config);
            }
        }
    }
}

void check_player_turn_end(GameState& state) {
    if (state.game_phase != GamePhase::Playing) return;
    if (state.turn_phase != TurnPhase::PlayerTurn) return;
    if (any_units_busy(state)) return;
    if (!all_units_acted(state, UnitType::Player)) return;
    
    begin_turn_transition(state, TurnPhase::EnemyTurn);
}

void update(GameState& state, float dt, const RenderConfig& config) {
    check_attack_damage(state);
    process_pending_damage(state, config);

    for (auto& unit : state.units) {
        unit.update(dt, config);
    }

    update_floating_texts(state, dt, config);
    update_active_fx(state, dt);
    remove_dead_units(state);

    check_win_lose_condition(state);

    if (state.game_phase != GamePhase::Playing) return;

    update_selected_ranges(state);
    update_turn_transition(state, dt);
    update_ai(state, dt, config);
    check_player_turn_end(state);
}

void render_floating_texts(const GameState& state, const RenderConfig& config) {
    for (const auto& ft : state.floating_texts) {
        float alpha = ft.get_alpha();

        float size = 12.0f * config.scale;
        SDL_FRect rect = {
            ft.pos.x - size * 0.5f,
            ft.pos.y - size * 0.5f,
            size,
            size
        };

        g_gpu.draw_quad_colored(rect, {1.0f, 50.0f/255.0f, 50.0f/255.0f, alpha});

        SDL_FRect inner = {
            rect.x + 2,
            rect.y + 2,
            rect.w - 4,
            rect.h - 4
        };
        g_gpu.draw_quad_colored(inner, {1.0f, 200.0f/255.0f, 200.0f/255.0f, alpha});
    }
}

void render_active_fx(const GameState& state, const RenderConfig& config) {
    for (const auto& fx : state.active_fx) {
        fx.render(config);
    }
}

void render_turn_indicator(const GameState& state, const RenderConfig& config) {
    float indicator_w = 200.0f * config.scale;
    float indicator_h = 40.0f * config.scale;
    float x = (config.window_w - indicator_w) * 0.5f;
    float y = 20.0f * config.scale;

    SDL_FRect bg = {x - 2, y - 2, indicator_w + 4, indicator_h + 4};
    g_gpu.draw_quad_colored(bg, {0.0f, 0.0f, 0.0f, 200.0f/255.0f});

    SDL_FRect inner = {x, y, indicator_w, indicator_h};

    SDL_FColor inner_color;
    switch (state.turn_phase) {
        case TurnPhase::PlayerTurn:
            inner_color = {50.0f/255.0f, 150.0f/255.0f, 1.0f, 1.0f};
            break;
        case TurnPhase::EnemyTurn:
            inner_color = {1.0f, 80.0f/255.0f, 80.0f/255.0f, 1.0f};
            break;
        case TurnPhase::TurnTransition:
            inner_color = {150.0f/255.0f, 150.0f/255.0f, 150.0f/255.0f, 1.0f};
            break;
    }
    g_gpu.draw_quad_colored(inner, inner_color);

    float bar_w = 60.0f * config.scale;
    float bar_h = 20.0f * config.scale;
    float bar_x = x + (indicator_w - bar_w) * 0.5f;
    float bar_y = y + (indicator_h - bar_h) * 0.5f;

    SDL_FRect label = {bar_x, bar_y, bar_w, bar_h};
    g_gpu.draw_quad_colored(label, {1.0f, 1.0f, 1.0f, 200.0f/255.0f});
}

void render_game_over_overlay(const GameState& state, const RenderConfig& config) {
    SDL_FRect fullscreen = {0, 0, static_cast<float>(config.window_w), static_cast<float>(config.window_h)};
    g_gpu.draw_quad_colored(fullscreen, {0.0f, 0.0f, 0.0f, 180.0f/255.0f});

    float box_w = 400.0f * config.scale;
    float box_h = 200.0f * config.scale;
    float box_x = (config.window_w - box_w) * 0.5f;
    float box_y = (config.window_h - box_h) * 0.5f;

    SDL_FRect box_bg = {box_x - 4, box_y - 4, box_w + 8, box_h + 8};
    g_gpu.draw_quad_colored(box_bg, {40.0f/255.0f, 40.0f/255.0f, 60.0f/255.0f, 1.0f});

    SDL_FRect box = {box_x, box_y, box_w, box_h};
    SDL_FColor box_color = state.game_phase == GamePhase::Victory ?
        SDL_FColor{50.0f/255.0f, 120.0f/255.0f, 50.0f/255.0f, 1.0f} :
        SDL_FColor{120.0f/255.0f, 50.0f/255.0f, 50.0f/255.0f, 1.0f};
    g_gpu.draw_quad_colored(box, box_color);

    float title_w = 200.0f * config.scale;
    float title_h = 60.0f * config.scale;
    float title_x = box_x + (box_w - title_w) * 0.5f;
    float title_y = box_y + 30.0f * config.scale;

    SDL_FRect title_rect = {title_x, title_y, title_w, title_h};
    SDL_FColor title_color = state.game_phase == GamePhase::Victory ?
        SDL_FColor{100.0f/255.0f, 1.0f, 100.0f/255.0f, 1.0f} :
        SDL_FColor{1.0f, 100.0f/255.0f, 100.0f/255.0f, 1.0f};
    g_gpu.draw_quad_colored(title_rect, title_color);

    float hint_w = 250.0f * config.scale;
    float hint_h = 30.0f * config.scale;
    float hint_x = box_x + (box_w - hint_w) * 0.5f;
    float hint_y = box_y + box_h - 50.0f * config.scale;

    SDL_FRect hint_rect = {hint_x, hint_y, hint_w, hint_h};
    g_gpu.draw_quad_colored(hint_rect, {200.0f/255.0f, 200.0f/255.0f, 200.0f/255.0f, 200.0f/255.0f});
}

std::vector<size_t> get_render_order(const GameState& state) {
    std::vector<size_t> order(state.units.size());
    std::iota(order.begin(), order.end(), 0);
    std::sort(order.begin(), order.end(), [&](size_t a, size_t b) {
        return state.units[a].screen_pos.y < state.units[b].screen_pos.y;
    });
    return order;
}

void render(GameState& state, const RenderConfig& config) {
    g_gpu.begin_frame();

    state.grid_renderer.render(config);

    if (state.selected_unit_idx >= 0 && state.game_phase == GamePhase::Playing) {
        auto occupied = get_occupied_positions(state, state.selected_unit_idx);
        state.grid_renderer.render_move_range(config,
            state.units[state.selected_unit_idx].board_pos, MOVE_RANGE, occupied);
        state.grid_renderer.render_attack_range(config, state.attackable_tiles);
    }

    auto render_order = get_render_order(state);

    for (size_t idx : render_order) {
        if (!state.units[idx].is_dead()) {
            state.units[idx].render_shadow(config);
        }
    }

    for (size_t idx : render_order) {
        if (!state.units[idx].is_dead()) {
            state.units[idx].render(config);
        }
    }

    render_active_fx(state, config);

    for (size_t idx : render_order) {
        if (!state.units[idx].is_dead()) {
            state.units[idx].render_hp_bar(config);
        }
    }

    render_floating_texts(state, config);

    if (state.game_phase == GamePhase::Playing) {
        render_turn_indicator(state, config);
    } else {
        render_game_over_overlay(state, config);
    }

    g_gpu.end_frame();
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

    UnitTiming timing = state.timing_data.get(unit_name);
    unit.set_timing(timing.attack_damage_delay);

    spawn_unit_spawn_fx(state, unit.screen_pos);

    return unit;
}

int main(int argc, char* argv[]) {
    RenderConfig config = parse_args(argc, argv);

    // Initialize asset manager (loads assets.json from dist/)
    if (!AssetManager::instance().init("dist")) {
        SDL_Log("Failed to initialize AssetManager from dist/");
        SDL_Log("Run 'python3 build_assets.py' to build the asset manifest");
        return 1;
    }

    WindowHandle window;

    if (!init(config, window)) {
        return 1;
    }

    if (!g_gpu.init(window.get())) {
        SDL_Log("Failed to initialize GPU renderer");
        return 1;
    }

    GameState state;

    if (!state.grid_renderer.init(config)) {
        SDL_Log("Failed to initialize grid renderer");
        return 1;
    }

    // FX mappings and timing data are now loaded via AssetManager from assets.json
    // The old TSV-based loading is no longer needed

    if (!Entity::load_shadow()) {
        SDL_Log("Warning: Failed to load shadow texture");
    }

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
    SDL_Log("=== PLAYER TURN ===");

    bool running = true;
    Uint64 last_time = SDL_GetTicks();

    while (running) {
        Uint64 current_time = SDL_GetTicks();
        float dt = (current_time - last_time) / 1000.0f;
        last_time = current_time;

        handle_events(running, state, config);
        update(state, dt, config);
        render(state, config);

        SDL_Delay(16);
    }

    g_gpu.shutdown();
    SDL_Quit();

    return 0;
}
