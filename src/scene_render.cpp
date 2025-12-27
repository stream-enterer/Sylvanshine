#include "scene_render.hpp"
#include "game_logic.hpp"
#include "settings_menu.hpp"
#include "gpu_renderer.hpp"
#include <algorithm>
#include <numeric>
#include <cmath>

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

// Single-pass rendering (legacy path)
void render_single_pass(GameState& state, const RenderConfig& config) {
    // Render order (back to front) — matches TileZOrder
    // 1. BOARD:             render_floor_grid()
    // 2. MOVE_RANGE:        render_move_range_alpha()
    // 4. ATTACK_RANGE:      render_attack_blob()
    // 5. ATTACKABLE_TARGET: render_attack_reticle()
    // 7. SELECT:            render_select_box()
    // 8. PATH:              render_path()
    // 9. HOVER:             render_hover()

    // 1. Floor grid (semi-transparent dark tiles with gaps)
    state.grid_renderer.render_floor_grid(config);

    // 1b. Ownership indicators for idle enemies (Duelyst: red tile under non-interacted enemies)
    auto is_enemy_idle = [&](size_t idx) -> bool {
        const Entity& enemy = state.units[idx];
        if (enemy.is_dead() || enemy.is_spawning() || enemy.is_moving() || enemy.is_attacking()) {
            return false;
        }
        if (state.turn_phase == TurnPhase::EnemyTurn) {
            return false;
        }
        if (state.hover_valid && enemy.board_pos == state.hover_pos) {
            return false;
        }
        for (const auto& target : state.attackable_tiles) {
            if (target == enemy.board_pos) {
                return false;
            }
        }
        return true;
    };

    for (size_t i = 0; i < state.units.size(); i++) {
        if (state.units[i].type == UnitType::Enemy && is_enemy_idle(i)) {
            state.grid_renderer.render_enemy_indicator(config, state.units[i].board_pos);
        }
    }

    // 2. Grid lines (disabled — using tile gaps instead)
    // state.grid_renderer.render(config);

    // 3. Selection highlights (hide during movement/attack - Duelyst behavior)
    bool unit_is_busy = state.selected_unit_idx >= 0 &&
        (state.units[state.selected_unit_idx].is_moving() ||
         state.units[state.selected_unit_idx].is_attacking());

    if (state.selected_unit_idx >= 0 && state.game_phase == GamePhase::Playing && !unit_is_busy) {
        // Include unit position in blob for visual continuity
        BoardPos unit_pos = state.units[state.selected_unit_idx].board_pos;
        std::vector<BoardPos> blob_tiles = state.reachable_tiles;
        blob_tiles.push_back(unit_pos);

        // Attack blob = attack range EXTENSION beyond movement (Duelyst behavior)
        // Yellow blob shows tiles you could attack that are NOT in your movement range
        // After moving (reachable_tiles empty), there's no "extension" - only show reticles
        const auto& unit = state.units[state.selected_unit_idx];
        std::vector<BoardPos> attack_blob;

        // Only calculate attack blob if movement is available (extension concept)
        if (!state.reachable_tiles.empty()) {
            attack_blob = get_attack_pattern(unit_pos, unit.attack_range);

            // Remove tiles that are in the movement blob (they're already highlighted)
            std::erase_if(attack_blob, [&](const BoardPos& p) {
                return std::find(blob_tiles.begin(), blob_tiles.end(), p) != blob_tiles.end();
            });

            // Remove enemy positions (they get reticles instead of yellow blob)
            std::erase_if(attack_blob, [&](const BoardPos& p) {
                return std::find(state.attackable_tiles.begin(), state.attackable_tiles.end(), p)
                       != state.attackable_tiles.end();
            });
        }

        // Render move blob (z=2) with seam detection
        state.grid_renderer.render_move_range_alpha(config,
            blob_tiles, state.move_blob_opacity, attack_blob);

        // Render attack blob (z=4) only if there's an extension to show
        if (!attack_blob.empty()) {
            state.grid_renderer.render_attack_blob(config, attack_blob, 200.0f/255.0f, blob_tiles);
        }

        // Render attack reticles (z=5) - always show on attackable enemies
        for (const auto& target : state.attackable_tiles) {
            state.grid_renderer.render_attack_reticle(config, target);
        }

        // Selection box at selected unit position
        state.grid_renderer.render_select_box(config, unit_pos);

        // Movement path and destination select box with pulsing
        if (!state.movement_path.empty()) {
            state.grid_renderer.render_path(config, state.movement_path);

            // Glow tile + pulsing select box at path destination
            BoardPos dest = state.movement_path.back();

            // Glow tile (subtle 20% opacity)
            state.grid_renderer.render_glow(config, dest);

            // Select box (bracket corners) with pulsing (0.85-1.0 scale oscillation)
            // scale = 0.85 + 0.15 * (0.5 + 0.5 * cos(phase * 2π))
            float pulse_scale = 0.85f + 0.15f * (0.5f + 0.5f * std::cos(state.target_pulse_phase * 2.0f * M_PI));
            state.grid_renderer.render_select_box(config, dest, pulse_scale);
        }
    }

    // 4. Hover highlight
    if (state.hover_valid) {
        // Enemy attack preview: show attack range when hovering enemy (no unit selected)
        // Shows contiguous blob (enemy position + attack pattern) with hover on top
        if (state.selected_unit_idx < 0 && state.turn_phase == TurnPhase::PlayerTurn) {
            int hovered_idx = find_unit_at_pos(state, state.hover_pos);
            if (hovered_idx >= 0 && state.units[hovered_idx].type == UnitType::Enemy) {
                const Entity& enemy = state.units[hovered_idx];
                auto attack_preview = get_attack_pattern(enemy.board_pos, enemy.attack_range);
                attack_preview.push_back(enemy.board_pos);  // Include enemy position for contiguous blob
                state.grid_renderer.render_attack_blob(config, attack_preview,
                                                        TileOpacity::FULL, {},
                                                        TileColor::ENEMY_ATTACK);
            }
        }
        state.grid_renderer.render_hover(config, state.hover_pos);
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
}

void render(GameState& state, const RenderConfig& config) {
    g_gpu.begin_frame();
    render_single_pass(state, config);

    // Render settings menu overlay if visible
    if (g_show_settings_menu) {
        render_settings_menu(config);
    }

    g_gpu.end_frame();
}
