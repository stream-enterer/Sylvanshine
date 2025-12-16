#pragma once
#include <string>
#include <unordered_map>

struct UnitTiming {
    float attack_damage_delay = 0.5f;
};

struct TimingData {
    std::unordered_map<std::string, UnitTiming> unit_timings;
    
    bool load(const char* filepath);
    UnitTiming get(const std::string& unit_folder) const;
};
