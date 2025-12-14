#include "fx_composition_resolver.hpp"

#include "types.hpp"

#include <algorithm>
#include <fstream>
#include <iostream>
#include <sstream>

FxCompositionResolver::FxCompositionResolver() {}

bool FxCompositionResolver::Load(const std::string& compositions_path,
                                  const std::string& timing_path) {
    std::ifstream comp_file(compositions_path);
    if (!comp_file.is_open()) {
        std::cerr << "Failed to open compositions file: " << compositions_path << std::endl;
        return false;
    }

    std::stringstream buffer;
    buffer << comp_file.rdbuf();
    std::string comp_content = buffer.str();

    if (!ParseCompositionsJson(comp_content)) {
        std::cerr << "Failed to parse compositions JSON" << std::endl;
        return false;
    }

    std::ifstream timing_file(timing_path);
    if (timing_file.is_open()) {
        std::stringstream timing_buffer;
        timing_buffer << timing_file.rdbuf();
        ParseTimingJson(timing_buffer.str());
    }

    return true;
}

std::string FxCompositionResolver::Trim(const std::string& s) {
    size_t start = s.find_first_not_of(" \t\n\r\"");
    if (start == std::string::npos) return "";
    size_t end = s.find_last_not_of(" \t\n\r\"");
    return s.substr(start, end - start + 1);
}

std::string FxCompositionResolver::ExtractString(const std::string& json, const std::string& key) {
    std::string search = "\"" + key + "\"";
    size_t pos = json.find(search);
    if (pos == std::string::npos) return "";

    pos = json.find(':', pos);
    if (pos == std::string::npos) return "";

    size_t start = json.find('"', pos + 1);
    if (start == std::string::npos) return "";

    size_t end = json.find('"', start + 1);
    if (end == std::string::npos) return "";

    return json.substr(start + 1, end - start - 1);
}

std::vector<std::string> FxCompositionResolver::ExtractStringArray(const std::string& json, const std::string& key) {
    std::vector<std::string> result;

    std::string search = "\"" + key + "\"";
    size_t pos = json.find(search);
    if (pos == std::string::npos) return result;

    size_t array_start = json.find('[', pos);
    if (array_start == std::string::npos) return result;

    size_t array_end = json.find(']', array_start);
    if (array_end == std::string::npos) return result;

    std::string array_content = json.substr(array_start + 1, array_end - array_start - 1);

    size_t i = 0;
    while (i < array_content.size()) {
        size_t quote_start = array_content.find('"', i);
        if (quote_start == std::string::npos) break;

        size_t quote_end = array_content.find('"', quote_start + 1);
        if (quote_end == std::string::npos) break;

        result.push_back(array_content.substr(quote_start + 1, quote_end - quote_start - 1));
        i = quote_end + 1;
    }

    return result;
}

float FxCompositionResolver::ExtractFloat(const std::string& json, const std::string& key, float default_val) {
    std::string search = "\"" + key + "\"";
    size_t pos = json.find(search);
    if (pos == std::string::npos) return default_val;

    pos = json.find(':', pos);
    if (pos == std::string::npos) return default_val;

    size_t start = pos + 1;
    while (start < json.size() && (json[start] == ' ' || json[start] == '\t')) {
        start++;
    }

    size_t end = start;
    while (end < json.size() && (std::isdigit(json[end]) || json[end] == '.' || json[end] == '-')) {
        end++;
    }

    if (end == start) return default_val;

    try {
        return std::stof(json.substr(start, end - start));
    } catch (...) {
        return default_val;
    }
}

bool FxCompositionResolver::ParseCompositionsJson(const std::string& json_content) {
    size_t pos = 0;

    while (pos < json_content.size()) {
        size_t key_start = json_content.find('"', pos);
        if (key_start == std::string::npos) break;

        size_t key_end = json_content.find('"', key_start + 1);
        if (key_end == std::string::npos) break;

        std::string key = json_content.substr(key_start + 1, key_end - key_start - 1);

        size_t colon = json_content.find(':', key_end);
        if (colon == std::string::npos) break;

        size_t brace_start = json_content.find('{', colon);
        if (brace_start == std::string::npos) break;

        int brace_count = 1;
        size_t brace_end = brace_start + 1;
        while (brace_end < json_content.size() && brace_count > 0) {
            if (json_content[brace_end] == '{') brace_count++;
            else if (json_content[brace_end] == '}') brace_count--;
            brace_end++;
        }

        std::string object_json = json_content.substr(brace_start, brace_end - brace_start);

        if (key.find('.') != std::string::npos) {
            FxComposition comp;
            comp.fx_type = ExtractString(object_json, "fx_type");
            comp.parent = ExtractString(object_json, "parent");
            comp.sprites = ExtractStringArray(object_json, "sprites");
            comp.particles = ExtractStringArray(object_json, "particles");
            comp.sounds = ExtractStringArray(object_json, "sounds");

            compositions_[key] = comp;
        }

        pos = brace_end;
    }

    std::cout << "Loaded " << compositions_.size() << " FX compositions" << std::endl;
    return !compositions_.empty();
}

bool FxCompositionResolver::ParseTimingJson(const std::string& json_content) {
    size_t pos = 0;

    while (pos < json_content.size()) {
        size_t key_start = json_content.find('"', pos);
        if (key_start == std::string::npos) break;

        size_t key_end = json_content.find('"', key_start + 1);
        if (key_end == std::string::npos) break;

        std::string key = json_content.substr(key_start + 1, key_end - key_start - 1);

        size_t colon = json_content.find(':', key_end);
        if (colon == std::string::npos) break;

        size_t brace_start = json_content.find('{', colon);
        if (brace_start == std::string::npos) break;

        size_t brace_end = json_content.find('}', brace_start);
        if (brace_end == std::string::npos) break;

        std::string object_json = json_content.substr(brace_start, brace_end - brace_start + 1);

        float frame_delay = ExtractFloat(object_json, "frameDelay", DEFAULT_FRAME_DELAY);
        timing_[key] = frame_delay;

        pos = brace_end + 1;
    }

    std::cout << "Loaded " << timing_.size() << " FX timings" << std::endl;
    return true;
}

FxComposition* FxCompositionResolver::ResolveUnitFx(const std::string& unit_id,
                                                     int faction_id,
                                                     const std::string& fx_type) {
    std::vector<std::string> identifiers;

    if (faction_id == 0) {
        identifiers.push_back("Neutral." + fx_type);
    } else if (faction_id >= 1 && faction_id <= 6) {
        identifiers.push_back("Faction" + std::to_string(faction_id) + "." + fx_type);
    }

    identifiers.push_back("Factions." + fx_type);

    if (!unit_id.empty()) {
        identifiers.push_back("FX.Cards." + unit_id + "." + fx_type);
    }

    for (auto it = identifiers.rbegin(); it != identifiers.rend(); ++it) {
        FxComposition* comp = FindComposition(*it);
        if (comp) {
            return comp;
        }
    }

    return FindComposition("Unknown." + fx_type);
}

FxComposition* FxCompositionResolver::FindComposition(const std::string& key) {
    auto it = compositions_.find(key);
    if (it != compositions_.end()) {
        return &it->second;
    }
    return nullptr;
}

float FxCompositionResolver::GetFrameDelay(const std::string& rsx_name) const {
    auto it = timing_.find(rsx_name);
    if (it != timing_.end()) {
        return it->second;
    }
    return DEFAULT_FRAME_DELAY;
}
