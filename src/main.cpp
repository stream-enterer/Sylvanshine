#include <SDL3/SDL.h>
#include <SDL3_image/SDL_image.h>
#include "types.hpp"
#include "entity.hpp"
#include "grid.hpp"
#include <vector>
#include <cstring>

constexpr int MOVE_RANGE = 3;

struct GameState {
    Entity player_unit;
    BoardPos selected_tile{-1, -1};
    bool unit_selected = false;
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

bool init(const RenderConfig& config, SDL_Window** window, SDL_Renderer** renderer) {
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        SDL_Log("SDL_Init failed: %s", SDL_GetError());
        return false;
    }

    Uint32 window_flags = 0;
    if (config.scale == 2) {
        window_flags = SDL_WINDOW_FULLSCREEN;
    }

    if (!SDL_CreateWindowAndRenderer("Duelyst Tactics", config.window_w, config.window_h, window_flags, window, renderer)) {
        SDL_Log("Window creation failed: %s", SDL_GetError());
        SDL_Quit();
        return false;
    }

    return true;
}

void handle_click(GameState& state, Vec2 mouse, const RenderConfig& config) {
    BoardPos clicked = screen_to_board(config, mouse);
    
    if (!clicked.is_valid()) return;
    
    if (state.player_unit.is_moving()) return;
    
    if (!state.unit_selected) {
        if (clicked == state.player_unit.board_pos) {
            state.unit_selected = true;
            state.selected_tile = clicked;
            state.reachable_tiles = get_reachable_tiles(clicked, MOVE_RANGE);
            SDL_Log("Unit selected at (%d, %d)", clicked.x, clicked.y);
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
            SDL_Log("Moving to (%d, %d)", clicked.x, clicked.y);
            state.player_unit.start_move(config, clicked);
            state.unit_selected = false;
            state.reachable_tiles.clear();
        } else {
            state.unit_selected = false;
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
    state.player_unit.update(dt, config);
}

void render(SDL_Renderer* renderer, const GameState& state, const RenderConfig& config) {
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);
    
    render_grid(renderer, config);
    
    if (state.unit_selected) {
        render_move_range(renderer, config, state.player_unit.board_pos, MOVE_RANGE);
    }
    
    state.player_unit.render(renderer, config);
    
    SDL_RenderPresent(renderer);
}

int main(int argc, char* argv[]) {
    RenderConfig config = parse_args(argc, argv);
    
    SDL_Window* window = nullptr;
    SDL_Renderer* renderer = nullptr;

    if (!init(config, &window, &renderer)) {
        return 1;
    }

    GameState state;
    if (!state.player_unit.load(renderer, "f1_general")) {
        SDL_Log("Failed to load unit");
        return 1;
    }
    
    state.player_unit.set_board_position(config, {4, 2});

    bool running = true;
    Uint64 last_time = SDL_GetTicks();
    
    while (running) {
        Uint64 current_time = SDL_GetTicks();
        float dt = (current_time - last_time) / 1000.0f;
        last_time = current_time;
        
        handle_events(running, state, config);
        update(state, dt, config);
        render(renderer, state, config);
        
        SDL_Delay(16);
    }

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}
