#pragma once
#include "game_state.hpp"

// Unit queries
int find_unit_at_pos(const GameState& state, BoardPos pos);
std::vector<BoardPos> get_occupied_positions(const GameState& state, int exclude_idx);
std::vector<BoardPos> get_enemy_positions(const GameState& state, int unit_idx);

// Selection management
void clear_selection(GameState& state);
void update_selected_ranges(GameState& state);
void update_selected_facing(GameState& state, const RenderConfig& config);

// FX spawning
void spawn_damage_number(GameState& state, Vec2 pos, int damage, const RenderConfig& config);
void spawn_fx_at_pos(GameState& state, const std::string& rsx_name, Vec2 pos);
void spawn_unit_spawn_fx(GameState& state, Vec2 pos);
void spawn_unit_death_fx(GameState& state, Vec2 pos);
void spawn_attack_fx(GameState& state, Vec2 target_pos);

// Turn management
void reset_actions(GameState& state);
void start_player_turn(GameState& state);
void start_enemy_turn(GameState& state);
void begin_turn_transition(GameState& state, TurnPhase next_phase);
bool all_units_acted(const GameState& state, UnitType type);
bool any_units_busy(const GameState& state);
bool has_living_units(const GameState& state, UnitType type);
void check_win_lose_condition(GameState& state);

// AI
int find_nearest_enemy(const GameState& state, int unit_idx);
BoardPos find_best_move_toward(const GameState& state, int unit_idx, BoardPos target);
bool try_ai_attack(GameState& state, int unit_idx);
bool try_ai_move(GameState& state, int unit_idx, const RenderConfig& config);
int find_next_ai_unit(const GameState& state);
void execute_ai_action(GameState& state, int unit_idx, const RenderConfig& config);
void update_ai(GameState& state, float dt, const RenderConfig& config);
void update_turn_transition(GameState& state, float dt);

// Combat and damage
void process_pending_damage(GameState& state, const RenderConfig& config);
void check_attack_damage(GameState& state);

// Updates
void update_floating_texts(GameState& state, float dt, const RenderConfig& config);
void update_active_fx(GameState& state, float dt);
void remove_dead_units(GameState& state);
void update_game(GameState& state, float dt, const RenderConfig& config);

// Unit creation and game reset
Entity create_unit(GameState& state, const RenderConfig& config,
                   const char* unit_name, UnitType type, int hp, int atk, BoardPos pos);
void reset_game(GameState& state, const RenderConfig& config);
