# C++ Coding Standards for SDL3 Tactics Game

## Core Principles

### 1. RAII and Resource Management (CRITICAL)
- **NEVER use raw `new`/`delete`**
- **Wrap ALL SDL resources** in RAII types with proper move semantics
- Resources include: `SDL_Texture*`, `SDL_Surface*`, `SDL_Renderer*`, `SDL_Window*`
- Each wrapper type implements:
  - Destructor calling appropriate SDL cleanup function
  - Move constructor transferring ownership and nulling source
  - Move assignment with cleanup before transfer
  - Deleted copy constructor/assignment
- **Example pattern:**
```cpp
struct TextureHandle {
    SDL_Texture* ptr = nullptr;
    ~TextureHandle() { if (ptr) SDL_DestroyTexture(ptr); }
    TextureHandle(TextureHandle&& o) noexcept : ptr(o.ptr) { o.ptr = nullptr; }
    TextureHandle& operator=(TextureHandle&& o) noexcept {
        if (this != &o) {
            if (ptr) SDL_DestroyTexture(ptr);
            ptr = o.ptr;
            o.ptr = nullptr;
        }
        return *this;
    }
    TextureHandle(const TextureHandle&) = delete;
    TextureHandle& operator=(const TextureHandle&) = delete;
};
```

### 2. Value Semantics Over Pointers
- **Store objects directly, not pointers** whenever possible
- Use `std::vector<Entity>` not `std::vector<Entity*>`
- Use `AnimationSet animations` not `AnimationSet* animations`
- Only use pointers for:
  - Non-owning references (prefer `const T*` or `T&`)
  - Polymorphic types requiring dynamic dispatch
  - Optional values (prefer `std::optional<T>` over `T*`)

### 3. Smart Pointers (When Pointers Needed)
- `std::unique_ptr<T>` for single ownership
- `std::shared_ptr<T>` only when multiple owners genuinely needed (rare)
- `std::weak_ptr<T>` to break circular dependencies
- **Never mix smart pointers with manual delete**

### 4. No Manual Memory Management in Business Logic
- Entity, GameState, and game systems should NEVER have destructors for cleanup
- All cleanup handled by RAII wrappers and standard containers
- If a class needs a destructor, it should be an RAII wrapper, not a game object

### 5. Modern C++ Features (C++20)
- Use `std::span<T>` for non-owning array views
- Use `std::optional<T>` for optional values, not `-1` sentinel or `nullptr`
- Use `auto` for complex iterator types
- Use range-based for loops: `for (auto& item : container)`
- Use structured bindings: `auto [x, y] = get_position();`
- Use `constexpr` for compile-time constants
- Use `[[nodiscard]]` for functions where ignoring return is error-prone

### 6. Component-Based Architecture (For Complex Entities)
When Entity grows beyond ~200 lines or has >10 distinct behaviors:
- Split into components: `PositionComponent`, `RenderComponent`, `MovementComponent`, etc.
- Entity becomes container: `std::unordered_map<ComponentTypeID, std::unique_ptr<Component>>`
- Systems operate on components: `RenderSystem`, `MovementSystem`, `CombatSystem`
- **Benefits:**
  - Units with different feature sets (some have ranged attack, some don't)
  - Easy addition of new unit types
  - Clear data ownership and lifecycle
  - Hot-swappable behaviors

### 7. Separation of Concerns
- **Rendering code** only in render functions, never in update logic
- **Game logic** never directly calls SDL render functions
- **File I/O** isolated in loader functions, not mixed with game logic
- **Each function does ONE thing** - no 100-line functions with multiple responsibilities

### 8. Function Complexity Limits
- **Never nest more than 3 levels deep** - extract helper functions instead
- If function needs comments to explain sections, split into multiple functions
- Early returns for error cases, avoid deep nesting
- **Example refactor:**
```cpp
// BAD - 4 levels deep
void handle_click() {
    if (valid) {
        if (unit_selected) {
            if (reachable) {
                if (not_occupied) {
                    // do thing
                }
            }
        }
    }
}

// GOOD - extracted helpers
void handle_click() {
    if (!valid) return;
    if (unit_selected) handle_move_click();
    else handle_select_click();
}

void handle_move_click() {
    if (!is_reachable_and_unoccupied()) return;
    start_unit_move();
}
```

### 9. Type Safety
- Use `enum class` not plain `enum`
- Use strong types for coordinates: `BoardPos` not `std::pair<int,int>`
- Avoid `void*` - use templates or `std::variant`
- Mark single-argument constructors `explicit` to prevent implicit conversion

### 10. Const Correctness
- Member functions that don't modify state are `const`
- Pass large objects by `const&` not by value
- Return `const T&` when returning member that shouldn't be modified
- Use `const` for parameters that won't be modified

## Specific to This Codebase

### Entity Class Rules
- Texture wrapped in RAII type, not raw pointer
- No destructor needed (handled by wrapper)
- No manual move constructor needed (compiler-generated works)
- `current_anim` should be `const Animation*` (non-owning) or index into `animations`

### GameState Rules  
- Store entities by value in vector: `std::vector<Entity>`
- Selection by index: `int selected_idx = -1` (not bool flag + separate storage)
- No raw pointers to entities (they may move/reallocate in vector)

### Resource Loading Rules
- Load all assets through RAII factory functions
- Return wrapped handles, never raw pointers
- Handle errors by returning `std::optional<WrappedType>` or throwing exceptions
- Log failures but propagate errors to caller

### Update/Render Loop Rules
- `update()` modifies state, never renders
- `render()` reads state, never modifies
- Delta time always `float dt` in seconds
- No `SDL_Delay()` in update/render (handled by main loop)

## Forbidden Patterns
- ❌ `new` / `delete`
- ❌ Raw owning pointers (`SDL_Texture* texture` as member)
- ❌ Manual resource cleanup in destructors (except RAII wrappers)
- ❌ Mixing smart pointers with manual delete
- ❌ Functions longer than 50 lines without extraction
- ❌ Nesting deeper than 3 levels
- ❌ Global mutable state (except for SDL context)
- ❌ Plain `enum` (use `enum class`)
- ❌ Implicit type conversions
- ❌ Ignoring return values from resource creation

## Migration Strategy

### Phase 1: RAII Wrappers (Do First)
1. Create `TextureHandle`, `SurfaceHandle`, `RendererHandle`, `WindowHandle`
2. Replace all raw SDL pointers with handles
3. Remove manual destructors/move constructors from Entity
4. Test that resources properly cleanup

### Phase 2: Value Semantics
1. Replace pointer members with value members where possible
2. Use `std::optional<T>` for optional values
3. Prefer `const T&` parameters over `const T*`

### Phase 3: Component System (When Entity > 200 lines)
1. Define `Component` base class
2. Split Entity into component classes
3. Create systems that operate on components
4. Migrate Entity to component container

## Example Before/After

### Before (Current)
```cpp
struct Entity {
    SDL_Texture* spritesheet;
    Entity() : spritesheet(nullptr) {}
    ~Entity() { if (spritesheet) SDL_DestroyTexture(spritesheet); }
    Entity(Entity&& o) : spritesheet(o.spritesheet) { 
        o.spritesheet = nullptr; 
    }
    // Manual move assignment...
};
```

### After (Target)
```cpp
struct Entity {
    TextureHandle spritesheet;
    // No destructor needed
    // Compiler-generated move works correctly
    // Automatic cleanup via RAII
};
```

## Enforcement Checklist
When writing or reviewing code, ask:
- [ ] Are all resources wrapped in RAII types?
- [ ] Are there any raw `new`/`delete`?
- [ ] Are objects stored by value when possible?
- [ ] Is nesting ≤ 3 levels?
- [ ] Does each function do one thing?
- [ ] Are all SDL resources properly managed?
- [ ] Is `const` used correctly?
- [ ] Are enums `enum class`?
- [ ] Is error handling consistent?
- [ ] Would a 3-month-future-you understand this code?
