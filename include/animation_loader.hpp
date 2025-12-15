#pragma once
#include "types.hpp"
#include <vector>
#include <string>

struct AnimationSet {
    std::vector<Animation> animations;
    
    const Animation* find(const char* name) const;
};

AnimationSet load_animations(const char* filepath);
