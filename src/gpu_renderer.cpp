#include "gpu_renderer.hpp"
#include "types.hpp"  // For SHADOW_OFFSET
#include <SDL3_image/SDL_image.h>
#include <algorithm>
#include <cmath>
#include <cstring>
#include <vector>

GPURenderer g_gpu;

GPUTextureHandle::~GPUTextureHandle() {
    if (ptr && g_gpu.device) {
        SDL_WaitForGPUIdle(g_gpu.device);
        SDL_ReleaseGPUTexture(g_gpu.device, ptr);
    }
}

GPUTextureHandle::GPUTextureHandle(GPUTextureHandle&& o) noexcept
    : ptr(o.ptr), sampler(o.sampler), width(o.width), height(o.height) {
    o.ptr = nullptr;
    o.sampler = nullptr;
}

GPUTextureHandle& GPUTextureHandle::operator=(GPUTextureHandle&& o) noexcept {
    if (this != &o) {
        if (ptr && g_gpu.device) {
            SDL_WaitForGPUIdle(g_gpu.device);
            SDL_ReleaseGPUTexture(g_gpu.device, ptr);
        }
        ptr = o.ptr;
        sampler = o.sampler;
        width = o.width;
        height = o.height;
        o.ptr = nullptr;
        o.sampler = nullptr;
    }
    return *this;
}

static const char* get_shader_path(SDL_GPUShaderFormat format) {
    switch (format) {
        case SDL_GPU_SHADERFORMAT_SPIRV: return "spirv";
        case SDL_GPU_SHADERFORMAT_DXIL: return "dxil";
        case SDL_GPU_SHADERFORMAT_MSL: return "msl";
        default: return nullptr;
    }
}

static const char* get_shader_ext(SDL_GPUShaderFormat format) {
    switch (format) {
        case SDL_GPU_SHADERFORMAT_SPIRV: return ".spv";
        case SDL_GPU_SHADERFORMAT_DXIL: return ".dxil";
        case SDL_GPU_SHADERFORMAT_MSL: return ".msl";
        default: return nullptr;
    }
}

bool GPURenderer::init(SDL_Window* win) {
    window = win;

    device = SDL_CreateGPUDevice(
        SDL_GPU_SHADERFORMAT_SPIRV | SDL_GPU_SHADERFORMAT_DXIL | SDL_GPU_SHADERFORMAT_MSL,
        true,
        nullptr
    );

    if (!device) {
        SDL_Log("Failed to create GPU device: %s", SDL_GetError());
        return false;
    }

    if (!SDL_ClaimWindowForGPUDevice(device, window)) {
        SDL_Log("Failed to claim window for GPU: %s", SDL_GetError());
        return false;
    }

    SDL_GPUSamplerCreateInfo sampler_info = {};
    sampler_info.min_filter = SDL_GPU_FILTER_NEAREST;
    sampler_info.mag_filter = SDL_GPU_FILTER_NEAREST;
    sampler_info.mipmap_mode = SDL_GPU_SAMPLERMIPMAPMODE_NEAREST;
    sampler_info.address_mode_u = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;
    sampler_info.address_mode_v = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;
    sampler_info.address_mode_w = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;

    default_sampler = SDL_CreateGPUSampler(device, &sampler_info);
    if (!default_sampler) {
        SDL_Log("Failed to create sampler: %s", SDL_GetError());
        return false;
    }

    // Create linear sampler for bloom/post-processing
    SDL_GPUSamplerCreateInfo linear_info = {};
    linear_info.min_filter = SDL_GPU_FILTER_LINEAR;
    linear_info.mag_filter = SDL_GPU_FILTER_LINEAR;
    linear_info.mipmap_mode = SDL_GPU_SAMPLERMIPMAPMODE_NEAREST;
    linear_info.address_mode_u = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;
    linear_info.address_mode_v = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;
    linear_info.address_mode_w = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;

    linear_sampler = SDL_CreateGPUSampler(device, &linear_info);
    if (!linear_sampler) {
        SDL_Log("Failed to create linear sampler: %s", SDL_GetError());
        return false;
    }

    if (!create_quad_buffers()) {
        return false;
    }

    if (!create_pipelines()) {
        return false;
    }

    // Initialize multi-pass pipelines (optional, may fail if shaders not yet compiled)
    if (!create_multipass_pipelines()) {
        SDL_Log("Multi-pass pipelines not available (shaders not compiled)");
        fx_config.use_multipass = false;
    }

    // Get window size for pass manager
    int win_w, win_h;
    SDL_GetWindowSize(window, &win_w, &win_h);

    // Initialize pass manager for multi-pass rendering
    if (fx_config.use_multipass) {
        SDL_GPUTextureFormat swap_format = SDL_GetGPUSwapchainTextureFormat(device, window);
        if (!pass_manager.init(device, static_cast<Uint32>(win_w), static_cast<Uint32>(win_h), swap_format)) {
            SDL_Log("Pass manager initialization failed, disabling multi-pass");
            fx_config.use_multipass = false;
        }
    }

    // Setup render context
    render_ctx.device = device;

    SDL_Log("GPU renderer initialized (backend: %s, multipass: %s)",
            SDL_GetGPUDeviceDriver(device),
            fx_config.use_multipass ? "enabled" : "disabled");
    return true;
}

void GPURenderer::shutdown() {
    if (device) {
        // Wait for GPU to finish
        SDL_WaitForGPUIdle(device);

        // Shutdown pass manager first
        pass_manager.shutdown();

        // Release original pipelines
        if (sprite_pipeline) SDL_ReleaseGPUGraphicsPipeline(device, sprite_pipeline);
        if (dissolve_pipeline && dissolve_pipeline != sprite_pipeline) SDL_ReleaseGPUGraphicsPipeline(device, dissolve_pipeline);
        if (shadow_pipeline) SDL_ReleaseGPUGraphicsPipeline(device, shadow_pipeline);
        if (color_pipeline) SDL_ReleaseGPUGraphicsPipeline(device, color_pipeline);
        if (line_pipeline) SDL_ReleaseGPUGraphicsPipeline(device, line_pipeline);

        // Release multi-pass pipelines
        if (fullscreen_pipeline) SDL_ReleaseGPUGraphicsPipeline(device, fullscreen_pipeline);
        if (highpass_pipeline) SDL_ReleaseGPUGraphicsPipeline(device, highpass_pipeline);
        if (blur_pipeline) SDL_ReleaseGPUGraphicsPipeline(device, blur_pipeline);
        if (bloom_pipeline) SDL_ReleaseGPUGraphicsPipeline(device, bloom_pipeline);
        if (composite_pipeline) SDL_ReleaseGPUGraphicsPipeline(device, composite_pipeline);
        if (copy_pipeline) SDL_ReleaseGPUGraphicsPipeline(device, copy_pipeline);
        if (lit_sprite_pipeline) SDL_ReleaseGPUGraphicsPipeline(device, lit_sprite_pipeline);
        if (lighting_pipeline) SDL_ReleaseGPUGraphicsPipeline(device, lighting_pipeline);
        if (shadow_perpass_pipeline) SDL_ReleaseGPUGraphicsPipeline(device, shadow_perpass_pipeline);
        if (radial_blur_pipeline) SDL_ReleaseGPUGraphicsPipeline(device, radial_blur_pipeline);
        if (tone_curve_pipeline) SDL_ReleaseGPUGraphicsPipeline(device, tone_curve_pipeline);
        if (vignette_pipeline) SDL_ReleaseGPUGraphicsPipeline(device, vignette_pipeline);
        if (sprite_offscreen_pipeline) SDL_ReleaseGPUGraphicsPipeline(device, sprite_offscreen_pipeline);

        // Release buffers and samplers
        if (quad_vertex_buffer) SDL_ReleaseGPUBuffer(device, quad_vertex_buffer);
        if (shadow_vertex_buffer) SDL_ReleaseGPUBuffer(device, shadow_vertex_buffer);
        if (quad_index_buffer) SDL_ReleaseGPUBuffer(device, quad_index_buffer);
        if (default_sampler) SDL_ReleaseGPUSampler(device, default_sampler);
        if (linear_sampler) SDL_ReleaseGPUSampler(device, linear_sampler);

        SDL_ReleaseWindowFromGPUDevice(device, window);
        SDL_DestroyGPUDevice(device);
    }

    // Reset all pointers
    device = nullptr;
    sprite_pipeline = nullptr;
    dissolve_pipeline = nullptr;
    shadow_pipeline = nullptr;
    color_pipeline = nullptr;
    line_pipeline = nullptr;
    fullscreen_pipeline = nullptr;
    highpass_pipeline = nullptr;
    blur_pipeline = nullptr;
    bloom_pipeline = nullptr;
    composite_pipeline = nullptr;
    copy_pipeline = nullptr;
    lit_sprite_pipeline = nullptr;
    lighting_pipeline = nullptr;
    shadow_perpass_pipeline = nullptr;
    radial_blur_pipeline = nullptr;
    tone_curve_pipeline = nullptr;
    vignette_pipeline = nullptr;
    sprite_offscreen_pipeline = nullptr;
    quad_vertex_buffer = nullptr;
    shadow_vertex_buffer = nullptr;
    quad_index_buffer = nullptr;
    default_sampler = nullptr;
    linear_sampler = nullptr;
    current_surface_pass = nullptr;
}

SDL_GPUShader* GPURenderer::load_shader(const char* filename, SDL_GPUShaderStage stage,
                                         Uint32 sampler_count, Uint32 uniform_buffer_count) {
    SDL_GPUShaderFormat formats = SDL_GetGPUShaderFormats(device);
    SDL_GPUShaderFormat format;
    const char* entrypoint = "main";
    char path[256];

    if (formats & SDL_GPU_SHADERFORMAT_SPIRV) {
        format = SDL_GPU_SHADERFORMAT_SPIRV;
        SDL_snprintf(path, sizeof(path), "dist/shaders/%s.spv", filename);
    } else if (formats & SDL_GPU_SHADERFORMAT_MSL) {
        format = SDL_GPU_SHADERFORMAT_MSL;
        SDL_snprintf(path, sizeof(path), "dist/shaders/%s.msl", filename);
        entrypoint = "main0";
    } else {
        SDL_Log("No supported shader format (need SPIRV or MSL)");
        return nullptr;
    }

    size_t code_size;
    void* code = SDL_LoadFile(path, &code_size);
    if (!code) {
        SDL_Log("Failed to load shader: %s", path);
        return nullptr;
    }

    SDL_GPUShaderCreateInfo info = {};
    info.code = static_cast<const Uint8*>(code);
    info.code_size = code_size;
    info.entrypoint = entrypoint;
    info.format = format;
    info.stage = stage;
    info.num_samplers = sampler_count;
    info.num_uniform_buffers = uniform_buffer_count;

    SDL_GPUShader* shader = SDL_CreateGPUShader(device, &info);
    SDL_free(code);

    if (!shader) {
        SDL_Log("Failed to create shader %s: %s", filename, SDL_GetError());
    }
    return shader;
}

bool GPURenderer::create_pipelines() {
    SDL_GPUShader* sprite_vert = load_shader("sprite.vert", SDL_GPU_SHADERSTAGE_VERTEX, 0, 0);
    SDL_GPUShader* sprite_frag = load_shader("sprite.frag", SDL_GPU_SHADERSTAGE_FRAGMENT, 1, 1);

    if (!sprite_vert || !sprite_frag) {
        if (sprite_vert) SDL_ReleaseGPUShader(device, sprite_vert);
        if (sprite_frag) SDL_ReleaseGPUShader(device, sprite_frag);
        return false;
    }

    SDL_GPUColorTargetDescription color_target = {};
    color_target.format = SDL_GetGPUSwapchainTextureFormat(device, window);
    color_target.blend_state.enable_blend = true;
    color_target.blend_state.src_color_blendfactor = SDL_GPU_BLENDFACTOR_SRC_ALPHA;
    color_target.blend_state.dst_color_blendfactor = SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_ALPHA;
    color_target.blend_state.color_blend_op = SDL_GPU_BLENDOP_ADD;
    color_target.blend_state.src_alpha_blendfactor = SDL_GPU_BLENDFACTOR_ONE;
    color_target.blend_state.dst_alpha_blendfactor = SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_ALPHA;
    color_target.blend_state.alpha_blend_op = SDL_GPU_BLENDOP_ADD;

    SDL_GPUVertexBufferDescription vb_desc = {};
    vb_desc.slot = 0;
    vb_desc.pitch = sizeof(SpriteVertex);
    vb_desc.input_rate = SDL_GPU_VERTEXINPUTRATE_VERTEX;

    SDL_GPUVertexAttribute attrs[2] = {};
    attrs[0].location = 0;
    attrs[0].buffer_slot = 0;
    attrs[0].format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2;
    attrs[0].offset = 0;
    attrs[1].location = 1;
    attrs[1].buffer_slot = 0;
    attrs[1].format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2;
    attrs[1].offset = sizeof(float) * 2;

    SDL_GPUGraphicsPipelineCreateInfo pipeline_info = {};
    pipeline_info.vertex_shader = sprite_vert;
    pipeline_info.fragment_shader = sprite_frag;
    pipeline_info.primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST;
    pipeline_info.target_info.num_color_targets = 1;
    pipeline_info.target_info.color_target_descriptions = &color_target;
    pipeline_info.vertex_input_state.num_vertex_buffers = 1;
    pipeline_info.vertex_input_state.vertex_buffer_descriptions = &vb_desc;
    pipeline_info.vertex_input_state.num_vertex_attributes = 2;
    pipeline_info.vertex_input_state.vertex_attributes = attrs;

    sprite_pipeline = SDL_CreateGPUGraphicsPipeline(device, &pipeline_info);
    SDL_ReleaseGPUShader(device, sprite_vert);
    SDL_ReleaseGPUShader(device, sprite_frag);

    if (!sprite_pipeline) {
        SDL_Log("Failed to create sprite pipeline: %s", SDL_GetError());
        return false;
    }

    SDL_GPUShader* dissolve_vert = load_shader("sprite.vert", SDL_GPU_SHADERSTAGE_VERTEX, 0, 0);
    SDL_GPUShader* dissolve_frag = load_shader("dissolve.frag", SDL_GPU_SHADERSTAGE_FRAGMENT, 1, 1);

    if (!dissolve_vert || !dissolve_frag) {
        if (dissolve_vert) SDL_ReleaseGPUShader(device, dissolve_vert);
        if (dissolve_frag) SDL_ReleaseGPUShader(device, dissolve_frag);
        SDL_Log("Dissolve shader not found, using sprite shader for dissolve");
        dissolve_pipeline = sprite_pipeline;
        return true;
    }

    pipeline_info.vertex_shader = dissolve_vert;
    pipeline_info.fragment_shader = dissolve_frag;

    dissolve_pipeline = SDL_CreateGPUGraphicsPipeline(device, &pipeline_info);
    SDL_ReleaseGPUShader(device, dissolve_vert);
    SDL_ReleaseGPUShader(device, dissolve_frag);

    if (!dissolve_pipeline) {
        SDL_Log("Failed to create dissolve pipeline: %s", SDL_GetError());
        dissolve_pipeline = sprite_pipeline;
    }

    // Shadow pipeline - uses shadow.vert with shadow.frag (progressive blur)
    // Shadow vertices have 3 attributes: position, texcoord, quadpos
    SDL_GPUShader* shadow_vert = load_shader("shadow.vert", SDL_GPU_SHADERSTAGE_VERTEX, 0, 0);
    SDL_GPUShader* shadow_frag = load_shader("shadow.frag", SDL_GPU_SHADERSTAGE_FRAGMENT, 1, 1);

    if (!shadow_vert || !shadow_frag) {
        if (shadow_vert) SDL_ReleaseGPUShader(device, shadow_vert);
        if (shadow_frag) SDL_ReleaseGPUShader(device, shadow_frag);
        SDL_Log("Shadow shader not found, silhouette shadows disabled");
        shadow_pipeline = nullptr;
    } else {
        // Shadow vertex format: position (2f), texcoord (2f), quadpos (2f)
        SDL_GPUVertexBufferDescription shadow_vb_desc = {};
        shadow_vb_desc.slot = 0;
        shadow_vb_desc.pitch = sizeof(ShadowVertex);
        shadow_vb_desc.input_rate = SDL_GPU_VERTEXINPUTRATE_VERTEX;

        SDL_GPUVertexAttribute shadow_attrs[3] = {};
        shadow_attrs[0].location = 0;  // a_position
        shadow_attrs[0].buffer_slot = 0;
        shadow_attrs[0].format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2;
        shadow_attrs[0].offset = 0;
        shadow_attrs[1].location = 1;  // a_texCoord
        shadow_attrs[1].buffer_slot = 0;
        shadow_attrs[1].format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2;
        shadow_attrs[1].offset = sizeof(float) * 2;
        shadow_attrs[2].location = 2;  // a_quadPos
        shadow_attrs[2].buffer_slot = 0;
        shadow_attrs[2].format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2;
        shadow_attrs[2].offset = sizeof(float) * 4;

        pipeline_info.vertex_shader = shadow_vert;
        pipeline_info.fragment_shader = shadow_frag;
        pipeline_info.vertex_input_state.vertex_buffer_descriptions = &shadow_vb_desc;
        pipeline_info.vertex_input_state.num_vertex_attributes = 3;
        pipeline_info.vertex_input_state.vertex_attributes = shadow_attrs;

        shadow_pipeline = SDL_CreateGPUGraphicsPipeline(device, &pipeline_info);
        SDL_ReleaseGPUShader(device, shadow_vert);
        SDL_ReleaseGPUShader(device, shadow_frag);

        // Restore original vertex state for subsequent pipelines
        pipeline_info.vertex_input_state.vertex_buffer_descriptions = &vb_desc;
        pipeline_info.vertex_input_state.num_vertex_attributes = 2;
        pipeline_info.vertex_input_state.vertex_attributes = attrs;

        if (!shadow_pipeline) {
            SDL_Log("Failed to create shadow pipeline: %s", SDL_GetError());
        } else {
            SDL_Log("Shadow pipeline created (progressive blur)");
        }
    }

    SDL_GPUShader* color_vert = load_shader("color.vert", SDL_GPU_SHADERSTAGE_VERTEX, 0, 0);
    SDL_GPUShader* color_frag = load_shader("color.frag", SDL_GPU_SHADERSTAGE_FRAGMENT, 0, 0);

    if (!color_vert || !color_frag) {
        if (color_vert) SDL_ReleaseGPUShader(device, color_vert);
        if (color_frag) SDL_ReleaseGPUShader(device, color_frag);
        SDL_Log("Color shaders not found, grid rendering will not work");
        color_pipeline = nullptr;
        return true;
    }

    SDL_GPUVertexBufferDescription color_vb_desc = {};
    color_vb_desc.slot = 0;
    color_vb_desc.pitch = sizeof(ColorVertex);
    color_vb_desc.input_rate = SDL_GPU_VERTEXINPUTRATE_VERTEX;

    SDL_GPUVertexAttribute color_attrs[2] = {};
    color_attrs[0].location = 0;
    color_attrs[0].buffer_slot = 0;
    color_attrs[0].format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2;
    color_attrs[0].offset = 0;
    color_attrs[1].location = 1;
    color_attrs[1].buffer_slot = 0;
    color_attrs[1].format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT4;
    color_attrs[1].offset = sizeof(float) * 2;

    pipeline_info.vertex_shader = color_vert;
    pipeline_info.fragment_shader = color_frag;
    pipeline_info.vertex_input_state.num_vertex_buffers = 1;
    pipeline_info.vertex_input_state.vertex_buffer_descriptions = &color_vb_desc;
    pipeline_info.vertex_input_state.num_vertex_attributes = 2;
    pipeline_info.vertex_input_state.vertex_attributes = color_attrs;
    pipeline_info.primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST;

    color_pipeline = SDL_CreateGPUGraphicsPipeline(device, &pipeline_info);
    SDL_ReleaseGPUShader(device, color_vert);
    SDL_ReleaseGPUShader(device, color_frag);

    if (!color_pipeline) {
        SDL_Log("Failed to create color pipeline: %s", SDL_GetError());
    }

    SDL_GPUShader* line_vert = load_shader("color.vert", SDL_GPU_SHADERSTAGE_VERTEX, 0, 0);
    SDL_GPUShader* line_frag = load_shader("color.frag", SDL_GPU_SHADERSTAGE_FRAGMENT, 0, 0);

    if (!line_vert || !line_frag) {
        if (line_vert) SDL_ReleaseGPUShader(device, line_vert);
        if (line_frag) SDL_ReleaseGPUShader(device, line_frag);
        SDL_Log("Line shaders not found, line rendering will not work");
        line_pipeline = nullptr;
        return true;
    }

    pipeline_info.vertex_shader = line_vert;
    pipeline_info.fragment_shader = line_frag;
    pipeline_info.primitive_type = SDL_GPU_PRIMITIVETYPE_LINELIST;

    line_pipeline = SDL_CreateGPUGraphicsPipeline(device, &pipeline_info);
    SDL_ReleaseGPUShader(device, line_vert);
    SDL_ReleaseGPUShader(device, line_frag);

    if (!line_pipeline) {
        SDL_Log("Failed to create line pipeline: %s", SDL_GetError());
    }

    return true;
}

bool GPURenderer::create_multipass_pipelines() {
    // Fullscreen pipeline (no vertex buffer, uses gl_VertexIndex)
    SDL_GPUShader* fullscreen_vert = load_shader("fullscreen.vert", SDL_GPU_SHADERSTAGE_VERTEX, 0, 0);
    SDL_GPUShader* copy_frag = load_shader("copy.frag", SDL_GPU_SHADERSTAGE_FRAGMENT, 1, 1);

    if (!fullscreen_vert || !copy_frag) {
        if (fullscreen_vert) SDL_ReleaseGPUShader(device, fullscreen_vert);
        if (copy_frag) SDL_ReleaseGPUShader(device, copy_frag);
        SDL_Log("Fullscreen shaders not found");
        return false;
    }

    // Get swapchain format for fullscreen pipelines
    SDL_GPUTextureFormat swapchain_format = SDL_GetGPUSwapchainTextureFormat(device, window);

    SDL_GPUColorTargetDescription color_target = {};
    color_target.format = swapchain_format;
    color_target.blend_state.enable_blend = true;
    color_target.blend_state.src_color_blendfactor = SDL_GPU_BLENDFACTOR_SRC_ALPHA;
    color_target.blend_state.dst_color_blendfactor = SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_ALPHA;
    color_target.blend_state.color_blend_op = SDL_GPU_BLENDOP_ADD;
    color_target.blend_state.src_alpha_blendfactor = SDL_GPU_BLENDFACTOR_ONE;
    color_target.blend_state.dst_alpha_blendfactor = SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_ALPHA;
    color_target.blend_state.alpha_blend_op = SDL_GPU_BLENDOP_ADD;

    SDL_GPUGraphicsPipelineCreateInfo pipeline_info = {};
    pipeline_info.vertex_shader = fullscreen_vert;
    pipeline_info.fragment_shader = copy_frag;
    pipeline_info.primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST;
    pipeline_info.target_info.num_color_targets = 1;
    pipeline_info.target_info.color_target_descriptions = &color_target;
    // No vertex buffers - uses gl_VertexIndex
    pipeline_info.vertex_input_state.num_vertex_buffers = 0;
    pipeline_info.vertex_input_state.num_vertex_attributes = 0;

    copy_pipeline = SDL_CreateGPUGraphicsPipeline(device, &pipeline_info);

    if (!copy_pipeline) {
        SDL_Log("Failed to create copy pipeline: %s", SDL_GetError());
        SDL_ReleaseGPUShader(device, fullscreen_vert);
        SDL_ReleaseGPUShader(device, copy_frag);
        return false;
    }

    SDL_ReleaseGPUShader(device, copy_frag);

    // Highpass pipeline
    SDL_GPUShader* highpass_frag = load_shader("highpass.frag", SDL_GPU_SHADERSTAGE_FRAGMENT, 1, 1);
    if (highpass_frag) {
        pipeline_info.fragment_shader = highpass_frag;
        // Highpass renders to RGBA8 texture (not swapchain)
        color_target.format = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM;
        highpass_pipeline = SDL_CreateGPUGraphicsPipeline(device, &pipeline_info);
        SDL_ReleaseGPUShader(device, highpass_frag);
        if (highpass_pipeline) {
            SDL_Log("Highpass pipeline created");
        }
    }

    // Blur pipeline
    SDL_GPUShader* blur_frag = load_shader("blur.frag", SDL_GPU_SHADERSTAGE_FRAGMENT, 1, 1);
    if (blur_frag) {
        pipeline_info.fragment_shader = blur_frag;
        blur_pipeline = SDL_CreateGPUGraphicsPipeline(device, &pipeline_info);
        SDL_ReleaseGPUShader(device, blur_frag);
        if (blur_pipeline) {
            SDL_Log("Blur pipeline created");
        }
    }

    // Bloom accumulation pipeline
    SDL_GPUShader* bloom_frag = load_shader("bloom.frag", SDL_GPU_SHADERSTAGE_FRAGMENT, 2, 1);
    if (bloom_frag) {
        pipeline_info.fragment_shader = bloom_frag;
        bloom_pipeline = SDL_CreateGPUGraphicsPipeline(device, &pipeline_info);
        SDL_ReleaseGPUShader(device, bloom_frag);
        if (bloom_pipeline) {
            SDL_Log("Bloom pipeline created");
        }
    }

    // Surface composite pipeline (renders to swapchain)
    SDL_GPUShader* composite_frag = load_shader("surface_composite.frag", SDL_GPU_SHADERSTAGE_FRAGMENT, 2, 1);
    if (composite_frag) {
        pipeline_info.fragment_shader = composite_frag;
        color_target.format = swapchain_format;
        composite_pipeline = SDL_CreateGPUGraphicsPipeline(device, &pipeline_info);
        SDL_ReleaseGPUShader(device, composite_frag);
        if (composite_pipeline) {
            SDL_Log("Composite pipeline created");
        }
    }

    SDL_ReleaseGPUShader(device, fullscreen_vert);

    // Sprite offscreen pipeline (for rendering sprites to R8G8B8A8 FBOs)
    // Same as sprite_pipeline but for R8G8B8A8_UNORM render targets
    SDL_GPUShader* sprite_vert = load_shader("sprite.vert", SDL_GPU_SHADERSTAGE_VERTEX, 0, 0);
    SDL_GPUShader* sprite_frag = load_shader("sprite.frag", SDL_GPU_SHADERSTAGE_FRAGMENT, 1, 1);

    if (sprite_vert && sprite_frag) {
        SDL_GPUVertexBufferDescription sprite_vb_desc = {};
        sprite_vb_desc.slot = 0;
        sprite_vb_desc.pitch = sizeof(SpriteVertex);
        sprite_vb_desc.input_rate = SDL_GPU_VERTEXINPUTRATE_VERTEX;

        SDL_GPUVertexAttribute sprite_attrs[2] = {};
        sprite_attrs[0].location = 0;
        sprite_attrs[0].buffer_slot = 0;
        sprite_attrs[0].format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2;
        sprite_attrs[0].offset = 0;
        sprite_attrs[1].location = 1;
        sprite_attrs[1].buffer_slot = 0;
        sprite_attrs[1].format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2;
        sprite_attrs[1].offset = sizeof(float) * 2;

        SDL_GPUColorTargetDescription offscreen_target = {};
        offscreen_target.format = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM;
        offscreen_target.blend_state.enable_blend = true;
        offscreen_target.blend_state.src_color_blendfactor = SDL_GPU_BLENDFACTOR_SRC_ALPHA;
        offscreen_target.blend_state.dst_color_blendfactor = SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_ALPHA;
        offscreen_target.blend_state.color_blend_op = SDL_GPU_BLENDOP_ADD;
        offscreen_target.blend_state.src_alpha_blendfactor = SDL_GPU_BLENDFACTOR_ONE;
        offscreen_target.blend_state.dst_alpha_blendfactor = SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_ALPHA;
        offscreen_target.blend_state.alpha_blend_op = SDL_GPU_BLENDOP_ADD;

        SDL_GPUGraphicsPipelineCreateInfo sprite_offscreen_info = {};
        sprite_offscreen_info.vertex_shader = sprite_vert;
        sprite_offscreen_info.fragment_shader = sprite_frag;
        sprite_offscreen_info.primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST;
        sprite_offscreen_info.target_info.num_color_targets = 1;
        sprite_offscreen_info.target_info.color_target_descriptions = &offscreen_target;
        sprite_offscreen_info.vertex_input_state.num_vertex_buffers = 1;
        sprite_offscreen_info.vertex_input_state.vertex_buffer_descriptions = &sprite_vb_desc;
        sprite_offscreen_info.vertex_input_state.num_vertex_attributes = 2;
        sprite_offscreen_info.vertex_input_state.vertex_attributes = sprite_attrs;

        sprite_offscreen_pipeline = SDL_CreateGPUGraphicsPipeline(device, &sprite_offscreen_info);
        if (sprite_offscreen_pipeline) {
            SDL_Log("Sprite offscreen pipeline created");
        } else {
            SDL_Log("Failed to create sprite offscreen pipeline: %s", SDL_GetError());
        }
    }
    if (sprite_vert) SDL_ReleaseGPUShader(device, sprite_vert);
    if (sprite_frag) SDL_ReleaseGPUShader(device, sprite_frag);

    // Per-sprite shadow pipeline (uses same vertex format as existing shadow pipeline)
    SDL_GPUShader* shadow_vert = load_shader("shadow.vert", SDL_GPU_SHADERSTAGE_VERTEX, 0, 0);
    SDL_GPUShader* shadow_perpass_frag = load_shader("shadow_perpass.frag", SDL_GPU_SHADERSTAGE_FRAGMENT, 1, 1);

    if (shadow_vert && shadow_perpass_frag) {
        // Shadow vertex format: position (2f), texcoord (2f), localpos (2f)
        SDL_GPUVertexBufferDescription shadow_vb_desc = {};
        shadow_vb_desc.slot = 0;
        shadow_vb_desc.pitch = sizeof(ShadowVertex);
        shadow_vb_desc.input_rate = SDL_GPU_VERTEXINPUTRATE_VERTEX;

        SDL_GPUVertexAttribute shadow_attrs[3] = {};
        shadow_attrs[0].location = 0;  // a_position
        shadow_attrs[0].buffer_slot = 0;
        shadow_attrs[0].format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2;
        shadow_attrs[0].offset = 0;
        shadow_attrs[1].location = 1;  // a_texCoord
        shadow_attrs[1].buffer_slot = 0;
        shadow_attrs[1].format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2;
        shadow_attrs[1].offset = sizeof(float) * 2;
        shadow_attrs[2].location = 2;  // a_localPos
        shadow_attrs[2].buffer_slot = 0;
        shadow_attrs[2].format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2;
        shadow_attrs[2].offset = sizeof(float) * 4;

        SDL_GPUGraphicsPipelineCreateInfo shadow_pipeline_info = {};
        shadow_pipeline_info.vertex_shader = shadow_vert;
        shadow_pipeline_info.fragment_shader = shadow_perpass_frag;
        shadow_pipeline_info.primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST;
        shadow_pipeline_info.target_info.num_color_targets = 1;
        shadow_pipeline_info.target_info.color_target_descriptions = &color_target;
        shadow_pipeline_info.vertex_input_state.num_vertex_buffers = 1;
        shadow_pipeline_info.vertex_input_state.vertex_buffer_descriptions = &shadow_vb_desc;
        shadow_pipeline_info.vertex_input_state.num_vertex_attributes = 3;
        shadow_pipeline_info.vertex_input_state.vertex_attributes = shadow_attrs;

        shadow_perpass_pipeline = SDL_CreateGPUGraphicsPipeline(device, &shadow_pipeline_info);
        if (shadow_perpass_pipeline) {
            SDL_Log("Shadow per-pass pipeline created");
        }
    }
    if (shadow_vert) SDL_ReleaseGPUShader(device, shadow_vert);
    if (shadow_perpass_frag) SDL_ReleaseGPUShader(device, shadow_perpass_frag);

    // Post-processing pipelines (fullscreen quad based)
    // These render to SurfaceA/B which use swapchain format
    SDL_GPUShader* fs_vert = load_shader("fullscreen.vert", SDL_GPU_SHADERSTAGE_VERTEX, 0, 0);

    if (fs_vert) {
        // Use swapchain format for post-processing (they render to surface passes)
        color_target.format = swapchain_format;

        // Radial blur pipeline
        SDL_GPUShader* radial_frag = load_shader("radial_blur.frag", SDL_GPU_SHADERSTAGE_FRAGMENT, 1, 1);
        if (radial_frag) {
            pipeline_info.vertex_shader = fs_vert;
            pipeline_info.fragment_shader = radial_frag;
            pipeline_info.vertex_input_state.num_vertex_buffers = 0;
            pipeline_info.vertex_input_state.num_vertex_attributes = 0;
            radial_blur_pipeline = SDL_CreateGPUGraphicsPipeline(device, &pipeline_info);
            SDL_ReleaseGPUShader(device, radial_frag);
            if (radial_blur_pipeline) SDL_Log("Radial blur pipeline created");
        }

        // Tone curve pipeline
        SDL_GPUShader* tone_frag = load_shader("tone_curve.frag", SDL_GPU_SHADERSTAGE_FRAGMENT, 2, 1);
        if (tone_frag) {
            pipeline_info.fragment_shader = tone_frag;
            tone_curve_pipeline = SDL_CreateGPUGraphicsPipeline(device, &pipeline_info);
            SDL_ReleaseGPUShader(device, tone_frag);
            if (tone_curve_pipeline) SDL_Log("Tone curve pipeline created");
        }

        // Vignette pipeline
        SDL_GPUShader* vignette_frag = load_shader("vignette.frag", SDL_GPU_SHADERSTAGE_FRAGMENT, 1, 1);
        if (vignette_frag) {
            pipeline_info.fragment_shader = vignette_frag;
            vignette_pipeline = SDL_CreateGPUGraphicsPipeline(device, &pipeline_info);
            SDL_ReleaseGPUShader(device, vignette_frag);
            if (vignette_pipeline) SDL_Log("Vignette pipeline created");
        }

        SDL_ReleaseGPUShader(device, fs_vert);
    }

    SDL_Log("Multi-pass pipelines initialized");
    return true;
}

bool GPURenderer::create_quad_buffers() {
    SDL_GPUBufferCreateInfo vb_info = {};
    vb_info.usage = SDL_GPU_BUFFERUSAGE_VERTEX;
    vb_info.size = sizeof(ColorVertex) * 4;

    quad_vertex_buffer = SDL_CreateGPUBuffer(device, &vb_info);
    if (!quad_vertex_buffer) {
        SDL_Log("Failed to create vertex buffer: %s", SDL_GetError());
        return false;
    }

    // Shadow vertex buffer (larger - includes quad position for progressive blur)
    SDL_GPUBufferCreateInfo shadow_vb_info = {};
    shadow_vb_info.usage = SDL_GPU_BUFFERUSAGE_VERTEX;
    shadow_vb_info.size = sizeof(ShadowVertex) * 4;

    shadow_vertex_buffer = SDL_CreateGPUBuffer(device, &shadow_vb_info);
    if (!shadow_vertex_buffer) {
        SDL_Log("Failed to create shadow vertex buffer: %s", SDL_GetError());
        return false;
    }

    SDL_GPUBufferCreateInfo ib_info = {};
    ib_info.usage = SDL_GPU_BUFFERUSAGE_INDEX;
    ib_info.size = sizeof(Uint16) * 6;

    quad_index_buffer = SDL_CreateGPUBuffer(device, &ib_info);
    if (!quad_index_buffer) {
        SDL_Log("Failed to create index buffer: %s", SDL_GetError());
        return false;
    }

    Uint16 indices[6] = {0, 1, 2, 0, 2, 3};

    SDL_GPUTransferBufferCreateInfo transfer_info = {};
    transfer_info.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
    transfer_info.size = sizeof(indices);

    SDL_GPUTransferBuffer* transfer = SDL_CreateGPUTransferBuffer(device, &transfer_info);
    void* map = SDL_MapGPUTransferBuffer(device, transfer, false);
    std::memcpy(map, indices, sizeof(indices));
    SDL_UnmapGPUTransferBuffer(device, transfer);

    SDL_GPUCommandBuffer* cmd = SDL_AcquireGPUCommandBuffer(device);
    SDL_GPUCopyPass* copy = SDL_BeginGPUCopyPass(cmd);

    SDL_GPUTransferBufferLocation src = {};
    src.transfer_buffer = transfer;
    src.offset = 0;

    SDL_GPUBufferRegion dst = {};
    dst.buffer = quad_index_buffer;
    dst.offset = 0;
    dst.size = sizeof(indices);

    SDL_UploadToGPUBuffer(copy, &src, &dst, false);
    SDL_EndGPUCopyPass(copy);
    SDL_SubmitGPUCommandBuffer(cmd);
    SDL_WaitForGPUIdle(device);

    SDL_ReleaseGPUTransferBuffer(device, transfer);

    return true;
}

GPUTextureHandle GPURenderer::load_texture(const char* filepath) {
    SDL_Surface* surface = IMG_Load(filepath);
    if (!surface) {
        SDL_Log("Failed to load image: %s", filepath);
        return {};
    }

    GPUTextureHandle handle = create_texture_from_surface(surface);
    SDL_DestroySurface(surface);
    return handle;
}

GPUTextureHandle GPURenderer::create_texture_from_surface(SDL_Surface* surface) {
    GPUTextureHandle handle;

    SDL_Surface* rgba = SDL_ConvertSurface(surface, SDL_PIXELFORMAT_RGBA32);
    if (!rgba) {
        SDL_Log("Failed to convert surface: %s", SDL_GetError());
        return handle;
    }

    SDL_GPUTextureCreateInfo tex_info = {};
    tex_info.type = SDL_GPU_TEXTURETYPE_2D;
    tex_info.format = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM;
    tex_info.width = rgba->w;
    tex_info.height = rgba->h;
    tex_info.layer_count_or_depth = 1;
    tex_info.num_levels = 1;
    tex_info.usage = SDL_GPU_TEXTUREUSAGE_SAMPLER;

    handle.ptr = SDL_CreateGPUTexture(device, &tex_info);
    if (!handle.ptr) {
        SDL_Log("Failed to create texture: %s", SDL_GetError());
        SDL_DestroySurface(rgba);
        return handle;
    }

    handle.width = rgba->w;
    handle.height = rgba->h;
    handle.sampler = default_sampler;

    SDL_GPUTransferBufferCreateInfo transfer_info = {};
    transfer_info.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
    transfer_info.size = rgba->w * rgba->h * 4;

    SDL_GPUTransferBuffer* transfer = SDL_CreateGPUTransferBuffer(device, &transfer_info);
    void* map = SDL_MapGPUTransferBuffer(device, transfer, false);
    std::memcpy(map, rgba->pixels, transfer_info.size);
    SDL_UnmapGPUTransferBuffer(device, transfer);

    SDL_GPUCommandBuffer* cmd = SDL_AcquireGPUCommandBuffer(device);
    SDL_GPUCopyPass* copy = SDL_BeginGPUCopyPass(cmd);

    SDL_GPUTextureTransferInfo src = {};
    src.transfer_buffer = transfer;
    src.offset = 0;

    SDL_GPUTextureRegion dst = {};
    dst.texture = handle.ptr;
    dst.w = rgba->w;
    dst.h = rgba->h;
    dst.d = 1;

    SDL_UploadToGPUTexture(copy, &src, &dst, false);
    SDL_EndGPUCopyPass(copy);
    SDL_SubmitGPUCommandBuffer(cmd);
    SDL_WaitForGPUIdle(device);

    SDL_ReleaseGPUTransferBuffer(device, transfer);
    SDL_DestroySurface(rgba);

    return handle;
}

bool GPURenderer::begin_frame() {
    cmd_buffer = SDL_AcquireGPUCommandBuffer(device);
    if (!cmd_buffer) {
        SDL_Log("Failed to acquire command buffer: %s", SDL_GetError());
        return false;
    }

    if (!SDL_WaitAndAcquireGPUSwapchainTexture(cmd_buffer, window,
                                                &swapchain_texture,
                                                &swapchain_w, &swapchain_h)) {
        SDL_Log("Failed to acquire swapchain: %s", SDL_GetError());
        return false;
    }

    if (!swapchain_texture) {
        return false;
    }

    SDL_GPUColorTargetInfo color_target = {};
    color_target.texture = swapchain_texture;
    color_target.load_op = SDL_GPU_LOADOP_CLEAR;
    color_target.store_op = SDL_GPU_STOREOP_STORE;
    color_target.clear_color = {40.0f/255.0f, 40.0f/255.0f, 60.0f/255.0f, 1.0f};

    render_pass = SDL_BeginGPURenderPass(cmd_buffer, &color_target, 1, nullptr);

    if (render_pass) {
        SDL_GPUViewport viewport = {0, 0, (float)swapchain_w, (float)swapchain_h, 0.0f, 1.0f};
        SDL_SetGPUViewport(render_pass, &viewport);

        SDL_Rect scissor = {0, 0, (int)swapchain_w, (int)swapchain_h};
        SDL_SetGPUScissor(render_pass, &scissor);
    }

    return render_pass != nullptr;
}

void GPURenderer::end_frame() {
    if (render_pass) {
        SDL_EndGPURenderPass(render_pass);
        render_pass = nullptr;
    }
    if (cmd_buffer) {
        SDL_SubmitGPUCommandBuffer(cmd_buffer);
        cmd_buffer = nullptr;
    }
    swapchain_texture = nullptr;
}

void GPURenderer::draw_sprite(const GPUTextureHandle& texture, const SDL_FRect& src,
                               const SDL_FRect& dst, bool flip_x, float opacity) {
    if (!render_pass || !texture.ptr) return;

    float u0 = src.x / texture.width;
    float v0 = src.y / texture.height;
    float u1 = (src.x + src.w) / texture.width;
    float v1 = (src.y + src.h) / texture.height;

    if (flip_x) {
        float tmp = u0;
        u0 = u1;
        u1 = tmp;
    }

    float x0 = (dst.x / swapchain_w) * 2.0f - 1.0f;
    float y0 = 1.0f - (dst.y / swapchain_h) * 2.0f;
    float x1 = ((dst.x + dst.w) / swapchain_w) * 2.0f - 1.0f;
    float y1 = 1.0f - ((dst.y + dst.h) / swapchain_h) * 2.0f;

    SpriteVertex vertices[4] = {
        {x0, y0, u0, v0},
        {x1, y0, u1, v0},
        {x1, y1, u1, v1},
        {x0, y1, u0, v1}
    };

    SDL_GPUTransferBufferCreateInfo transfer_info = {};
    transfer_info.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
    transfer_info.size = sizeof(vertices);

    SDL_GPUTransferBuffer* transfer = SDL_CreateGPUTransferBuffer(device, &transfer_info);
    void* map = SDL_MapGPUTransferBuffer(device, transfer, false);
    std::memcpy(map, vertices, sizeof(vertices));
    SDL_UnmapGPUTransferBuffer(device, transfer);

    SDL_EndGPURenderPass(render_pass);

    SDL_GPUCopyPass* copy = SDL_BeginGPUCopyPass(cmd_buffer);
    SDL_GPUTransferBufferLocation src_loc = {};
    src_loc.transfer_buffer = transfer;
    SDL_GPUBufferRegion dst_region = {};
    dst_region.buffer = quad_vertex_buffer;
    dst_region.size = sizeof(vertices);
    SDL_UploadToGPUBuffer(copy, &src_loc, &dst_region, false);
    SDL_EndGPUCopyPass(copy);

    SDL_GPUColorTargetInfo color_target = {};
    color_target.texture = swapchain_texture;
    color_target.load_op = SDL_GPU_LOADOP_LOAD;
    color_target.store_op = SDL_GPU_STOREOP_STORE;
    render_pass = SDL_BeginGPURenderPass(cmd_buffer, &color_target, 1, nullptr);

    SDL_GPUViewport viewport = {0, 0, (float)swapchain_w, (float)swapchain_h, 0.0f, 1.0f};
    SDL_SetGPUViewport(render_pass, &viewport);

    SDL_Rect scissor = {0, 0, (int)swapchain_w, (int)swapchain_h};
    SDL_SetGPUScissor(render_pass, &scissor);

    SDL_ReleaseGPUTransferBuffer(device, transfer);

    SDL_BindGPUGraphicsPipeline(render_pass, sprite_pipeline);

    SDL_GPUBufferBinding vb_binding = {};
    vb_binding.buffer = quad_vertex_buffer;
    SDL_BindGPUVertexBuffers(render_pass, 0, &vb_binding, 1);

    SDL_GPUBufferBinding ib_binding = {};
    ib_binding.buffer = quad_index_buffer;
    SDL_BindGPUIndexBuffer(render_pass, &ib_binding, SDL_GPU_INDEXELEMENTSIZE_16BIT);

    SDL_GPUTextureSamplerBinding tex_binding = {};
    tex_binding.texture = texture.ptr;
    tex_binding.sampler = texture.sampler;
    SDL_BindGPUFragmentSamplers(render_pass, 0, &tex_binding, 1);

    SpriteUniforms uniforms = {opacity, 0.0f, 0.0f, 0.0f};
    SDL_PushGPUFragmentUniformData(cmd_buffer, 0, &uniforms, sizeof(uniforms));

    SDL_DrawGPUIndexedPrimitives(render_pass, 6, 1, 0, 0, 0);
}

void GPURenderer::draw_sprite_dissolve(const GPUTextureHandle& texture, const SDL_FRect& src,
                                        const SDL_FRect& dst, bool flip_x,
                                        float opacity, float dissolve_time, float seed) {
    if (!render_pass || !texture.ptr) return;
    if (!dissolve_pipeline || dissolve_pipeline == sprite_pipeline) {
        draw_sprite(texture, src, dst, flip_x, opacity * (1.0f - dissolve_time));
        return;
    }

    float u0 = src.x / texture.width;
    float v0 = src.y / texture.height;
    float u1 = (src.x + src.w) / texture.width;
    float v1 = (src.y + src.h) / texture.height;

    if (flip_x) {
        float tmp = u0;
        u0 = u1;
        u1 = tmp;
    }

    float x0 = (dst.x / swapchain_w) * 2.0f - 1.0f;
    float y0 = 1.0f - (dst.y / swapchain_h) * 2.0f;
    float x1 = ((dst.x + dst.w) / swapchain_w) * 2.0f - 1.0f;
    float y1 = 1.0f - ((dst.y + dst.h) / swapchain_h) * 2.0f;

    SpriteVertex vertices[4] = {
        {x0, y0, u0, v0},
        {x1, y0, u1, v0},
        {x1, y1, u1, v1},
        {x0, y1, u0, v1}
    };

    SDL_GPUTransferBufferCreateInfo transfer_info = {};
    transfer_info.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
    transfer_info.size = sizeof(vertices);

    SDL_GPUTransferBuffer* transfer = SDL_CreateGPUTransferBuffer(device, &transfer_info);
    void* map = SDL_MapGPUTransferBuffer(device, transfer, false);
    std::memcpy(map, vertices, sizeof(vertices));
    SDL_UnmapGPUTransferBuffer(device, transfer);

    SDL_EndGPURenderPass(render_pass);

    SDL_GPUCopyPass* copy = SDL_BeginGPUCopyPass(cmd_buffer);
    SDL_GPUTransferBufferLocation src_loc = {};
    src_loc.transfer_buffer = transfer;
    SDL_GPUBufferRegion dst_region = {};
    dst_region.buffer = quad_vertex_buffer;
    dst_region.size = sizeof(vertices);
    SDL_UploadToGPUBuffer(copy, &src_loc, &dst_region, false);
    SDL_EndGPUCopyPass(copy);

    SDL_GPUColorTargetInfo color_target = {};
    color_target.texture = swapchain_texture;
    color_target.load_op = SDL_GPU_LOADOP_LOAD;
    color_target.store_op = SDL_GPU_STOREOP_STORE;
    render_pass = SDL_BeginGPURenderPass(cmd_buffer, &color_target, 1, nullptr);

    SDL_GPUViewport viewport = {0, 0, (float)swapchain_w, (float)swapchain_h, 0.0f, 1.0f};
    SDL_SetGPUViewport(render_pass, &viewport);

    SDL_Rect scissor = {0, 0, (int)swapchain_w, (int)swapchain_h};
    SDL_SetGPUScissor(render_pass, &scissor);

    SDL_ReleaseGPUTransferBuffer(device, transfer);

    SDL_BindGPUGraphicsPipeline(render_pass, dissolve_pipeline);

    SDL_GPUBufferBinding vb_binding = {};
    vb_binding.buffer = quad_vertex_buffer;
    SDL_BindGPUVertexBuffers(render_pass, 0, &vb_binding, 1);

    SDL_GPUBufferBinding ib_binding = {};
    ib_binding.buffer = quad_index_buffer;
    SDL_BindGPUIndexBuffer(render_pass, &ib_binding, SDL_GPU_INDEXELEMENTSIZE_16BIT);

    SDL_GPUTextureSamplerBinding tex_binding = {};
    tex_binding.texture = texture.ptr;
    tex_binding.sampler = texture.sampler;
    SDL_BindGPUFragmentSamplers(render_pass, 0, &tex_binding, 1);

    SpriteUniforms uniforms = {opacity, dissolve_time, seed, 0.0f};
    SDL_PushGPUFragmentUniformData(cmd_buffer, 0, &uniforms, sizeof(uniforms));

    SDL_DrawGPUIndexedPrimitives(render_pass, 6, 1, 0, 0, 0);
}

void GPURenderer::draw_sprite_shadow(const GPUTextureHandle& texture, const SDL_FRect& src,
                                      Vec2 feet_pos, float scale, bool flip_x, float opacity,
                                      const PointLight* light) {
    if (!render_pass || !texture.ptr || !shadow_pipeline) return;

    // Use provided light, or scene light, or default
    const PointLight* active_light = light;
    if (!active_light && has_scene_light) {
        active_light = &scene_light;
    }

    // Shadow parameters - silhouette projected onto ground below unit
    // Duelyst uses 45° cast matrix: cos(45°) ≈ 0.707 foreshortening
    constexpr float SHADOW_SQUASH = 0.7f;     // 45° foreshortening: cos(45°) ≈ 0.707
    constexpr float SHADOW_STRETCH = 1.0f;    // No horizontal stretch

    // Shadow parameters from FX config
    const float SHADOW_INTENSITY = fx_config.shadow_intensity;
    const float BLUR_SHIFT_MODIFIER = fx_config.shadow_blur_shift;
    const float BLUR_INTENSITY_MODIFIER = fx_config.shadow_blur_intensity;

    // Compute light distance attenuation and effective intensity
    // Duelyst formula: lightDistPctInv * alpha * intensity * v_fragmentColor.a
    // v_fragmentColor.a = light's effective alpha (color.a * displayedOpacity)
    float lightDistPctInv = 1.0f;  // Default: full shadow if no light
    float lightAlpha = 1.0f;       // Light's contribution to shadow intensity
    if (active_light) {
        float dx = feet_pos.x - active_light->x;
        float dy = feet_pos.y - active_light->y;
        float lightDist = std::sqrt(dx * dx + dy * dy);
        float lightDistPct = std::pow(lightDist / active_light->radius, 2.0f);
        lightDistPctInv = std::max(0.0f, 1.0f - lightDistPct);
        // In Duelyst, brighter lights cast darker shadows
        // effectiveIntensity = intensity * (color.a/255) * (opacity/255)
        lightAlpha = active_light->a * active_light->intensity;
    }

    // Combine light attenuation with light's intensity contribution
    // This replaces v_fragmentColor.a from Duelyst's per-vertex light data
    float effectiveLightFactor = lightDistPctInv * lightAlpha;

    float sprite_w = src.w * scale * SHADOW_STRETCH;
    float sprite_h = src.h * scale * SHADOW_SQUASH;

    // Texture coordinates - flip V so head silhouette is at bottom of shadow quad
    // (shadow lies on ground with head part furthest from unit)
    float u0 = src.x / texture.width;
    float v0 = (src.y + src.h) / texture.height;  // Bottom of sprite texture
    float u1 = (src.x + src.w) / texture.width;
    float v1 = src.y / texture.height;            // Top of sprite texture

    if (flip_x) {
        float tmp = u0;
        u0 = u1;
        u1 = tmp;
    }

    // Shadow positioning: The feet are SHADOW_OFFSET pixels above the sprite's bottom.
    // After vertical flip and squash, the feet pixels in the shadow texture are
    // SHADOW_OFFSET * SHADOW_SQUASH from the TOP of the shadow quad.
    // We need to shift the quad UP so these feet pixels align with feet_pos.y.
    float feet_offset = SHADOW_OFFSET * scale * SHADOW_SQUASH;

    // Shadow quad: top edge shifted up so feet pixels land at feet_pos.y
    float shadow_top_y = feet_pos.y - feet_offset;

    float tl_x = feet_pos.x - sprite_w * 0.5f;
    float tl_y = shadow_top_y;
    float tr_x = feet_pos.x + sprite_w * 0.5f;
    float tr_y = shadow_top_y;
    float bl_x = feet_pos.x - sprite_w * 0.5f;
    float bl_y = shadow_top_y + sprite_h;
    float br_x = feet_pos.x + sprite_w * 0.5f;
    float br_y = shadow_top_y + sprite_h;

    // Convert to NDC
    float x0 = (tl_x / swapchain_w) * 2.0f - 1.0f;
    float y0 = 1.0f - (tl_y / swapchain_h) * 2.0f;
    float x1 = (tr_x / swapchain_w) * 2.0f - 1.0f;
    float y1 = 1.0f - (tr_y / swapchain_h) * 2.0f;
    float x2 = (br_x / swapchain_w) * 2.0f - 1.0f;
    float y2 = 1.0f - (br_y / swapchain_h) * 2.0f;
    float x3 = (bl_x / swapchain_w) * 2.0f - 1.0f;
    float y3 = 1.0f - (bl_y / swapchain_h) * 2.0f;

    // Local sprite positions in PIXELS (matching Duelyst's v_position)
    // For a sprite with anchor at horizontal center (0.5) and bottom (0):
    // - Top row (feet): y = 0 (at anchor.y = SHADOW_OFFSET)
    // - Bottom row (head): y = src.h (furthest from anchor)
    // The anchor in Duelyst is (anchorX * width, shadowOffset) = (0.5 * src.w, 19.5)
    // Local coords: (0,0) is top-left of sprite content, (src.w, src.h) is bottom-right
    // After texture V-flip, top of shadow quad = feet = local y near anchor.y
    float local_left = 0.0f;
    float local_right = src.w;
    float local_top = SHADOW_OFFSET;      // Feet are at shadowOffset from bottom
    float local_bottom = src.h;           // Head is at top of sprite (becomes bottom after flip)

    // Shadow vertices with LOCAL position in pixels for Duelyst-style blur
    ShadowVertex vertices[4] = {
        {x0, y0, u0, v0, local_left,  local_top},     // top-left     (feet end)
        {x1, y1, u1, v0, local_right, local_top},     // top-right    (feet end)
        {x2, y2, u1, v1, local_right, local_bottom},  // bottom-right (head end)
        {x3, y3, u0, v1, local_left,  local_bottom}   // bottom-left  (head end)
    };

    SDL_GPUTransferBufferCreateInfo transfer_info = {};
    transfer_info.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
    transfer_info.size = sizeof(vertices);

    SDL_GPUTransferBuffer* transfer = SDL_CreateGPUTransferBuffer(device, &transfer_info);
    void* map = SDL_MapGPUTransferBuffer(device, transfer, false);
    std::memcpy(map, vertices, sizeof(vertices));
    SDL_UnmapGPUTransferBuffer(device, transfer);

    // Interrupt current pass for copy operation
    interrupt_render_pass();

    SDL_GPUCopyPass* copy = SDL_BeginGPUCopyPass(cmd_buffer);
    SDL_GPUTransferBufferLocation src_loc = {};
    src_loc.transfer_buffer = transfer;
    SDL_GPUBufferRegion dst_region = {};
    dst_region.buffer = shadow_vertex_buffer;
    dst_region.size = sizeof(vertices);
    SDL_UploadToGPUBuffer(copy, &src_loc, &dst_region, false);
    SDL_EndGPUCopyPass(copy);

    SDL_ReleaseGPUTransferBuffer(device, transfer);

    // Resume to correct target (surface pass in multipass, swapchain otherwise)
    resume_render_pass();
    if (!render_pass) return;

    SDL_BindGPUGraphicsPipeline(render_pass, shadow_pipeline);

    SDL_GPUBufferBinding vb_binding = {};
    vb_binding.buffer = shadow_vertex_buffer;
    SDL_BindGPUVertexBuffers(render_pass, 0, &vb_binding, 1);

    SDL_GPUBufferBinding ib_binding = {};
    ib_binding.buffer = quad_index_buffer;
    SDL_BindGPUIndexBuffer(render_pass, &ib_binding, SDL_GPU_INDEXELEMENTSIZE_16BIT);

    SDL_GPUTextureSamplerBinding tex_binding = {};
    tex_binding.texture = texture.ptr;
    tex_binding.sampler = texture.sampler;
    SDL_BindGPUFragmentSamplers(render_pass, 0, &tex_binding, 1);

    // Calculate UV bounds for the sprite region in the atlas
    // Note: v0/v1 are flipped (v0 is bottom of sprite, v1 is top) but min/max handles it
    float uv_min_x = std::min(u0, u1);
    float uv_max_x = std::max(u0, u1);
    float uv_min_y = std::min(v0, v1);
    float uv_max_y = std::max(v0, v1);

    // One-time diagnostic
    static bool logged = false;
    if (!logged) {
        logged = true;
        SDL_Log("=== SHADOW DIAGNOSTIC (with lighting) ===");
        SDL_Log("Sprite src: %.1f x %.1f", src.w, src.h);
        SDL_Log("Shadow size: %.1f x %.1f (scale=%.2f)", sprite_w, sprite_h, scale);
        SDL_Log("SHADOW_SQUASH=%.2f SHADOW_OFFSET=%.1f", SHADOW_SQUASH, SHADOW_OFFSET);
        SDL_Log("UV span: %.4f x %.4f", uv_max_x - uv_min_x, uv_max_y - uv_min_y);
        SDL_Log("Intensity=%.2f BlurMod=%.1f RenderScale=%.1f", SHADOW_INTENSITY, BLUR_INTENSITY_MODIFIER, scale);
        SDL_Log("Has scene light: %s", has_scene_light ? "yes" : "no");
        if (active_light) {
            SDL_Log("Light: pos=(%.1f, %.1f) radius=%.1f intensity=%.2f alpha=%.2f",
                    active_light->x, active_light->y, active_light->radius,
                    active_light->intensity, active_light->a);
            SDL_Log("lightDistPctInv=%.3f lightAlpha=%.3f effectiveLightFactor=%.3f",
                    lightDistPctInv, lightAlpha, effectiveLightFactor);
        }
        SDL_Log("=========================================");
    }

    // Uniforms matching Duelyst's ShadowHighQualityFragment.glsl
    // u_anchor = (anchorX * width, shadowOffset) = (0.5 * src.w, SHADOW_OFFSET)
    ShadowUniforms uniforms = {
        opacity,                    // u_opacity
        SHADOW_INTENSITY,           // u_intensity
        BLUR_SHIFT_MODIFIER,        // u_blurShiftModifier
        BLUR_INTENSITY_MODIFIER,    // u_blurIntensityModifier
        src.w, src.h,               // u_size (sprite content size for distance calc)
        src.w * 0.5f, SHADOW_OFFSET,// u_anchor (center X, shadow offset Y)
        uv_min_x, uv_min_y,         // u_uvMin (for clamping blur samples)
        uv_max_x, uv_max_y,         // u_uvMax
        scale,                      // u_renderScale
        effectiveLightFactor,       // u_lightDistPctInv (includes light alpha)
        0.0f, 0.0f                  // padding
    };
    SDL_PushGPUFragmentUniformData(cmd_buffer, 0, &uniforms, sizeof(uniforms));

    SDL_DrawGPUIndexedPrimitives(render_pass, 6, 1, 0, 0, 0);
}

void GPURenderer::draw_quad_colored(const SDL_FRect& dst, SDL_FColor color) {
    if (!render_pass || !color_pipeline) return;

    float x0 = (dst.x / swapchain_w) * 2.0f - 1.0f;
    float y0 = 1.0f - (dst.y / swapchain_h) * 2.0f;
    float x1 = ((dst.x + dst.w) / swapchain_w) * 2.0f - 1.0f;
    float y1 = 1.0f - ((dst.y + dst.h) / swapchain_h) * 2.0f;

    ColorVertex vertices[4] = {
        {x0, y0, color.r, color.g, color.b, color.a},
        {x1, y0, color.r, color.g, color.b, color.a},
        {x1, y1, color.r, color.g, color.b, color.a},
        {x0, y1, color.r, color.g, color.b, color.a}
    };

    SDL_GPUTransferBufferCreateInfo transfer_info = {};
    transfer_info.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
    transfer_info.size = sizeof(vertices);

    SDL_GPUTransferBuffer* transfer = SDL_CreateGPUTransferBuffer(device, &transfer_info);
    void* map = SDL_MapGPUTransferBuffer(device, transfer, false);
    std::memcpy(map, vertices, sizeof(vertices));
    SDL_UnmapGPUTransferBuffer(device, transfer);

    SDL_EndGPURenderPass(render_pass);

    SDL_GPUCopyPass* copy = SDL_BeginGPUCopyPass(cmd_buffer);
    SDL_GPUTransferBufferLocation src_loc = {};
    src_loc.transfer_buffer = transfer;
    SDL_GPUBufferRegion dst_region = {};
    dst_region.buffer = quad_vertex_buffer;
    dst_region.size = sizeof(vertices);
    SDL_UploadToGPUBuffer(copy, &src_loc, &dst_region, false);
    SDL_EndGPUCopyPass(copy);

    SDL_GPUColorTargetInfo color_target = {};
    color_target.texture = swapchain_texture;
    color_target.load_op = SDL_GPU_LOADOP_LOAD;
    color_target.store_op = SDL_GPU_STOREOP_STORE;
    render_pass = SDL_BeginGPURenderPass(cmd_buffer, &color_target, 1, nullptr);

    SDL_GPUViewport viewport = {0, 0, (float)swapchain_w, (float)swapchain_h, 0.0f, 1.0f};
    SDL_SetGPUViewport(render_pass, &viewport);

    SDL_Rect scissor = {0, 0, (int)swapchain_w, (int)swapchain_h};
    SDL_SetGPUScissor(render_pass, &scissor);

    SDL_ReleaseGPUTransferBuffer(device, transfer);

    SDL_BindGPUGraphicsPipeline(render_pass, color_pipeline);

    SDL_GPUBufferBinding vb_binding = {};
    vb_binding.buffer = quad_vertex_buffer;
    SDL_BindGPUVertexBuffers(render_pass, 0, &vb_binding, 1);

    SDL_GPUBufferBinding ib_binding = {};
    ib_binding.buffer = quad_index_buffer;
    SDL_BindGPUIndexBuffer(render_pass, &ib_binding, SDL_GPU_INDEXELEMENTSIZE_16BIT);

    SDL_DrawGPUIndexedPrimitives(render_pass, 6, 1, 0, 0, 0);
}

void GPURenderer::draw_quad_transformed(Vec2 tl, Vec2 tr, Vec2 br, Vec2 bl, SDL_FColor color) {
    if (!render_pass || !color_pipeline) return;

    float x0 = (tl.x / swapchain_w) * 2.0f - 1.0f;
    float y0 = 1.0f - (tl.y / swapchain_h) * 2.0f;
    float x1 = (tr.x / swapchain_w) * 2.0f - 1.0f;
    float y1 = 1.0f - (tr.y / swapchain_h) * 2.0f;
    float x2 = (br.x / swapchain_w) * 2.0f - 1.0f;
    float y2 = 1.0f - (br.y / swapchain_h) * 2.0f;
    float x3 = (bl.x / swapchain_w) * 2.0f - 1.0f;
    float y3 = 1.0f - (bl.y / swapchain_h) * 2.0f;

    ColorVertex vertices[4] = {
        {x0, y0, color.r, color.g, color.b, color.a},
        {x1, y1, color.r, color.g, color.b, color.a},
        {x2, y2, color.r, color.g, color.b, color.a},
        {x3, y3, color.r, color.g, color.b, color.a}
    };

    SDL_GPUTransferBufferCreateInfo transfer_info = {};
    transfer_info.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
    transfer_info.size = sizeof(vertices);

    SDL_GPUTransferBuffer* transfer = SDL_CreateGPUTransferBuffer(device, &transfer_info);
    void* map = SDL_MapGPUTransferBuffer(device, transfer, false);
    std::memcpy(map, vertices, sizeof(vertices));
    SDL_UnmapGPUTransferBuffer(device, transfer);

    SDL_EndGPURenderPass(render_pass);

    SDL_GPUCopyPass* copy = SDL_BeginGPUCopyPass(cmd_buffer);
    SDL_GPUTransferBufferLocation src_loc = {};
    src_loc.transfer_buffer = transfer;
    SDL_GPUBufferRegion dst_region = {};
    dst_region.buffer = quad_vertex_buffer;
    dst_region.size = sizeof(vertices);
    SDL_UploadToGPUBuffer(copy, &src_loc, &dst_region, false);
    SDL_EndGPUCopyPass(copy);

    SDL_GPUColorTargetInfo color_target = {};
    color_target.texture = swapchain_texture;
    color_target.load_op = SDL_GPU_LOADOP_LOAD;
    color_target.store_op = SDL_GPU_STOREOP_STORE;
    render_pass = SDL_BeginGPURenderPass(cmd_buffer, &color_target, 1, nullptr);

    SDL_GPUViewport viewport = {0, 0, (float)swapchain_w, (float)swapchain_h, 0.0f, 1.0f};
    SDL_SetGPUViewport(render_pass, &viewport);

    SDL_Rect scissor = {0, 0, (int)swapchain_w, (int)swapchain_h};
    SDL_SetGPUScissor(render_pass, &scissor);

    SDL_ReleaseGPUTransferBuffer(device, transfer);

    SDL_BindGPUGraphicsPipeline(render_pass, color_pipeline);

    SDL_GPUBufferBinding vb_binding = {};
    vb_binding.buffer = quad_vertex_buffer;
    SDL_BindGPUVertexBuffers(render_pass, 0, &vb_binding, 1);

    SDL_GPUBufferBinding ib_binding = {};
    ib_binding.buffer = quad_index_buffer;
    SDL_BindGPUIndexBuffer(render_pass, &ib_binding, SDL_GPU_INDEXELEMENTSIZE_16BIT);

    SDL_DrawGPUIndexedPrimitives(render_pass, 6, 1, 0, 0, 0);
}

void GPURenderer::draw_line(Vec2 start, Vec2 end, SDL_FColor color) {
    if (!render_pass || !line_pipeline) return;

    float x0 = (start.x / swapchain_w) * 2.0f - 1.0f;
    float y0 = 1.0f - (start.y / swapchain_h) * 2.0f;
    float x1 = (end.x / swapchain_w) * 2.0f - 1.0f;
    float y1 = 1.0f - (end.y / swapchain_h) * 2.0f;

    ColorVertex vertices[2] = {
        {x0, y0, color.r, color.g, color.b, color.a},
        {x1, y1, color.r, color.g, color.b, color.a}
    };

    SDL_GPUTransferBufferCreateInfo transfer_info = {};
    transfer_info.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
    transfer_info.size = sizeof(vertices);

    SDL_GPUTransferBuffer* transfer = SDL_CreateGPUTransferBuffer(device, &transfer_info);
    void* map = SDL_MapGPUTransferBuffer(device, transfer, false);
    std::memcpy(map, vertices, sizeof(vertices));
    SDL_UnmapGPUTransferBuffer(device, transfer);

    SDL_EndGPURenderPass(render_pass);

    SDL_GPUCopyPass* copy = SDL_BeginGPUCopyPass(cmd_buffer);
    SDL_GPUTransferBufferLocation src_loc = {};
    src_loc.transfer_buffer = transfer;
    SDL_GPUBufferRegion dst_region = {};
    dst_region.buffer = quad_vertex_buffer;
    dst_region.size = sizeof(vertices);
    SDL_UploadToGPUBuffer(copy, &src_loc, &dst_region, false);
    SDL_EndGPUCopyPass(copy);

    SDL_GPUColorTargetInfo color_target = {};
    color_target.texture = swapchain_texture;
    color_target.load_op = SDL_GPU_LOADOP_LOAD;
    color_target.store_op = SDL_GPU_STOREOP_STORE;
    render_pass = SDL_BeginGPURenderPass(cmd_buffer, &color_target, 1, nullptr);

    SDL_GPUViewport viewport = {0, 0, (float)swapchain_w, (float)swapchain_h, 0.0f, 1.0f};
    SDL_SetGPUViewport(render_pass, &viewport);

    SDL_Rect scissor = {0, 0, (int)swapchain_w, (int)swapchain_h};
    SDL_SetGPUScissor(render_pass, &scissor);

    SDL_ReleaseGPUTransferBuffer(device, transfer);

    SDL_BindGPUGraphicsPipeline(render_pass, line_pipeline);

    SDL_GPUBufferBinding vb_binding = {};
    vb_binding.buffer = quad_vertex_buffer;
    SDL_BindGPUVertexBuffers(render_pass, 0, &vb_binding, 1);

    SDL_DrawGPUPrimitives(render_pass, 2, 1, 0, 0);
}

void GPURenderer::set_scene_light(const PointLight& light) {
    scene_light = light;
    has_scene_light = true;
}

void GPURenderer::clear_scene_light() {
    has_scene_light = false;
}

// ============================================================================
// Multi-pass Rendering Methods
// ============================================================================

void GPURenderer::add_light(const PointLight& light) {
    lights.push_back(light);
}

void GPURenderer::clear_lights() {
    lights.clear();
}

void GPURenderer::draw_fullscreen_triangle() {
    // Draw fullscreen triangle (3 vertices, no vertex buffer)
    SDL_DrawGPUPrimitives(render_pass, 3, 1, 0, 0);
}

void GPURenderer::interrupt_render_pass() {
    if (render_pass) {
        SDL_EndGPURenderPass(render_pass);
        render_pass = nullptr;
    }
}

void GPURenderer::resume_render_pass() {
    if (render_pass) return;  // Already have an active pass

    // In multi-pass mode with surface pass active, resume to surface
    if (current_surface_pass && current_surface_pass->is_valid()) {
        render_ctx.cmd_buffer = cmd_buffer;
        if (render_ctx.begin_pass(current_surface_pass, 0.0f, 0.0f, 0.0f, 0.0f, false)) {
            render_pass = render_ctx.render_pass;
        }
        return;
    }

    // Otherwise resume to swapchain
    if (swapchain_texture) {
        SDL_GPUColorTargetInfo color_target = {};
        color_target.texture = swapchain_texture;
        color_target.load_op = SDL_GPU_LOADOP_LOAD;
        color_target.store_op = SDL_GPU_STOREOP_STORE;

        render_pass = SDL_BeginGPURenderPass(cmd_buffer, &color_target, 1, nullptr);

        if (render_pass) {
            SDL_GPUViewport viewport = {0, 0, (float)swapchain_w, (float)swapchain_h, 0.0f, 1.0f};
            SDL_SetGPUViewport(render_pass, &viewport);

            SDL_Rect scissor = {0, 0, (int)swapchain_w, (int)swapchain_h};
            SDL_SetGPUScissor(render_pass, &scissor);
        }
    }
}

void GPURenderer::begin_surface_pass() {
    if (!fx_config.use_multipass) return;

    interrupt_render_pass();

    RenderPass* surface = pass_manager.get(PassType::SurfaceA);
    if (!surface) return;

    render_ctx.cmd_buffer = cmd_buffer;
    render_ctx.begin_pass(surface, 0.0f, 0.0f, 0.0f, 0.0f, true);

    // Store current pass for later operations
    render_pass = render_ctx.render_pass;
    current_surface_pass = surface;  // Track for resume
}

void GPURenderer::end_surface_pass() {
    if (!fx_config.use_multipass) return;

    render_ctx.end_pass();
    render_pass = nullptr;
    current_surface_pass = nullptr;  // Clear tracking
}

void GPURenderer::blit_pass(RenderPass* src, float opacity) {
    if (!src || !copy_pipeline) return;

    SDL_BindGPUGraphicsPipeline(render_pass, copy_pipeline);

    SDL_GPUTextureSamplerBinding tex_binding = {};
    tex_binding.texture = src->get_texture();
    tex_binding.sampler = linear_sampler;
    SDL_BindGPUFragmentSamplers(render_pass, 0, &tex_binding, 1);

    CopyUniforms uniforms = {opacity, 0.0f, 0.0f, 0.0f};
    SDL_PushGPUFragmentUniformData(cmd_buffer, 0, &uniforms, sizeof(uniforms));

    draw_fullscreen_triangle();
}

void GPURenderer::execute_bloom_pass() {
    if (!fx_config.use_multipass || !fx_config.enable_bloom) return;
    if (!highpass_pipeline || !blur_pipeline || !bloom_pipeline) return;

    RenderPass* surface = pass_manager.get(PassType::SurfaceA);
    RenderPass* highpass = pass_manager.get(PassType::Highpass);
    RenderPass* blur = pass_manager.get(PassType::Blur);
    RenderPass* bloom = pass_manager.get(PassType::Bloom);
    RenderPass* bloomA = pass_manager.get(PassType::BloomCompositeA);
    RenderPass* bloomB = pass_manager.get(PassType::BloomCompositeB);

    if (!surface || !highpass || !blur || !bloom || !bloomA || !bloomB) return;

    // Step 1: Extract bright pixels to highpass
    render_ctx.begin_pass(highpass, 0.0f, 0.0f, 0.0f, 0.0f, true);
    render_pass = render_ctx.render_pass;

    SDL_BindGPUGraphicsPipeline(render_pass, highpass_pipeline);

    SDL_GPUTextureSamplerBinding tex_binding = {};
    tex_binding.texture = surface->get_texture();
    tex_binding.sampler = linear_sampler;
    SDL_BindGPUFragmentSamplers(render_pass, 0, &tex_binding, 1);

    HighpassUniforms hp_uniforms = {fx_config.bloom_threshold, 0.1f, 0.0f, 0.0f};
    SDL_PushGPUFragmentUniformData(cmd_buffer, 0, &hp_uniforms, sizeof(hp_uniforms));

    draw_fullscreen_triangle();
    render_ctx.end_pass();

    // Step 2: Horizontal blur
    render_ctx.begin_pass(blur, 0.0f, 0.0f, 0.0f, 0.0f, true);
    render_pass = render_ctx.render_pass;

    SDL_BindGPUGraphicsPipeline(render_pass, blur_pipeline);

    tex_binding.texture = highpass->get_texture();
    SDL_BindGPUFragmentSamplers(render_pass, 0, &tex_binding, 1);

    BlurUniforms blur_uniforms = {1.0f / highpass->width, 0.0f, 2.0f, 0.0f};
    SDL_PushGPUFragmentUniformData(cmd_buffer, 0, &blur_uniforms, sizeof(blur_uniforms));

    draw_fullscreen_triangle();
    render_ctx.end_pass();

    // Step 3: Vertical blur
    render_ctx.begin_pass(bloom, 0.0f, 0.0f, 0.0f, 0.0f, true);
    render_pass = render_ctx.render_pass;

    SDL_BindGPUGraphicsPipeline(render_pass, blur_pipeline);

    tex_binding.texture = blur->get_texture();
    SDL_BindGPUFragmentSamplers(render_pass, 0, &tex_binding, 1);

    blur_uniforms.direction_x = 0.0f;
    blur_uniforms.direction_y = 1.0f / blur->height;
    SDL_PushGPUFragmentUniformData(cmd_buffer, 0, &blur_uniforms, sizeof(blur_uniforms));

    draw_fullscreen_triangle();
    render_ctx.end_pass();

    // Step 4: Temporal bloom accumulation (blend with previous frame)
    render_ctx.begin_pass(bloomA, 0.0f, 0.0f, 0.0f, 0.0f, true);
    render_pass = render_ctx.render_pass;

    SDL_BindGPUGraphicsPipeline(render_pass, bloom_pipeline);

    tex_binding.texture = bloom->get_texture();
    SDL_BindGPUFragmentSamplers(render_pass, 0, &tex_binding, 1);

    SDL_GPUTextureSamplerBinding tex_binding2 = {};
    tex_binding2.texture = bloomB->get_texture();
    tex_binding2.sampler = linear_sampler;
    SDL_BindGPUFragmentSamplers(render_pass, 1, &tex_binding2, 1);

    BloomUniforms bloom_uniforms = {fx_config.bloom_transition, fx_config.bloom_intensity, 0.0f, 0.0f};
    SDL_PushGPUFragmentUniformData(cmd_buffer, 0, &bloom_uniforms, sizeof(bloom_uniforms));

    draw_fullscreen_triangle();
    render_ctx.end_pass();

    render_pass = nullptr;
}

void GPURenderer::composite_to_screen() {
    if (!fx_config.use_multipass) return;
    if (!composite_pipeline) return;

    RenderPass* surface = pass_manager.get(PassType::SurfaceA);
    RenderPass* bloomA = pass_manager.get(PassType::BloomCompositeA);

    if (!surface || !bloomA) return;

    // Resume swapchain rendering
    resume_render_pass();
    if (!render_pass) return;

    SDL_BindGPUGraphicsPipeline(render_pass, composite_pipeline);

    // Bind surface texture
    SDL_GPUTextureSamplerBinding tex_binding = {};
    tex_binding.texture = surface->get_texture();
    tex_binding.sampler = linear_sampler;
    SDL_BindGPUFragmentSamplers(render_pass, 0, &tex_binding, 1);

    // Bind bloom texture
    SDL_GPUTextureSamplerBinding bloom_binding = {};
    bloom_binding.texture = bloomA->get_texture();
    bloom_binding.sampler = linear_sampler;
    SDL_BindGPUFragmentSamplers(render_pass, 1, &bloom_binding, 1);

    CompositeUniforms comp_uniforms = {
        fx_config.bloom_intensity,
        fx_config.bloom_scale,
        fx_config.exposure,
        fx_config.gamma
    };
    SDL_PushGPUFragmentUniformData(cmd_buffer, 0, &comp_uniforms, sizeof(comp_uniforms));

    draw_fullscreen_triangle();

    // Swap bloom double buffers for next frame
    // (This would need to be handled in the pass manager)
}

RenderPass* GPURenderer::draw_sprite_to_pass(
    const GPUTextureHandle& texture,
    const SDL_FRect& src,
    Uint32 sprite_w, Uint32 sprite_h
) {
    if (!fx_config.use_multipass) return nullptr;
    if (!texture.ptr) return nullptr;
    if (!sprite_offscreen_pipeline) return nullptr;

    // Acquire a sprite pass from the pool
    RenderPass* sprite_pass = pass_manager.acquire_sprite_shadow_pass(sprite_w, sprite_h);
    if (!sprite_pass) {
        SDL_Log("draw_sprite_to_pass: Failed to acquire sprite pass");
        return nullptr;
    }

    // Interrupt current render pass
    interrupt_render_pass();

    // Draw sprite to fill the entire FBO (UV 0-1)
    // This will be used for shadow/lighting calculations
    float u0 = src.x / texture.width;
    float v0 = src.y / texture.height;
    float u1 = (src.x + src.w) / texture.width;
    float v1 = (src.y + src.h) / texture.height;

    SpriteVertex vertices[4] = {
        {-1.0f,  1.0f, u0, v0},
        { 1.0f,  1.0f, u1, v0},
        { 1.0f, -1.0f, u1, v1},
        {-1.0f, -1.0f, u0, v1}
    };

    // Upload vertices via copy pass first (before render pass)
    SDL_GPUTransferBufferCreateInfo transfer_info = {};
    transfer_info.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
    transfer_info.size = sizeof(vertices);

    SDL_GPUTransferBuffer* transfer = SDL_CreateGPUTransferBuffer(device, &transfer_info);
    if (!transfer) {
        SDL_Log("draw_sprite_to_pass: Failed to create transfer buffer");
        resume_render_pass();
        return nullptr;
    }

    void* map = SDL_MapGPUTransferBuffer(device, transfer, false);
    std::memcpy(map, vertices, sizeof(vertices));
    SDL_UnmapGPUTransferBuffer(device, transfer);

    SDL_GPUCopyPass* copy = SDL_BeginGPUCopyPass(cmd_buffer);
    SDL_GPUTransferBufferLocation src_loc = {};
    src_loc.transfer_buffer = transfer;
    SDL_GPUBufferRegion dst_region = {};
    dst_region.buffer = quad_vertex_buffer;
    dst_region.size = sizeof(vertices);
    SDL_UploadToGPUBuffer(copy, &src_loc, &dst_region, false);
    SDL_EndGPUCopyPass(copy);

    // Now begin render pass and draw
    render_ctx.cmd_buffer = cmd_buffer;
    if (!render_ctx.begin_pass(sprite_pass, 0.0f, 0.0f, 0.0f, 0.0f, true)) {
        SDL_Log("draw_sprite_to_pass: Failed to begin render pass");
        SDL_ReleaseGPUTransferBuffer(device, transfer);
        resume_render_pass();
        return nullptr;
    }

    SDL_BindGPUGraphicsPipeline(render_ctx.render_pass, sprite_offscreen_pipeline);

    SDL_GPUBufferBinding vb_binding = {};
    vb_binding.buffer = quad_vertex_buffer;
    SDL_BindGPUVertexBuffers(render_ctx.render_pass, 0, &vb_binding, 1);

    SDL_GPUBufferBinding ib_binding = {};
    ib_binding.buffer = quad_index_buffer;
    SDL_BindGPUIndexBuffer(render_ctx.render_pass, &ib_binding, SDL_GPU_INDEXELEMENTSIZE_16BIT);

    SDL_GPUTextureSamplerBinding tex_binding = {};
    tex_binding.texture = texture.ptr;
    tex_binding.sampler = texture.sampler;
    SDL_BindGPUFragmentSamplers(render_ctx.render_pass, 0, &tex_binding, 1);

    SpriteUniforms uniforms = {1.0f, 0.0f, 0.0f, 0.0f};
    SDL_PushGPUFragmentUniformData(cmd_buffer, 0, &uniforms, sizeof(uniforms));

    SDL_DrawGPUIndexedPrimitives(render_ctx.render_pass, 6, 1, 0, 0, 0);

    render_ctx.end_pass();
    SDL_ReleaseGPUTransferBuffer(device, transfer);

    // Resume main render pass
    resume_render_pass();

    return sprite_pass;
}

void GPURenderer::draw_shadow_from_pass(
    RenderPass* sprite_pass,
    Vec2 feet_pos,
    float scale,
    bool flip_x,
    float opacity,
    const PointLight* light
) {
    if (!sprite_pass || !shadow_perpass_pipeline) return;
    if (!render_pass) return;

    // Shadow parameters from Duelyst
    constexpr float SHADOW_SQUASH = 0.7f;     // 45° foreshortening
    constexpr float SHADOW_STRETCH = 1.0f;

    const float SHADOW_INTENSITY = fx_config.shadow_intensity;
    const float BLUR_SHIFT_MODIFIER = fx_config.shadow_blur_shift;
    const float BLUR_INTENSITY_MODIFIER = fx_config.shadow_blur_intensity;

    // Use provided light, or scene light, or default
    const PointLight* active_light = light;
    if (!active_light && has_scene_light) {
        active_light = &scene_light;
    }

    // Calculate light distance attenuation
    float lightDistPctInv = 1.0f;
    if (active_light) {
        float dx = feet_pos.x - active_light->x;
        float dy = feet_pos.y - active_light->y;
        float lightDist = std::sqrt(dx * dx + dy * dy);
        float lightDistPct = std::pow(lightDist / active_light->radius, 2.0f);
        lightDistPctInv = std::max(0.0f, 1.0f - lightDistPct);
        lightDistPctInv *= active_light->a * active_light->intensity;
    }

    // Sprite size from the pass
    float sprite_w = static_cast<float>(sprite_pass->width) * scale * SHADOW_STRETCH;
    float sprite_h = static_cast<float>(sprite_pass->height) * scale * SHADOW_SQUASH;

    // UV coordinates are 0-1 for the entire sprite texture
    // Flip V so head silhouette is at bottom of shadow quad
    float u0 = flip_x ? 1.0f : 0.0f;
    float v0 = 1.0f;  // Bottom of sprite
    float u1 = flip_x ? 0.0f : 1.0f;
    float v1 = 0.0f;  // Top of sprite

    // Shadow positioning
    float feet_offset = SHADOW_OFFSET * scale * SHADOW_SQUASH;
    float shadow_top_y = feet_pos.y - feet_offset;

    float tl_x = feet_pos.x - sprite_w * 0.5f;
    float tl_y = shadow_top_y;
    float tr_x = feet_pos.x + sprite_w * 0.5f;
    float tr_y = shadow_top_y;
    float bl_x = feet_pos.x - sprite_w * 0.5f;
    float bl_y = shadow_top_y + sprite_h;
    float br_x = feet_pos.x + sprite_w * 0.5f;
    float br_y = shadow_top_y + sprite_h;

    // Convert to NDC
    float x0 = (tl_x / swapchain_w) * 2.0f - 1.0f;
    float y0 = 1.0f - (tl_y / swapchain_h) * 2.0f;
    float x1 = (tr_x / swapchain_w) * 2.0f - 1.0f;
    float y1 = 1.0f - (tr_y / swapchain_h) * 2.0f;
    float x2 = (br_x / swapchain_w) * 2.0f - 1.0f;
    float y2 = 1.0f - (br_y / swapchain_h) * 2.0f;
    float x3 = (bl_x / swapchain_w) * 2.0f - 1.0f;
    float y3 = 1.0f - (bl_y / swapchain_h) * 2.0f;

    // Local sprite positions for blur calculation
    float local_left = 0.0f;
    float local_right = static_cast<float>(sprite_pass->width);
    float local_top = SHADOW_OFFSET;
    float local_bottom = static_cast<float>(sprite_pass->height);

    ShadowVertex vertices[4] = {
        {x0, y0, u0, v0, local_left,  local_top},
        {x1, y1, u1, v0, local_right, local_top},
        {x2, y2, u1, v1, local_right, local_bottom},
        {x3, y3, u0, v1, local_left,  local_bottom}
    };

    // Upload vertices
    SDL_GPUTransferBufferCreateInfo transfer_info = {};
    transfer_info.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
    transfer_info.size = sizeof(vertices);

    SDL_GPUTransferBuffer* transfer = SDL_CreateGPUTransferBuffer(device, &transfer_info);
    void* map = SDL_MapGPUTransferBuffer(device, transfer, false);
    std::memcpy(map, vertices, sizeof(vertices));
    SDL_UnmapGPUTransferBuffer(device, transfer);

    // Interrupt current pass for copy operation
    interrupt_render_pass();

    SDL_GPUCopyPass* copy = SDL_BeginGPUCopyPass(cmd_buffer);
    SDL_GPUTransferBufferLocation src_loc = {};
    src_loc.transfer_buffer = transfer;
    SDL_GPUBufferRegion dst_region = {};
    dst_region.buffer = shadow_vertex_buffer;
    dst_region.size = sizeof(vertices);
    SDL_UploadToGPUBuffer(copy, &src_loc, &dst_region, false);
    SDL_EndGPUCopyPass(copy);

    SDL_ReleaseGPUTransferBuffer(device, transfer);

    // Resume to correct target (surface pass in multipass, swapchain otherwise)
    resume_render_pass();
    if (!render_pass) return;

    SDL_BindGPUGraphicsPipeline(render_pass, shadow_perpass_pipeline);

    SDL_GPUBufferBinding vb_binding = {};
    vb_binding.buffer = shadow_vertex_buffer;
    SDL_BindGPUVertexBuffers(render_pass, 0, &vb_binding, 1);

    SDL_GPUBufferBinding ib_binding = {};
    ib_binding.buffer = quad_index_buffer;
    SDL_BindGPUIndexBuffer(render_pass, &ib_binding, SDL_GPU_INDEXELEMENTSIZE_16BIT);

    SDL_GPUTextureSamplerBinding tex_binding = {};
    tex_binding.texture = sprite_pass->get_texture();
    tex_binding.sampler = sprite_pass->get_sampler();
    SDL_BindGPUFragmentSamplers(render_pass, 0, &tex_binding, 1);

    // Uniforms for per-sprite shadow (simpler than atlas version)
    ShadowPerPassUniforms uniforms = {
        opacity,
        SHADOW_INTENSITY,
        BLUR_SHIFT_MODIFIER,
        BLUR_INTENSITY_MODIFIER,
        static_cast<float>(sprite_pass->width),
        static_cast<float>(sprite_pass->height),
        static_cast<float>(sprite_pass->width) * 0.5f,  // anchor X
        SHADOW_OFFSET,                                   // anchor Y
        lightDistPctInv,
        scale,                                           // render scale for consistent blur
        0.0f, 0.0f                                       // padding
    };
    SDL_PushGPUFragmentUniformData(cmd_buffer, 0, &uniforms, sizeof(uniforms));

    SDL_DrawGPUIndexedPrimitives(render_pass, 6, 1, 0, 0, 0);
}

// ============================================================================
// Lighting System Methods
// ============================================================================

size_t GPURenderer::add_dynamic_light(const Light& light) {
    return lighting.add_light(light);
}

void GPURenderer::remove_dynamic_light(size_t idx) {
    lighting.remove_light(idx);
}

void GPURenderer::clear_dynamic_lights() {
    lighting.clear_lights();
}

void GPURenderer::update_lighting(float dt) {
    lighting.update(dt);
    lighting.prepare_batch();
}

void GPURenderer::draw_lit_sprite(
    const GPUTextureHandle& texture,
    const SDL_FRect& src,
    const SDL_FRect& dst,
    bool flip_x,
    float opacity
) {
    if (!fx_config.enable_lighting || lighting.lights.empty()) {
        // No dynamic lighting, use regular sprite draw
        draw_sprite(texture, src, dst, flip_x, opacity);
        return;
    }

    // For now, compute simple lighting and tint the sprite
    // Full implementation would:
    // 1. Render sprite to per-sprite FBO
    // 2. Render light accumulation to light FBO
    // 3. Multiply sprite with light map
    // 4. Composite to screen

    // Get light contribution at sprite center
    float cx = dst.x + dst.w * 0.5f;
    float cy = dst.y + dst.h * 0.5f;
    float lr, lg, lb;
    lighting.get_light_at(cx, cy, lr, lg, lb);

    // For now, just draw the regular sprite
    // TODO: Implement full per-sprite light accumulation
    draw_sprite(texture, src, dst, flip_x, opacity);
}

void GPURenderer::accumulate_sprite_light(
    RenderPass* sprite_light_pass,
    const GPUTextureHandle& sprite_texture,
    const SDL_FRect& sprite_bounds,
    const Light& light
) {
    if (!sprite_light_pass || !lighting_pipeline) return;

    // This would render a light contribution to the sprite's light FBO
    // Each light affecting the sprite gets drawn additively

    // The light is rendered as a fullscreen quad with the light shader
    // which calculates per-pixel light contribution based on distance

    // For now, this is a placeholder for the full implementation
    // The actual implementation would:
    // 1. Bind the sprite_light_pass as render target
    // 2. Set additive blending
    // 3. Bind the lighting_pipeline
    // 4. Set light uniforms (position, color, radius, falloff)
    // 5. Draw fullscreen quad
}

// ============================================================================
// Post-Processing Methods
// ============================================================================

void GPURenderer::execute_post_processing() {
    if (!fx_config.use_multipass) return;

    RenderPass* surfaceA = pass_manager.get(PassType::SurfaceA);
    RenderPass* surfaceB = pass_manager.get(PassType::SurfaceB);
    if (!surfaceA || !surfaceB) return;

    RenderPass* current = surfaceA;
    RenderPass* next = surfaceB;

    // Apply radial blur if enabled
    if (fx_config.enable_radial_blur && fx_config.radial_blur_strength > 0.001f) {
        apply_radial_blur(current, next);
        std::swap(current, next);
    }

    // Apply tone curve if enabled
    if (fx_config.enable_tone_curve) {
        apply_tone_curve(current, next);
        std::swap(current, next);
    }

    // Apply vignette if enabled
    if (fx_config.enable_vignette && fx_config.vignette_intensity > 0.001f) {
        apply_vignette(current, next);
        std::swap(current, next);
    }

    // If we ended up on surfaceB, copy back to surfaceA for final composite
    // (In practice we'd just use whatever surface we ended on)
}

void GPURenderer::apply_radial_blur(RenderPass* src, RenderPass* dst) {
    if (!radial_blur_pipeline || !src || !dst) return;

    render_ctx.cmd_buffer = cmd_buffer;
    render_ctx.begin_pass(dst, 0.0f, 0.0f, 0.0f, 0.0f, true);
    render_pass = render_ctx.render_pass;

    SDL_BindGPUGraphicsPipeline(render_pass, radial_blur_pipeline);

    SDL_GPUTextureSamplerBinding tex_binding = {};
    tex_binding.texture = src->get_texture();
    tex_binding.sampler = linear_sampler;
    SDL_BindGPUFragmentSamplers(render_pass, 0, &tex_binding, 1);

    RadialBlurUniforms uniforms = {
        fx_config.radial_blur_center_x,
        fx_config.radial_blur_center_y,
        fx_config.radial_blur_strength,
        fx_config.radial_blur_samples
    };
    SDL_PushGPUFragmentUniformData(cmd_buffer, 0, &uniforms, sizeof(uniforms));

    draw_fullscreen_triangle();
    render_ctx.end_pass();
    render_pass = nullptr;
}

void GPURenderer::apply_tone_curve(RenderPass* src, RenderPass* dst) {
    if (!tone_curve_pipeline || !src || !dst) return;

    render_ctx.cmd_buffer = cmd_buffer;
    render_ctx.begin_pass(dst, 0.0f, 0.0f, 0.0f, 0.0f, true);
    render_pass = render_ctx.render_pass;

    SDL_BindGPUGraphicsPipeline(render_pass, tone_curve_pipeline);

    SDL_GPUTextureSamplerBinding tex_binding = {};
    tex_binding.texture = src->get_texture();
    tex_binding.sampler = linear_sampler;
    SDL_BindGPUFragmentSamplers(render_pass, 0, &tex_binding, 1);

    // Second sampler would be LUT texture (not implemented yet)
    SDL_BindGPUFragmentSamplers(render_pass, 1, &tex_binding, 1);

    ToneCurveUniforms uniforms = {
        fx_config.tone_intensity,
        fx_config.tone_contrast,
        fx_config.tone_brightness,
        fx_config.tone_saturation
    };
    SDL_PushGPUFragmentUniformData(cmd_buffer, 0, &uniforms, sizeof(uniforms));

    draw_fullscreen_triangle();
    render_ctx.end_pass();
    render_pass = nullptr;
}

void GPURenderer::apply_vignette(RenderPass* src, RenderPass* dst) {
    if (!vignette_pipeline || !src || !dst) return;

    render_ctx.cmd_buffer = cmd_buffer;
    render_ctx.begin_pass(dst, 0.0f, 0.0f, 0.0f, 0.0f, true);
    render_pass = render_ctx.render_pass;

    SDL_BindGPUGraphicsPipeline(render_pass, vignette_pipeline);

    SDL_GPUTextureSamplerBinding tex_binding = {};
    tex_binding.texture = src->get_texture();
    tex_binding.sampler = linear_sampler;
    SDL_BindGPUFragmentSamplers(render_pass, 0, &tex_binding, 1);

    float aspect = static_cast<float>(swapchain_w) / static_cast<float>(swapchain_h);
    VignetteUniforms uniforms = {
        fx_config.vignette_intensity,
        fx_config.vignette_radius,
        fx_config.vignette_softness,
        aspect
    };
    SDL_PushGPUFragmentUniformData(cmd_buffer, 0, &uniforms, sizeof(uniforms));

    draw_fullscreen_triangle();
    render_ctx.end_pass();
    render_pass = nullptr;
}
