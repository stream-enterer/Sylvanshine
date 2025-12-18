#pragma once
#include "types.hpp"
#include "perspective.hpp"
#include <SDL3/SDL.h>
#include <vector>

struct GridRenderer {
    int fb_width;
    int fb_height;
    PerspectiveConfig persp_config;

    bool init(const RenderConfig& config);
    void render(const RenderConfig& config);
    void render_highlight(const RenderConfig& config, BoardPos pos, SDL_FColor color);
    void render_move_range(const RenderConfig& config,
                           BoardPos center, int range, const std::vector<BoardPos>& occupied);
    void render_attack_range(const RenderConfig& config,
                             const std::vector<BoardPos>& attackable_tiles);

private:
    Vec2 transform_board_point(const RenderConfig& config, float board_x, float board_y);
};

std::vector<BoardPos> get_reachable_tiles(BoardPos from, int range, 
                                          const std::vector<BoardPos>& occupied);
std::vector<BoardPos> get_attackable_tiles(BoardPos from, int range,
                                           const std::vector<BoardPos>& enemy_positions);
