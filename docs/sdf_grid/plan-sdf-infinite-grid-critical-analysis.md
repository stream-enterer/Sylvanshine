# Critical Analysis: SDF Infinite Grid Plan

Review of `plan-sdf-infinite-grid.md` for LLM implementation correctness.

---

## CRITICAL ISSUES (Will Cause Implementation Failure)

### 1. Wrong SDL3 API Usage

The plan uses non-existent APIs:

```cpp
// WRONG (plan line 662):
SDL_PushGPUVertexUniformData(
    SDL_GetGPUCommandBufferFromRenderPass(pass),  // Does not exist
    ...
);

// CORRECT (gpu_renderer.cpp:658 pattern):
SDL_PushGPUFragmentUniformData(cmd_buffer, 0, &uniforms, sizeof(uniforms));
```

**Evidence**: `gpu_renderer.cpp` uses `cmd_buffer` member directly, never retrieves it from render pass.

**Fix**: Change `SDFGridRenderer::render()` signature to take `SDL_GPUCommandBuffer* cmd_buffer` or access GPURenderer's `cmd_buffer` member.

---

### 2. Coordinate System Mismatch (Fundamental Flaw)

The current CPU perspective (`perspective.cpp:26-45`) is **not** a standard view-projection matrix:

```cpp
// CPU approach - screen-space pseudo-3D:
float rel_x = point.x - persp.center_x;   // Screen pixels from center
float rel_y = point.y - persp.center_y;   // Screen pixels from center

float rotated_y = rel_y * cos_a - z * sin_a;  // z=0 for all board points
float rotated_z = rel_y * sin_a + z * cos_a;

float scale = persp.zeye / (persp.zeye - rotated_z);  // Perspective divide

float proj_x = rel_x * scale + persp.center_x;
float proj_y = rotated_y * scale + persp.center_y;
```

Key observations:
- Input is screen-space pixels, not world coordinates
- `z` parameter is always `0.0f` for board points (see `grid_renderer.cpp:65`)
- The "rotation" is applied to screen Y coordinate, creating a shear effect
- This is NOT equivalent to a 3D camera looking at a ground plane

The plan's GPU approach assumes:
- 3D world space with camera at `(0, zeye, 0)`
- Raycast from screen to Y=0 ground plane
- Standard view-projection matrix math

**These models produce different perspective distortion.**

**Symptom**: Grid lines will NOT align with tile boundaries. The GPU grid will have different convergence/divergence than the CPU-transformed tiles.

**Fix options**:
1. **Finite quad approach**: Transform 4 corners of a large grid quad using `transform_board_point()` on CPU, pass to shader as vertices. Grid UV comes from interpolation.
2. **Reverse-engineer matrices**: Derive the exact matrices that replicate `apply_perspective_transform()` behavior — requires careful mathematical analysis.

---

### 3. Matrix Construction Order Wrong

```cpp
// Plan (line 601-607):
Mat4 cam_translate = Mat4::translation(0.0f, -zeye, 0.0f);
Mat4 cam_rotate = Mat4::rotation_x(angle_rad);
Mat4 view = cam_translate * cam_rotate;  // WRONG ORDER
```

View matrix = inverse of camera world transform. If camera rotates then translates in world space, view matrix should translate then rotate (inverse operations in reverse order).

```cpp
// Correct for "camera rotated then moved":
Mat4 view = cam_rotate * cam_translate;
```

But this is moot given Issue #2 — the entire matrix approach doesn't match the CPU transform.

---

### 4. Missing Render Infrastructure Access

The plan has:
```cpp
void SDFGridRenderer::render(SDL_GPURenderPass* pass, const RenderConfig& config);
```

But needs access to:
- `cmd_buffer` for uniform pushing
- `render_pass` for pipeline binding (or get from pass)
- Device for any dynamic resource creation

GPURenderer stores these as members. Options:
1. Pass `GPURenderer&` to render method
2. Make SDFGridRenderer a friend/component of GPURenderer
3. Pass all needed handles explicitly

---

## HIGH-PRIORITY ISSUES (Will Cause Bugs)

### 5. UBO Layout Needs Verification

Fragment UBO claimed layout:
```
offset 0:   viewProj (mat4, 64 bytes)
offset 64:  lineColor (vec4, 16 bytes)
offset 80:  cameraPos (vec4, 16 bytes)
offset 96:  near (float, 4 bytes)
offset 100: far (float, 4 bytes)
offset 104: gridScale (float, 4 bytes)
offset 108: lineWidth (float, 4 bytes)
offset 112: fadeStart (float, 4 bytes)
offset 116: fadeEnd (float, 4 bytes)
offset 120: boardOrigin (vec2, 8 bytes)
Total: 128 bytes
```

C++ struct:
```cpp
struct alignas(16) SDFGridFragmentUBO {
    float viewProj[16];     // 64 bytes
    float lineColor[4];     // 16 bytes
    float cameraPos[4];     // 16 bytes
    float near;             // 4 bytes
    float far;              // 4 bytes
    float gridScale;        // 4 bytes
    float lineWidth;        // 4 bytes
    float fadeStart;        // 4 bytes
    float fadeEnd;          // 4 bytes
    float boardOrigin[2];   // 8 bytes
};  // 128 bytes - correct IF no hidden padding
```

**Fix**: Add explicit verification:
```cpp
static_assert(sizeof(SDFGridFragmentUBO) == 128, "UBO size mismatch");
static_assert(offsetof(SDFGridFragmentUBO, boardOrigin) == 120, "boardOrigin offset wrong");
```

---

### 6. Depth Buffer Contradiction

Shader writes depth:
```glsl
gl_FragDepth = computeDepth(fragPos);  // Line 303
```

Pipeline disables depth:
```cpp
.enable_depth_test = false,   // Line 559
.enable_depth_write = false,  // Line 560
```

These are contradictory. Either:
- Remove `gl_FragDepth` write (grid is pure overlay)
- Enable depth test/write (grid occludes/is occluded by 3D content)

For overlay use case, remove `gl_FragDepth` entirely.

---

### 7. Linear Depth Formula Wrong for Vulkan

```glsl
// Plan (line 260-262):
float ndc = (clip.z / clip.w) * 2.0 - 1.0;  // OpenGL conversion
return (2.0 * near * far) / (far + near - ndc * (far - near)) / far;
```

Vulkan NDC z is already [0,1], not [-1,1]. The `* 2.0 - 1.0` conversion is for OpenGL.

**Fix**:
```glsl
float ndc = clip.z / clip.w;  // Already 0-1 in Vulkan
return (near * far) / (far - ndc * (far - near));
```

Or simply use world-space distance for fade (which the plan already does separately).

---

### 8. Missing Header Include and Member Declaration

Step 5 says modify `gpu_renderer.hpp` to add:
```cpp
void render_sdf_grid(const RenderConfig& config);
```

But doesn't specify:
- Where to `#include "sdf_grid_renderer.hpp"`
- Where to declare `SDFGridRenderer sdf_grid_renderer;` member
- How SDFGridRenderer accesses GPURenderer's `cmd_buffer` and `render_pass`

---

## MEDIUM-PRIORITY ISSUES (Integration Problems)

### 9. Swapchain Format Hardcoded

```cpp
// Plan (line 533):
.format = SDL_GPU_TEXTUREFORMAT_B8G8R8A8_UNORM,
```

Should query actual format. See `gpu_renderer.cpp` pattern or use:
```cpp
SDL_GetGPUSwapchainTextureFormat(device, window)
```

---

### 10. No Shader Format Fallback

Plan loads only SPIR-V:
```cpp
void* vert_code = SDL_LoadFile("dist/shaders/sdf_grid.vert.spv", &vert_size);
```

Codebase supports multiple formats (`gpu_renderer.cpp:160-170`):
```cpp
if (formats & SDL_GPU_SHADERFORMAT_SPIRV) {
    format = SDL_GPU_SHADERFORMAT_SPIRV;
    SDL_snprintf(path, sizeof(path), "dist/shaders/%s.spv", filename);
} else if (formats & SDL_GPU_SHADERFORMAT_MSL) {
    format = SDL_GPU_SHADERFORMAT_MSL;
    SDL_snprintf(path, sizeof(path), "dist/shaders/%s.msl", filename);
    entrypoint = "main0";
}
```

**Fix**: Use existing `GPURenderer::load_shader()` helper or replicate its format detection.

---

### 11. Wrong Header Include

```cpp
// Plan (line 319):
#include "gpu_types.hpp"  // Does not exist
```

Should be:
```cpp
#include "gpu_renderer.hpp"  // Contains GPUTextureHandle
```

---

### 12. No Singular Matrix Check

Gauss-Jordan inverse (lines 429-470) has:
```cpp
float scale = 1.0f / tmp.m[i * 4 + i];  // Division by zero if singular
```

Projection matrices are generally non-singular, but defensive coding would check:
```cpp
if (std::abs(tmp.m[i * 4 + i]) < 1e-10f) {
    // Handle singular matrix
}
```

---

## LOW-PRIORITY ISSUES (Polish/Clarity)

### 13. Build System Integration Vague

Plan says "Add to `build.fish` shader compilation section" without showing:
- Exact location in file
- Full command with error handling
- Whether to add MSL compilation for macOS

---

### 14. Main.cpp Integration Imprecise

"After `state.grid_renderer.render_floor_grid(config)` and before tile highlights" should include:
- Line number reference
- Surrounding context (5-10 lines)
- Exact insertion point

---

### 15. Testing Verification Lacks Detail

"Re-enable `GridRenderer::render()` temporarily" should show:
- Exact line to uncomment in main.cpp
- How to visually compare alignment
- Expected behavior if matrices are correct vs incorrect

---

## RECOMMENDED REVISIONS

### Option A: Finite Quad Approach (Recommended)

Replace infinite raycast with CPU-transformed quad:

1. Create a large grid quad (e.g., 50×50 tiles centered on board)
2. Transform 4 corners using existing `transform_board_point()`
3. Pass screen-space corners to vertex shader
4. Compute grid UV from world-space position in fragment shader
5. Grid automatically matches perspective because it uses same transform

```cpp
// Vertex data: 4 corners in screen space (after CPU perspective)
Vec2 corners[4] = {
    transform_board_point(config, -margin, -margin),
    transform_board_point(config, board_w + margin, -margin),
    transform_board_point(config, board_w + margin, board_h + margin),
    transform_board_point(config, -margin, board_h + margin),
};
```

Shader receives screen-space positions, computes UV from board-space interpolation.

### Option B: Correct Matrix Derivation

If raycast approach is required:

1. Analyze `apply_perspective_transform()` mathematically
2. Express as matrix operations (may require non-standard matrix)
3. Compute inverse for unprojection
4. Verify with test cases before full implementation

This is harder and error-prone.

### Option C: Hybrid Approach

1. Use raycast for geometry (full-screen triangle)
2. But transform ray endpoints using CPU's `apply_perspective_transform()` inverse
3. Requires deriving the inverse transform analytically

---

## IMPLEMENTATION ORDER (If Proceeding)

1. **First**: Create minimal test shader that renders solid color on ground plane
2. **Verify**: Raycast geometry covers expected screen region
3. **Then**: Add grid pattern with fixed scale
4. **Compare**: Enable old `GridRenderer::render()` and visually check alignment
5. **Iterate**: Adjust matrices/transforms until alignment is exact
6. **Finally**: Add fade, polish parameters

Do not proceed to Step N+1 until Step N is visually verified.
