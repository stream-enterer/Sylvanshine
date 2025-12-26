# Random Map Selection

Random background selection on game launch for Sylvanshine.

**Depends on:** `background_rendering_plan.md` (Phases 1-5 must be complete)

---

## Map Registry

```cpp
// include/battlemap_registry.hpp

class BattleMapRegistry {
public:
    static BattleMapRegistry& instance();

    // Initialize by scanning dist/backgrounds/ for config.json files
    bool init(const std::string& backgrounds_dir);

    // Get all available map IDs
    std::vector<std::string> get_available_maps() const;

    // Load a specific map
    BattleMapTemplate load_map(const std::string& map_id) const;

    // Select a random map
    std::string select_random_map() const;

private:
    std::vector<std::string> available_maps_;
    std::string backgrounds_dir_;
};
```

---

## Random Selection Implementation

```cpp
// src/battlemap_registry.cpp

#include <random>
#include <chrono>

std::string BattleMapRegistry::select_random_map() const {
    if (available_maps_.empty()) {
        return "";  // No maps available
    }

    // Seed with high-resolution clock for variety on each launch
    static std::mt19937 rng(
        static_cast<unsigned>(
            std::chrono::high_resolution_clock::now()
                .time_since_epoch()
                .count()
        )
    );

    std::uniform_int_distribution<size_t> dist(0, available_maps_.size() - 1);
    return available_maps_[dist(rng)];
}
```

---

## Integration in main.cpp

```cpp
int main(int argc, char* argv[]) {
    // ... existing initialization ...

    // Initialize map registry
    if (!BattleMapRegistry::instance().init("dist/backgrounds")) {
        SDL_Log("Warning: No background maps found");
    }

    // Select random background on launch
    std::string map_id = BattleMapRegistry::instance().select_random_map();
    if (!map_id.empty()) {
        BattleMapTemplate map = BattleMapRegistry::instance().load_map(map_id);
        state.background_renderer.load_map(map);
        SDL_Log("Loaded background: %s", map.name.c_str());
    }

    // ... game loop ...
}
```

---

## Map Discovery

Auto-discover maps by scanning for `config.json` files:

```cpp
bool BattleMapRegistry::init(const std::string& backgrounds_dir) {
    backgrounds_dir_ = backgrounds_dir;
    available_maps_.clear();

    // Scan for subdirectories containing config.json
    for (const auto& entry : std::filesystem::directory_iterator(backgrounds_dir)) {
        if (entry.is_directory()) {
            std::filesystem::path config_path = entry.path() / "config.json";
            if (std::filesystem::exists(config_path)) {
                available_maps_.push_back(entry.path().filename().string());
                SDL_Log("Found background map: %s", available_maps_.back().c_str());
            }
        }
    }

    SDL_Log("Discovered %zu background maps", available_maps_.size());
    return !available_maps_.empty();
}
```

---

## Command-Line Override

Allow specifying a map via command line for testing:

```cpp
// In parse_args()
for (int i = 1; i < argc; i++) {
    if (std::strcmp(argv[i], "--map") == 0 && i + 1 < argc) {
        config.forced_map = argv[++i];  // Override random selection
    }
}

// In main()
std::string map_id;
if (!config.forced_map.empty()) {
    map_id = config.forced_map;
    SDL_Log("Using forced map: %s", map_id.c_str());
} else {
    map_id = BattleMapRegistry::instance().select_random_map();
}
```

---

## Weighted Random Selection (Optional)

For maps with different "popularity" or to avoid recently-seen maps:

```cpp
struct MapWeight {
    std::string map_id;
    float weight;  // Higher = more likely
};

std::string select_weighted_random(const std::vector<MapWeight>& weights) {
    float total = 0.0f;
    for (const auto& w : weights) total += w.weight;

    std::uniform_real_distribution<float> dist(0.0f, total);
    float roll = dist(rng);

    float cumulative = 0.0f;
    for (const auto& w : weights) {
        cumulative += w.weight;
        if (roll <= cumulative) {
            return w.map_id;
        }
    }
    return weights.back().map_id;
}
```

---

## Success Criteria

- [ ] Random map selected on each game launch
- [ ] `--map` flag overrides random selection
- [ ] Maps auto-discovered from `dist/backgrounds/*/config.json`
