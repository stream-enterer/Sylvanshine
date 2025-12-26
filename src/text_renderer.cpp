#include "text_renderer.hpp"
#include <fstream>
#include <cstring>
#include "../third_party/json.hpp"

TextRenderer g_text;

bool TextRenderer::load(const char* atlas_png, const char* metrics_json) {
    // Load atlas texture
    atlas = g_gpu.load_texture(atlas_png);
    if (!atlas) {
        SDL_Log("Failed to load font atlas: %s", atlas_png);
        return false;
    }

    // Use linear filtering for MSDF
    atlas.sampler = g_gpu.linear_sampler;

    atlas_width = static_cast<float>(atlas.width);
    atlas_height = static_cast<float>(atlas.height);

    // Parse JSON metrics
    std::ifstream file(metrics_json);
    if (!file.is_open()) {
        SDL_Log("Failed to open font metrics: %s", metrics_json);
        return false;
    }

    nlohmann::json j;
    try {
        file >> j;
    } catch (const std::exception& e) {
        SDL_Log("Failed to parse font metrics JSON: %s", e.what());
        return false;
    }

    // Read atlas info
    auto& atlas_info = j["atlas"];
    em_size = atlas_info["size"].get<float>();

    // Read metrics
    auto& metrics = j["metrics"];
    line_height = metrics["lineHeight"].get<float>() * em_size;
    ascender = metrics["ascender"].get<float>() * em_size;
    descender = metrics["descender"].get<float>() * em_size;

    // Read glyphs
    auto& glyph_array = j["glyphs"];
    for (auto& g : glyph_array) {
        int unicode = g["unicode"].get<int>();
        Glyph glyph = {};

        glyph.advance = g["advance"].get<float>() * em_size;

        // Skip glyphs without bounds (e.g., space)
        if (g.contains("planeBounds") && g.contains("atlasBounds")) {
            auto& plane = g["planeBounds"];
            auto& ab = g["atlasBounds"];

            // planeBounds are in em units, convert to pixels at atlas size
            float plane_left = plane["left"].get<float>() * em_size;
            float plane_bottom = plane["bottom"].get<float>() * em_size;
            float plane_right = plane["right"].get<float>() * em_size;
            float plane_top = plane["top"].get<float>() * em_size;

            glyph.width = plane_right - plane_left;
            glyph.height = plane_top - plane_bottom;
            glyph.xoffset = plane_left;
            // yoffset is distance from baseline to top of glyph (for top-down rendering)
            glyph.yoffset = plane_top;

            // atlasBounds are in pixel coords, y-origin at bottom
            float ab_left = ab["left"].get<float>();
            float ab_bottom = ab["bottom"].get<float>();
            float ab_right = ab["right"].get<float>();
            float ab_top = ab["top"].get<float>();

            // Convert to UV coords (0-1), flipping y since atlas y-origin is bottom
            glyph.u0 = ab_left / atlas_width;
            glyph.v0 = 1.0f - ab_top / atlas_height;    // top in atlas = lower v
            glyph.u1 = ab_right / atlas_width;
            glyph.v1 = 1.0f - ab_bottom / atlas_height; // bottom in atlas = higher v
        }

        glyphs[unicode] = glyph;
    }

    SDL_Log("Loaded font: %zu glyphs, em_size=%.0f, line_height=%.1f",
            glyphs.size(), em_size, line_height);
    return true;
}

void TextRenderer::draw_text(const std::string& text, float x, float y,
                              float size, SDL_FColor color) {
    if (!g_gpu.text_pipeline || !atlas) return;

    float scale = size / em_size;
    float cursor_x = x;
    float baseline_y = y + ascender * scale;  // y is top of text, baseline is below

    // Generate vertices for all characters
    std::vector<TextVertex> vertices;
    std::vector<uint16_t> indices;

    for (char c : text) {
        auto it = glyphs.find(static_cast<int>(c));
        if (it == glyphs.end()) {
            // Try to find a replacement character
            it = glyphs.find('?');
            if (it == glyphs.end()) {
                cursor_x += size * 0.5f;  // Skip unknown chars
                continue;
            }
        }

        const Glyph& g = it->second;

        // Skip glyphs without visual representation (like space)
        if (g.width > 0 && g.height > 0) {
            // Calculate quad corners in screen space
            float gx = cursor_x + g.xoffset * scale;
            float gy = baseline_y - g.yoffset * scale;  // yoffset is from baseline to top
            float gw = g.width * scale;
            float gh = g.height * scale;

            uint16_t base_idx = static_cast<uint16_t>(vertices.size());

            // Convert to NDC
            float sw = static_cast<float>(g_gpu.swapchain_w);
            float sh = static_cast<float>(g_gpu.swapchain_h);

            auto to_ndc_x = [sw](float px) { return (px / sw) * 2.0f - 1.0f; };
            auto to_ndc_y = [sh](float py) { return 1.0f - (py / sh) * 2.0f; };

            float x0 = to_ndc_x(gx);
            float y0 = to_ndc_y(gy);
            float x1 = to_ndc_x(gx + gw);
            float y1 = to_ndc_y(gy + gh);

            // Add vertices (TL, TR, BR, BL)
            vertices.push_back({x0, y0, g.u0, g.v0, color.r, color.g, color.b, color.a});
            vertices.push_back({x1, y0, g.u1, g.v0, color.r, color.g, color.b, color.a});
            vertices.push_back({x1, y1, g.u1, g.v1, color.r, color.g, color.b, color.a});
            vertices.push_back({x0, y1, g.u0, g.v1, color.r, color.g, color.b, color.a});

            // Add indices (two triangles)
            indices.push_back(base_idx + 0);
            indices.push_back(base_idx + 1);
            indices.push_back(base_idx + 2);
            indices.push_back(base_idx + 0);
            indices.push_back(base_idx + 2);
            indices.push_back(base_idx + 3);
        }

        cursor_x += g.advance * scale;
    }

    if (vertices.empty()) return;

    // Upload vertices
    SDL_GPUTransferBufferCreateInfo vb_transfer_info = {};
    vb_transfer_info.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
    vb_transfer_info.size = vertices.size() * sizeof(TextVertex);

    SDL_GPUTransferBuffer* vb_transfer = SDL_CreateGPUTransferBuffer(g_gpu.device, &vb_transfer_info);
    void* vb_map = SDL_MapGPUTransferBuffer(g_gpu.device, vb_transfer, false);
    std::memcpy(vb_map, vertices.data(), vertices.size() * sizeof(TextVertex));
    SDL_UnmapGPUTransferBuffer(g_gpu.device, vb_transfer);

    // Upload indices
    SDL_GPUTransferBufferCreateInfo ib_transfer_info = {};
    ib_transfer_info.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
    ib_transfer_info.size = indices.size() * sizeof(uint16_t);

    SDL_GPUTransferBuffer* ib_transfer = SDL_CreateGPUTransferBuffer(g_gpu.device, &ib_transfer_info);
    void* ib_map = SDL_MapGPUTransferBuffer(g_gpu.device, ib_transfer, false);
    std::memcpy(ib_map, indices.data(), indices.size() * sizeof(uint16_t));
    SDL_UnmapGPUTransferBuffer(g_gpu.device, ib_transfer);

    // Create GPU buffers
    SDL_GPUBufferCreateInfo vb_info = {};
    vb_info.usage = SDL_GPU_BUFFERUSAGE_VERTEX;
    vb_info.size = vertices.size() * sizeof(TextVertex);
    SDL_GPUBuffer* vb = SDL_CreateGPUBuffer(g_gpu.device, &vb_info);

    SDL_GPUBufferCreateInfo ib_info = {};
    ib_info.usage = SDL_GPU_BUFFERUSAGE_INDEX;
    ib_info.size = indices.size() * sizeof(uint16_t);
    SDL_GPUBuffer* ib = SDL_CreateGPUBuffer(g_gpu.device, &ib_info);

    // Copy to GPU
    g_gpu.interrupt_render_pass();

    SDL_GPUCommandBuffer* cmd = g_gpu.cmd_buffer;
    SDL_GPUCopyPass* copy = SDL_BeginGPUCopyPass(cmd);

    SDL_GPUTransferBufferLocation vb_src = {};
    vb_src.transfer_buffer = vb_transfer;
    SDL_GPUBufferRegion vb_dst = {};
    vb_dst.buffer = vb;
    vb_dst.size = vb_info.size;
    SDL_UploadToGPUBuffer(copy, &vb_src, &vb_dst, false);

    SDL_GPUTransferBufferLocation ib_src = {};
    ib_src.transfer_buffer = ib_transfer;
    SDL_GPUBufferRegion ib_dst = {};
    ib_dst.buffer = ib;
    ib_dst.size = ib_info.size;
    SDL_UploadToGPUBuffer(copy, &ib_src, &ib_dst, false);

    SDL_EndGPUCopyPass(copy);

    SDL_ReleaseGPUTransferBuffer(g_gpu.device, vb_transfer);
    SDL_ReleaseGPUTransferBuffer(g_gpu.device, ib_transfer);

    g_gpu.resume_render_pass();

    // Draw
    SDL_BindGPUGraphicsPipeline(g_gpu.render_pass, g_gpu.text_pipeline);

    SDL_GPUBufferBinding vb_binding = {};
    vb_binding.buffer = vb;
    SDL_BindGPUVertexBuffers(g_gpu.render_pass, 0, &vb_binding, 1);

    SDL_GPUBufferBinding ib_binding = {};
    ib_binding.buffer = ib;
    SDL_BindGPUIndexBuffer(g_gpu.render_pass, &ib_binding, SDL_GPU_INDEXELEMENTSIZE_16BIT);

    SDL_GPUTextureSamplerBinding tex_binding = {};
    tex_binding.texture = atlas.ptr;
    tex_binding.sampler = atlas.sampler;
    SDL_BindGPUFragmentSamplers(g_gpu.render_pass, 0, &tex_binding, 1);

    SDL_DrawGPUIndexedPrimitives(g_gpu.render_pass, static_cast<Uint32>(indices.size()), 1, 0, 0, 0);

    // Cleanup (deferred)
    SDL_ReleaseGPUBuffer(g_gpu.device, vb);
    SDL_ReleaseGPUBuffer(g_gpu.device, ib);
}

float TextRenderer::measure_width(const std::string& text, float size) const {
    float scale = size / em_size;
    float width = 0;

    for (char c : text) {
        auto it = glyphs.find(static_cast<int>(c));
        if (it != glyphs.end()) {
            width += it->second.advance * scale;
        } else {
            width += size * 0.5f;
        }
    }

    return width;
}
