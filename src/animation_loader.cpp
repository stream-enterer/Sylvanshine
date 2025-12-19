#include "animation_loader.hpp"
#include "plist_parser.hpp"
#include <fstream>
#include <sstream>
#include <cstring>
#include <SDL3/SDL.h>

const Animation* AnimationSet::find(const char* name) const {
    for (const auto& anim : animations) {
        if (std::strcmp(anim.name, name) == 0) {
            return &anim;
        }
    }
    return nullptr;
}

AnimationSet load_animations(const char* filepath) {
    AnimationSet set;
    
    std::ifstream file(filepath);
    if (!file) {
        SDL_Log("Failed to open animations file: %s", filepath);
        return set;
    }
    
    std::string line;
    while (std::getline(file, line)) {
        if (line.empty()) continue;
        
        size_t tab1 = line.find('\t');
        if (tab1 == std::string::npos) continue;
        
        size_t tab2 = line.find('\t', tab1 + 1);
        if (tab2 == std::string::npos) continue;
        
        Animation anim{};
        
        std::string name = line.substr(0, tab1);
        std::strncpy(anim.name, name.c_str(), sizeof(anim.name) - 1);
        
        std::string fps_str = line.substr(tab1 + 1, tab2 - tab1 - 1);
        anim.fps = std::stoi(fps_str);
        
        std::string frame_data = line.substr(tab2 + 1);
        std::stringstream ss(frame_data);
        std::string token;
        std::vector<int> values;
        
        while (std::getline(ss, token, ',')) {
            if (!token.empty()) {
                values.push_back(std::stoi(token));
            }
        }
        
        if (values.size() % 5 != 0) {
            SDL_Log("Warning: frame data for '%s' not divisible by 5 (got %zu values)", 
                name.c_str(), values.size());
            continue;
        }
        
        int frame_count = values.size() / 5;
        anim.frames.resize(frame_count);
        
        for (int i = 0; i < frame_count; i++) {
            int base = i * 5;
            anim.frames[i].idx = values[base + 0];
            anim.frames[i].rect.x = values[base + 1];
            anim.frames[i].rect.y = values[base + 2];
            anim.frames[i].rect.w = values[base + 3];
            anim.frames[i].rect.h = values[base + 4];
        }
        
        set.animations.push_back(anim);
    }
    
    SDL_Log("Loaded %zu animations from %s", set.animations.size(), filepath);
    return set;
}

AnimationSet load_animations_from_plist(const std::string& unit_name, const char* plist_path) {
    PlistData plist = parse_plist(plist_path);

    if (plist.frames.empty()) {
        SDL_Log("Failed to load animations from plist: %s", plist_path);
        return AnimationSet{};
    }

    return plist_to_animations(plist, unit_name);
}
