#include "asset_paths.hpp"
#include <SDL3/SDL.h>
#include <filesystem>

namespace AssetPaths {

static std::string s_duelyst_repo_path;
static bool s_initialized = false;

bool init() {
    // Get the Duelyst repo path from the CMake define
    #ifdef DUELYST_REPO_PATH
    s_duelyst_repo_path = DUELYST_REPO_PATH;
    #else
    s_duelyst_repo_path = "";
    #endif

    if (s_duelyst_repo_path.empty()) {
        SDL_Log("Error: DUELYST_REPO_PATH not set. Cannot load assets.");
        SDL_Log("Set it with: cmake -DDUELYST_REPO_PATH=/path/to/duelyst ..");
        s_initialized = false;
        return false;
    }

    // Verify the path exists and has expected structure
    std::filesystem::path repo_path(s_duelyst_repo_path);
    std::filesystem::path resources_path = repo_path / "app" / "resources";

    if (!std::filesystem::exists(resources_path)) {
        SDL_Log("Error: Duelyst resources not found at: %s", resources_path.c_str());
        SDL_Log("Expected directory structure: %s/app/resources/", s_duelyst_repo_path.c_str());
        s_initialized = false;
        return false;
    }

    // Check for units and fx directories
    std::filesystem::path units_path = resources_path / "units";
    std::filesystem::path fx_path = resources_path / "fx";

    if (!std::filesystem::exists(units_path)) {
        SDL_Log("Warning: Units directory not found at: %s", units_path.c_str());
    }

    if (!std::filesystem::exists(fx_path)) {
        SDL_Log("Warning: FX directory not found at: %s", fx_path.c_str());
    }

    s_initialized = true;
    SDL_Log("Asset paths initialized with Duelyst repo: %s", s_duelyst_repo_path.c_str());

    return true;
}

bool is_initialized() {
    return s_initialized;
}

const std::string& get_duelyst_repo_path() {
    return s_duelyst_repo_path;
}

std::string get_resources_path() {
    return s_duelyst_repo_path + "/app/resources";
}

std::string get_unit_plist_path(const std::string& unit_name) {
    return get_resources_path() + "/units/" + unit_name + ".plist";
}

std::string get_unit_spritesheet_path(const std::string& unit_name) {
    return get_resources_path() + "/units/" + unit_name + ".png";
}

std::string get_fx_plist_path(const std::string& fx_name) {
    return get_resources_path() + "/fx/" + fx_name + ".plist";
}

std::string get_fx_spritesheet_path(const std::string& fx_name) {
    return get_resources_path() + "/fx/" + fx_name + ".png";
}

std::string get_sfx_path(const std::string& sfx_name) {
    return get_resources_path() + "/sfx/" + sfx_name + ".m4a";
}

std::string get_music_path(const std::string& music_name) {
    return get_resources_path() + "/music/" + music_name + ".m4a";
}

std::string get_local_data_path() {
    return "data";
}

std::string get_timing_path(const std::string& filename) {
    return get_local_data_path() + "/timing/" + filename;
}

std::string get_shader_path(const std::string& filename) {
    return get_local_data_path() + "/shaders/" + filename;
}

std::string get_shadow_texture_path() {
    return get_local_data_path() + "/unit_shadow.png";
}

} // namespace AssetPaths
