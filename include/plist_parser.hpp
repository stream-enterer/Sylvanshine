#pragma once
#include "types.hpp"
#include <string>
#include <vector>
#include <unordered_map>

// Represents a single sprite frame from a Duelyst plist file
struct PlistFrame {
    std::string name;       // e.g., "f1_general_attack_000.png"
    int x, y, w, h;         // Position and size in spritesheet
    int offset_x, offset_y; // Offset for rendering
    int source_w, source_h; // Original sprite dimensions
    bool rotated;
};

// Parsed plist data containing all frames and metadata
struct PlistData {
    std::unordered_map<std::string, PlistFrame> frames;
    int texture_width;
    int texture_height;
    std::string texture_filename;

    // Group frames by animation name (e.g., "attack", "idle", "run")
    // Returns map of animation_name -> vector of frames sorted by index
    std::unordered_map<std::string, std::vector<PlistFrame>> group_by_animation(
        const std::string& unit_name) const;
};

// Parse a Duelyst .plist file and return the frame data
// Returns empty PlistData on failure
PlistData parse_plist(const char* filepath);

// Convert plist data to AnimationSet using Duelyst's typical FPS (12)
AnimationSet plist_to_animations(const PlistData& plist, const std::string& unit_name);
