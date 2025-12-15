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
    auto tiles = get_reachable_tiles(center, range);
    SDL_Color highlight = {255, 255, 255, 200};
    
    for (const auto& tile : tiles) {
        render_tile_highlight(renderer, config, tile, highlight);
    }
}

std::vector<BoardPos> get_reachable_tiles(BoardPos from, int range) {
    std::vector<BoardPos> result;
    
    for (int x = 0; x < BOARD_COLS; x++) {
        for (int y = 0; y < BOARD_ROWS; y++) {
            BoardPos pos{x, y};
            if (pos == from) continue;
            
            int dist = std::abs(x - from.x) + std::abs(y - from.y);
            if (dist <= range) {
                result.push_back(pos);
            }
        }
    }
    
    return result;
}
