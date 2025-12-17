#pragma once
#include "types.hpp"
#include "perspective.hpp"
#include <SDL3/SDL.h>
#include <vector>

struct GridRenderer {
    int fb_width;
    int fb_height;
    PerspectiveConfig persp_config;
    
    bool init(SDL_Renderer* renderer, const RenderConfig& config);
    void render(SDL_Renderer* renderer, const RenderConfig& config);
    void render_highlight(SDL_Renderer* renderer, const RenderConfig& config, 
                          BoardPos pos, SDL_Color color);
    void render_move_range(SDL_Renderer* renderer, const RenderConfig& config,
                           BoardPos center, int range, const std::vector<BoardPos>& occupied);
    void render_attack_range(SDL_Renderer* renderer, const RenderConfig& config,
                             const std::vector<BoardPos>& attackable_tiles);
    
private:
    Vec2 transform_board_point(const RenderConfig& config, float board_x, float board_y);
};

std::vector<BoardPos> get_reachable_tiles(BoardPos from, int range, 
                                          const std::vector<BoardPos>& occupied);
std::vector<BoardPos> get_attackable_tiles(BoardPos from, int range,
                                           const std::vector<BoardPos>& enemy_positions);
