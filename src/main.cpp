#include <SDL3/SDL.h>
#include <SDL3_image/SDL_image.h>
#include "game_state.hpp"
#include "game_logic.hpp"
#include "input.hpp"
#include "scene_render.hpp"
#include "lighting_presets.hpp"
#include "settings_menu.hpp"
#include "asset_manager.hpp"
#include "sdl_handles.hpp"
#include "text_renderer.hpp"
#include <cstring>
#include <algorithm>

void print_help() {
    SDL_Log("Usage: tactics [OPTIONS]");
    SDL_Log("");
    SDL_Log("OPTIONS:");
    SDL_Log("  -f, --fullscreen    Launch in 1920x1080 fullscreen with 2x sprite scaling");
    SDL_Log("  -h, --help          Show this help message and exit");
    SDL_Log("");
    SDL_Log("CONTROLS:");
    SDL_Log("  Left click unit     Select unit and show movement range");
    SDL_Log("  Left click tile     Move selected unit to that tile");
    SDL_Log("  Left click enemy    Attack enemy in range");
    SDL_Log("  Right click         Deselect unit");
    SDL_Log("  Space               End turn early");
    SDL_Log("  R                   Restart (after game over)");
}

RenderConfig parse_args(int argc, char* argv[]) {
    bool fullscreen = false;
    
    for (int i = 1; i < argc; i++) {
        if (std::strcmp(argv[i], "-h") == 0 || std::strcmp(argv[i], "--help") == 0) {
            print_help();
            exit(0);
        } else if (std::strcmp(argv[i], "-f") == 0 || std::strcmp(argv[i], "--fullscreen") == 0) {
            fullscreen = true;
        } else {
            SDL_Log("Unknown option: %s", argv[i]);
            print_help();
            exit(1);
        }
    }
    
    if (fullscreen) {
        return {1920, 1080, 2};
    } else {
        return {1280, 720, 1};
    }
}

bool init(const RenderConfig& config, WindowHandle& window) {
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        SDL_Log("SDL_Init failed: %s", SDL_GetError());
        return false;
    }

    Uint32 window_flags = 0;
    if (config.scale == 2) {
        window_flags = SDL_WINDOW_FULLSCREEN;
    }

    SDL_Window* raw_window = SDL_CreateWindow("Duelyst Tactics",
        config.window_w, config.window_h, window_flags);

    if (!raw_window) {
        SDL_Log("Window creation failed: %s", SDL_GetError());
        SDL_Quit();
        return false;
    }

    window = WindowHandle(raw_window);

    return true;
}

int main(int argc, char* argv[]) {
    RenderConfig config = parse_args(argc, argv);

    // Initialize asset manager (loads assets.json from dist/)
    if (!AssetManager::instance().init("dist")) {
        SDL_Log("Failed to initialize AssetManager from dist/");
        SDL_Log("Run 'python3 build_assets.py' to build the asset manifest");
        return 1;
    }

    WindowHandle window;

    if (!init(config, window)) {
        return 1;
    }

    if (!g_gpu.init(window.get())) {
        SDL_Log("Failed to initialize GPU renderer");
        return 1;
    }

    // Load font for UI text
    if (!g_text.load("dist/fonts/audiowide.png", "dist/fonts/audiowide.json")) {
        SDL_Log("Warning: Failed to load font, text rendering disabled");
    }

    // Load titlebar font (Iceland)
    if (!g_title_text.load("dist/fonts/iceland.png", "dist/fonts/iceland.json")) {
        SDL_Log("Warning: Failed to load title font");
    }

    // Apply default lighting preset (press 0-9 to switch)
    // 0 = Sylvanshine default, 1-9 = Duelyst battle maps
    apply_lighting_preset(1, config);  // Start with Lyonar Highlands (Duelyst-style)
    SDL_Log("Press 0-9 to switch lighting presets");

    GameState state;

    if (!state.grid_renderer.init(config)) {
        SDL_Log("Failed to initialize grid renderer");
        return 1;
    }

    // FX mappings and timing data are now loaded via AssetManager from assets.json
    // The old TSV-based loading is no longer needed

    if (!Entity::load_shadow()) {
        SDL_Log("Warning: Failed to load shadow texture");
    }

    Entity unit1 = create_unit(state, config, "f1_general", UnitType::Player, 25, 5, {2, 2});
    if (unit1.spritesheet.ptr) {
        state.units.push_back(std::move(unit1));
    }

    Entity unit2 = create_unit(state, config, "f1_general", UnitType::Enemy, 10, 2, {6, 2});
    if (unit2.spritesheet.ptr) {
        state.units.push_back(std::move(unit2));
    }

    Entity unit3 = create_unit(state, config, "f1_general", UnitType::Enemy, 5, 3, {4, 1});
    if (unit3.spritesheet.ptr) {
        state.units.push_back(std::move(unit3));
    }

    reset_actions(state);
    SDL_Log("=== PLAYER TURN ===");

    bool running = true;
    Uint64 last_time = SDL_GetTicks();

    while (running) {
        Uint64 current_time = SDL_GetTicks();
        float dt = std::min((current_time - last_time) / 1000.0f, 0.1f);
        last_time = current_time;

        handle_events(running, state, config);
        update_game(state, dt, config);
        render(state, config);

        SDL_Delay(16);
    }

    g_gpu.shutdown();
    SDL_Quit();

    return 0;
}
