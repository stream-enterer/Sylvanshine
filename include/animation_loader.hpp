#pragma once
#include "types.hpp"
#include <vector>
#include <string>

struct AnimationSet {
    std::vector<Animation> animations;

    const Animation* find(const char* name) const;
};

// Load animations from legacy .txt format (used for local data files)
AnimationSet load_animations(const char* filepath);

// Load animations from Duelyst plist format
// unit_name: e.g., "f1_general"
// plist_path: full path to the .plist file
AnimationSet load_animations_from_plist(const std::string& unit_name, const char* plist_path);
