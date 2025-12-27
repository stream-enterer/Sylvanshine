#pragma once
#include "types.hpp"
#include "animation_loader.hpp"
#include "gpu_renderer.hpp"
#include <SDL3/SDL.h>
#include <string>
#include <unordered_map>

struct FXAsset {
    GPUTextureHandle texture;
    AnimationSet animations;
};

struct FXCache {
    std::unordered_map<std::string, FXAsset> loaded_assets;

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
