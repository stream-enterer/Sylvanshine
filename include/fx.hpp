#pragma once
#include "types.hpp"
#include "animation_loader.hpp"
#include "sdl_handles.hpp"
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
    TextureHandle texture;
    AnimationSet animations;
};

struct FXCache {
    std::unordered_map<std::string, RSXMapping> rsx_mappings;
    std::unordered_map<std::string, FXManifestEntry> manifest;
    std::unordered_map<std::string, FXAsset> loaded_assets;
    
    bool load_mappings(const char* rsx_mapping_path, const char* manifest_path);
    FXAsset* get_asset(SDL_Renderer* renderer, const std::string& folder);
    const Animation* get_animation(SDL_Renderer* renderer, const std::string& rsx_name);
    SDL_Texture* get_texture(SDL_Renderer* renderer, const std::string& rsx_name);
};

struct FXEntity {
    Vec2 pos;
    SDL_Texture* spritesheet;
    const Animation* anim;
    float elapsed;
    bool complete;
    float scale;
    
    FXEntity();
    
    void update(float dt);
    void render(SDL_Renderer* renderer, const RenderConfig& config) const;
    bool is_complete() const { return complete; }
};

FXEntity create_fx(FXCache& cache, SDL_Renderer* renderer, const std::string& rsx_name, Vec2 position);
