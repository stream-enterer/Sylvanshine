#include "renderer.hpp"
#include <GL/glew.h>

#include <glm/gtc/matrix_transform.hpp>

#include <iostream>

Renderer::Renderer()
    : window_(nullptr),
      gl_context_(nullptr),
      should_quit_(false),
      fullscreen_(false),
      window_width_(WINDOW_WIDTH),
      window_height_(WINDOW_HEIGHT),
      render_scale_(1) {}

Renderer::~Renderer() {
    Shutdown();
}

bool Renderer::Initialize() {
    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO)) {
        std::cerr << "Failed to initialize SDL: " << SDL_GetError() << std::endl;
        return false;
    }

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);

    window_ = SDL_CreateWindow("Duelyst Prototype", WINDOW_WIDTH, WINDOW_HEIGHT,
                                SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);

    if (!window_) {
        std::cerr << "Failed to create window: " << SDL_GetError() << std::endl;
        return false;
    }

    gl_context_ = SDL_GL_CreateContext(window_);
    if (!gl_context_) {
        std::cerr << "Failed to create OpenGL context: " << SDL_GetError() << std::endl;
        return false;
    }

    if (!SDL_GL_MakeCurrent(window_, gl_context_)) {
        std::cerr << "Failed to make GL context current: " << SDL_GetError() << std::endl;
        return false;
    }

    SDL_GL_SetSwapInterval(1);

    glewExperimental = GL_TRUE;
    while (glGetError() != GL_NO_ERROR) {}
    GLenum glew_err = glewInit();
    if (glew_err != GLEW_OK) {
        std::cerr << "Failed to initialize GLEW: " << glewGetErrorString(glew_err) << std::endl;
        return false;
    }
    while (glGetError() != GL_NO_ERROR) {}

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    if (!sprite_batch_.Initialize()) {
        std::cerr << "Failed to initialize sprite batch" << std::endl;
        return false;
    }

    std::string data_root = "/home/user/Sylvanshine/data";

    if (!assets_.Initialize(data_root)) {
        std::cerr << "Failed to initialize asset manager" << std::endl;
        return false;
    }

    std::string compositions_path = data_root + "/fx_compositions/compositions.json";
    std::string timing_path = data_root + "/fx_compositions/rsx_timing.json";

    if (!fx_resolver_.Load(compositions_path, timing_path)) {
        std::cerr << "Failed to load FX resolver" << std::endl;
        return false;
    }

    if (!game_state_.Initialize(&assets_, &fx_resolver_)) {
        std::cerr << "Failed to initialize game state" << std::endl;
        return false;
    }

    UpdateProjection();

    std::cout << "Renderer initialized successfully" << std::endl;
    return true;
}

void Renderer::Shutdown() {
    sprite_batch_.Shutdown();
    assets_.Shutdown();

    if (gl_context_) {
        SDL_GL_DestroyContext(gl_context_);
        gl_context_ = nullptr;
    }

    if (window_) {
        SDL_DestroyWindow(window_);
        window_ = nullptr;
    }

    SDL_Quit();
}

void Renderer::HandleEvent(const SDL_Event& event) {
    switch (event.type) {
        case SDL_EVENT_QUIT:
            should_quit_ = true;
            break;

        case SDL_EVENT_KEY_DOWN:
            if (event.key.key == SDLK_ESCAPE) {
                should_quit_ = true;
            } else if (event.key.key == SDLK_F) {
                ToggleFullscreen();
            } else if (event.key.key == SDLK_R) {
                game_state_.Reset();
            }
            break;

        case SDL_EVENT_MOUSE_BUTTON_DOWN:
            if (event.button.button == SDL_BUTTON_LEFT) {
                glm::vec2 game_coords = ScreenToGameCoords(
                    event.button.x,
                    event.button.y);
                game_state_.HandleClick(game_coords.x, game_coords.y);
            } else if (event.button.button == SDL_BUTTON_RIGHT) {
                game_state_.HandleRightClick();
            }
            break;

        case SDL_EVENT_WINDOW_RESIZED:
            if (event.type == SDL_EVENT_WINDOW_RESIZED) {
                window_width_ = event.window.data1;
                window_height_ = event.window.data2;
                UpdateProjection();
            }
            break;

        default:
            break;
    }
}

void Renderer::Update(float dt) {
    game_state_.Update(dt);
}

void Renderer::Render() {
    glViewport(0, 0, window_width_, window_height_);
    glClearColor(0.1f, 0.1f, 0.15f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    sprite_batch_.Begin(projection_);
    game_state_.Render(sprite_batch_);
    sprite_batch_.End();

    SDL_GL_SwapWindow(window_);
}

void Renderer::ToggleFullscreen() {
    fullscreen_ = !fullscreen_;

    if (fullscreen_) {
        SDL_SetWindowFullscreen(window_, SDL_WINDOW_FULLSCREEN);
        SDL_GetWindowSize(window_, &window_width_, &window_height_);
        render_scale_ = 2;
    } else {
        SDL_SetWindowFullscreen(window_, 0);
        SDL_SetWindowSize(window_, WINDOW_WIDTH, WINDOW_HEIGHT);
        window_width_ = WINDOW_WIDTH;
        window_height_ = WINDOW_HEIGHT;
        render_scale_ = 1;
    }

    UpdateProjection();
}

void Renderer::UpdateProjection() {
    float scale_x = static_cast<float>(window_width_) / WINDOW_WIDTH;
    float scale_y = static_cast<float>(window_height_) / WINDOW_HEIGHT;
    float scale = std::min(scale_x, scale_y);

    float scaled_width = WINDOW_WIDTH * scale;
    float scaled_height = WINDOW_HEIGHT * scale;
    float offset_x = (window_width_ - scaled_width) / 2.0f;
    float offset_y = (window_height_ - scaled_height) / 2.0f;

    projection_ = glm::ortho(
        -offset_x / scale,
        static_cast<float>(WINDOW_WIDTH) + offset_x / scale,
        static_cast<float>(WINDOW_HEIGHT) + offset_y / scale,
        -offset_y / scale,
        -1.0f, 1.0f
    );
}

glm::vec2 Renderer::ScreenToGameCoords(int screen_x, int screen_y) {
    float scale_x = static_cast<float>(window_width_) / WINDOW_WIDTH;
    float scale_y = static_cast<float>(window_height_) / WINDOW_HEIGHT;
    float scale = std::min(scale_x, scale_y);

    float scaled_width = WINDOW_WIDTH * scale;
    float scaled_height = WINDOW_HEIGHT * scale;
    float offset_x = (window_width_ - scaled_width) / 2.0f;
    float offset_y = (window_height_ - scaled_height) / 2.0f;

    float game_x = (screen_x - offset_x) / scale;
    float game_y = (screen_y - offset_y) / scale;

    return glm::vec2(game_x, game_y);
}
