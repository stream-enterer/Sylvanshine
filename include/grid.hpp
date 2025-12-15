#pragma once
#include "types.hpp"
#include <SDL3/SDL.h>
#include <vector>

void render_grid(SDL_Renderer* renderer, const RenderConfig& config);
void render_tile_highlight(SDL_Renderer* renderer, const RenderConfig& config, BoardPos pos, SDL_Color color);
void render_move_range(SDL_Renderer* renderer, const RenderConfig& config, BoardPos center, int range);
std::vector<BoardPos> get_reachable_tiles(BoardPos from, int range, const std::vector<BoardPos>& occupied);
