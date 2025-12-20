#include "asset_manager.hpp"
#include <SDL3/SDL.h>
#include <fstream>
#include <cstring>

// nlohmann/json for JSON parsing
#include "../third_party/json.hpp"
using json = nlohmann::json;

AssetManager& AssetManager::instance() {
    static AssetManager instance;
    return instance;
}

bool AssetManager::init(const std::string& dist_path) {
    if (initialized_) {
        SDL_Log("AssetManager already initialized");
        return true;
    }

    dist_path_ = dist_path;
    std::string assets_json_path = dist_path_ + "/assets.json";

    // Open and parse assets.json
    std::ifstream file(assets_json_path);
    if (!file.is_open()) {
        SDL_Log("Failed to open assets.json: %s", assets_json_path.c_str());
        return false;
    }

    json assets;
    try {
        file >> assets;
    } catch (const json::parse_error& e) {
        SDL_Log("Failed to parse assets.json: %s", e.what());
        return false;
    }

    // ===== Load Units =====
    if (assets.contains("units") && assets["units"].is_object()) {
        for (auto& [name, unit_data] : assets["units"].items()) {
            UnitAsset unit;

            // Get spritesheet path
            if (unit_data.contains("spritesheet")) {
                unit.spritesheet_path = unit_data["spritesheet"].get<std::string>();
            }

            // Parse animations
            if (unit_data.contains("animations") && unit_data["animations"].is_object()) {
                for (auto& [anim_name, anim_data] : unit_data["animations"].items()) {
                    Animation anim;

                    // Copy animation name (max 31 chars + null)
                    strncpy(anim.name, anim_name.c_str(), sizeof(anim.name) - 1);
                    anim.name[sizeof(anim.name) - 1] = '\0';

                    // Get FPS
                    anim.fps = anim_data.value("fps", 12);

                    // Parse frames
                    if (anim_data.contains("frames") && anim_data["frames"].is_array()) {
                        int idx = 0;
                        for (auto& frame_data : anim_data["frames"]) {
                            AnimFrame frame;
                            frame.idx = idx++;
                            frame.rect.x = frame_data.value("x", 0);
                            frame.rect.y = frame_data.value("y", 0);
                            frame.rect.w = frame_data.value("w", 0);
                            frame.rect.h = frame_data.value("h", 0);
                            anim.frames.push_back(frame);
                        }
                    }

                    unit.animations.animations.push_back(std::move(anim));
                }
            }

            units_[name] = std::move(unit);
        }
    }
    SDL_Log("Loaded %zu units from assets.json", units_.size());

    // ===== Load FX =====
    if (assets.contains("fx") && assets["fx"].is_object()) {
        for (auto& [name, fx_data] : assets["fx"].items()) {
            FXAsset fx;

            // Get spritesheet path
            if (fx_data.contains("spritesheet")) {
                fx.spritesheet_path = fx_data["spritesheet"].get<std::string>();
            }

            // Parse animations
            if (fx_data.contains("animations") && fx_data["animations"].is_object()) {
                for (auto& [anim_name, anim_data] : fx_data["animations"].items()) {
                    Animation anim;

                    strncpy(anim.name, anim_name.c_str(), sizeof(anim.name) - 1);
                    anim.name[sizeof(anim.name) - 1] = '\0';

                    anim.fps = anim_data.value("fps", 12);

                    if (anim_data.contains("frames") && anim_data["frames"].is_array()) {
                        int idx = 0;
                        for (auto& frame_data : anim_data["frames"]) {
                            AnimFrame frame;
                            frame.idx = idx++;
                            frame.rect.x = frame_data.value("x", 0);
                            frame.rect.y = frame_data.value("y", 0);
                            frame.rect.w = frame_data.value("w", 0);
                            frame.rect.h = frame_data.value("h", 0);
                            anim.frames.push_back(frame);
                        }
                    }

                    fx.animations.animations.push_back(std::move(anim));
                }
            }

            fx_[name] = std::move(fx);
        }
    }
    SDL_Log("Loaded %zu FX from assets.json", fx_.size());

    // ===== Load Timing =====
    if (assets.contains("timing") && assets["timing"].is_object()) {
        for (auto& [name, timing_data] : assets["timing"].items()) {
            UnitTiming timing;
            timing.attack_damage_delay = timing_data.value("attack_delay", 0.5f);
            timing_[name] = timing;
        }
    }
    SDL_Log("Loaded %zu timing entries from assets.json", timing_.size());

    // ===== Load RSX Mapping =====
    if (assets.contains("fx_mapping") && assets["fx_mapping"].is_object()) {
        for (auto& [rsx_id, mapping_data] : assets["fx_mapping"].items()) {
            RSXMapping mapping;
            if (mapping_data.is_object()) {
                mapping.folder = mapping_data.value("folder", "");
                mapping.anim = mapping_data.value("anim", "");
            } else if (mapping_data.is_string()) {
                // Legacy format: just folder string
                mapping.folder = mapping_data.get<std::string>();
            }
            rsx_mapping_[rsx_id] = mapping;
        }
    }
    SDL_Log("Loaded %zu RSX mappings from assets.json", rsx_mapping_.size());

    initialized_ = true;
    SDL_Log("AssetManager initialized from: %s", dist_path_.c_str());
    return true;
}

const AssetManager::UnitAsset* AssetManager::get_unit(const std::string& name) const {
    auto it = units_.find(name);
    if (it != units_.end()) {
        return &it->second;
    }
    return nullptr;
}

std::string AssetManager::get_unit_spritesheet_path(const std::string& name) const {
    auto* unit = get_unit(name);
    if (unit) {
        return dist_path_ + "/" + unit->spritesheet_path;
    }
    return "";
}

const AssetManager::FXAsset* AssetManager::get_fx(const std::string& name) const {
    auto it = fx_.find(name);
    if (it != fx_.end()) {
        return &it->second;
    }
    return nullptr;
}

std::string AssetManager::get_fx_spritesheet_path(const std::string& name) const {
    auto* fx = get_fx(name);
    if (fx) {
        return dist_path_ + "/" + fx->spritesheet_path;
    }
    return "";
}

AssetManager::RSXMapping AssetManager::resolve_rsx(const std::string& rsx_id) const {
    auto it = rsx_mapping_.find(rsx_id);
    if (it != rsx_mapping_.end()) {
        return it->second;
    }
    return RSXMapping{};
}

UnitTiming AssetManager::get_timing(const std::string& unit_name) const {
    auto it = timing_.find(unit_name);
    if (it != timing_.end()) {
        return it->second;
    }
    // Return default timing if not found
    return UnitTiming{};
}

std::string AssetManager::get_shadow_texture_path() const {
    return dist_path_ + "/resources/unit_shadow.png";
}
