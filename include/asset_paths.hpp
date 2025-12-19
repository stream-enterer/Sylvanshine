#pragma once
#include <string>

// Asset path management for Duelyst repository assets
// All paths are relative to the DUELYST_REPO_PATH CMake variable

namespace AssetPaths {

// Initialize the asset path system
// Must be called before any asset loading
// Returns true if the Duelyst repo path is valid
bool init();

// Check if asset paths are properly initialized
bool is_initialized();

// Get the base Duelyst repository path
const std::string& get_duelyst_repo_path();

// Get the base path for game resources (app/resources)
std::string get_resources_path();

// Unit assets: app/resources/units/
// Unit files are named: {unit_name}.plist, {unit_name}.png
std::string get_unit_plist_path(const std::string& unit_name);
std::string get_unit_spritesheet_path(const std::string& unit_name);

// FX assets: app/resources/fx/
// FX files are named: {fx_name}.plist, {fx_name}.png
std::string get_fx_plist_path(const std::string& fx_name);
std::string get_fx_spritesheet_path(const std::string& fx_name);

// Sound effects: app/resources/sfx/
// Sound files are named: sfx_{name}.m4a
std::string get_sfx_path(const std::string& sfx_name);

// Music: app/resources/music/
// Music files are named: music_{name}.m4a
std::string get_music_path(const std::string& music_name);

// Local data directory (for timing data, manifests, shaders, etc.)
// These remain in the local data/ directory
std::string get_local_data_path();
std::string get_timing_path(const std::string& filename);
std::string get_shader_path(const std::string& filename);

// Shadow texture path - still from local data for now
std::string get_shadow_texture_path();

} // namespace AssetPaths
