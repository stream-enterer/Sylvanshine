#include "input.hpp"
#include "game_logic.hpp"
#include "lighting_presets.hpp"
#include "settings_menu.hpp"
#include <SDL3/SDL.h>
#include <algorithm>

void handle_select_click(GameState& state, BoardPos clicked) {
    if (state.turn_phase != TurnPhase::PlayerTurn) return;

    int unit_idx = find_unit_at_pos(state, clicked);
    if (unit_idx < 0) return;

    if (state.units[unit_idx].type != UnitType::Player) return;
    if (!state.units[unit_idx].can_act()) return;
    if (unit_idx < static_cast<int>(state.has_attacked.size()) && state.has_attacked[unit_idx]) return;

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

    if (unit_idx < static_cast<int>(state.has_moved.size())) {
        state.has_moved[unit_idx] = true;
    }

    // Keep selection — player can attack after move
    // Clear all movement-related UI state
    state.reachable_tiles.clear();
    state.movement_path.clear();
    state.attackable_tiles.clear();  // Will be recalculated after move completes
    state.move_blob_opacity = 1.0f;
    state.tile_anims.clear();

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

    if (attacker_idx < static_cast<int>(state.has_attacked.size())) {
        state.has_attacked[attacker_idx] = true;
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
        // Clicked non-reachable tile — deselect (same as right-click)
        clear_selection(state);
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

void check_player_turn_end(GameState& state) {
    if (state.game_phase != GamePhase::Playing) return;
    if (state.turn_phase != TurnPhase::PlayerTurn) return;
    if (any_units_busy(state)) return;
    if (!all_units_acted(state, UnitType::Player)) return;

    begin_turn_transition(state, TurnPhase::EnemyTurn);
}

// =============================================================================
// Tile Hover System
// =============================================================================

void start_opacity_fade(GameState& state, FadeTarget target,
                        float from, float to, float duration) {
    if (duration <= 0.0f) {
        // Instant — apply directly
        switch (target) {
            case FadeTarget::MoveBlobOpacity:
                state.move_blob_opacity = to;
                break;
            case FadeTarget::AttackBlobOpacity:
                state.attack_blob_opacity = to;
                break;
        }
        return;
    }
    // Remove existing animation on this target
    state.tile_anims.erase(
        std::remove_if(state.tile_anims.begin(), state.tile_anims.end(),
            [target](const TileFadeAnim& a) { return a.target == target; }),
        state.tile_anims.end());

    state.tile_anims.push_back({target, from, to, duration, 0.0f});
}

void update_hover_path(GameState& state, const RenderConfig& config) {
    // Don't calculate path while unit is moving
    if (state.selected_unit_idx >= 0 && state.units[state.selected_unit_idx].is_moving()) {
        return;
    }

    // Check if hover is in movement range
    bool in_move_range = false;
    for (const auto& t : state.reachable_tiles) {
        if (t == state.hover_pos) { in_move_range = true; break; }
    }

    if (in_move_range) {
        // Calculate path
        auto occupied = get_occupied_positions(state, state.selected_unit_idx);
        BoardPos start = state.units[state.selected_unit_idx].board_pos;
        state.movement_path = get_path_to(start, state.hover_pos, occupied);

        // Dim blob (instant if already on board, else animate)
        float fade_dur = state.was_hovering_on_board ? 0.0f : FADE_FAST;
        start_opacity_fade(state, FadeTarget::MoveBlobOpacity,
                          state.move_blob_opacity, TileOpacity::DIM, fade_dur);
    } else {
        // Clear path, restore blob
        state.movement_path.clear();
        if (state.move_blob_opacity < 1.0f) {
            start_opacity_fade(state, FadeTarget::MoveBlobOpacity,
                              state.move_blob_opacity, 1.0f, FADE_FAST);
        }
    }
}

void update_hover_state(GameState& state, const RenderConfig& config) {
    state.was_hovering_on_board = state.hover_valid;
    BoardPos new_hover = screen_to_board_perspective(config, state.mouse_pos);
    state.hover_valid = new_hover.is_valid();
    state.hover_pos = new_hover;

    // Update path if unit selected and hovering valid move target
    if (state.hover_valid && state.selected_unit_idx >= 0) {
        update_hover_path(state, config);
    } else if (!state.hover_valid && !state.movement_path.empty()) {
        // Clear path when leaving board
        state.movement_path.clear();
        start_opacity_fade(state, FadeTarget::MoveBlobOpacity,
                          state.move_blob_opacity, 1.0f, FADE_FAST);
    }
}

void update_tile_animations(GameState& state, float dt) {
    // Update tile animations and apply values
    for (auto& anim : state.tile_anims) {
        anim.update(dt);
        float val = anim.current_value();
        switch (anim.target) {
            case FadeTarget::MoveBlobOpacity:
                state.move_blob_opacity = val;
                break;
            case FadeTarget::AttackBlobOpacity:
                state.attack_blob_opacity = val;
                break;
        }
    }

    // Remove completed animations
    state.tile_anims.erase(
        std::remove_if(state.tile_anims.begin(), state.tile_anims.end(),
            [](const TileFadeAnim& a) { return a.elapsed >= a.duration; }),
        state.tile_anims.end());
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
            else if (event.key.key == SDLK_TAB) {
                g_show_settings_menu = !g_show_settings_menu;
            }
            // Lighting preset selection (0-9)
            else if (event.key.key >= SDLK_0 && event.key.key <= SDLK_9) {
                int preset = event.key.key - SDLK_0;
                apply_lighting_preset(preset, config);
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
