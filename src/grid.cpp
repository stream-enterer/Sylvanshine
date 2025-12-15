#include "grid.hpp"
#include <cmath>

void render_grid(SDL_Renderer* renderer, int window_w, int window_h) {
    float board_origin_x = (window_w - BOARD_COLS * TILE_SIZE) * 0.5f + TILE_OFFSET_X;
    float board_origin_y = (window_h - BOARD_ROWS * TILE_SIZE) * 0.5f + TILE_OFFSET_Y;
    
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    
    for (int x = 0; x <= BOARD_COLS; x++) {
        float px = board_origin_x + x * TILE_SIZE;
        SDL_RenderLine(renderer, px, board_origin_y, px, board_origin_y + BOARD_ROWS * TILE_SIZE);
    }
    
    for (int y = 0; y <= BOARD_ROWS; y++) {
        float py = board_origin_y + y * TILE_SIZE;
        SDL_RenderLine(renderer, board_origin_x, py, board_origin_x + BOARD_COLS * TILE_SIZE, py);
    }
}

void render_tile_highlight(SDL_Renderer* renderer, int window_w, int window_h, BoardPos pos, SDL_Color color) {
    Vec2 screen = board_to_screen(window_w, window_h, pos);
    
    SDL_FRect rect = {
        screen.x - TILE_SIZE * 0.5f,
        screen.y - TILE_SIZE * 0.5f,
        static_cast<float>(TILE_SIZE),
        static_cast<float>(TILE_SIZE)
    };
    
    SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
    SDL_RenderFillRect(renderer, &rect);
}

void render_move_range(SDL_Renderer* renderer, int window_w, int window_h, BoardPos center, int range) {
    auto tiles = get_reachable_tiles(center, range);
    SDL_Color highlight = {255, 255, 255, 200};
    
    for (const auto& tile : tiles) {
        render_tile_highlight(renderer, window_w, window_h, tile, highlight);
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
