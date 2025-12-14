#ifndef ASSET_MANAGER_HPP
#define ASSET_MANAGER_HPP

#include "texture_atlas.hpp"
#include "types.hpp"

#include <string>
#include <unordered_map>
#include <vector>

struct UnitAssets {
    TextureAtlas spritesheet;
    std::unordered_map<std::string, std::string> sounds;
    std::string unit_id;
    int shadow_offset;
};

struct FxAssets {
    TextureAtlas spritesheet;
    std::string folder;
    std::string prefix;
};

class AssetManager {
public:
    AssetManager();
    ~AssetManager();

    bool Initialize(const std::string& data_root);
    void Shutdown();

    UnitAssets* LoadUnit(const std::string& unit_id);
    FxAssets* LoadFx(const std::string& rsx_name);

    unsigned int LoadTexture(const std::string& path);
    unsigned int GetTexture(const std::string& path) const;

    unsigned int GetShadowTexture() const { return shadow_texture_; }
    unsigned int GetTilesBoardTexture() const { return tiles_board_texture_; }
    unsigned int GetTilesActionTexture() const { return tiles_action_texture_; }

    const std::string& GetDataRoot() const { return data_root_; }

private:
    std::string data_root_;
    std::unordered_map<std::string, UnitAssets> units_;
    std::unordered_map<std::string, FxAssets> fx_cache_;
    std::unordered_map<std::string, unsigned int> textures_;
    std::unordered_map<std::string, std::pair<std::string, std::string>> rsx_mapping_;

    unsigned int shadow_texture_;
    unsigned int tiles_board_texture_;
    unsigned int tiles_action_texture_;

    bool LoadRsxMapping();
    bool LoadSounds(const std::string& path, std::unordered_map<std::string, std::string>& sounds);
};

#endif
