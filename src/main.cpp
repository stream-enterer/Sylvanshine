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
    std::vector<BoardPos> attackable_tiles;
    Vec2 mouse_pos = {0.0f, 0.0f};
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
    SDL_Log("  Left click enemy    Attack enemy in range");
    SDL_Log("  Right click         Deselect unit");
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

std::vector<BoardPos> get_enemy_positions(const GameState& state, int unit_idx) {
    std::vector<BoardPos> enemies;
    UnitType unit_type = state.units[unit_idx].type;
    
    for (size_t i = 0; i < state.units.size(); i++) {
        if (static_cast<int>(i) != unit_idx && state.units[i].type != unit_type) {
            if (state.units[i].can_act()) {
                enemies.push_back(state.units[i].board_pos);
            }
        }
    }
    return enemies;
}

void clear_selection(GameState& state) {
    if (state.selected_unit_idx >= 0) {
        state.units[state.selected_unit_idx].restore_facing();
    }
    state.selected_unit_idx = -1;
    state.reachable_tiles.clear();
    state.attackable_tiles.clear();
}

void update_selected_ranges(GameState& state) {
    if (state.selected_unit_idx < 0) return;
    
    BoardPos selected_pos = state.units[state.selected_unit_idx].board_pos;
    
    auto occupied = get_occupied_positions(state, state.selected_unit_idx);
    state.reachable_tiles = get_reachable_tiles(selected_pos, MOVE_RANGE, occupied);
    
    auto enemy_positions = get_enemy_positions(state, state.selected_unit_idx);
    state.attackable_tiles = get_attackable_tiles(
        selected_pos,
        state.units[state.selected_unit_idx].attack_range,
        enemy_positions
    );
    
    static size_t last_attackable_count = 0;
    if (state.attackable_tiles.size() != last_attackable_count) {
        SDL_Log("Attackable tiles updated: %zu enemies in range", state.attackable_tiles.size());
        last_attackable_count = state.attackable_tiles.size();
    }
}

void update_selected_facing(GameState& state, const RenderConfig& config) {
    if (state.selected_unit_idx < 0) return;
    
    BoardPos mouse_board = screen_to_board(config, state.mouse_pos);
    if (!mouse_board.is_valid()) return;
    
    state.units[state.selected_unit_idx].face_position(mouse_board);
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

void handle_select_click(GameState& state, BoardPos clicked) {
    int unit_idx = find_unit_at_pos(state, clicked);
    if (unit_idx < 0) return;
    
    if (!state.units[unit_idx].can_act()) return;
    
    if (state.selected_unit_idx >= 0 && state.selected_unit_idx != unit_idx) {
        state.units[state.selected_unit_idx].restore_facing();
    }
    
    state.selected_unit_idx = unit_idx;
    state.units[unit_idx].store_facing();
    update_selected_ranges(state);
    
    SDL_Log("Unit %d selected at (%d, %d)", unit_idx, clicked.x, clicked.y);
}

void handle_move_click(GameState& state, BoardPos clicked, const RenderConfig& config) {
    bool is_reachable = false;
    for (const auto& tile : state.reachable_tiles) {
        if (tile == clicked) {
            is_reachable = true;
            break;
        }
    }
    
    if (!is_reachable) return;
    
    auto occupied = get_occupied_positions(state, state.selected_unit_idx);
    for (const auto& occ : occupied) {
        if (clicked == occ) return;
    }
    
    int unit_idx = state.selected_unit_idx;
    SDL_Log("Moving unit %d to (%d, %d)", unit_idx, clicked.x, clicked.y);
    
    state.selected_unit_idx = -1;
    state.reachable_tiles.clear();
    state.attackable_tiles.clear();
    
    state.units[unit_idx].start_move(config, clicked);
}

void handle_attack_click(GameState& state, BoardPos clicked) {
    bool is_attackable = false;
    for (const auto& tile : state.attackable_tiles) {
        if (tile == clicked) {
            is_attackable = true;
            break;
        }
    }
    
    if (!is_attackable) return;
    
    int target_idx = find_unit_at_pos(state, clicked);
    if (target_idx < 0) return;
    
    int attacker_idx = state.selected_unit_idx;
    BoardPos target_pos = state.units[target_idx].board_pos;
    
    state.selected_unit_idx = -1;
    state.reachable_tiles.clear();
    state.attackable_tiles.clear();
    
    state.units[attacker_idx].face_position(target_pos);
    state.units[attacker_idx].start_attack(target_idx);
}

void handle_selected_click(GameState& state, BoardPos clicked, const RenderConfig& config) {
    int clicked_unit = find_unit_at_pos(state, clicked);
    
    if (clicked_unit >= 0) {
        handle_attack_click(state, clicked);
    } else {
        handle_move_click(state, clicked, config);
    }
}

void handle_click(GameState& state, Vec2 mouse, const RenderConfig& config) {
    BoardPos clicked = screen_to_board(config, mouse);
    if (!clicked.is_valid()) return;
    
    if (state.selected_unit_idx >= 0 && state.units[state.selected_unit_idx].is_moving()) {
        return;
    }
    
    if (state.selected_unit_idx >= 0 && state.units[state.selected_unit_idx].is_attacking()) {
        return;
    }
    
    if (state.selected_unit_idx == -1) {
        handle_select_click(state, clicked);
    } else {
        handle_selected_click(state, clicked, config);
    }
}

void handle_events(bool& running, GameState& state, const RenderConfig& config) {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        if (event.type == SDL_EVENT_QUIT) {
            running = false;
        }
        else if (event.type == SDL_EVENT_MOUSE_BUTTON_DOWN) {
            if (event.button.button == SDL_BUTTON_LEFT) {
                Vec2 mouse{static_cast<float>(event.button.x), static_cast<float>(event.button.y)};
                handle_click(state, mouse, config);
            }
            else if (event.button.button == SDL_BUTTON_RIGHT) {
                clear_selection(state);
            }
        }
        else if (event.type == SDL_EVENT_MOUSE_MOTION) {
            state.mouse_pos = {static_cast<float>(event.motion.x), static_cast<float>(event.motion.y)};
            update_selected_facing(state, config);
        }
    }
}

void update(GameState& state, float dt, const RenderConfig& config) {
    for (auto& unit : state.units) {
        unit.update(dt, config);
    }
    
    update_selected_ranges(state);
}

void render(SDL_Renderer* renderer, const GameState& state, const RenderConfig& config) {
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);
    
    render_grid(renderer, config);
    
    if (state.selected_unit_idx >= 0) {
        render_move_range(renderer, config, state.units[state.selected_unit_idx].board_pos, MOVE_RANGE);
        render_attack_range(renderer, config, state.attackable_tiles);
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
    unit1.type = UnitType::Player;
    unit1.set_board_position(config, {2, 2});
    state.units.push_back(std::move(unit1));

    Entity unit2;
    if (!unit2.load(renderer.get(), "f1_general")) {
        SDL_Log("Failed to load unit 2");
        return 1;
    }
    unit2.type = UnitType::Enemy;
    unit2.set_board_position(config, {6, 2});
    state.units.push_back(std::move(unit2));

    Entity unit3;
    if (!unit3.load(renderer.get(), "f1_general")) {
        SDL_Log("Failed to load unit 3");
        return 1;
    }
    unit3.type = UnitType::Enemy;
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

    SDL_Quit();

    return 0;
}
