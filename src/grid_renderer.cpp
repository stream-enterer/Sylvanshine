#include "grid_renderer.hpp"
#include <cmath>

bool GridRenderer::init(SDL_Renderer* renderer, const RenderConfig& config) {
    (void)renderer;
    
    int ts = config.tile_size();
    fb_width = BOARD_COLS * ts;
    fb_height = BOARD_ROWS * ts;
    
    persp_config = PerspectiveConfig::for_board(config);
    
    SDL_Log("Grid renderer initialized: %dx%d board", BOARD_COLS, BOARD_ROWS);
    return true;
}

Vec2 GridRenderer::transform_board_point(const RenderConfig& config, float board_x, float board_y) {
    float board_origin_x = config.board_origin_x();
    float board_origin_y = config.board_origin_y();
    
    Vec2 screen_point = {
        board_origin_x + board_x,
        board_origin_y + board_y
    };
    
    return apply_perspective_transform(screen_point, 0.0f, persp_config);
}

void GridRenderer::render(SDL_Renderer* renderer, const RenderConfig& config) {
    int ts = config.tile_size();
    
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    
    for (int x = 0; x <= BOARD_COLS; x++) {
        float bx = static_cast<float>(x * ts);
        Vec2 top = transform_board_point(config, bx, 0);
        Vec2 bottom = transform_board_point(config, bx, static_cast<float>(BOARD_ROWS * ts));
        SDL_RenderLine(renderer, top.x, top.y, bottom.x, bottom.y);
    }
    
    for (int y = 0; y <= BOARD_ROWS; y++) {
        float by = static_cast<float>(y * ts);
        Vec2 left = transform_board_point(config, 0, by);
        Vec2 right = transform_board_point(config, static_cast<float>(BOARD_COLS * ts), by);
        SDL_RenderLine(renderer, left.x, left.y, right.x, right.y);
    }
}

void GridRenderer::render_highlight(SDL_Renderer* renderer, const RenderConfig& config,
                                    BoardPos pos, SDL_Color color) {
    int ts = config.tile_size();
    
    float tile_x = static_cast<float>(pos.x * ts);
    float tile_y = static_cast<float>(pos.y * ts);
    
    Vec2 tl = transform_board_point(config, tile_x, tile_y);
    Vec2 tr = transform_board_point(config, tile_x + ts, tile_y);
    Vec2 br = transform_board_point(config, tile_x + ts, tile_y + ts);
    Vec2 bl = transform_board_point(config, tile_x, tile_y + ts);
    
    SDL_Vertex vertices[4];
    SDL_FColor fcolor = {
        color.r / 255.0f,
        color.g / 255.0f,
        color.b / 255.0f,
        color.a / 255.0f
    };
    
    vertices[0].position = {tl.x, tl.y};
    vertices[0].color = fcolor;
    vertices[0].tex_coord = {0.0f, 0.0f};
    
    vertices[1].position = {tr.x, tr.y};
    vertices[1].color = fcolor;
    vertices[1].tex_coord = {0.0f, 0.0f};
    
    vertices[2].position = {br.x, br.y};
    vertices[2].color = fcolor;
    vertices[2].tex_coord = {0.0f, 0.0f};
    
    vertices[3].position = {bl.x, bl.y};
    vertices[3].color = fcolor;
    vertices[3].tex_coord = {0.0f, 0.0f};
    
    int indices[6] = {0, 1, 2, 0, 2, 3};
    
    SDL_RenderGeometry(renderer, nullptr, vertices, 4, indices, 6);
}

void GridRenderer::render_move_range(SDL_Renderer* renderer, const RenderConfig& config,
                                     BoardPos center, int range, 
                                     const std::vector<BoardPos>& occupied) {
    auto tiles = get_reachable_tiles(center, range, occupied);
    SDL_Color highlight = {255, 255, 255, 200};
    
    for (const auto& tile : tiles) {
        render_highlight(renderer, config, tile, highlight);
    }
}

void GridRenderer::render_attack_range(SDL_Renderer* renderer, const RenderConfig& config,
                                       const std::vector<BoardPos>& attackable_tiles) {
    SDL_Color highlight = {255, 100, 100, 200};
    
    for (const auto& tile : attackable_tiles) {
        render_highlight(renderer, config, tile, highlight);
    }
}

std::vector<BoardPos> get_reachable_tiles(BoardPos from, int range, 
                                          const std::vector<BoardPos>& occupied) {
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

std::vector<BoardPos> get_attackable_tiles(BoardPos from, int range,
                                           const std::vector<BoardPos>& enemy_positions) {
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
