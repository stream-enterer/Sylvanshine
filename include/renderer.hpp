#ifndef RENDERER_HPP
#define RENDERER_HPP

#include "asset_manager.hpp"
#include "fx_composition_resolver.hpp"
#include "game_state.hpp"
#include "sprite_batch.hpp"

#include <SDL2/SDL.h>

#include <glm/glm.hpp>

class Renderer {
public:
    Renderer();
    ~Renderer();

    bool Initialize();
    void Shutdown();

    void HandleEvent(const SDL_Event& event);
    void Update(float dt);
    void Render();

    bool ShouldQuit() const { return should_quit_; }

private:
    SDL_Window* window_;
    SDL_GLContext gl_context_;

    AssetManager assets_;
    FxCompositionResolver fx_resolver_;
    GameState game_state_;
    SpriteBatch sprite_batch_;

    glm::mat4 projection_;
    bool should_quit_;
    bool fullscreen_;
    int window_width_;
    int window_height_;
    int render_scale_;

    void ToggleFullscreen();
    void UpdateProjection();
    glm::vec2 ScreenToGameCoords(int screen_x, int screen_y);
};

#endif
