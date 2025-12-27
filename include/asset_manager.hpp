#pragma once
#include "types.hpp"
#include "animation_loader.hpp"
#include <string>
#include <unordered_map>

// Unit timing data (attack animation delays, etc.)
struct UnitTiming {
    float attack_damage_delay = 0.5f;  // Time from attack start to damage application
};

/**
 * AssetManager - Singleton that loads and caches game assets from dist/assets.json
 *
 * This replaces the direct plist parsing approach, loading all asset metadata
 * at startup from the pre-built assets.json manifest.
 */
class AssetManager {
public:
    static AssetManager& instance();

    /**
     * Initialize the asset manager by loading dist/assets.json
     * @param dist_path Path to the dist directory (default: "dist")
     * @return true if initialization succeeded
     */
    bool init(const std::string& dist_path = "dist");

    /**
     * Check if the asset manager has been initialized
     */
    bool is_initialized() const { return initialized_; }

    /**
     * Get the dist directory path
     */
    const std::string& get_dist_path() const { return dist_path_; }

    // ===== Unit Assets =====

    struct UnitAsset {
        std::string spritesheet_path;  // Relative to dist_path
        std::string sdf_atlas_path;    // Relative to dist_path (may be empty if no SDF)
        AnimationSet animations;
    };

    /**
     * Get unit asset data by name (e.g., "f1_general")
     * @return Pointer to unit asset, or nullptr if not found
     */
    const UnitAsset* get_unit(const std::string& name) const;

    /**
     * Get the full path to a unit's spritesheet
     */
    std::string get_unit_spritesheet_path(const std::string& name) const;

    /**
     * Get the full path to a unit's SDF atlas (empty if none exists)
     */
    std::string get_unit_sdf_atlas_path(const std::string& name) const;

    // ===== FX Assets =====

    struct FXAsset {
        std::string spritesheet_path;
        AnimationSet animations;
    };

    /**
     * Get FX asset data by name (e.g., "fx_blood_explosion")
     * @return Pointer to FX asset, or nullptr if not found
     */
    const FXAsset* get_fx(const std::string& name) const;

    /**
     * Get the full path to an FX spritesheet
     */
    std::string get_fx_spritesheet_path(const std::string& name) const;

    struct RSXMapping {
        std::string folder;  // FX folder name
        std::string anim;    // Animation name within that folder
    };

    /**
     * Resolve an RSX identifier to FX folder and animation name
     * @param rsx_id e.g., "fxSmokeGround"
     * @return RSXMapping with folder and anim, or empty strings if not found
     */
    RSXMapping resolve_rsx(const std::string& rsx_id) const;

    // ===== Timing Data =====

    /**
     * Get timing data for a unit
     */
    UnitTiming get_timing(const std::string& unit_name) const;

    // ===== Resource Paths =====

    /**
     * Get the path to the shadow texture
     */
    std::string get_shadow_texture_path() const;

private:
    AssetManager() = default;
    ~AssetManager() = default;
    AssetManager(const AssetManager&) = delete;
    AssetManager& operator=(const AssetManager&) = delete;

    bool initialized_ = false;
    std::string dist_path_;

    std::unordered_map<std::string, UnitAsset> units_;
    std::unordered_map<std::string, FXAsset> fx_;
    std::unordered_map<std::string, UnitTiming> timing_;
    std::unordered_map<std::string, RSXMapping> rsx_mapping_;  // RSX ID -> folder + anim
};
