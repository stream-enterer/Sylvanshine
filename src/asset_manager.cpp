#include "asset_manager.hpp"
#include <GL/glew.h>

#include <SDL3_image/SDL_image.h>

#include <fstream>
#include <iostream>
#include <sstream>

AssetManager::AssetManager()
    : shadow_texture_(0),
      tiles_board_texture_(0),
      tiles_action_texture_(0) {}

AssetManager::~AssetManager() {
    Shutdown();
}

bool AssetManager::Initialize(const std::string& data_root) {
    data_root_ = data_root;

    if (!LoadRsxMapping()) {
        std::cerr << "Failed to load RSX mapping" << std::endl;
        return false;
    }

    shadow_texture_ = LoadTexture(data_root_ + "/units/unit_shadow.png");
    tiles_board_texture_ = LoadTexture(data_root_ + "/tiles/tiles_board.png");
    tiles_action_texture_ = LoadTexture(data_root_ + "/tiles/tiles_action.png");

    return true;
}

void AssetManager::Shutdown() {
    for (auto& pair : textures_) {
        if (pair.second != 0) {
            glDeleteTextures(1, &pair.second);
        }
    }
    textures_.clear();
    units_.clear();
    fx_cache_.clear();
}

bool AssetManager::LoadRsxMapping() {
    std::string path = data_root_ + "/fx/rsx_mapping.tsv";
    std::ifstream file(path);
    if (!file.is_open()) {
        std::cerr << "Failed to open RSX mapping: " << path << std::endl;
        return false;
    }

    std::string line;
    std::getline(file, line);

    while (std::getline(file, line)) {
        if (line.empty()) continue;

        std::istringstream iss(line);
        std::string rsx_name, folder, prefix;

        if (!std::getline(iss, rsx_name, '\t')) continue;
        if (!std::getline(iss, folder, '\t')) continue;
        std::getline(iss, prefix, '\t');

        rsx_mapping_[rsx_name] = {folder, prefix};
    }

    return true;
}

UnitAssets* AssetManager::LoadUnit(const std::string& unit_id) {
    auto it = units_.find(unit_id);
    if (it != units_.end()) {
        return &it->second;
    }

    std::string unit_path = data_root_ + "/units/" + unit_id;

    UnitAssets assets;
    assets.unit_id = unit_id;
    assets.shadow_offset = DEFAULT_SHADOW_OFFSET;

    if (!assets.spritesheet.Load(unit_path + "/spritesheet.png")) {
        std::cerr << "Failed to load unit spritesheet: " << unit_id << std::endl;
        return nullptr;
    }

    if (!assets.spritesheet.LoadAnimations(unit_path + "/animations.txt")) {
        std::cerr << "Failed to load unit animations: " << unit_id << std::endl;
        return nullptr;
    }

    LoadSounds(unit_path + "/sounds.txt", assets.sounds);

    units_[unit_id] = std::move(assets);
    return &units_[unit_id];
}

FxAssets* AssetManager::LoadFx(const std::string& rsx_name) {
    auto it = fx_cache_.find(rsx_name);
    if (it != fx_cache_.end()) {
        return &it->second;
    }

    auto mapping_it = rsx_mapping_.find(rsx_name);
    if (mapping_it == rsx_mapping_.end()) {
        std::cerr << "RSX name not found in mapping: " << rsx_name << std::endl;
        return nullptr;
    }

    const std::string& folder = mapping_it->second.first;
    const std::string& prefix = mapping_it->second.second;

    std::string fx_path = data_root_ + "/fx/" + folder;

    FxAssets assets;
    assets.folder = folder;
    assets.prefix = prefix;

    if (!assets.spritesheet.Load(fx_path + "/spritesheet.png")) {
        std::cerr << "Failed to load FX spritesheet: " << rsx_name << std::endl;
        return nullptr;
    }

    if (!assets.spritesheet.LoadAnimations(fx_path + "/animations.txt")) {
        std::cerr << "Failed to load FX animations: " << rsx_name << std::endl;
        return nullptr;
    }

    fx_cache_[rsx_name] = std::move(assets);
    return &fx_cache_[rsx_name];
}

bool AssetManager::LoadSounds(const std::string& path,
                               std::unordered_map<std::string, std::string>& sounds) {
    std::ifstream file(path);
    if (!file.is_open()) {
        return false;
    }

    std::string line;
    while (std::getline(file, line)) {
        if (line.empty()) continue;

        std::istringstream iss(line);
        std::string event_name, sound_path;

        if (!std::getline(iss, event_name, '\t')) continue;
        if (!std::getline(iss, sound_path)) continue;

        sounds[event_name] = sound_path;
    }

    return true;
}

unsigned int AssetManager::LoadTexture(const std::string& path) {
    auto it = textures_.find(path);
    if (it != textures_.end()) {
        return it->second;
    }

    SDL_Surface* surface = IMG_Load(path.c_str());
    if (!surface) {
        std::cerr << "Failed to load texture: " << path << std::endl;
        return 0;
    }

    SDL_Surface* rgba_surface = SDL_ConvertSurface(surface, SDL_PIXELFORMAT_RGBA32);
    SDL_DestroySurface(surface);

    if (!rgba_surface) {
        std::cerr << "Failed to convert surface to RGBA: " << path << std::endl;
        return 0;
    }

    unsigned int texture_id;
    glGenTextures(1, &texture_id);
    glBindTexture(GL_TEXTURE_2D, texture_id);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, rgba_surface->w, rgba_surface->h, 0,
                 GL_RGBA, GL_UNSIGNED_BYTE, rgba_surface->pixels);

    SDL_DestroySurface(rgba_surface);

    textures_[path] = texture_id;
    return texture_id;
}

unsigned int AssetManager::GetTexture(const std::string& path) const {
    auto it = textures_.find(path);
    if (it != textures_.end()) {
        return it->second;
    }
    return 0;
}
