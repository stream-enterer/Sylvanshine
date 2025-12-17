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

struct GPURenderer {
    SDL_GPUDevice* device = nullptr;
    SDL_Window* window = nullptr;
    SDL_GPUGraphicsPipeline* sprite_pipeline = nullptr;
    SDL_GPUGraphicsPipeline* dissolve_pipeline = nullptr;
    SDL_GPUBuffer* quad_vertex_buffer = nullptr;
    SDL_GPUBuffer* quad_index_buffer = nullptr;
    SDL_GPUSampler* default_sampler = nullptr;

    bool init(SDL_Window* win);
    void shutdown();

    GPUTextureHandle load_texture(const char* filepath);
    GPUTextureHandle create_texture_from_surface(SDL_Surface* surface);

    bool begin_frame();
    void end_frame();

    void draw_sprite(
        GPUTextureHandle& texture,
        const SDL_FRect& src,
        const SDL_FRect& dst,
        bool flip_x,
        float opacity
    );

    void draw_sprite_dissolve(
        GPUTextureHandle& texture,
        const SDL_FRect& src,
        const SDL_FRect& dst,
        bool flip_x,
        float opacity,
        float dissolve_time,
        float seed
    );

    void draw_quad_colored(const SDL_FRect& dst, SDL_FColor color);

private:
    SDL_GPUCommandBuffer* cmd_buffer = nullptr;
    SDL_GPURenderPass* render_pass = nullptr;
    SDL_GPUTexture* swapchain_texture = nullptr;
    Uint32 swapchain_w = 0;
    Uint32 swapchain_h = 0;

    bool create_pipelines();
    bool create_quad_buffers();
    SDL_GPUShader* load_shader(const char* filename, SDL_GPUShaderStage stage,
                                Uint32 sampler_count, Uint32 uniform_buffer_count);
};

extern GPURenderer g_gpu;
