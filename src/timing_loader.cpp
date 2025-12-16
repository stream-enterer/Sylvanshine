#include "timing_loader.hpp"
#include <fstream>
#include <sstream>
#include <SDL3/SDL.h>

bool TimingData::load(const char* filepath) {
    std::ifstream file(filepath);
    if (!file) {
        SDL_Log("Failed to open timing file: %s", filepath);
        return false;
    }
    
    std::string line;
    bool first_line = true;
    int loaded = 0;
    
    while (std::getline(file, line)) {
        if (line.empty()) continue;
        
        if (first_line) {
            first_line = false;
            continue;
        }
        
        std::stringstream ss(line);
        std::string folder, card_id, delay_str, release_str;
        
        if (!std::getline(ss, folder, '\t')) continue;
        if (!std::getline(ss, card_id, '\t')) continue;
        if (!std::getline(ss, delay_str, '\t')) continue;
        
        try {
            UnitTiming timing;
            timing.attack_damage_delay = std::stof(delay_str);
            unit_timings[folder] = timing;
            loaded++;
        } catch (...) {
        }
    }
    
    SDL_Log("Loaded timing data for %d units", loaded);
    return loaded > 0;
}

UnitTiming TimingData::get(const std::string& unit_folder) const {
    auto it = unit_timings.find(unit_folder);
    if (it != unit_timings.end()) {
        return it->second;
    }
    return UnitTiming{};
}
