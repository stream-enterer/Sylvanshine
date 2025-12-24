#pragma once
#include "types.hpp"
#include "perspective.hpp"
#include "gpu_renderer.hpp"
#include <SDL3/SDL.h>
#include <vector>
#include <algorithm>

// Target identifiers for fade animations (avoids raw pointers)
enum class FadeTarget {
    MoveBlobOpacity,
    AttackBlobOpacity,
};

struct TileFadeAnim {
    FadeTarget target;
    float from, to;
    float duration;
    float elapsed = 0.0f;

    bool update(float dt) {
        elapsed += dt;
        return elapsed >= duration;
    }

    float current_value() const {
        float t = std::min(elapsed / duration, 1.0f);
        return from + (to - from) * t;
    }
};

namespace TileColor {
    // Current Sylvanshine colors (keep for compatibility)
    constexpr SDL_FColor MOVE_CURRENT   = {1.0f, 1.0f, 1.0f, 200.0f/255.0f};
    constexpr SDL_FColor ATTACK_CURRENT = {1.0f, 100.0f/255.0f, 100.0f/255.0f, 200.0f/255.0f};

    // Duelyst colors (for future use)
    constexpr SDL_FColor MOVE_DUELYST   = {0.941f, 0.941f, 0.941f, 1.0f};  // #F0F0F0
    constexpr SDL_FColor AGGRO_DUELYST  = {1.0f, 0.851f, 0.0f, 1.0f};      // #FFD900 yellow!

    // Path and hover
    constexpr SDL_FColor PATH  = {1.0f, 1.0f, 1.0f, 150.0f/255.0f};
    constexpr SDL_FColor HOVER = {1.0f, 1.0f, 1.0f, 200.0f/255.0f};
}

namespace TileOpacity {
    constexpr float FULL  = 200.0f / 255.0f;  // 0.784 — Selected/hover
    constexpr float DIM   = 127.0f / 255.0f;  // 0.498 — Blob during hover
    constexpr float FAINT = 75.0f / 255.0f;   // 0.294 — Passive hover
}

// Path segment types for directional path rendering
enum class PathSegment {
    Start,              // First tile (unit position)
    Straight,           // Middle, no turn
    Corner,             // 90° turn (clockwise)
    CornerFlipped,      // 90° turn (counter-clockwise)
    End,                // Final tile (arrow head)
};

// Neighbor state for a corner
struct CornerNeighbors {
    bool edge1;     // Adjacent on first edge
    bool diagonal;  // Diagonally adjacent
    bool edge2;     // Adjacent on second edge
};

// Get neighbor state for a specific corner of a tile
CornerNeighbors get_corner_neighbors(BoardPos pos, int corner,
                                      const std::vector<BoardPos>& blob);

// Determine path segment type based on neighbors
PathSegment select_path_segment(const std::vector<BoardPos>& path, size_t idx);

// Get rotation in degrees for a path segment based on direction
float path_segment_rotation(BoardPos from, BoardPos to);

struct GridRenderer {
    int fb_width;
    int fb_height;
    PerspectiveConfig persp_config;

    // Scale-specific textures (pre-scaled, no runtime scaling)
    GPUTextureHandle floor_tile;
    GPUTextureHandle hover_tile;
    GPUTextureHandle select_box;  // Selection/targeting reticle (bracket corners)

    // Corner textures (quarter tile size)
    GPUTextureHandle corner_0;
    GPUTextureHandle corner_01;
    GPUTextureHandle corner_03;
    GPUTextureHandle corner_013;
    GPUTextureHandle corner_0123;
    bool corner_textures_loaded = false;

    // Path textures (tile size)
    GPUTextureHandle path_start;
    GPUTextureHandle path_straight;
    GPUTextureHandle path_straight_from_start;
    GPUTextureHandle path_corner;
    GPUTextureHandle path_corner_from_start;
    GPUTextureHandle path_end;
    GPUTextureHandle path_end_from_start;
    bool path_textures_loaded = false;

    bool init(const RenderConfig& config);
    void render(const RenderConfig& config);
    void render_floor_grid(const RenderConfig& config);
    void render_highlight(const RenderConfig& config, BoardPos pos, SDL_FColor color);
    void render_move_range(const RenderConfig& config,
                           BoardPos center, int range, const std::vector<BoardPos>& occupied);
    void render_attack_range(const RenderConfig& config,
                             const std::vector<BoardPos>& attackable_tiles);

    // Render move range with opacity
    void render_move_range_alpha(const RenderConfig& config,
                                  const std::vector<BoardPos>& tiles,
                                  float opacity);

    // Render movement path
    void render_path(const RenderConfig& config,
                     const std::vector<BoardPos>& path);

    // Render hover highlight
    void render_hover(const RenderConfig& config, BoardPos pos);

    // Render selection box (bracket-corner reticle) at a position
    void render_select_box(const RenderConfig& config, BoardPos pos);

private:
    Vec2 transform_board_point(const RenderConfig& config, float board_x, float board_y);

    // Render a path segment with rotation
    void render_path_segment(const RenderConfig& config, BoardPos pos,
                             const GPUTextureHandle& texture, float rotation_deg);

    // Render a corner quad with rotation (TL=0°, TR=90°, BR=180°, BL=270°)
    void render_corner_quad_rotated(const RenderConfig& config, BoardPos pos, int corner,
                                     const GPUTextureHandle& texture, SDL_FColor tint);

    // Get corner texture based on neighbor state
    const GPUTextureHandle& get_corner_texture(CornerNeighbors n);

    // Get path texture based on segment type
    const GPUTextureHandle& get_path_texture(PathSegment seg, bool from_start);
};

std::vector<BoardPos> get_reachable_tiles(BoardPos from, int range,
                                          const std::vector<BoardPos>& occupied);
std::vector<BoardPos> get_attackable_tiles(BoardPos from, int range,
                                           const std::vector<BoardPos>& enemy_positions);

// BFS pathfinding from start to goal, avoiding blocked tiles
std::vector<BoardPos> get_path_to(BoardPos start, BoardPos goal,
                                   const std::vector<BoardPos>& blocked);
