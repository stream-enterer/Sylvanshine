#include "gpu_renderer.hpp"
#include <SDL3_image/SDL_image.h>
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

    if (!create_quad_buffers()) {
        return false;
    }

    if (!create_pipelines()) {
        return false;
    }

    SDL_Log("GPU renderer initialized (backend: %s)",
            SDL_GetGPUDeviceDriver(device));
    return true;
}

void GPURenderer::shutdown() {
    if (device) {
        if (sprite_pipeline) SDL_ReleaseGPUGraphicsPipeline(device, sprite_pipeline);
        if (dissolve_pipeline) SDL_ReleaseGPUGraphicsPipeline(device, dissolve_pipeline);
        if (color_pipeline) SDL_ReleaseGPUGraphicsPipeline(device, color_pipeline);
        if (line_pipeline) SDL_ReleaseGPUGraphicsPipeline(device, line_pipeline);
        if (quad_vertex_buffer) SDL_ReleaseGPUBuffer(device, quad_vertex_buffer);
        if (quad_index_buffer) SDL_ReleaseGPUBuffer(device, quad_index_buffer);
        if (default_sampler) SDL_ReleaseGPUSampler(device, default_sampler);
        SDL_ReleaseWindowFromGPUDevice(device, window);
        SDL_DestroyGPUDevice(device);
    }
    device = nullptr;
    sprite_pipeline = nullptr;
    dissolve_pipeline = nullptr;
    color_pipeline = nullptr;
    line_pipeline = nullptr;
    quad_vertex_buffer = nullptr;
    quad_index_buffer = nullptr;
    default_sampler = nullptr;
}

SDL_GPUShader* GPURenderer::load_shader(const char* filename, SDL_GPUShaderStage stage,
                                         Uint32 sampler_count, Uint32 uniform_buffer_count) {
    SDL_GPUShaderFormat formats = SDL_GetGPUShaderFormats(device);
    SDL_GPUShaderFormat format;
    const char* entrypoint = "main";
    char path[256];

    if (formats & SDL_GPU_SHADERFORMAT_SPIRV) {
        format = SDL_GPU_SHADERFORMAT_SPIRV;
        SDL_snprintf(path, sizeof(path), "data/shaders/compiled/spirv/%s.spv", filename);
    } else if (formats & SDL_GPU_SHADERFORMAT_MSL) {
        format = SDL_GPU_SHADERFORMAT_MSL;
        SDL_snprintf(path, sizeof(path), "data/shaders/compiled/msl/%s.msl", filename);
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
