#include "grid_renderer.hpp"
#include "gpu_renderer.hpp"
#include <cmath>

bool GridRenderer::init(const RenderConfig& config) {
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

void GridRenderer::render(const RenderConfig& config) {
    int ts = config.tile_size();

    SDL_FColor white = {1.0f, 1.0f, 1.0f, 1.0f};

    for (int x = 0; x <= BOARD_COLS; x++) {
        float bx = static_cast<float>(x * ts);
        Vec2 top = transform_board_point(config, bx, 0);
        Vec2 bottom = transform_board_point(config, bx, static_cast<float>(BOARD_ROWS * ts));
        g_gpu.draw_line(top, bottom, white);
    }

    for (int y = 0; y <= BOARD_ROWS; y++) {
        float by = static_cast<float>(y * ts);
        Vec2 left = transform_board_point(config, 0, by);
        Vec2 right = transform_board_point(config, static_cast<float>(BOARD_COLS * ts), by);
        g_gpu.draw_line(left, right, white);
    }
}

void GridRenderer::render_highlight(const RenderConfig& config,
                                    BoardPos pos, SDL_FColor color) {
    int ts = config.tile_size();

    float tile_x = static_cast<float>(pos.x * ts);
    float tile_y = static_cast<float>(pos.y * ts);

    Vec2 tl = transform_board_point(config, tile_x, tile_y);
    Vec2 tr = transform_board_point(config, tile_x + ts, tile_y);
    Vec2 br = transform_board_point(config, tile_x + ts, tile_y + ts);
    Vec2 bl = transform_board_point(config, tile_x, tile_y + ts);

    g_gpu.draw_quad_transformed(tl, tr, br, bl, color);
}

void GridRenderer::render_move_range(const RenderConfig& config,
                                     BoardPos center, int range,
                                     const std::vector<BoardPos>& occupied) {
    auto tiles = get_reachable_tiles(center, range, occupied);
    SDL_FColor highlight = {1.0f, 1.0f, 1.0f, 200.0f/255.0f};

    for (const auto& tile : tiles) {
        render_highlight(config, tile, highlight);
    }
}

void GridRenderer::render_attack_range(const RenderConfig& config,
                                       const std::vector<BoardPos>& attackable_tiles) {
    SDL_FColor highlight = {1.0f, 100.0f/255.0f, 100.0f/255.0f, 200.0f/255.0f};

    for (const auto& tile : attackable_tiles) {
        render_highlight(config, tile, highlight);
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
