#include "fx.hpp"
#include "asset_manager.hpp"
#include <SDL3_image/SDL_image.h>
#include <fstream>
#include <sstream>

std::vector<std::string> split_string(const std::string& str, char delimiter) {
    std::vector<std::string> result;
    std::stringstream ss(str);
    std::string token;
    while (std::getline(ss, token, delimiter)) {
        if (!token.empty()) {
            result.push_back(token);
        }
    }
    return result;
}

// Legacy TSV parsing - kept for reference but no longer used
// RSX mappings are now loaded from assets.json via AssetManager
bool parse_rsx_mapping(const char* filepath, std::unordered_map<std::string, RSXMapping>& mappings) {
    (void)filepath;
    (void)mappings;
    // This function is now a no-op - RSX mappings come from assets.json
    return true;
}

// Legacy TSV parsing - kept for reference but no longer used
// FX data is now loaded from assets.json via AssetManager
bool parse_fx_manifest(const char* filepath, std::unordered_map<std::string, FXManifestEntry>& manifest) {
    (void)filepath;
    (void)manifest;
    // This function is now a no-op - FX manifest comes from assets.json
    return true;
}

bool FXCache::load_mappings(const char* rsx_mapping_path, const char* manifest_path) {
    // Legacy TSV loading - now handled by AssetManager
    // This function is kept for API compatibility but mappings come from assets.json
    (void)rsx_mapping_path;
    (void)manifest_path;
    return true;
}

FXAsset* FXCache::get_asset(const std::string& folder) {
    auto it = loaded_assets.find(folder);
    if (it != loaded_assets.end()) {
        return &it->second;
    }

    // Get FX data from AssetManager (pre-parsed from assets.json)
    const auto* am_asset = AssetManager::instance().get_fx(folder);
    if (!am_asset) {
        SDL_Log("FX folder not found in assets: %s", folder.c_str());
        return nullptr;
    }

    FXAsset asset;
    std::string spritesheet_path = AssetManager::instance().get_fx_spritesheet_path(folder);
    asset.texture = g_gpu.load_texture(spritesheet_path.c_str());
    if (!asset.texture.ptr) {
        SDL_Log("Failed to load FX spritesheet: %s", spritesheet_path.c_str());
        return nullptr;
    }

    // Copy pre-parsed animations from AssetManager
    asset.animations = am_asset->animations;

    if (asset.animations.animations.empty()) {
        SDL_Log("Warning: No animations for FX: %s", folder.c_str());
    }

    loaded_assets[folder] = std::move(asset);
    SDL_Log("Loaded FX asset: %s from dist/", folder.c_str());

    return &loaded_assets[folder];
}

const Animation* FXCache::get_animation(const std::string& rsx_name) {
    // Resolve RSX name to FX folder and animation name
    auto mapping = AssetManager::instance().resolve_rsx(rsx_name);
    if (mapping.folder.empty()) {
        SDL_Log("RSX name not found: %s", rsx_name.c_str());
        return nullptr;
    }

    FXAsset* asset = get_asset(mapping.folder);
    if (!asset) {
        return nullptr;
    }

    // Use the animation name from the mapping
    const char* anim_name = mapping.anim.empty() ? rsx_name.c_str() : mapping.anim.c_str();
    return asset->animations.find(anim_name);
}

const GPUTextureHandle* FXCache::get_texture(const std::string& rsx_name) {
    auto mapping = AssetManager::instance().resolve_rsx(rsx_name);
    if (mapping.folder.empty()) {
        return nullptr;
    }

    FXAsset* asset = get_asset(mapping.folder);
    if (!asset) {
        return nullptr;
    }

    return &asset->texture;
}

FXEntity::FXEntity()
    : pos{0, 0}
    , spritesheet(nullptr)
    , anim(nullptr)
    , elapsed(0.0f)
    , complete(false)
    , scale(1.0f) {
}

void FXEntity::update(float dt) {
    if (complete || !anim) return;
    
    elapsed += dt;
    float duration = anim->duration();
    
    if (elapsed >= duration) {
        complete = true;
    }
}

void FXEntity::render(const RenderConfig& config) const {
    if (complete || !spritesheet || !spritesheet->ptr || !anim || anim->frames.empty()) return;

    int frame_idx = static_cast<int>(elapsed * anim->fps);
    if (frame_idx >= static_cast<int>(anim->frames.size())) {
        frame_idx = anim->frames.size() - 1;
    }

    const SDL_Rect& src_rect = anim->frames[frame_idx].rect;

    SDL_FRect src = {
        static_cast<float>(src_rect.x),
        static_cast<float>(src_rect.y),
        static_cast<float>(src_rect.w),
        static_cast<float>(src_rect.h)
    };

    float scaled_w = src.w * config.scale * scale;
    float scaled_h = src.h * config.scale * scale;

    SDL_FRect dst = {
        pos.x - scaled_w * 0.5f,
        pos.y - scaled_h * 0.5f,
        scaled_w,
        scaled_h
    };

    g_gpu.draw_sprite(*spritesheet, src, dst, false, 1.0f);
}

FXEntity create_fx(FXCache& cache, const std::string& rsx_name, Vec2 position) {
    FXEntity fx;
    fx.pos = position;
    fx.anim = cache.get_animation(rsx_name);
    fx.spritesheet = cache.get_texture(rsx_name);
    fx.elapsed = 0.0f;
    fx.complete = (fx.anim == nullptr || fx.spritesheet == nullptr);
    fx.scale = 1.0f;

    if (fx.complete) {
        SDL_Log("Failed to create FX: %s", rsx_name.c_str());
    }

    return fx;
}
