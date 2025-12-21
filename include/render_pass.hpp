#pragma once
#include <SDL3/SDL.h>
#include <vector>
#include <string>
#include <unordered_map>

// Forward declaration
struct GPURenderer;

// RenderPass: FBO wrapper following Duelyst's RenderPass.js pattern
// Provides framebuffer + texture creation, viewport management, and begin/end semantics
struct RenderPass {
    std::string name;
    SDL_GPUTexture* texture = nullptr;
    SDL_GPUSampler* sampler = nullptr;
    Uint32 width = 0;
    Uint32 height = 0;
    float scale = 1.0f;           // Scale relative to screen (0.5 for bloom passes)
    SDL_GPUTextureFormat format = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM;

    // Lifecycle
    bool create(SDL_GPUDevice* device, const std::string& pass_name,
                Uint32 w, Uint32 h, float scale_factor = 1.0f,
                SDL_GPUTextureFormat tex_format = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM);
    void destroy(SDL_GPUDevice* device);

    // Query
    bool is_valid() const { return texture != nullptr; }
    SDL_GPUTexture* get_texture() const { return texture; }
    SDL_GPUSampler* get_sampler() const { return sampler; }

    RenderPass() = default;
    ~RenderPass() = default;

    // Non-copyable, movable
    RenderPass(const RenderPass&) = delete;
    RenderPass& operator=(const RenderPass&) = delete;
    RenderPass(RenderPass&& o) noexcept;
    RenderPass& operator=(RenderPass&& o) noexcept;
};

// All named render passes (14 from Duelyst spec)
enum class PassType {
    Cache,              // Cached screen contents
    Screen,             // Main screen buffer
    BlurComposite,      // Blur composite
    SurfaceA,           // Surface double-buffer A
    SurfaceB,           // Surface double-buffer B
    Depth,              // Depth buffer (packed RGBA)
    Highpass,           // Bloom extraction (0.5 scale)
    Blur,               // Bloom blur (0.5 scale)
    Bloom,              // Bloom accumulation (0.5 scale)
    BloomCompositeA,    // Bloom double-buffer A (0.5 scale)
    BloomCompositeB,    // Bloom double-buffer B (0.5 scale)
    RadialBlur,         // Screen radial blur
    ToneCurve,          // Color grading
    GradientColorMap,   // Color mapping
    COUNT
};

// PassManager: Creates and manages all render passes
struct PassManager {
    std::vector<RenderPass> passes;
    SDL_GPUDevice* device = nullptr;
    Uint32 screen_width = 0;
    Uint32 screen_height = 0;
    SDL_GPUTextureFormat swapchain_format = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM;

    // Dynamic per-sprite passes (created on demand)
    std::vector<RenderPass> sprite_shadow_passes;
    std::vector<RenderPass> sprite_light_passes;
    size_t shadow_pass_pool_idx = 0;
    size_t light_pass_pool_idx = 0;

    bool init(SDL_GPUDevice* dev, Uint32 width, Uint32 height, SDL_GPUTextureFormat swap_format);
    void shutdown();
    void resize(Uint32 width, Uint32 height);

    // Access named passes
    RenderPass* get(PassType type);
    const RenderPass* get(PassType type) const;

    // Get or create a per-sprite shadow pass (pooled for reuse)
    RenderPass* acquire_sprite_shadow_pass(Uint32 sprite_w, Uint32 sprite_h);
    RenderPass* acquire_sprite_light_pass(Uint32 sprite_w, Uint32 sprite_h);

    // Reset pool indices for next frame
    void reset_sprite_pass_pools();
};

// Rendering context for multi-pass rendering
// Tracks current state and provides helpers for pass transitions
struct RenderContext {
    SDL_GPUDevice* device = nullptr;
    SDL_GPUCommandBuffer* cmd_buffer = nullptr;
    SDL_GPURenderPass* render_pass = nullptr;
    SDL_GPUTexture* current_target = nullptr;
    Uint32 target_width = 0;
    Uint32 target_height = 0;

    // Begin rendering to a specific pass
    bool begin_pass(RenderPass* pass, float clear_r = 0.0f, float clear_g = 0.0f,
                   float clear_b = 0.0f, float clear_a = 0.0f, bool clear = true);

    // Begin rendering to swapchain
    bool begin_swapchain(SDL_GPUTexture* swapchain_tex, Uint32 w, Uint32 h,
                        float clear_r = 0.0f, float clear_g = 0.0f,
                        float clear_b = 0.0f, float clear_a = 1.0f, bool clear = true);

    // End current pass
    void end_pass();

    // Check if we're in an active pass
    bool is_active() const { return render_pass != nullptr; }

    // Transition: end current pass and begin new one
    bool transition_to(RenderPass* pass, bool clear = true);

    // Set viewport to full target
    void set_full_viewport();
};

// Blend modes for compositing
enum class BlendMode {
    Alpha,      // Standard alpha blending
    Additive,   // Additive blending for lights
    Multiply,   // Multiply for lighting
    Replace     // No blending (overwrite)
};
