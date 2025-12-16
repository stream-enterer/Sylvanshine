#include "fx.hpp"
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

bool parse_rsx_mapping(const char* filepath, std::unordered_map<std::string, RSXMapping>& mappings) {
    std::ifstream file(filepath);
    if (!file) {
        SDL_Log("Failed to open RSX mapping file: %s", filepath);
        return false;
    }
    
    std::string line;
    bool first_line = true;
    
    while (std::getline(file, line)) {
        if (line.empty()) continue;
        if (first_line) {
            first_line = false;
            continue;
        }
        
        size_t tab1 = line.find('\t');
        if (tab1 == std::string::npos) continue;
        
        size_t tab2 = line.find('\t', tab1 + 1);
        if (tab2 == std::string::npos) continue;
        
        RSXMapping mapping;
        mapping.rsx_name = line.substr(0, tab1);
        mapping.folder = line.substr(tab1 + 1, tab2 - tab1 - 1);
        mapping.prefix = line.substr(tab2 + 1);
        
        mappings[mapping.rsx_name] = mapping;
    }
    
    SDL_Log("Loaded %zu RSX mappings", mappings.size());
    return true;
}

bool parse_fx_manifest(const char* filepath, std::unordered_map<std::string, FXManifestEntry>& manifest) {
    std::ifstream file(filepath);
    if (!file) {
        SDL_Log("Failed to open FX manifest file: %s", filepath);
        return false;
    }
    
    std::string line;
    bool first_line = true;
    
    while (std::getline(file, line)) {
        if (line.empty()) continue;
        if (first_line) {
            first_line = false;
            continue;
        }
        
        size_t tab1 = line.find('\t');
        if (tab1 == std::string::npos) continue;
        
        size_t tab2 = line.find('\t', tab1 + 1);
        if (tab2 == std::string::npos) continue;
        
        FXManifestEntry entry;
        entry.folder = line.substr(0, tab1);
        entry.spritesheet_path = "data/" + line.substr(tab1 + 1, tab2 - tab1 - 1);
        
        std::string anims_str = line.substr(tab2 + 1);
        entry.animations = split_string(anims_str, ',');
        
        manifest[entry.folder] = entry;
    }
    
    SDL_Log("Loaded %zu FX manifest entries", manifest.size());
    return true;
}

bool FXCache::load_mappings(const char* rsx_mapping_path, const char* manifest_path) {
    if (!parse_rsx_mapping(rsx_mapping_path, rsx_mappings)) {
        return false;
    }
    if (!parse_fx_manifest(manifest_path, manifest)) {
        return false;
    }
    return true;
}

FXAsset* FXCache::get_asset(SDL_Renderer* renderer, const std::string& folder) {
    auto it = loaded_assets.find(folder);
    if (it != loaded_assets.end()) {
        return &it->second;
    }
    
    auto manifest_it = manifest.find(folder);
    if (manifest_it == manifest.end()) {
        SDL_Log("FX folder not in manifest: %s", folder.c_str());
        return nullptr;
    }
    
    const FXManifestEntry& entry = manifest_it->second;
    
    SurfaceHandle surface(IMG_Load(entry.spritesheet_path.c_str()));
    if (!surface) {
        SDL_Log("Failed to load FX spritesheet: %s - %s", entry.spritesheet_path.c_str(), SDL_GetError());
        return nullptr;
    }
    
    FXAsset asset;
    asset.texture = TextureHandle(SDL_CreateTextureFromSurface(renderer, surface.get()));
    if (!asset.texture) {
        SDL_Log("Failed to create FX texture: %s", SDL_GetError());
        return nullptr;
    }
    
    SDL_SetTextureScaleMode(asset.texture.get(), SDL_SCALEMODE_NEAREST);
    SDL_SetTextureBlendMode(asset.texture.get(), SDL_BLENDMODE_BLEND);
    
    std::string anim_path = "data/fx/" + folder + "/animations.txt";
    asset.animations = load_animations(anim_path.c_str());
    
    loaded_assets[folder] = std::move(asset);
    SDL_Log("Loaded FX asset: %s", folder.c_str());
    
    return &loaded_assets[folder];
}

const Animation* FXCache::get_animation(SDL_Renderer* renderer, const std::string& rsx_name) {
    auto mapping_it = rsx_mappings.find(rsx_name);
    if (mapping_it == rsx_mappings.end()) {
        SDL_Log("RSX name not found: %s", rsx_name.c_str());
        return nullptr;
    }
    
    const RSXMapping& mapping = mapping_it->second;
    FXAsset* asset = get_asset(renderer, mapping.folder);
    if (!asset) {
        return nullptr;
    }
    
    return asset->animations.find(rsx_name.c_str());
}

SDL_Texture* FXCache::get_texture(SDL_Renderer* renderer, const std::string& rsx_name) {
    auto mapping_it = rsx_mappings.find(rsx_name);
    if (mapping_it == rsx_mappings.end()) {
        return nullptr;
    }
    
    const RSXMapping& mapping = mapping_it->second;
    FXAsset* asset = get_asset(renderer, mapping.folder);
    if (!asset) {
        return nullptr;
    }
    
    return asset->texture.get();
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

void FXEntity::render(SDL_Renderer* renderer, const RenderConfig& config) const {
    if (complete || !spritesheet || !anim || anim->frames.empty()) return;
    
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
    
    SDL_RenderTexture(renderer, spritesheet, &src, &dst);
}

FXEntity create_fx(FXCache& cache, SDL_Renderer* renderer, const std::string& rsx_name, Vec2 position) {
    FXEntity fx;
    fx.pos = position;
    fx.anim = cache.get_animation(renderer, rsx_name);
    fx.spritesheet = cache.get_texture(renderer, rsx_name);
    fx.elapsed = 0.0f;
    fx.complete = (fx.anim == nullptr || fx.spritesheet == nullptr);
    fx.scale = 1.0f;
    
    if (fx.complete) {
        SDL_Log("Failed to create FX: %s", rsx_name.c_str());
    }
    
    return fx;
}
