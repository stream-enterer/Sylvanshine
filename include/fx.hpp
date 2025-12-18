#pragma once
#include "types.hpp"
#include "animation_loader.hpp"
#include "gpu_renderer.hpp"
#include <SDL3/SDL.h>
#include <string>
#include <unordered_map>
#include <vector>

struct RSXMapping {
    std::string rsx_name;
    std::string folder;
    std::string prefix;
};

struct FXManifestEntry {
    std::string folder;
    std::string spritesheet_path;
    std::vector<std::string> animations;
};

struct FXAsset {
    GPUTextureHandle texture;
    AnimationSet animations;
};

struct FXCache {
    std::unordered_map<std::string, RSXMapping> rsx_mappings;
    std::unordered_map<std::string, FXManifestEntry> manifest;
    std::unordered_map<std::string, FXAsset> loaded_assets;

    bool load_mappings(const char* rsx_mapping_path, const char* manifest_path);
    FXAsset* get_asset(const std::string& folder);
    const Animation* get_animation(const std::string& rsx_name);
    const GPUTextureHandle* get_texture(const std::string& rsx_name);
};

struct FXEntity {
    Vec2 pos;
    const GPUTextureHandle* spritesheet;
    const Animation* anim;
    float elapsed;
    bool complete;
    float scale;

    FXEntity();

    void update(float dt);
    void render(const RenderConfig& config) const;
    bool is_complete() const { return complete; }
};

FXEntity create_fx(FXCache& cache, const std::string& rsx_name, Vec2 position);
