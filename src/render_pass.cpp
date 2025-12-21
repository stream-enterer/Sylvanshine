#include "render_pass.hpp"
#include <SDL3/SDL.h>
#include <algorithm>

// ========== RenderPass Implementation ==========

RenderPass::RenderPass(RenderPass&& o) noexcept
    : name(std::move(o.name)), texture(o.texture), sampler(o.sampler),
      width(o.width), height(o.height), scale(o.scale), format(o.format) {
    o.texture = nullptr;
    o.sampler = nullptr;
}

RenderPass& RenderPass::operator=(RenderPass&& o) noexcept {
    if (this != &o) {
        name = std::move(o.name);
        texture = o.texture;
        sampler = o.sampler;
        width = o.width;
        height = o.height;
        scale = o.scale;
        format = o.format;
        o.texture = nullptr;
        o.sampler = nullptr;
    }
    return *this;
}

bool RenderPass::create(SDL_GPUDevice* device, const std::string& pass_name,
                        Uint32 w, Uint32 h, float scale_factor,
                        SDL_GPUTextureFormat tex_format) {
    name = pass_name;
    scale = scale_factor;
    format = tex_format;
    width = static_cast<Uint32>(w * scale_factor);
    height = static_cast<Uint32>(h * scale_factor);

    // Ensure minimum size
    width = std::max(width, 1u);
    height = std::max(height, 1u);

    // Check if format supports render target usage
    // Some backends don't support all formats as render targets
    SDL_GPUTextureFormat safe_format = format;

    // For render targets, R8G8B8A8_UNORM is universally supported
    // Fall back to it if the requested format might not work
    if (format != SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM &&
        format != SDL_GPU_TEXTUREFORMAT_B8G8R8A8_UNORM) {
        SDL_Log("RenderPass '%s': Using format %d, may need fallback", name.c_str(), (int)format);
    }

    // Create texture that can be used as both render target and sampler
    SDL_GPUTextureCreateInfo tex_info = {};
    tex_info.type = SDL_GPU_TEXTURETYPE_2D;
    tex_info.format = safe_format;
    tex_info.width = width;
    tex_info.height = height;
    tex_info.layer_count_or_depth = 1;
    tex_info.num_levels = 1;
    tex_info.usage = SDL_GPU_TEXTUREUSAGE_COLOR_TARGET | SDL_GPU_TEXTUREUSAGE_SAMPLER;

    texture = SDL_CreateGPUTexture(device, &tex_info);
    if (!texture) {
        SDL_Log("RenderPass '%s': Failed to create texture (fmt=%d, %ux%u): %s",
                name.c_str(), (int)safe_format, width, height, SDL_GetError());
        return false;
    }

    SDL_Log("RenderPass '%s': Created texture %p (fmt=%d, %ux%u)",
            name.c_str(), (void*)texture, (int)safe_format, width, height);

    // Create sampler for reading from this pass
    SDL_GPUSamplerCreateInfo sampler_info = {};
    sampler_info.min_filter = SDL_GPU_FILTER_LINEAR;
    sampler_info.mag_filter = SDL_GPU_FILTER_LINEAR;
    sampler_info.mipmap_mode = SDL_GPU_SAMPLERMIPMAPMODE_NEAREST;
    sampler_info.address_mode_u = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;
    sampler_info.address_mode_v = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;
    sampler_info.address_mode_w = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;

    sampler = SDL_CreateGPUSampler(device, &sampler_info);
    if (!sampler) {
        SDL_Log("RenderPass '%s': Failed to create sampler: %s", name.c_str(), SDL_GetError());
        SDL_ReleaseGPUTexture(device, texture);
        texture = nullptr;
        return false;
    }

    SDL_Log("RenderPass '%s' created: %ux%u (scale %.2f)", name.c_str(), width, height, scale);
    return true;
}

void RenderPass::destroy(SDL_GPUDevice* device) {
    if (sampler) {
        SDL_ReleaseGPUSampler(device, sampler);
        sampler = nullptr;
    }
    if (texture) {
        SDL_ReleaseGPUTexture(device, texture);
        texture = nullptr;
    }
    width = 0;
    height = 0;
}

// ========== PassManager Implementation ==========

static const char* pass_names[] = {
    "cache", "screen", "blurComposite", "surfaceA", "surfaceB",
    "depth", "highpass", "blur", "bloom", "bloomCompositeA",
    "bloomCompositeB", "radialBlur", "toneCurve", "gradientColorMap"
};

static float pass_scales[] = {
    1.0f, 1.0f, 1.0f, 1.0f, 1.0f,  // cache, screen, blurComposite, surfaceA, surfaceB
    1.0f,                          // depth
    0.5f, 0.5f, 0.5f, 0.5f, 0.5f,  // highpass, blur, bloom, bloomCompositeA, bloomCompositeB
    1.0f, 1.0f, 1.0f               // radialBlur, toneCurve, gradientColorMap
};

bool PassManager::init(SDL_GPUDevice* dev, Uint32 width, Uint32 height, SDL_GPUTextureFormat swap_format) {
    device = dev;
    screen_width = width;
    screen_height = height;
    swapchain_format = swap_format;

    passes.resize(static_cast<size_t>(PassType::COUNT));

    for (size_t i = 0; i < static_cast<size_t>(PassType::COUNT); i++) {
        // Use swapchain format for surface passes (where we'll use sprite_pipeline)
        // Use R8G8B8A8_UNORM for bloom/post-processing passes (use fullscreen pipelines)
        SDL_GPUTextureFormat pass_format = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM;
        PassType pass_type = static_cast<PassType>(i);
        if (pass_type == PassType::SurfaceA || pass_type == PassType::SurfaceB ||
            pass_type == PassType::Screen || pass_type == PassType::Cache ||
            pass_type == PassType::BlurComposite) {
            pass_format = swapchain_format;
        }

        if (!passes[i].create(device, pass_names[i], width, height, pass_scales[i], pass_format)) {
            SDL_Log("PassManager: Failed to create pass '%s'", pass_names[i]);
            shutdown();
            return false;
        }
    }

    SDL_Log("PassManager: Created %zu render passes (swapchain format: %d)", passes.size(), (int)swapchain_format);
    return true;
}

void PassManager::shutdown() {
    for (auto& pass : passes) {
        pass.destroy(device);
    }
    passes.clear();

    for (auto& pass : sprite_shadow_passes) {
        pass.destroy(device);
    }
    sprite_shadow_passes.clear();

    for (auto& pass : sprite_light_passes) {
        pass.destroy(device);
    }
    sprite_light_passes.clear();

    device = nullptr;
}

void PassManager::resize(Uint32 width, Uint32 height) {
    if (width == screen_width && height == screen_height) return;

    screen_width = width;
    screen_height = height;

    // Recreate all passes at new size
    for (size_t i = 0; i < passes.size(); i++) {
        passes[i].destroy(device);
        passes[i].create(device, pass_names[i], width, height, pass_scales[i]);
    }

    // Clear sprite pass pools (they'll be recreated on demand)
    for (auto& pass : sprite_shadow_passes) {
        pass.destroy(device);
    }
    sprite_shadow_passes.clear();

    for (auto& pass : sprite_light_passes) {
        pass.destroy(device);
    }
    sprite_light_passes.clear();

    SDL_Log("PassManager: Resized to %ux%u", width, height);
}

RenderPass* PassManager::get(PassType type) {
    size_t idx = static_cast<size_t>(type);
    if (idx < passes.size()) {
        return &passes[idx];
    }
    return nullptr;
}

const RenderPass* PassManager::get(PassType type) const {
    size_t idx = static_cast<size_t>(type);
    if (idx < passes.size()) {
        return &passes[idx];
    }
    return nullptr;
}

RenderPass* PassManager::acquire_sprite_shadow_pass(Uint32 sprite_w, Uint32 sprite_h) {
    // Pool strategy: reuse passes if they exist, otherwise create new ones
    if (shadow_pass_pool_idx < sprite_shadow_passes.size()) {
        RenderPass* pass = &sprite_shadow_passes[shadow_pass_pool_idx++];
        // Check if size matches (allow reuse if pass is large enough)
        if (pass->width >= sprite_w && pass->height >= sprite_h) {
            return pass;
        }
        // Need larger pass - destroy and recreate
        pass->destroy(device);
        char name[64];
        SDL_snprintf(name, sizeof(name), "spriteShadow_%zu", shadow_pass_pool_idx - 1);
        pass->create(device, name, sprite_w, sprite_h, 1.0f);
        return pass;
    }

    // Create new pass
    RenderPass new_pass;
    char name[64];
    SDL_snprintf(name, sizeof(name), "spriteShadow_%zu", sprite_shadow_passes.size());
    if (!new_pass.create(device, name, sprite_w, sprite_h, 1.0f)) {
        return nullptr;
    }
    sprite_shadow_passes.push_back(std::move(new_pass));
    shadow_pass_pool_idx++;
    return &sprite_shadow_passes.back();
}

RenderPass* PassManager::acquire_sprite_light_pass(Uint32 sprite_w, Uint32 sprite_h) {
    // Same pooling strategy as shadow passes
    if (light_pass_pool_idx < sprite_light_passes.size()) {
        RenderPass* pass = &sprite_light_passes[light_pass_pool_idx++];
        if (pass->width >= sprite_w && pass->height >= sprite_h) {
            return pass;
        }
        pass->destroy(device);
        char name[64];
        SDL_snprintf(name, sizeof(name), "spriteLight_%zu", light_pass_pool_idx - 1);
        pass->create(device, name, sprite_w, sprite_h, 1.0f);
        return pass;
    }

    RenderPass new_pass;
    char name[64];
    SDL_snprintf(name, sizeof(name), "spriteLight_%zu", sprite_light_passes.size());
    if (!new_pass.create(device, name, sprite_w, sprite_h, 1.0f)) {
        return nullptr;
    }
    sprite_light_passes.push_back(std::move(new_pass));
    light_pass_pool_idx++;
    return &sprite_light_passes.back();
}

void PassManager::reset_sprite_pass_pools() {
    shadow_pass_pool_idx = 0;
    light_pass_pool_idx = 0;
}

// ========== RenderContext Implementation ==========

bool RenderContext::begin_pass(RenderPass* pass, float clear_r, float clear_g,
                               float clear_b, float clear_a, bool clear) {
    if (!pass) {
        SDL_Log("RenderContext: pass is null");
        return false;
    }
    if (!pass->is_valid()) {
        SDL_Log("RenderContext: pass '%s' is not valid (texture=%p, w=%u, h=%u)",
                pass->name.c_str(), (void*)pass->texture, pass->width, pass->height);
        return false;
    }

    if (!cmd_buffer) {
        SDL_Log("RenderContext: cmd_buffer is null");
        return false;
    }

    // End previous pass if active
    if (render_pass) {
        end_pass();
    }

    current_target = pass->get_texture();
    if (!current_target) {
        SDL_Log("RenderContext: pass '%s' texture is null", pass->name.c_str());
        return false;
    }

    target_width = pass->width;
    target_height = pass->height;

    SDL_GPUColorTargetInfo color_target = {};
    color_target.texture = current_target;
    color_target.load_op = clear ? SDL_GPU_LOADOP_CLEAR : SDL_GPU_LOADOP_LOAD;
    color_target.store_op = SDL_GPU_STOREOP_STORE;
    color_target.clear_color = {clear_r, clear_g, clear_b, clear_a};

    render_pass = SDL_BeginGPURenderPass(cmd_buffer, &color_target, 1, nullptr);
    if (!render_pass) {
        SDL_Log("RenderContext: Failed to begin pass '%s' (%ux%u fmt=%d): %s",
                pass->name.c_str(), pass->width, pass->height, (int)pass->format, SDL_GetError());
        return false;
    }

    set_full_viewport();
    return true;
}

bool RenderContext::begin_swapchain(SDL_GPUTexture* swapchain_tex, Uint32 w, Uint32 h,
                                    float clear_r, float clear_g,
                                    float clear_b, float clear_a, bool clear) {
    if (!swapchain_tex) {
        return false;
    }

    if (render_pass) {
        end_pass();
    }

    current_target = swapchain_tex;
    target_width = w;
    target_height = h;

    SDL_GPUColorTargetInfo color_target = {};
    color_target.texture = current_target;
    color_target.load_op = clear ? SDL_GPU_LOADOP_CLEAR : SDL_GPU_LOADOP_LOAD;
    color_target.store_op = SDL_GPU_STOREOP_STORE;
    color_target.clear_color = {clear_r, clear_g, clear_b, clear_a};

    render_pass = SDL_BeginGPURenderPass(cmd_buffer, &color_target, 1, nullptr);
    if (!render_pass) {
        return false;
    }

    set_full_viewport();
    return true;
}

void RenderContext::end_pass() {
    if (render_pass) {
        SDL_EndGPURenderPass(render_pass);
        render_pass = nullptr;
    }
    current_target = nullptr;
}

bool RenderContext::transition_to(RenderPass* pass, bool clear) {
    end_pass();
    return begin_pass(pass);
}

void RenderContext::set_full_viewport() {
    if (!render_pass) return;

    SDL_GPUViewport viewport = {
        0, 0,
        static_cast<float>(target_width),
        static_cast<float>(target_height),
        0.0f, 1.0f
    };
    SDL_SetGPUViewport(render_pass, &viewport);

    SDL_Rect scissor = {0, 0, static_cast<int>(target_width), static_cast<int>(target_height)};
    SDL_SetGPUScissor(render_pass, &scissor);
}
