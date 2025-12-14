#ifndef TEXTURE_ATLAS_HPP
#define TEXTURE_ATLAS_HPP

#include "types.hpp"

#include <string>
#include <unordered_map>
#include <vector>

class TextureAtlas {
public:
    TextureAtlas();
    ~TextureAtlas();

    bool Load(const std::string& image_path);
    void Unload();

    bool LoadAnimations(const std::string& animations_path);

    unsigned int GetTextureId() const { return texture_id_; }
    int GetWidth() const { return width_; }
    int GetHeight() const { return height_; }

    const Animation* GetAnimation(const std::string& name) const;
    std::vector<std::string> GetAnimationNames() const;

    bool IsValid() const { return texture_id_ != 0; }

private:
    unsigned int texture_id_;
    int width_;
    int height_;
    std::unordered_map<std::string, Animation> animations_;

    bool ParseAnimationLine(const std::string& line);
};

#endif
