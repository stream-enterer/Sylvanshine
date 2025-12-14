#include "texture_atlas.hpp"
#include <GL/glew.h>

#include <SDL3_image/SDL_image.h>

#include <fstream>
#include <iostream>
#include <sstream>

TextureAtlas::TextureAtlas() : texture_id_(0), width_(0), height_(0) {}

TextureAtlas::~TextureAtlas() {
    Unload();
}

bool TextureAtlas::Load(const std::string& image_path) {
    SDL_Surface* surface = IMG_Load(image_path.c_str());
    if (!surface) {
        std::cerr << "Failed to load texture: " << image_path << std::endl;
        return false;
    }

    SDL_Surface* rgba_surface = SDL_ConvertSurface(surface, SDL_PIXELFORMAT_RGBA32);
    SDL_DestroySurface(surface);

    if (!rgba_surface) {
        std::cerr << "Failed to convert surface to RGBA: " << image_path << std::endl;
        return false;
    }

    width_ = rgba_surface->w;
    height_ = rgba_surface->h;

    glGenTextures(1, &texture_id_);
    glBindTexture(GL_TEXTURE_2D, texture_id_);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width_, height_, 0,
                 GL_RGBA, GL_UNSIGNED_BYTE, rgba_surface->pixels);

    SDL_DestroySurface(rgba_surface);

    return true;
}

void TextureAtlas::Unload() {
    if (texture_id_ != 0) {
        glDeleteTextures(1, &texture_id_);
        texture_id_ = 0;
    }
    animations_.clear();
}

bool TextureAtlas::LoadAnimations(const std::string& animations_path) {
    std::ifstream file(animations_path);
    if (!file.is_open()) {
        std::cerr << "Failed to open animations file: " << animations_path << std::endl;
        return false;
    }

    std::string line;
    while (std::getline(file, line)) {
        if (!line.empty()) {
            ParseAnimationLine(line);
        }
    }

    return true;
}

bool TextureAtlas::ParseAnimationLine(const std::string& line) {
    std::istringstream iss(line);
    std::string name;
    int fps;
    std::string frame_data;

    if (!std::getline(iss, name, '\t')) return false;
    if (!(iss >> fps)) return false;
    if (!std::getline(iss >> std::ws, frame_data)) return false;

    Animation anim;
    anim.name = name;
    anim.fps = fps;

    std::istringstream frame_stream(frame_data);
    std::string token;

    std::vector<int> values;
    while (std::getline(frame_stream, token, ',')) {
        try {
            values.push_back(std::stoi(token));
        } catch (...) {
            continue;
        }
    }

    if (values.size() >= 5 && (values.size() % 5) == 0) {
        for (size_t i = 0; i < values.size(); i += 5) {
            AnimationFrame frame;
            frame.index = values[i];
            frame.x = values[i + 1];
            frame.y = values[i + 2];
            frame.width = values[i + 3];
            frame.height = values[i + 4];
            anim.frames.push_back(frame);
        }
    }

    animations_[name] = anim;
    return true;
}

const Animation* TextureAtlas::GetAnimation(const std::string& name) const {
    auto it = animations_.find(name);
    if (it != animations_.end()) {
        return &it->second;
    }
    return nullptr;
}

std::vector<std::string> TextureAtlas::GetAnimationNames() const {
    std::vector<std::string> names;
    for (const auto& pair : animations_) {
        names.push_back(pair.first);
    }
    return names;
}
