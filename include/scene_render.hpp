#pragma once
#include "game_state.hpp"
#include <vector>

// UI rendering
void render_floating_texts(const GameState& state, const RenderConfig& config);
void render_active_fx(const GameState& state, const RenderConfig& config);
void render_turn_indicator(const GameState& state, const RenderConfig& config);
void render_game_over_overlay(const GameState& state, const RenderConfig& config);

// Depth sorting
std::vector<size_t> get_render_order(const GameState& state);

// Main scene rendering
void render_single_pass(GameState& state, const RenderConfig& config);
void render(GameState& state, const RenderConfig& config);
