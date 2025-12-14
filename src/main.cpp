#include "renderer.hpp"

#include <SDL3/SDL.h>

#include <iostream>

int main(int argc, char* argv[]) {
    (void)argc;
    (void)argv;

    Renderer renderer;

    if (!renderer.Initialize()) {
        std::cerr << "Failed to initialize renderer" << std::endl;
        return 1;
    }

    std::cout << "Duelyst Prototype Started" << std::endl;
    std::cout << "Controls:" << std::endl;
    std::cout << "  Left-click: Select unit / Move / Attack" << std::endl;
    std::cout << "  Right-click: Cancel selection" << std::endl;
    std::cout << "  F: Toggle fullscreen" << std::endl;
    std::cout << "  R: Reset game" << std::endl;
    std::cout << "  ESC: Quit" << std::endl;

    Uint32 last_time = SDL_GetTicks();

    while (!renderer.ShouldQuit()) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            renderer.HandleEvent(event);
        }

        Uint32 current_time = SDL_GetTicks();
        float dt = (current_time - last_time) / 1000.0f;
        last_time = current_time;

        dt = std::min(dt, 0.1f);

        renderer.Update(dt);
        renderer.Render();
    }

    std::cout << "Shutting down..." << std::endl;

    return 0;
}
