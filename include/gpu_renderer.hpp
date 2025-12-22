#pragma once
#include "types.hpp"
#include "sdl_handles.hpp"
#include "render_pass.hpp"
#include "lighting.hpp"
#include <SDL3/SDL.h>
#include <string>
#include <unordered_map>
#include <vector>

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

struct ShadowUniforms {
    float opacity;              // External opacity control
    float intensity;            // Duelyst default: 0.15
    float blur_shift_modifier;  // Duelyst default: 1.0
    float blur_intensity_modifier; // Duelyst default: 3.0
    float size_x, size_y;       // Sprite content size in pixels (for distance calc)
    float anchor_x, anchor_y;   // (anchorX * width, shadowOffset) in pixels
    float uv_min_x, uv_min_y;   // UV bounds for clamping blur samples
    float uv_max_x, uv_max_y;   // UV bounds for clamping blur samples
    float render_scale;         // For scale-aware blur
    float light_dist_pct_inv;   // Light distance attenuation: 1 at light center, 0 at radius edge
    float padding1, padding2;   // Alignment padding
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

// Uniforms for post-processing passes
struct HighpassUniforms {
    float threshold;
    float softness;
    float _pad1, _pad2;
};

struct BlurUniforms {
    float direction_x, direction_y;
    float radius;
    float _pad;
};

struct BloomUniforms {
    float transition;
    float intensity;
    float _pad1, _pad2;
};

struct CompositeUniforms {
    float bloom_intensity;
    float bloom_scale;
    float exposure;
    float gamma;
};

struct CopyUniforms {
    float opacity;
    float _pad1, _pad2, _pad3;
};

// Per-sprite shadow uniforms (simpler than atlas version)
struct ShadowPerPassUniforms {
    float opacity;
    float intensity;
    float blur_shift_modifier;
    float blur_intensity_modifier;
    float size_x, size_y;
    float anchor_x, anchor_y;
    float light_dist_pct_inv;
    float render_scale;        // For scale-independent blur
    float _pad1, _pad2;
};

// SDF shadow uniforms (for raymarched soft shadows)
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

// Shadow rendering type
enum class ShadowType {
    Legacy,  // Original box blur shadow
    SDF      // SDF raymarched soft shadow
};

// Radial blur uniforms
struct RadialBlurUniforms {
    float center_x, center_y;
    float strength;
    float samples;
};

// Tone curve uniforms
struct ToneCurveUniforms {
    float intensity;
    float contrast;
    float brightness;
    float saturation;
};

// Vignette uniforms
struct VignetteUniforms {
    float intensity;
    float radius;
    float softness;
    float aspect_ratio;
};

// FX configuration from Duelyst
struct FXConfig {
    // Ambient light
    float ambient_r = 89.0f / 255.0f;
    float ambient_g = 89.0f / 255.0f;
    float ambient_b = 89.0f / 255.0f;

    // Light modifiers
    float falloff_modifier = 1.0f;
    float intensity_modifier = 1.0f;

    // Shadow settings
    float shadow_intensity = 0.35f;  // Was 0.15 (too faint, feet-shadow imperceptible)
    float shadow_blur_shift = 1.0f;
    float shadow_blur_intensity = 3.0f;

    // Bloom settings
    float bloom_threshold = 0.6f;
    float bloom_intensity = 2.5f;
    float bloom_transition = 0.15f;
    float bloom_scale = 0.5f;

    // Post-processing
    float exposure = 1.0f;
    float gamma = 2.2f;

    // Radial blur settings
    float radial_blur_strength = 0.0f;  // 0 = disabled
    float radial_blur_center_x = 0.5f;
    float radial_blur_center_y = 0.5f;
    float radial_blur_samples = 8.0f;

    // Tone curve settings
    float tone_intensity = 1.0f;
    float tone_contrast = 1.0f;
    float tone_brightness = 0.0f;
    float tone_saturation = 1.0f;

    // Vignette settings
    float vignette_intensity = 0.3f;
    float vignette_radius = 0.7f;
    float vignette_softness = 0.4f;

    // Feature toggles
    bool enable_bloom = true;
    bool enable_lighting = true;
    bool enable_shadows = true;
    bool enable_vignette = true;
    bool enable_tone_curve = false;  // Off by default
    bool enable_radial_blur = false; // Off by default (triggered by events)
    bool use_multipass = true;  // Toggle between old single-pass and new multi-pass

    // Shadow type: Legacy (box blur) or SDF (raymarched)
    ShadowType shadow_type = ShadowType::SDF;

    // SDF shadow settings
    float sdf_penumbra_scale = 0.25f;  // Was 1.0→0.5→0.25 (8→4→2 SDF units)
    float sdf_max_raymarch = 0.3f;
    float sdf_raymarch_steps = 12.0f;
};

struct GPURenderer {
    SDL_GPUDevice* device = nullptr;
    SDL_Window* window = nullptr;

    // Original pipelines (single-pass rendering)
    SDL_GPUGraphicsPipeline* sprite_pipeline = nullptr;
    SDL_GPUGraphicsPipeline* dissolve_pipeline = nullptr;
    SDL_GPUGraphicsPipeline* shadow_pipeline = nullptr;
    SDL_GPUGraphicsPipeline* color_pipeline = nullptr;
    SDL_GPUGraphicsPipeline* line_pipeline = nullptr;

    // Multi-pass pipelines
    SDL_GPUGraphicsPipeline* fullscreen_pipeline = nullptr;   // Fullscreen quad rendering
    SDL_GPUGraphicsPipeline* highpass_pipeline = nullptr;     // Bloom extraction
    SDL_GPUGraphicsPipeline* blur_pipeline = nullptr;         // Gaussian blur
    SDL_GPUGraphicsPipeline* bloom_pipeline = nullptr;        // Bloom accumulation
    SDL_GPUGraphicsPipeline* composite_pipeline = nullptr;    // Final surface composite
    SDL_GPUGraphicsPipeline* copy_pipeline = nullptr;         // Simple texture copy
    SDL_GPUGraphicsPipeline* lit_sprite_pipeline = nullptr;   // Sprite with lighting
    SDL_GPUGraphicsPipeline* lighting_pipeline = nullptr;     // Light accumulation
    SDL_GPUGraphicsPipeline* shadow_perpass_pipeline = nullptr; // Per-sprite shadow (UV 0-1)
    SDL_GPUGraphicsPipeline* sdf_shadow_pipeline = nullptr;     // SDF raymarched shadow
    SDL_GPUGraphicsPipeline* radial_blur_pipeline = nullptr;    // Radial/zoom blur
    SDL_GPUGraphicsPipeline* tone_curve_pipeline = nullptr;     // Color grading
    SDL_GPUGraphicsPipeline* vignette_pipeline = nullptr;       // Vignette effect
    SDL_GPUGraphicsPipeline* sprite_offscreen_pipeline = nullptr; // Sprite to R8G8B8A8 FBO

    // Vertex buffers
    SDL_GPUBuffer* quad_vertex_buffer = nullptr;
    SDL_GPUBuffer* shadow_vertex_buffer = nullptr;
    SDL_GPUBuffer* quad_index_buffer = nullptr;
    SDL_GPUSampler* default_sampler = nullptr;
    SDL_GPUSampler* linear_sampler = nullptr;  // Linear filtering for bloom

    // Multi-pass rendering
    PassManager pass_manager;
    RenderContext render_ctx;
    FXConfig fx_config;
    std::vector<PointLight> lights;  // All active lights (legacy)

    // Dynamic lighting system
    LightingManager lighting;

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

    // Shadow rendering with light attenuation
    // If light is nullptr, uses scene_light; if no scene_light, uses default (full brightness)
    void draw_sprite_shadow(
        const GPUTextureHandle& texture,
        const SDL_FRect& src,
        Vec2 feet_pos,
        float scale,
        bool flip_x,
        float opacity,
        const PointLight* light = nullptr
    );

    // Set the primary scene light for shadow calculations
    void set_scene_light(const PointLight& light);
    void clear_scene_light();

    void draw_quad_colored(const SDL_FRect& dst, SDL_FColor color);
    void draw_quad_transformed(Vec2 tl, Vec2 tr, Vec2 br, Vec2 bl, SDL_FColor color);
    void draw_line(Vec2 start, Vec2 end, SDL_FColor color);

    // === Multi-pass rendering API ===

    // Light management
    void add_light(const PointLight& light);
    void clear_lights();

    // Draw sprite to its own FBO for per-sprite effects
    // Returns the RenderPass used (for shadow/lighting passes)
    RenderPass* draw_sprite_to_pass(
        const GPUTextureHandle& texture,
        const SDL_FRect& src,
        Uint32 sprite_w, Uint32 sprite_h
    );

    // Draw shadow from per-sprite FBO (proper UV 0-1)
    // sprite_w/sprite_h are the ACTUAL sprite dimensions (pass may be larger due to pooling)
    void draw_shadow_from_pass(
        RenderPass* sprite_pass,
        Uint32 sprite_w, Uint32 sprite_h,  // Actual sprite dimensions
        Vec2 feet_pos,
        float scale,
        bool flip_x,
        float opacity,
        const PointLight* light = nullptr
    );

    // Draw SDF-based raymarched shadow
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

    // Bloom pipeline execution
    void execute_bloom_pass();

    // Final composite to screen
    void composite_to_screen();

    // Render pass transitions
    void begin_surface_pass();
    void end_surface_pass();

    // Draw fullscreen quad with a pass's texture
    void blit_pass(RenderPass* src, float opacity = 1.0f);

    // === Lighting API ===

    // Add a dynamic light to the scene
    size_t add_dynamic_light(const Light& light);

    // Remove a dynamic light
    void remove_dynamic_light(size_t idx);

    // Clear all dynamic lights
    void clear_dynamic_lights();

    // Update lighting (call once per frame)
    void update_lighting(float dt);

    // Render a sprite with dynamic lighting
    void draw_lit_sprite(
        const GPUTextureHandle& texture,
        const SDL_FRect& src,
        const SDL_FRect& dst,
        bool flip_x,
        float opacity
    );

    // Accumulate light to a sprite's light FBO
    void accumulate_sprite_light(
        RenderPass* sprite_light_pass,
        const GPUTextureHandle& sprite_texture,
        const SDL_FRect& sprite_bounds,
        const Light& light
    );

    // === Post-Processing API ===

    // Execute post-processing pipeline on surface pass
    void execute_post_processing();

    // Individual post-processing effects
    void apply_radial_blur(RenderPass* src, RenderPass* dst);
    void apply_tone_curve(RenderPass* src, RenderPass* dst);
    void apply_vignette(RenderPass* src, RenderPass* dst);

private:
    SDL_GPUCommandBuffer* cmd_buffer = nullptr;
    SDL_GPURenderPass* render_pass = nullptr;
    SDL_GPUTexture* swapchain_texture = nullptr;
    Uint32 swapchain_w = 0;
    Uint32 swapchain_h = 0;

    // Scene lighting
    PointLight scene_light;
    bool has_scene_light = false;

    // Track current rendering target for interrupt/resume
    RenderPass* current_surface_pass = nullptr;  // Non-null during surface rendering

    bool create_pipelines();
    bool create_multipass_pipelines();
    bool create_quad_buffers();
    SDL_GPUShader* load_shader(const char* filename, SDL_GPUShaderStage stage,
                                Uint32 sampler_count, Uint32 uniform_buffer_count);

    // Helper to draw fullscreen triangle (no vertex buffer needed)
    void draw_fullscreen_triangle();

    // Interrupt/resume current render pass for multi-pass operations
    void interrupt_render_pass();
    void resume_render_pass();
};

extern GPURenderer g_gpu;
