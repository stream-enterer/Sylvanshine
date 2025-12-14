#ifndef FX_COMPOSITION_RESOLVER_HPP
#define FX_COMPOSITION_RESOLVER_HPP

#include <string>
#include <unordered_map>
#include <vector>

struct FxComposition {
    std::string fx_type;
    std::string parent;
    std::vector<std::string> sprites;
    std::vector<std::string> particles;
    std::vector<std::string> sounds;
};

struct FxTiming {
    float frame_delay;
    std::string frame_prefix;
};

class FxCompositionResolver {
public:
    FxCompositionResolver();

    bool Load(const std::string& compositions_path, const std::string& timing_path);

    FxComposition* ResolveUnitFx(const std::string& unit_id, int faction_id,
                                  const std::string& fx_type);

    float GetFrameDelay(const std::string& rsx_name) const;

private:
    std::unordered_map<std::string, FxComposition> compositions_;
    std::unordered_map<std::string, float> timing_;
    std::unordered_map<std::string, FxComposition> resolved_cache_;

    bool ParseCompositionsJson(const std::string& json_content);
    bool ParseTimingJson(const std::string& json_content);

    std::string ExtractString(const std::string& json, const std::string& key);
    std::vector<std::string> ExtractStringArray(const std::string& json, const std::string& key);
    float ExtractFloat(const std::string& json, const std::string& key, float default_val);
    std::string Trim(const std::string& s);
    FxComposition* FindComposition(const std::string& key);
};

#endif
