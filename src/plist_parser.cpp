#include "plist_parser.hpp"
#include <fstream>
#include <sstream>
#include <regex>
#include <algorithm>
#include <SDL3/SDL.h>

namespace {

// Simple XML helper functions for plist parsing
std::string extract_tag_content(const std::string& xml, const std::string& tag, size_t& pos) {
    std::string open_tag = "<" + tag + ">";
    std::string close_tag = "</" + tag + ">";

    size_t start = xml.find(open_tag, pos);
    if (start == std::string::npos) return "";

    start += open_tag.length();
    size_t end = xml.find(close_tag, start);
    if (end == std::string::npos) return "";

    pos = end + close_tag.length();
    return xml.substr(start, end - start);
}

// Parse a plist rect string like "{{404,0},{100,100}}" into x,y,w,h
bool parse_rect(const std::string& str, int& x, int& y, int& w, int& h) {
    std::regex rect_pattern(R"(\{\{(\d+),(\d+)\},\{(\d+),(\d+)\}\})");
    std::smatch match;
    if (std::regex_search(str, match, rect_pattern) && match.size() == 5) {
        x = std::stoi(match[1].str());
        y = std::stoi(match[2].str());
        w = std::stoi(match[3].str());
        h = std::stoi(match[4].str());
        return true;
    }
    return false;
}

// Parse a plist point string like "{0,0}" into x,y
bool parse_point(const std::string& str, int& x, int& y) {
    std::regex point_pattern(R"(\{(-?\d+),(-?\d+)\})");
    std::smatch match;
    if (std::regex_search(str, match, point_pattern) && match.size() == 3) {
        x = std::stoi(match[1].str());
        y = std::stoi(match[2].str());
        return true;
    }
    return false;
}

// Parse a plist size string like "{1024,1024}" into w,h
bool parse_size(const std::string& str, int& w, int& h) {
    std::regex size_pattern(R"(\{(\d+),(\d+)\})");
    std::smatch match;
    if (std::regex_search(str, match, size_pattern) && match.size() == 3) {
        w = std::stoi(match[1].str());
        h = std::stoi(match[2].str());
        return true;
    }
    return false;
}

// Parse a single frame entry from the plist
PlistFrame parse_frame_entry(const std::string& name, const std::string& dict_content) {
    PlistFrame frame;
    frame.name = name;
    frame.x = frame.y = frame.w = frame.h = 0;
    frame.offset_x = frame.offset_y = 0;
    frame.source_w = frame.source_h = 0;
    frame.rotated = false;

    // Find frame rect
    size_t frame_key_pos = dict_content.find("<key>frame</key>");
    if (frame_key_pos != std::string::npos) {
        size_t string_start = dict_content.find("<string>", frame_key_pos);
        size_t string_end = dict_content.find("</string>", string_start);
        if (string_start != std::string::npos && string_end != std::string::npos) {
            std::string frame_str = dict_content.substr(string_start + 8, string_end - string_start - 8);
            parse_rect(frame_str, frame.x, frame.y, frame.w, frame.h);
        }
    }

    // Find offset
    size_t offset_key_pos = dict_content.find("<key>offset</key>");
    if (offset_key_pos != std::string::npos) {
        size_t string_start = dict_content.find("<string>", offset_key_pos);
        size_t string_end = dict_content.find("</string>", string_start);
        if (string_start != std::string::npos && string_end != std::string::npos) {
            std::string offset_str = dict_content.substr(string_start + 8, string_end - string_start - 8);
            parse_point(offset_str, frame.offset_x, frame.offset_y);
        }
    }

    // Find sourceSize
    size_t source_key_pos = dict_content.find("<key>sourceSize</key>");
    if (source_key_pos != std::string::npos) {
        size_t string_start = dict_content.find("<string>", source_key_pos);
        size_t string_end = dict_content.find("</string>", string_start);
        if (string_start != std::string::npos && string_end != std::string::npos) {
            std::string size_str = dict_content.substr(string_start + 8, string_end - string_start - 8);
            parse_size(size_str, frame.source_w, frame.source_h);
        }
    }

    // Find rotated
    frame.rotated = dict_content.find("<key>rotated</key>") != std::string::npos &&
                    dict_content.find("<true/>", dict_content.find("<key>rotated</key>")) != std::string::npos;

    return frame;
}

} // anonymous namespace

PlistData parse_plist(const char* filepath) {
    PlistData result;
    result.texture_width = 0;
    result.texture_height = 0;

    std::ifstream file(filepath);
    if (!file) {
        SDL_Log("Failed to open plist file: %s", filepath);
        return result;
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string content = buffer.str();

    // Find the frames dictionary
    size_t frames_key = content.find("<key>frames</key>");
    if (frames_key == std::string::npos) {
        SDL_Log("No frames key found in plist: %s", filepath);
        return result;
    }

    // Find the main dict after frames key
    size_t frames_dict_start = content.find("<dict>", frames_key);
    if (frames_dict_start == std::string::npos) {
        return result;
    }

    // Parse each frame entry
    size_t pos = frames_dict_start;
    while (true) {
        // Find next frame key
        size_t key_start = content.find("<key>", pos);
        if (key_start == std::string::npos) break;

        size_t key_end = content.find("</key>", key_start);
        if (key_end == std::string::npos) break;

        std::string frame_name = content.substr(key_start + 5, key_end - key_start - 5);

        // Check if this is a frame entry (ends with .png) or metadata key
        if (frame_name.find(".png") == std::string::npos) {
            pos = key_end;
            // Check if we've hit the metadata section
            if (frame_name == "metadata") break;
            continue;
        }

        // Find the dict for this frame
        size_t dict_start = content.find("<dict>", key_end);
        if (dict_start == std::string::npos) break;

        // Find matching </dict>
        int depth = 1;
        size_t dict_end = dict_start + 6;
        while (depth > 0 && dict_end < content.length()) {
            size_t next_open = content.find("<dict>", dict_end);
            size_t next_close = content.find("</dict>", dict_end);

            if (next_close == std::string::npos) break;

            if (next_open != std::string::npos && next_open < next_close) {
                depth++;
                dict_end = next_open + 6;
            } else {
                depth--;
                dict_end = next_close + 7;
            }
        }

        std::string dict_content = content.substr(dict_start, dict_end - dict_start);
        PlistFrame frame = parse_frame_entry(frame_name, dict_content);
        result.frames[frame_name] = frame;

        pos = dict_end;
    }

    // Parse metadata
    size_t metadata_key = content.find("<key>metadata</key>");
    if (metadata_key != std::string::npos) {
        // Find size
        size_t size_key = content.find("<key>size</key>", metadata_key);
        if (size_key != std::string::npos) {
            size_t string_start = content.find("<string>", size_key);
            size_t string_end = content.find("</string>", string_start);
            if (string_start != std::string::npos && string_end != std::string::npos) {
                std::string size_str = content.substr(string_start + 8, string_end - string_start - 8);
                parse_size(size_str, result.texture_width, result.texture_height);
            }
        }

        // Find textureFileName
        size_t tex_key = content.find("<key>textureFileName</key>", metadata_key);
        if (tex_key != std::string::npos) {
            size_t string_start = content.find("<string>", tex_key);
            size_t string_end = content.find("</string>", string_start);
            if (string_start != std::string::npos && string_end != std::string::npos) {
                result.texture_filename = content.substr(string_start + 8, string_end - string_start - 8);
            }
        }
    }

    SDL_Log("Parsed plist %s: %zu frames, %dx%d texture",
            filepath, result.frames.size(), result.texture_width, result.texture_height);

    return result;
}

std::unordered_map<std::string, std::vector<PlistFrame>> PlistData::group_by_animation(
    const std::string& unit_name) const {

    std::unordered_map<std::string, std::vector<PlistFrame>> groups;

    // Frame names follow pattern: unit_name_animation_NNN.png
    // e.g., f1_general_attack_000.png
    std::string prefix = unit_name + "_";

    for (const auto& [name, frame] : frames) {
        // Check if frame belongs to this unit
        if (name.find(prefix) != 0) continue;

        // Extract animation name and frame index
        // Remove unit prefix and .png suffix
        std::string remainder = name.substr(prefix.length());
        size_t png_pos = remainder.rfind(".png");
        if (png_pos != std::string::npos) {
            remainder = remainder.substr(0, png_pos);
        }

        // Find the last underscore followed by digits (frame number)
        size_t last_underscore = remainder.rfind('_');
        if (last_underscore == std::string::npos) continue;

        std::string anim_name = remainder.substr(0, last_underscore);
        std::string frame_num_str = remainder.substr(last_underscore + 1);

        // Verify frame number is all digits
        bool all_digits = !frame_num_str.empty() &&
            std::all_of(frame_num_str.begin(), frame_num_str.end(), ::isdigit);
        if (!all_digits) continue;

        groups[anim_name].push_back(frame);
    }

    // Sort each animation's frames by frame number
    for (auto& [anim_name, frame_list] : groups) {
        std::sort(frame_list.begin(), frame_list.end(), [&](const PlistFrame& a, const PlistFrame& b) {
            // Extract frame numbers for comparison
            auto get_frame_num = [&](const PlistFrame& f) -> int {
                std::string s = f.name;
                size_t last_underscore = s.rfind('_');
                size_t png_pos = s.rfind(".png");
                if (last_underscore != std::string::npos && png_pos != std::string::npos) {
                    return std::stoi(s.substr(last_underscore + 1, png_pos - last_underscore - 1));
                }
                return 0;
            };
            return get_frame_num(a) < get_frame_num(b);
        });
    }

    return groups;
}

AnimationSet plist_to_animations(const PlistData& plist, const std::string& unit_name) {
    AnimationSet set;

    auto groups = plist.group_by_animation(unit_name);

    constexpr int DEFAULT_FPS = 12; // Duelyst uses 12 FPS for most animations

    for (const auto& [anim_name, frame_list] : groups) {
        Animation anim{};
        std::strncpy(anim.name, anim_name.c_str(), sizeof(anim.name) - 1);
        anim.fps = DEFAULT_FPS;

        int idx = 0;
        for (const auto& pf : frame_list) {
            AnimFrame frame;
            frame.idx = idx++;
            frame.rect.x = pf.x;
            frame.rect.y = pf.y;
            frame.rect.w = pf.w;
            frame.rect.h = pf.h;
            anim.frames.push_back(frame);
        }

        if (!anim.frames.empty()) {
            set.animations.push_back(anim);
        }
    }

    SDL_Log("Converted plist to %zu animations for unit '%s'", set.animations.size(), unit_name.c_str());

    return set;
}
