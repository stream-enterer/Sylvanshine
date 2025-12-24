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

    SDL_Log("GPU renderer initialized (backend: %s)", SDL_GetGPUDeviceDriver(device));
    return true;
}

void GPURenderer::shutdown() {
    if (device) {
        SDL_WaitForGPUIdle(device);

        // Release pipelines
        if (sprite_pipeline) SDL_ReleaseGPUGraphicsPipeline(device, sprite_pipeline);
        if (dissolve_pipeline && dissolve_pipeline != sprite_pipeline) SDL_ReleaseGPUGraphicsPipeline(device, dissolve_pipeline);
        if (color_pipeline) SDL_ReleaseGPUGraphicsPipeline(device, color_pipeline);
        if (line_pipeline) SDL_ReleaseGPUGraphicsPipeline(device, line_pipeline);
        if (sdf_shadow_pipeline) SDL_ReleaseGPUGraphicsPipeline(device, sdf_shadow_pipeline);

        // Release buffers and samplers
        if (quad_vertex_buffer) SDL_ReleaseGPUBuffer(device, quad_vertex_buffer);
        if (shadow_vertex_buffer) SDL_ReleaseGPUBuffer(device, shadow_vertex_buffer);
        if (quad_index_buffer) SDL_ReleaseGPUBuffer(device, quad_index_buffer);
        if (default_sampler) SDL_ReleaseGPUSampler(device, default_sampler);
        if (linear_sampler) SDL_ReleaseGPUSampler(device, linear_sampler);

        SDL_ReleaseWindowFromGPUDevice(device, window);
        SDL_DestroyGPUDevice(device);
    }

    device = nullptr;
    sprite_pipeline = nullptr;
    dissolve_pipeline = nullptr;
    color_pipeline = nullptr;
    line_pipeline = nullptr;
    sdf_shadow_pipeline = nullptr;
    quad_vertex_buffer = nullptr;
    shadow_vertex_buffer = nullptr;
    quad_index_buffer = nullptr;
    default_sampler = nullptr;
    linear_sampler = nullptr;
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

    // SDF shadow pipeline
    SDL_GPUShader* sdf_shadow_vert = load_shader("sdf_shadow.vert", SDL_GPU_SHADERSTAGE_VERTEX, 0, 0);
    SDL_GPUShader* sdf_shadow_frag = load_shader("sdf_shadow.frag", SDL_GPU_SHADERSTAGE_FRAGMENT, 1, 1);

    if (sdf_shadow_vert && sdf_shadow_frag) {
        // Shadow vertex format: position (2f), texcoord (2f), localpos (2f)
        SDL_GPUVertexBufferDescription sdf_vb_desc = {};
        sdf_vb_desc.slot = 0;
        sdf_vb_desc.pitch = sizeof(ShadowVertex);
        sdf_vb_desc.input_rate = SDL_GPU_VERTEXINPUTRATE_VERTEX;

        SDL_GPUVertexAttribute sdf_attrs[3] = {};
        sdf_attrs[0].location = 0;
        sdf_attrs[0].buffer_slot = 0;
        sdf_attrs[0].format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2;
        sdf_attrs[0].offset = 0;
        sdf_attrs[1].location = 1;
        sdf_attrs[1].buffer_slot = 0;
        sdf_attrs[1].format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2;
        sdf_attrs[1].offset = sizeof(float) * 2;
        sdf_attrs[2].location = 2;
        sdf_attrs[2].buffer_slot = 0;
        sdf_attrs[2].format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2;
        sdf_attrs[2].offset = sizeof(float) * 4;

        pipeline_info.vertex_shader = sdf_shadow_vert;
        pipeline_info.fragment_shader = sdf_shadow_frag;
        pipeline_info.vertex_input_state.vertex_buffer_descriptions = &sdf_vb_desc;
        pipeline_info.vertex_input_state.num_vertex_attributes = 3;
        pipeline_info.vertex_input_state.vertex_attributes = sdf_attrs;

        sdf_shadow_pipeline = SDL_CreateGPUGraphicsPipeline(device, &pipeline_info);

        // Restore original vertex state for subsequent pipelines
        pipeline_info.vertex_input_state.vertex_buffer_descriptions = &vb_desc;
        pipeline_info.vertex_input_state.num_vertex_attributes = 2;
        pipeline_info.vertex_input_state.vertex_attributes = attrs;

        if (sdf_shadow_pipeline) {
            SDL_Log("SDF shadow pipeline created");
        } else {
            SDL_Log("Failed to create SDF shadow pipeline: %s", SDL_GetError());
        }
    } else {
        SDL_Log("SDF shadow shaders not found, shadows disabled");
    }
    if (sdf_shadow_vert) SDL_ReleaseGPUShader(device, sdf_shadow_vert);
    if (sdf_shadow_frag) SDL_ReleaseGPUShader(device, sdf_shadow_frag);

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

void GPURenderer::draw_sprite_transformed(const GPUTextureHandle& texture,
                                           const SDL_FRect& src,
                                           Vec2 tl, Vec2 tr, Vec2 br, Vec2 bl,
                                           float opacity) {
    if (!render_pass || !texture.ptr) return;

    // Calculate UV coordinates from source rect
    float u0 = src.x / texture.width;
    float v0 = src.y / texture.height;
    float u1 = (src.x + src.w) / texture.width;
    float v1 = (src.y + src.h) / texture.height;

    // Convert screen coords to NDC
    auto to_ndc = [this](Vec2 p) -> Vec2 {
        return {
            (p.x / swapchain_w) * 2.0f - 1.0f,
            1.0f - (p.y / swapchain_h) * 2.0f
        };
    };

    Vec2 tl_ndc = to_ndc(tl);
    Vec2 tr_ndc = to_ndc(tr);
    Vec2 br_ndc = to_ndc(br);
    Vec2 bl_ndc = to_ndc(bl);

    SpriteVertex vertices[4] = {
        {tl_ndc.x, tl_ndc.y, u0, v0},
        {tr_ndc.x, tr_ndc.y, u1, v0},
        {br_ndc.x, br_ndc.y, u1, v1},
        {bl_ndc.x, bl_ndc.y, u0, v1}
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

void GPURenderer::interrupt_render_pass() {
    if (render_pass) {
        SDL_EndGPURenderPass(render_pass);
        render_pass = nullptr;
    }
}

void GPURenderer::resume_render_pass() {
    if (render_pass) return;  // Already have an active pass

    // Resume to swapchain
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


void GPURenderer::draw_sdf_shadow(
    const GPUTextureHandle& sdf_texture,
    const SDL_FRect& src,
    Vec2 feet_pos,
    float scale,
    bool flip_x,
    float opacity,
    const PointLight* light
) {
    if (!sdf_shadow_pipeline || !sdf_texture.ptr) return;
    if (!cmd_buffer) return;

    // Get light to use
    const PointLight* active_light = light;
    if (!active_light && has_scene_light) {
        active_light = &scene_light;
    }
    if (!active_light) {
        // Default light (no attenuation)
        static PointLight default_light;
        default_light.x = feet_pos.x;
        default_light.y = feet_pos.y - 200.0f;
        default_light.radius = 1000.0f;
        default_light.intensity = 1.0f;
        default_light.a = 1.0f;
        active_light = &default_light;
    }

    // Calculate light direction (2D, normalized, in screen space)
    float dx = active_light->x - feet_pos.x;
    float dy = active_light->y - feet_pos.y;
    float light_dist = std::sqrt(dx * dx + dy * dy);
    if (light_dist < 0.001f) light_dist = 0.001f;

    // Normalize light direction (pointing FROM shadow TOWARD light)
    float light_dir_x = dx / light_dist;
    float light_dir_y = dy / light_dist;

    // Light attenuation
    float lightDistPct = std::pow(light_dist / active_light->radius, 2.0f);
    float lightDistPctInv = std::max(0.0f, 1.0f - lightDistPct);
    lightDistPctInv *= active_light->a * active_light->intensity;

    // Sprite dimensions
    float sprite_w = src.w;
    float sprite_h = src.h;

    // Shadow geometry - transform quad to cast shadow away from light
    // Supports both downward shadows (light above) and upward shadows (light below)
    constexpr float COS_45 = 0.7071067811865475f;
    constexpr float SHADOW_OFFSET = 19.5f;

    float shadow_w = sprite_w * scale;
    float shadow_h = sprite_h * scale * COS_45;  // Foreshortened onto ground plane

    // Determine shadow direction using DEPTH-BASED FLIP (Duelyst ShadowVertex.glsl:27-30)
    // Duelyst coordinate convention (from Light.js:319-322):
    // - In Cocos2d Y-up: screen Y becomes depth Z after swap
    // - High screen Y (top) = far from camera, low screen Y (bottom) = close
    // SDL is inverted: high screen Y = bottom = close to camera
    // So we invert: use (light.y - sprite.y) to match Duelyst's depth semantics
    float depth_diff = active_light->y - feet_pos.y;  // INVERTED for SDL coords
    // depth_diff < SHADOW_OFFSET: light behind sprite (far) → shadow DOWN (flip=1)
    // depth_diff >= SHADOW_OFFSET: light in front (close) → shadow UP (flip=-1)
    // This matches draw_shadow_from_pass: flip = (depth_diff - SHADOW_OFFSET) < 0 ? -1 : 1
    bool shadow_below = (depth_diff - SHADOW_OFFSET) >= 0.0f;
    float shadow_dir = shadow_below ? 1.0f : -1.0f;  // +1 = down, -1 = up

    // Calculate skew from light direction
    // Shadow skews opposite to light's horizontal component
    float skew = 0.0f;
    if (std::abs(light_dir_y) > 0.001f) {
        skew = -light_dir_x / std::abs(light_dir_y) * 0.5f;
    }

    // Stretch shadow based on light "altitude"
    float altitude = std::abs(light_dir_y);
    float stretch = 1.0f;
    if (altitude > 0.2f) {
        stretch = std::min(1.0f / std::sqrt(altitude), 1.6f);
    }
    shadow_h *= stretch;

    // Position shadow quad - anchor at feet, project in shadow direction
    float half_w = shadow_w * 0.5f;
    float shadow_center_x = feet_pos.x;

    // Skew offset for the far edge (head projection)
    float skew_offset = skew * shadow_h;

    // Build shadow quad vertices
    // y_near = edge closest to sprite, y_far = edge away from sprite
    // For shadow_below: near at small Y (top of quad), far at large Y (bottom)
    // For shadow_above: near at large Y (bottom of quad), far at small Y (top)
    float y_near = feet_pos.y;
    float y_far = feet_pos.y + shadow_dir * shadow_h;

    // Near edge has no skew (anchored at feet), far edge skewed based on light
    float x_near_left = shadow_center_x - half_w;
    float x_near_right = shadow_center_x + half_w;
    float x_far_left = shadow_center_x - half_w + skew_offset;
    float x_far_right = shadow_center_x + half_w + skew_offset;

    // Convert to NDC
    auto to_ndc_x = [this](float x) { return (x / swapchain_w) * 2.0f - 1.0f; };
    auto to_ndc_y = [this](float y) { return 1.0f - (y / swapchain_h) * 2.0f; };

    // UV coordinates in SDF atlas
    // Note: In the atlas, sprites are stored with head at TOP (small V), feet at BOTTOM (large V)
    // But the sprite visual has feet at bottom, so we need to flip V for correct shadow orientation
    float u0 = flip_x ? (src.x + src.w) / sdf_texture.width : src.x / sdf_texture.width;
    float u1 = flip_x ? src.x / sdf_texture.width : (src.x + src.w) / sdf_texture.width;
    float v_top = src.y / sdf_texture.height;             // Top of frame in atlas
    // Feet are SHADOW_OFFSET pixels above frame bottom - sample there, not at empty space below
    float v_bottom = (src.y + src.h - SHADOW_OFFSET) / sdf_texture.height;

    // Local positions for shader anchor calculations
    float local_left = 0.0f;
    float local_right = sprite_w;
    float local_bottom = sprite_h;  // Bottom of local space (feet in sprite)
    float local_top = 0.0f;         // Top of local space (head in sprite)

    // Build vertices
    // Near edge = v_bottom (feet in texture), Far edge = v_top (head in texture)
    // Local coords match: near=local_bottom (feet), far=local_top (head)
    // Quad winding: 0=TL, 1=TR, 2=BR, 3=BL
    ShadowVertex vertices[4];
    if (shadow_below) {
        // Shadow extends downward: y_near < y_far in screen coords
        vertices[0] = {to_ndc_x(x_near_left),  to_ndc_y(y_near), u0, v_bottom, local_left,  local_bottom};
        vertices[1] = {to_ndc_x(x_near_right), to_ndc_y(y_near), u1, v_bottom, local_right, local_bottom};
        vertices[2] = {to_ndc_x(x_far_right),  to_ndc_y(y_far),  u1, v_top,    local_right, local_top};
        vertices[3] = {to_ndc_x(x_far_left),   to_ndc_y(y_far),  u0, v_top,    local_left,  local_top};
    } else {
        // Shadow extends upward: y_far < y_near in screen coords
        vertices[0] = {to_ndc_x(x_far_left),   to_ndc_y(y_far),  u0, v_top,    local_left,  local_top};
        vertices[1] = {to_ndc_x(x_far_right),  to_ndc_y(y_far),  u1, v_top,    local_right, local_top};
        vertices[2] = {to_ndc_x(x_near_right), to_ndc_y(y_near), u1, v_bottom, local_right, local_bottom};
        vertices[3] = {to_ndc_x(x_near_left),  to_ndc_y(y_near), u0, v_bottom, local_left,  local_bottom};
    }

    // Upload vertices
    SDL_GPUTransferBufferCreateInfo transfer_info = {};
    transfer_info.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
    transfer_info.size = sizeof(vertices);

    SDL_GPUTransferBuffer* transfer = SDL_CreateGPUTransferBuffer(device, &transfer_info);
    void* map = SDL_MapGPUTransferBuffer(device, transfer, false);
    std::memcpy(map, vertices, sizeof(vertices));
    SDL_UnmapGPUTransferBuffer(device, transfer);

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

    resume_render_pass();
    if (!render_pass) return;

    SDL_BindGPUGraphicsPipeline(render_pass, sdf_shadow_pipeline);

    SDL_GPUBufferBinding vb_binding = {};
    vb_binding.buffer = shadow_vertex_buffer;
    SDL_BindGPUVertexBuffers(render_pass, 0, &vb_binding, 1);

    SDL_GPUBufferBinding ib_binding = {};
    ib_binding.buffer = quad_index_buffer;
    SDL_BindGPUIndexBuffer(render_pass, &ib_binding, SDL_GPU_INDEXELEMENTSIZE_16BIT);

    SDL_GPUTextureSamplerBinding tex_binding = {};
    tex_binding.texture = sdf_texture.ptr;
    tex_binding.sampler = linear_sampler;  // Use linear for smooth SDF sampling
    SDL_BindGPUFragmentSamplers(render_pass, 0, &tex_binding, 1);

    // Convert light direction to UV space
    // UV space is 0-1, so we need to scale by sprite size ratio
    float uv_light_dir_x = light_dir_x * (sprite_h / sprite_w);  // Adjust for aspect
    float uv_light_dir_y = -light_dir_y;  // Flip Y for UV space

    // Normalize UV light direction
    float uv_dir_len = std::sqrt(uv_light_dir_x * uv_light_dir_x + uv_light_dir_y * uv_light_dir_y);
    if (uv_dir_len > 0.001f) {
        uv_light_dir_x /= uv_dir_len;
        uv_light_dir_y /= uv_dir_len;
    }

    SDFShadowUniforms uniforms = {
        opacity,
        fx_config.shadow_intensity,
        fx_config.sdf_penumbra_scale,
        32.0f,  // SDF max distance (matches build_assets.py)

        sprite_w,
        sprite_h,
        sprite_w * 0.5f,  // anchor X (center)
        sprite_h - SHADOW_OFFSET,  // anchor Y (feet are SHADOW_OFFSET pixels up from bottom)

        uv_light_dir_x,
        uv_light_dir_y,
        light_dist,
        lightDistPctInv,

        fx_config.sdf_max_raymarch,
        fx_config.sdf_raymarch_steps,
        0.0f, 0.0f  // padding
    };
    SDL_PushGPUFragmentUniformData(cmd_buffer, 0, &uniforms, sizeof(uniforms));

    SDL_DrawGPUIndexedPrimitives(render_pass, 6, 1, 0, 0, 0);
}

