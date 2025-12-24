#pragma once
#include "types.hpp"
#include "sdl_handles.hpp"
#include <SDL3/SDL.h>
#include <string>
#include <unordered_map>

struct SpriteVertex {
    float x, y;
    float u, v;
};

struct ShadowVertex {
    float x, y;      // NDC position
    float u, v;      // Texture coordinates
    float lx, ly;    // Local sprite position in PIXELS (for Duelyst-style blur calculation)
};

struct ColorVertex {
    float x, y;
    float r, g, b, a;
};

// Point light source for dynamic lighting
struct PointLight {
    float x, y;          // Screen position
    float radius;        // Light radius in pixels
    float intensity;     // Light intensity (higher dims lower intensity lights)
    float r, g, b, a;    // Light color (alpha used for effective intensity)
    bool casts_shadows;  // Whether this light affects shadow rendering

    PointLight() : x(0), y(0), radius(285.0f), intensity(1.0f),
                   r(1), g(1), b(1), a(1), casts_shadows(true) {}
};

struct SpriteUniforms {
    float opacity;
    float dissolve_time;
    float seed;
    float padding;
};


struct GPUTextureHandle {
    SDL_GPUTexture* ptr = nullptr;
    SDL_GPUSampler* sampler = nullptr;
    int width = 0;
    int height = 0;

    GPUTextureHandle() = default;
    ~GPUTextureHandle();
    GPUTextureHandle(GPUTextureHandle&& o) noexcept;
    GPUTextureHandle& operator=(GPUTextureHandle&& o) noexcept;
    GPUTextureHandle(const GPUTextureHandle&) = delete;
    GPUTextureHandle& operator=(const GPUTextureHandle&) = delete;

    explicit operator bool() const { return ptr != nullptr; }
};

// SDF shadow uniforms
struct SDFShadowUniforms {
    float opacity;              // External opacity control
    float intensity;            // Shadow darkness (default: 0.15)
    float penumbra_scale;       // Penumbra softness multiplier (default: 1.0)
    float sdf_max_dist;         // SDF max distance for decoding (default: 32.0)

    float sprite_size_x, sprite_size_y;  // Sprite dimensions in pixels
    float anchor_x, anchor_y;            // Anchor point in pixels (shadow pivot)

    float light_dir_x, light_dir_y;      // Normalized 2D direction toward light (UV space)
    float light_distance;                // Distance to light (affects penumbra)
    float light_intensity;               // Light contribution to shadow

    float max_raymarch;         // Max raymarch distance in UV space (default: 0.3)
    float raymarch_steps;       // Number of raymarch steps (default: 12.0)
    float _pad1, _pad2;         // Alignment padding
};

// FX configuration - shadow settings
struct FXConfig {
    // Shadow settings
    float shadow_intensity = 0.35f;
    bool enable_shadows = true;

    // SDF shadow settings
    float sdf_penumbra_scale = 0.25f;
    float sdf_max_raymarch = 0.3f;      // Unused but kept for shader struct compatibility
    float sdf_raymarch_steps = 12.0f;   // Unused but kept for shader struct compatibility
};

struct GPURenderer {
    SDL_GPUDevice* device = nullptr;
    SDL_Window* window = nullptr;

    // Core pipelines
    SDL_GPUGraphicsPipeline* sprite_pipeline = nullptr;
    SDL_GPUGraphicsPipeline* dissolve_pipeline = nullptr;
    SDL_GPUGraphicsPipeline* color_pipeline = nullptr;
    SDL_GPUGraphicsPipeline* line_pipeline = nullptr;
    SDL_GPUGraphicsPipeline* sdf_shadow_pipeline = nullptr;

    // Vertex buffers
    SDL_GPUBuffer* quad_vertex_buffer = nullptr;
    SDL_GPUBuffer* shadow_vertex_buffer = nullptr;
    SDL_GPUBuffer* quad_index_buffer = nullptr;
    SDL_GPUSampler* default_sampler = nullptr;
    SDL_GPUSampler* linear_sampler = nullptr;  // Linear filtering for bloom

    // Configuration
    FXConfig fx_config;

    bool init(SDL_Window* win);
    void shutdown();

    GPUTextureHandle load_texture(const char* filepath);
    GPUTextureHandle create_texture_from_surface(SDL_Surface* surface);

    bool begin_frame();
    void end_frame();

    void draw_sprite(
        const GPUTextureHandle& texture,
        const SDL_FRect& src,
        const SDL_FRect& dst,
        bool flip_x,
        float opacity
    );

    void draw_sprite_dissolve(
        const GPUTextureHandle& texture,
        const SDL_FRect& src,
        const SDL_FRect& dst,
        bool flip_x,
        float opacity,
        float dissolve_time,
        float seed
    );

    // Set the primary scene light for shadow calculations
    void set_scene_light(const PointLight& light);
    void clear_scene_light();

    void draw_quad_colored(const SDL_FRect& dst, SDL_FColor color);
    void draw_quad_transformed(Vec2 tl, Vec2 tr, Vec2 br, Vec2 bl, SDL_FColor color);
    void draw_line(Vec2 start, Vec2 end, SDL_FColor color);

    // Draw sprite with arbitrary quad corners (for rotation)
    void draw_sprite_transformed(
        const GPUTextureHandle& texture,
        const SDL_FRect& src,
        Vec2 tl, Vec2 tr, Vec2 br, Vec2 bl,
        float opacity
    );

    // SDF shadow rendering
    // sdf_texture is the pre-computed SDF atlas
    // src is the frame rect in the atlas (same as spritesheet)
    void draw_sdf_shadow(
        const GPUTextureHandle& sdf_texture,
        const SDL_FRect& src,
        Vec2 feet_pos,
        float scale,
        bool flip_x,
        float opacity,
        const PointLight* light = nullptr
    );

private:
    SDL_GPUCommandBuffer* cmd_buffer = nullptr;
    SDL_GPURenderPass* render_pass = nullptr;
    SDL_GPUTexture* swapchain_texture = nullptr;
    Uint32 swapchain_w = 0;
    Uint32 swapchain_h = 0;

    // Scene lighting
    PointLight scene_light;
    bool has_scene_light = false;

    bool create_pipelines();
    bool create_quad_buffers();
    SDL_GPUShader* load_shader(const char* filename, SDL_GPUShaderStage stage,
                                Uint32 sampler_count, Uint32 uniform_buffer_count);

    // Interrupt/resume current render pass for buffer uploads
    void interrupt_render_pass();
    void resume_render_pass();
};

extern GPURenderer g_gpu;
