#pragma once
#include "gpu_renderer.hpp"
#include <string>
#include <unordered_map>

struct Glyph {
    float u0, v0, u1, v1;  // UV coords in atlas
    float width, height;   // Glyph size in pixels (at atlas em size)
    float xoffset, yoffset; // Bearing (offset from baseline)
    float advance;         // Horizontal advance
};

struct TextRenderer {
    GPUTextureHandle atlas;
    std::unordered_map<int, Glyph> glyphs;  // keyed by unicode codepoint
    float atlas_width = 0;
    float atlas_height = 0;
    float em_size = 0;     // Font size used when generating atlas
    float line_height = 0;
    float ascender = 0;
    float descender = 0;

    bool load(const char* atlas_png, const char* metrics_json);

    // Draw text at screen position (top-left origin)
    // size is the desired font height in pixels
    void draw_text(const std::string& text, float x, float y,
                   float size, SDL_FColor color);

    // Measure text width in pixels at given size
    float measure_width(const std::string& text, float size) const;
};

extern TextRenderer g_text;
extern TextRenderer g_title_text;
