#include "grid.hpp"
#include <cmath>

void render_grid(SDL_Renderer* renderer, const RenderConfig& config) {
    int ts = config.tile_size();
    float board_origin_x = (config.window_w - BOARD_COLS * ts) * 0.5f + config.tile_offset_x();
    float board_origin_y = (config.window_h - BOARD_ROWS * ts) * 0.5f + config.tile_offset_y();
    
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    
    for (int x = 0; x <= BOARD_COLS; x++) {
        float px = board_origin_x + x * ts;
        SDL_RenderLine(renderer, px, board_origin_y, px, board_origin_y + BOARD_ROWS * ts);
    }
    
    for (int y = 0; y <= BOARD_ROWS; y++) {
        float py = board_origin_y + y * ts;
        SDL_RenderLine(renderer, board_origin_x, py, board_origin_x + BOARD_COLS * ts, py);
    }
}

void render_tile_highlight(SDL_Renderer* renderer, const RenderConfig& config, BoardPos pos, SDL_Color color) {
    Vec2 screen = board_to_screen(config, pos);
    int ts = config.tile_size();
    
    SDL_FRect rect = {
        screen.x - ts * 0.5f,
        screen.y - ts * 0.5f,
        static_cast<float>(ts),
        static_cast<float>(ts)
    };
    
    SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
    SDL_RenderFillRect(renderer, &rect);
}

void render_move_range(SDL_Renderer* renderer, const RenderConfig& config, BoardPos center, int range) {
    std::vector<BoardPos> empty_occupied;
    auto tiles = get_reachable_tiles(center, range, empty_occupied);
    SDL_Color highlight = {255, 255, 255, 200};
    
    for (const auto& tile : tiles) {
        render_tile_highlight(renderer, config, tile, highlight);
    }
}

void render_attack_range(SDL_Renderer* renderer, const RenderConfig& config, const std::vector<BoardPos>& attackable_tiles) {
    SDL_Color highlight = {255, 100, 100, 200};
    
    for (const auto& tile : attackable_tiles) {
        render_tile_highlight(renderer, config, tile, highlight);
    }
}

std::vector<BoardPos> get_reachable_tiles(BoardPos from, int range, const std::vector<BoardPos>& occupied) {
    std::vector<BoardPos> result;
    
    for (int x = 0; x < BOARD_COLS; x++) {
        for (int y = 0; y < BOARD_ROWS; y++) {
            BoardPos pos{x, y};
            if (pos == from) continue;
            
            int dist = std::abs(x - from.x) + std::abs(y - from.y);
            if (dist <= range) {
                bool is_occupied = false;
                for (const auto& occ : occupied) {
                    if (pos == occ) {
                        is_occupied = true;
                        break;
                    }
                }
                
                if (!is_occupied) {
                    result.push_back(pos);
                }
            }
        }
    }
    
    return result;
}

std::vector<BoardPos> get_attackable_tiles(BoardPos from, int range, const std::vector<BoardPos>& enemy_positions) {
    std::vector<BoardPos> result;
    
    for (const auto& enemy_pos : enemy_positions) {
        int dx = std::abs(enemy_pos.x - from.x);
        int dy = std::abs(enemy_pos.y - from.y);
        int dist = std::max(dx, dy);
        if (dist <= range) {
            result.push_back(enemy_pos);
        }
    }
    
    return result;
}
