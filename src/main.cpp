#include <SDL3/SDL.h>
#include <SDL3_image/SDL_image.h>
#include "types.hpp"
#include "entity.hpp"
#include "grid.hpp"
#include <vector>

constexpr int WINDOW_WIDTH = 1280;
constexpr int WINDOW_HEIGHT = 720;
constexpr int MOVE_RANGE = 3;

struct GameState {
    Entity player_unit;
    BoardPos selected_tile{-1, -1};
    bool unit_selected = false;
    std::vector<BoardPos> reachable_tiles;
};

bool init(SDL_Window** window, SDL_Renderer** renderer) {
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        SDL_Log("SDL_Init failed: %s", SDL_GetError());
        return false;
    }

    if (!SDL_CreateWindowAndRenderer("Duelyst Tactics", WINDOW_WIDTH, WINDOW_HEIGHT, 0, window, renderer)) {
        SDL_Log("Window creation failed: %s", SDL_GetError());
        SDL_Quit();
        return false;
    }

    return true;
}

void handle_click(GameState& state, Vec2 mouse, int window_w, int window_h) {
    BoardPos clicked = screen_to_board(window_w, window_h, mouse);
    
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
            state.player_unit.start_move(window_w, window_h, clicked);
            state.unit_selected = false;
            state.reachable_tiles.clear();
        } else {
            state.unit_selected = false;
            state.reachable_tiles.clear();
        }
    }
}

void handle_events(bool& running, GameState& state, int window_w, int window_h) {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        if (event.type == SDL_EVENT_QUIT) {
            running = false;
        }
        else if (event.type == SDL_EVENT_MOUSE_BUTTON_DOWN && event.button.button == SDL_BUTTON_LEFT) {
            Vec2 mouse{static_cast<float>(event.button.x), static_cast<float>(event.button.y)};
            handle_click(state, mouse, window_w, window_h);
        }
    }
}

void update(GameState& state, float dt) {
    state.player_unit.update(dt, WINDOW_WIDTH, WINDOW_HEIGHT);
}

void render(SDL_Renderer* renderer, const GameState& state) {
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);
    
    render_grid(renderer, WINDOW_WIDTH, WINDOW_HEIGHT);
    
    if (state.unit_selected) {
        render_move_range(renderer, WINDOW_WIDTH, WINDOW_HEIGHT, state.player_unit.board_pos, MOVE_RANGE);
    }
    
    state.player_unit.render(renderer, WINDOW_WIDTH, WINDOW_HEIGHT);
    
    SDL_RenderPresent(renderer);
}

int main(int argc, char* argv[]) {
    SDL_Window* window = nullptr;
    SDL_Renderer* renderer = nullptr;

    if (!init(&window, &renderer)) {
        return 1;
    }

    GameState state;
    if (!state.player_unit.load(renderer, "f1_general")) {
        SDL_Log("Failed to load unit");
        return 1;
    }
    
    state.player_unit.set_board_position(WINDOW_WIDTH, WINDOW_HEIGHT, {4, 2});

    bool running = true;
    Uint64 last_time = SDL_GetTicks();
    
    while (running) {
        Uint64 current_time = SDL_GetTicks();
        float dt = (current_time - last_time) / 1000.0f;
        last_time = current_time;
        
        handle_events(running, state, WINDOW_WIDTH, WINDOW_HEIGHT);
        update(state, dt);
        render(renderer, state);
        
        SDL_Delay(16);
    }

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}
