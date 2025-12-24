#include "grid_renderer.hpp"
#include "gpu_renderer.hpp"
#include <cmath>
#include <queue>
#include <unordered_map>

// Forward declaration for seam detection
static bool needs_seam_at_corner(BoardPos pos, int corner,
                                  const std::vector<BoardPos>& current_blob,
                                  const std::vector<BoardPos>& alt_blob);

bool GridRenderer::init(const RenderConfig& config) {
    int ts = config.tile_size();
    fb_width = BOARD_COLS * ts;
    fb_height = BOARD_ROWS * ts;
    persp_config = PerspectiveConfig::for_board(config);

    std::string prefix = "dist/resources/tiles/s" + std::to_string(config.scale) + "/";

    // Floor and special tiles
    floor_tile = g_gpu.load_texture((prefix + "floor.png").c_str());
    hover_tile = g_gpu.load_texture((prefix + "hover.png").c_str());
    select_box = g_gpu.load_texture((prefix + "select_box.png").c_str());
    glow_tile = g_gpu.load_texture((prefix + "glow.png").c_str());
    target_tile = g_gpu.load_texture((prefix + "target.png").c_str());
    enemy_indicator = g_gpu.load_texture((prefix + "enemy_indicator.png").c_str());
    attack_reticle = g_gpu.load_texture((prefix + "attack_reticle.png").c_str());

    // Corner tiles
    corner_0 = g_gpu.load_texture((prefix + "corner_0.png").c_str());
    corner_01 = g_gpu.load_texture((prefix + "corner_01.png").c_str());
    corner_03 = g_gpu.load_texture((prefix + "corner_03.png").c_str());
    corner_013 = g_gpu.load_texture((prefix + "corner_013.png").c_str());
    corner_0123 = g_gpu.load_texture((prefix + "corner_0123.png").c_str());
    corner_0_seam = g_gpu.load_texture((prefix + "corner_0_seam.png").c_str());
    corner_textures_loaded = corner_0 && corner_01 && corner_03 &&
                              corner_013 && corner_0123 && corner_0_seam;

    // Path tiles
    path_start = g_gpu.load_texture((prefix + "path_start.png").c_str());
    path_straight = g_gpu.load_texture((prefix + "path_straight.png").c_str());
    path_straight_from_start = g_gpu.load_texture((prefix + "path_straight_from_start.png").c_str());
    path_corner = g_gpu.load_texture((prefix + "path_corner.png").c_str());
    path_corner_from_start = g_gpu.load_texture((prefix + "path_corner_from_start.png").c_str());
    path_end = g_gpu.load_texture((prefix + "path_end.png").c_str());
    path_end_from_start = g_gpu.load_texture((prefix + "path_end_from_start.png").c_str());
    path_textures_loaded = path_start && path_straight && path_end;

    SDL_Log("Grid renderer initialized: scale=%d, corners=%s, paths=%s",
            config.scale,
            corner_textures_loaded ? "OK" : "MISSING",
            path_textures_loaded ? "OK" : "MISSING");
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

void GridRenderer::render_floor_grid(const RenderConfig& config) {
    if (!floor_tile) return;

    int ts = config.tile_size();
    SDL_FColor floor_tint = {0.0f, 0.0f, 0.0f, 0.08f};  // 8% opacity black

    for (int y = 0; y < BOARD_ROWS; y++) {
        for (int x = 0; x < BOARD_COLS; x++) {
            float tx = static_cast<float>(x * ts);
            float ty = static_cast<float>(y * ts);

            Vec2 tl = transform_board_point(config, tx, ty);
            Vec2 tr = transform_board_point(config, tx + ts, ty);
            Vec2 br = transform_board_point(config, tx + ts, ty + ts);
            Vec2 bl = transform_board_point(config, tx, ty + ts);

            SDL_FRect src = {0, 0, (float)floor_tile.width, (float)floor_tile.height};
            g_gpu.draw_sprite_transformed(floor_tile, src, tl, tr, br, bl, floor_tint.a);
        }
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
    for (const auto& tile : attackable_tiles) {
        render_highlight(config, tile, TileColor::ATTACK_CURRENT);
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

std::vector<BoardPos> get_path_to(BoardPos start, BoardPos goal,
                                   const std::vector<BoardPos>& blocked) {
    if (start == goal) return {start};

    // BFS with parent tracking
    std::queue<BoardPos> frontier;
    std::unordered_map<int, BoardPos> came_from;  // key = y * BOARD_COLS + x

    auto key = [](BoardPos p) { return p.y * BOARD_COLS + p.x; };
    auto is_blocked = [&](BoardPos p) {
        for (const auto& b : blocked) if (b == p) return true;
        return false;
    };

    frontier.push(start);
    came_from[key(start)] = {-1, -1};

    const BoardPos dirs[] = {{0, -1}, {1, 0}, {0, 1}, {-1, 0}};

    while (!frontier.empty()) {
        BoardPos current = frontier.front();
        frontier.pop();

        if (current == goal) {
            // Reconstruct path
            std::vector<BoardPos> path;
            BoardPos p = goal;
            while (p.x >= 0) {
                path.push_back(p);
                p = came_from[key(p)];
            }
            std::reverse(path.begin(), path.end());
            return path;
        }

        for (const auto& d : dirs) {
            BoardPos next = {current.x + d.x, current.y + d.y};
            if (!next.is_valid()) continue;
            if (is_blocked(next) && next != goal) continue;
            if (came_from.count(key(next))) continue;

            frontier.push(next);
            came_from[key(next)] = current;
        }
    }

    return {};  // No path found
}

void GridRenderer::render_move_range_alpha(const RenderConfig& config,
                                            const std::vector<BoardPos>& tiles,
                                            float opacity,
                                            const std::vector<BoardPos>& alt_blob) {
    if (tiles.empty()) return;

    SDL_FColor color = TileColor::MOVE_CURRENT;
    color.a *= opacity;

    if (!corner_textures_loaded) {
        for (const auto& pos : tiles) {
            render_highlight(config, pos, color);
        }
        return;
    }

    for (const auto& pos : tiles) {
        for (int corner = 0; corner < 4; corner++) {
            CornerNeighbors neighbors = get_corner_neighbors(pos, corner, tiles);
            bool seam = needs_seam_at_corner(pos, corner, tiles, alt_blob);
            const GPUTextureHandle& tex = get_corner_texture(neighbors, seam);
            render_corner_quad_rotated(config, pos, corner, tex, color);
        }
    }
}

const GPUTextureHandle& GridRenderer::get_path_texture(PathSegment seg, bool from_start) {
    switch (seg) {
        case PathSegment::Start:
            return path_start;
        case PathSegment::Straight:
            return (from_start && path_straight_from_start) ? path_straight_from_start : path_straight;
        case PathSegment::Corner:
        case PathSegment::CornerFlipped:
            return (from_start && path_corner_from_start) ? path_corner_from_start : path_corner;
        case PathSegment::End:
            return (from_start && path_end_from_start) ? path_end_from_start : path_end;
        default:
            return path_straight;
    }
}

void GridRenderer::render_path(const RenderConfig& config,
                                const std::vector<BoardPos>& path) {
    if (path.size() < 2) return;

    if (!path_textures_loaded) {
        for (size_t i = 1; i < path.size(); i++) {
            render_highlight(config, path[i], TileColor::PATH);
        }
        return;
    }

    for (size_t i = 0; i < path.size(); i++) {
        PathSegment seg = select_path_segment(path, i);
        bool from_start = (i == 1);  // Second tile uses FromStart variant

        float rotation = 0.0f;
        if (i == 0) {
            rotation = path_segment_rotation(path[0], path[1]);
        } else if (i == path.size() - 1) {
            rotation = path_segment_rotation(path[i-1], path[i]);
            from_start = (path.size() == 2);  // End uses FromStart if path length = 2
        } else {
            rotation = path_segment_rotation(path[i-1], path[i]);
        }

        if (seg == PathSegment::CornerFlipped) {
            rotation += 90.0f;
        }

        const GPUTextureHandle& texture = get_path_texture(seg, from_start);
        if (texture) {
            render_path_segment(config, path[i], texture, rotation);
        }
    }
}

void GridRenderer::render_hover(const RenderConfig& config, BoardPos pos) {
    if (!pos.is_valid()) return;

    if (hover_tile) {
        int ts = config.tile_size();
        float tx = static_cast<float>(pos.x * ts);
        float ty = static_cast<float>(pos.y * ts);

        Vec2 tl = transform_board_point(config, tx, ty);
        Vec2 tr = transform_board_point(config, tx + ts, ty);
        Vec2 br = transform_board_point(config, tx + ts, ty + ts);
        Vec2 bl = transform_board_point(config, tx, ty + ts);

        SDL_FRect src = {0, 0, (float)hover_tile.width, (float)hover_tile.height};
        g_gpu.draw_sprite_transformed(hover_tile, src, tl, tr, br, bl, TileColor::HOVER.a);
    } else {
        render_highlight(config, pos, TileColor::HOVER);
    }
}

void GridRenderer::render_select_box(const RenderConfig& config, BoardPos pos, float pulse_scale) {
    if (!pos.is_valid() || !select_box) return;

    int ts = config.tile_size();

    // Duelyst: tile_box.png is 80x80 on 95px tiles = 84.2% of tile
    // With pulsing: size oscillates between 84.2% and 84.2% * 0.85 = 71.6%
    float base_ratio = 80.0f / 95.0f;  // 0.842
    float box_size = ts * base_ratio * pulse_scale;
    float half_size = box_size * 0.5f;

    // Center of tile
    float cx = (pos.x + 0.5f) * ts;
    float cy = (pos.y + 0.5f) * ts;

    Vec2 tl = transform_board_point(config, cx - half_size, cy - half_size);
    Vec2 tr = transform_board_point(config, cx + half_size, cy - half_size);
    Vec2 br = transform_board_point(config, cx + half_size, cy + half_size);
    Vec2 bl = transform_board_point(config, cx - half_size, cy + half_size);

    SDL_FRect src = {0, 0, (float)select_box.width, (float)select_box.height};
    g_gpu.draw_sprite_transformed(select_box, src, tl, tr, br, bl, 200.0f/255.0f);  // 78% opacity (Duelyst CONFIG.TILE_SELECT_OPACITY)
}

void GridRenderer::render_glow(const RenderConfig& config, BoardPos pos) {
    if (!pos.is_valid() || !glow_tile) return;

    int ts = config.tile_size();
    float tx = static_cast<float>(pos.x * ts);
    float ty = static_cast<float>(pos.y * ts);

    // Duelyst: glow is 90/95 = 0.947 of tile size
    float glow_ratio = 90.0f / 95.0f;
    float inset = ts * (1.0f - glow_ratio) / 2.0f;

    Vec2 tl = transform_board_point(config, tx + inset, ty + inset);
    Vec2 tr = transform_board_point(config, tx + ts - inset, ty + inset);
    Vec2 br = transform_board_point(config, tx + ts - inset, ty + ts - inset);
    Vec2 bl = transform_board_point(config, tx + inset, ty + ts - inset);

    SDL_FRect src = {0, 0, (float)glow_tile.width, (float)glow_tile.height};
    g_gpu.draw_sprite_transformed(glow_tile, src, tl, tr, br, bl, 50.0f/255.0f);  // 20% opacity (Duelyst hardcoded 50)
}

void GridRenderer::render_target(const RenderConfig& config, BoardPos pos, float pulse_scale) {
    if (!pos.is_valid() || !target_tile) return;

    int ts = config.tile_size();
    float tx = static_cast<float>(pos.x * ts);
    float ty = static_cast<float>(pos.y * ts);

    // Duelyst: target is 100/95 = 1.053 of tile size (slightly larger)
    // With pulsing, this scales between 1.0 and 0.85
    float target_ratio = (100.0f / 95.0f) * pulse_scale;
    float half_size = ts * target_ratio * 0.5f;

    // Center of tile
    float cx = tx + ts * 0.5f;
    float cy = ty + ts * 0.5f;

    Vec2 tl = transform_board_point(config, cx - half_size, cy - half_size);
    Vec2 tr = transform_board_point(config, cx + half_size, cy - half_size);
    Vec2 br = transform_board_point(config, cx + half_size, cy + half_size);
    Vec2 bl = transform_board_point(config, cx - half_size, cy + half_size);

    SDL_FRect src = {0, 0, (float)target_tile.width, (float)target_tile.height};
    g_gpu.draw_sprite_transformed(target_tile, src, tl, tr, br, bl, 200.0f/255.0f);  // 78% opacity (Duelyst CONFIG.TARGET_ACTIVE_OPACITY)
}

// =============================================================================
// Path Segment Selection and Rendering
// =============================================================================

PathSegment select_path_segment(const std::vector<BoardPos>& path, size_t idx) {
    if (idx == 0) return PathSegment::Start;
    if (idx == path.size() - 1) return PathSegment::End;

    // Direction vectors
    BoardPos prev = path[idx - 1];
    BoardPos curr = path[idx];
    BoardPos next = path[idx + 1];

    int dx_in = curr.x - prev.x;
    int dy_in = curr.y - prev.y;
    int dx_out = next.x - curr.x;
    int dy_out = next.y - curr.y;

    // Same direction = straight
    if (dx_in == dx_out && dy_in == dy_out) return PathSegment::Straight;

    // Corner — cross product determines flip
    int cross = dx_in * dy_out - dy_in * dx_out;
    return cross > 0 ? PathSegment::Corner : PathSegment::CornerFlipped;
}

float path_segment_rotation(BoardPos from, BoardPos to) {
    int dx = to.x - from.x;
    int dy = to.y - from.y;

    // Return degrees for sprite rotation
    if (dx > 0) return 0.0f;    // Right
    if (dx < 0) return 180.0f;  // Left
    if (dy > 0) return 90.0f;   // Down
    return 270.0f;              // Up
}

// =============================================================================
// Merged Tile Corner System
// =============================================================================

// Check if a corner needs seam texture (boundary between two blobs)
// Duelyst only shows seams at horizontal boundaries (left/right + diagonals),
// not vertical boundaries (top/bottom). See TileLayer.js:251-265.
static bool needs_seam_at_corner(BoardPos pos, int corner,
                                  const std::vector<BoardPos>& current_blob,
                                  const std::vector<BoardPos>& alt_blob) {
    if (alt_blob.empty()) return false;

    // Offset patterns for each corner (same as get_corner_neighbors)
    // Order: [side edge, diagonal, orthogonal edge]
    constexpr int offsets[4][3][2] = {
        {{-1, 0}, {-1,-1}, { 0,-1}},  // TL: left, top-left, top
        {{ 0,-1}, { 1,-1}, { 1, 0}},  // TR: top, top-right, right
        {{ 1, 0}, { 1, 1}, { 0, 1}},  // BR: right, bottom-right, bottom
        {{ 0, 1}, {-1, 1}, {-1, 0}},  // BL: bottom, bottom-left, left
    };

    // Alt-blob check indices per corner (Duelyst checks side + diagonal only):
    // TL, BR: indices 0, 1 (side edge + diagonal)
    // TR, BL: indices 1, 2 (diagonal + side edge)
    constexpr int alt_check[4][2] = {
        {0, 1},  // TL: left, top-left (skip top)
        {1, 2},  // TR: top-right, right (skip top)
        {0, 1},  // BR: right, bottom-right (skip bottom)
        {1, 2},  // BL: bottom-left, left (skip bottom)
    };

    auto in_blob = [](BoardPos p, const std::vector<BoardPos>& blob) {
        return std::find(blob.begin(), blob.end(), p) != blob.end();
    };

    // Check for same-blob neighbor (if found, no seam needed) - all 3 neighbors
    for (int i = 0; i < 3; i++) {
        BoardPos neighbor = {pos.x + offsets[corner][i][0], pos.y + offsets[corner][i][1]};
        if (in_blob(neighbor, current_blob)) return false;
    }

    // Check for alt-blob neighbor (if found, seam needed) - only 2 neighbors per Duelyst
    for (int i = 0; i < 2; i++) {
        int idx = alt_check[corner][i];
        BoardPos neighbor = {pos.x + offsets[corner][idx][0], pos.y + offsets[corner][idx][1]};
        if (in_blob(neighbor, alt_blob)) return true;
    }

    return false;
}

CornerNeighbors get_corner_neighbors(BoardPos pos, int corner,
                                      const std::vector<BoardPos>& blob) {
    // Offset patterns for each corner:
    //   TL(0): left(-1,0), top-left(-1,-1), top(0,-1)
    //   TR(1): top(0,-1), top-right(+1,-1), right(+1,0)
    //   BR(2): right(+1,0), bottom-right(+1,+1), bottom(0,+1)
    //   BL(3): bottom(0,+1), bottom-left(-1,+1), left(-1,0)
    constexpr int offsets[4][3][2] = {
        {{-1, 0}, {-1,-1}, { 0,-1}},  // TL
        {{ 0,-1}, { 1,-1}, { 1, 0}},  // TR
        {{ 1, 0}, { 1, 1}, { 0, 1}},  // BR
        {{ 0, 1}, {-1, 1}, {-1, 0}},  // BL
    };

    auto in_blob = [&](int dx, int dy) {
        BoardPos check = {pos.x + dx, pos.y + dy};
        for (const auto& p : blob) if (p == check) return true;
        return false;
    };

    return {
        in_blob(offsets[corner][0][0], offsets[corner][0][1]),
        in_blob(offsets[corner][1][0], offsets[corner][1][1]),
        in_blob(offsets[corner][2][0], offsets[corner][2][1]),
    };
}

const GPUTextureHandle& GridRenderer::get_corner_texture(CornerNeighbors n, bool use_seam) {
    if (use_seam && corner_0_seam) return corner_0_seam;
    if (!n.edge1 && !n.edge2) return corner_0;
    if (n.edge1 && !n.edge2) return corner_01;
    if (!n.edge1 && n.edge2) return corner_03;
    if (n.edge1 && n.edge2 && !n.diagonal) return corner_013;
    return corner_0123;
}

void GridRenderer::render_corner_quad_rotated(const RenderConfig& config, BoardPos pos,
                                               int corner, const GPUTextureHandle& texture,
                                               SDL_FColor tint) {
    int ts = config.tile_size();
    int half = ts / 2;

    // Corner base position: 0=TL, 1=TR, 2=BR, 3=BL
    float base_x = pos.x * ts + ((corner == 1 || corner == 2) ? half : 0);
    float base_y = pos.y * ts + ((corner == 2 || corner == 3) ? half : 0);

    // Center of quarter-tile
    float cx = base_x + half * 0.5f;
    float cy = base_y + half * 0.5f;

    // Rotation: TL=0°, TR=90°, BR=180°, BL=270°
    float rotation_deg = corner * 90.0f;
    float rad = rotation_deg * (M_PI / 180.0f);
    float cos_r = std::cos(rad);
    float sin_r = std::sin(rad);
    float h = half * 0.5f;

    auto rotate = [&](float rx, float ry) -> Vec2 {
        return {cx + rx * cos_r - ry * sin_r, cy + rx * sin_r + ry * cos_r};
    };

    Vec2 tl_b = rotate(-h, -h);
    Vec2 tr_b = rotate(h, -h);
    Vec2 br_b = rotate(h, h);
    Vec2 bl_b = rotate(-h, h);

    Vec2 tl = transform_board_point(config, tl_b.x, tl_b.y);
    Vec2 tr = transform_board_point(config, tr_b.x, tr_b.y);
    Vec2 br = transform_board_point(config, br_b.x, br_b.y);
    Vec2 bl = transform_board_point(config, bl_b.x, bl_b.y);

    SDL_FRect src = {0, 0, (float)texture.width, (float)texture.height};
    g_gpu.draw_sprite_transformed_tinted(texture, src, tl, tr, br, bl, tint);
}

void GridRenderer::render_path_segment(const RenderConfig& config, BoardPos pos,
                                        const GPUTextureHandle& texture, float rotation_deg) {
    int ts = config.tile_size();
    float tile_x = static_cast<float>(pos.x * ts);
    float tile_y = static_cast<float>(pos.y * ts);

    // Get tile center
    float cx = tile_x + ts * 0.5f;
    float cy = tile_y + ts * 0.5f;

    // Convert rotation to radians
    float rad = rotation_deg * (M_PI / 180.0f);
    float cos_r = std::cos(rad);
    float sin_r = std::sin(rad);

    // Half tile size
    float half = ts * 0.5f;

    // Compute rotated corners around center (before perspective)
    // Original corners relative to center: TL(-half,-half), TR(half,-half), BR(half,half), BL(-half,half)
    auto rotate_point = [&](float rx, float ry) -> Vec2 {
        float rotated_x = rx * cos_r - ry * sin_r;
        float rotated_y = rx * sin_r + ry * cos_r;
        return {cx + rotated_x, cy + rotated_y};
    };

    Vec2 tl_board = rotate_point(-half, -half);
    Vec2 tr_board = rotate_point(half, -half);
    Vec2 br_board = rotate_point(half, half);
    Vec2 bl_board = rotate_point(-half, half);

    // Apply perspective transform to each corner
    Vec2 tl = transform_board_point(config, tl_board.x, tl_board.y);
    Vec2 tr = transform_board_point(config, tr_board.x, tr_board.y);
    Vec2 br = transform_board_point(config, br_board.x, br_board.y);
    Vec2 bl = transform_board_point(config, bl_board.x, bl_board.y);

    // Draw textured quad with the rotated corners
    // Duelyst: CONFIG.PATH_TILE_ACTIVE_OPACITY = 150/255 = 0.588
    SDL_FRect src = {0, 0, static_cast<float>(texture.width), static_cast<float>(texture.height)};
    g_gpu.draw_sprite_transformed(texture, src, tl, tr, br, bl, 150.0f/255.0f);
}

// =============================================================================
// Attack System (Phase 7)
// =============================================================================

std::vector<BoardPos> get_attack_pattern(BoardPos from, int range) {
    std::vector<BoardPos> result;
    for (int x = 0; x < BOARD_COLS; x++) {
        for (int y = 0; y < BOARD_ROWS; y++) {
            if (x == from.x && y == from.y) continue;
            int dist = std::max(std::abs(x - from.x), std::abs(y - from.y));  // Chebyshev
            if (dist <= range) result.push_back({x, y});
        }
    }
    return result;
}

void GridRenderer::render_enemy_indicator(const RenderConfig& config, BoardPos pos) {
    if (!pos.is_valid()) return;
    // Duelyst OPPONENT_OWNER_COLOR: #FF0000 @ 31% opacity (config.js:812-813)
    constexpr SDL_FColor ENEMY_COLOR = {1.0f, 0.0f, 0.0f, 80.0f/255.0f};  // #FF0000, 31%
    render_highlight(config, pos, ENEMY_COLOR);
}

void GridRenderer::render_attack_reticle(const RenderConfig& config, BoardPos pos, float opacity) {
    if (!pos.is_valid() || !attack_reticle) return;

    int ts = config.tile_size();
    float tx = static_cast<float>(pos.x * ts);
    float ty = static_cast<float>(pos.y * ts);

    Vec2 tl = transform_board_point(config, tx, ty);
    Vec2 tr = transform_board_point(config, tx + ts, ty);
    Vec2 br = transform_board_point(config, tx + ts, ty + ts);
    Vec2 bl = transform_board_point(config, tx, ty + ts);

    // Apply yellow tint (Duelyst: CONFIG.AGGRO_COLOR = #FFD900)
    SDL_FColor tint = TileColor::ATTACK_CURRENT;
    tint.a *= opacity;

    SDL_FRect src = {0, 0, (float)attack_reticle.width, (float)attack_reticle.height};
    g_gpu.draw_sprite_transformed_tinted(attack_reticle, src, tl, tr, br, bl, tint);
}

void GridRenderer::render_attack_blob(const RenderConfig& config,
                                       const std::vector<BoardPos>& tiles,
                                       float opacity,
                                       const std::vector<BoardPos>& alt_blob,
                                       SDL_FColor color) {
    if (tiles.empty()) return;

    color.a *= opacity;

    if (!corner_textures_loaded) {
        for (const auto& pos : tiles) {
            render_highlight(config, pos, color);
        }
        return;
    }

    for (const auto& pos : tiles) {
        for (int corner = 0; corner < 4; corner++) {
            CornerNeighbors neighbors = get_corner_neighbors(pos, corner, tiles);
            bool seam = needs_seam_at_corner(pos, corner, tiles, alt_blob);
            const GPUTextureHandle& tex = get_corner_texture(neighbors, seam);
            render_corner_quad_rotated(config, pos, corner, tex, color);
        }
    }
}
