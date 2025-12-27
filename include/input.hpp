#pragma once
#include "game_state.hpp"

// Click handlers
void handle_select_click(GameState& state, BoardPos clicked);
void handle_move_click(GameState& state, BoardPos clicked, const RenderConfig& config);
void handle_attack_click(GameState& state, BoardPos clicked);
void handle_selected_click(GameState& state, BoardPos clicked, const RenderConfig& config);
void handle_click(GameState& state, Vec2 mouse, const RenderConfig& config);

// Turn control
void handle_end_turn(GameState& state);
void check_player_turn_end(GameState& state);

// Tile hover system
void start_opacity_fade(GameState& state, FadeTarget target,
                        float from, float to, float duration);
void update_hover_path(GameState& state, const RenderConfig& config);
void update_hover_state(GameState& state, const RenderConfig& config);
void update_tile_animations(GameState& state, float dt);

// Event handling
void handle_events(bool& running, GameState& state, const RenderConfig& config);
