#include <SDL3/SDL.h>
#include <SDL3_image/SDL_image.h>
#include "types.hpp"
#include "entity.hpp"
#include "grid.hpp"
#include "sdl_handles.hpp"
#include <vector>
#include <cstring>

constexpr int MOVE_RANGE = 3;

struct GameState {
    std::vector<Entity> units;
    int selected_unit_idx = -1;
    std::vector<BoardPos> reachable_tiles;
};

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
}

int find_unit_at_pos(const GameState& state, BoardPos pos) {
    for (size_t i = 0; i < state.units.size(); i++) {
        if (state.units[i].board_pos == pos) {
            return static_cast<int>(i);
        }
    }
    return -1;
}

std::vector<BoardPos> get_occupied_positions(const GameState& state, int exclude_idx) {
    std::vector<BoardPos> occupied;
    for (size_t i = 0; i < state.units.size(); i++) {
        if (static_cast<int>(i) != exclude_idx) {
            if (state.units[i].is_moving()) {
                occupied.push_back(state.units[i].move_target);
            } else {
                occupied.push_back(state.units[i].board_pos);
            }
        }
    }
    return occupied;
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

bool init(const RenderConfig& config, WindowHandle& window, RendererHandle& renderer) {
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        SDL_Log("SDL_Init failed: %s", SDL_GetError());
        return false;
    }

    Uint32 window_flags = 0;
    if (config.scale == 2) {
        window_flags = SDL_WINDOW_FULLSCREEN;
    }

    SDL_Window* raw_window = nullptr;
    SDL_Renderer* raw_renderer = nullptr;

    if (!SDL_CreateWindowAndRenderer("Duelyst Tactics", config.window_w, config.window_h, window_flags, &raw_window, &raw_renderer)) {
        SDL_Log("Window creation failed: %s", SDL_GetError());
        SDL_Quit();
        return false;
    }

    window = WindowHandle(raw_window);
    renderer = RendererHandle(raw_renderer);

    return true;
}

void handle_click(GameState& state, Vec2 mouse, const RenderConfig& config) {
    BoardPos clicked = screen_to_board(config, mouse);
    
    if (!clicked.is_valid()) return;
    
    if (state.selected_unit_idx >= 0 && state.units[state.selected_unit_idx].is_moving()) {
        return;
    }
    
    if (state.selected_unit_idx == -1) {
        int unit_idx = find_unit_at_pos(state, clicked);
        if (unit_idx >= 0) {
            state.selected_unit_idx = unit_idx;
            auto occupied = get_occupied_positions(state, unit_idx);
            state.reachable_tiles = get_reachable_tiles(clicked, MOVE_RANGE, occupied);
            SDL_Log("Unit %d selected at (%d, %d)", unit_idx, clicked.x, clicked.y);
        }
    } else {
        bool is_reachable = false;
        for (const auto& tile : state.reachable_tiles) {
            if (tile == clicked) {
                is_reachable = true;
                break;
            }
        }
        
        if (is_reachable) {
            auto occupied = get_occupied_positions(state, state.selected_unit_idx);
            bool tile_occupied = false;
            for (const auto& occ : occupied) {
                if (clicked == occ) {
                    tile_occupied = true;
                    break;
                }
            }
            
            if (!tile_occupied) {
                SDL_Log("Moving unit %d to (%d, %d)", state.selected_unit_idx, clicked.x, clicked.y);
                state.units[state.selected_unit_idx].start_move(config, clicked);
            }
            state.selected_unit_idx = -1;
            state.reachable_tiles.clear();
        } else {
            state.selected_unit_idx = -1;
            state.reachable_tiles.clear();
        }
    }
}

void handle_events(bool& running, GameState& state, const RenderConfig& config) {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        if (event.type == SDL_EVENT_QUIT) {
            running = false;
        }
        else if (event.type == SDL_EVENT_MOUSE_BUTTON_DOWN && event.button.button == SDL_BUTTON_LEFT) {
            Vec2 mouse{static_cast<float>(event.button.x), static_cast<float>(event.button.y)};
            handle_click(state, mouse, config);
        }
    }
}

void update(GameState& state, float dt, const RenderConfig& config) {
    for (auto& unit : state.units) {
        unit.update(dt, config);
    }
}

void render(SDL_Renderer* renderer, const GameState& state, const RenderConfig& config) {
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);
    
    render_grid(renderer, config);
    
    if (state.selected_unit_idx >= 0) {
        render_move_range(renderer, config, state.units[state.selected_unit_idx].board_pos, MOVE_RANGE);
    }
    
    for (const auto& unit : state.units) {
        unit.render(renderer, config);
    }
    
    SDL_RenderPresent(renderer);
}

int main(int argc, char* argv[]) {
    RenderConfig config = parse_args(argc, argv);

    WindowHandle window;
    RendererHandle renderer;

    if (!init(config, window, renderer)) {
        return 1;
    }

    GameState state;

    Entity unit1;
    if (!unit1.load(renderer.get(), "f1_general")) {
        SDL_Log("Failed to load unit 1");
        return 1;
    }
    unit1.set_board_position(config, {2, 2});
    state.units.push_back(std::move(unit1));

    Entity unit2;
    if (!unit2.load(renderer.get(), "f1_general")) {
        SDL_Log("Failed to load unit 2");
        return 1;
    }
    unit2.set_board_position(config, {6, 2});
    state.units.push_back(std::move(unit2));

    Entity unit3;
    if (!unit3.load(renderer.get(), "f1_general")) {
        SDL_Log("Failed to load unit 3");
        return 1;
    }
    unit3.set_board_position(config, {4, 1});
    state.units.push_back(std::move(unit3));

    bool running = true;
    Uint64 last_time = SDL_GetTicks();

    while (running) {
        Uint64 current_time = SDL_GetTicks();
        float dt = (current_time - last_time) / 1000.0f;
        last_time = current_time;

        handle_events(running, state, config);
        update(state, dt, config);
        render(renderer.get(), state, config);

        SDL_Delay(16);
    }

    // RAII handles cleanup automatically
    SDL_Quit();

    return 0;
}
