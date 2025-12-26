# SDF Infinite Grid Implementation Plan

## Overview

Implement an SDF-based infinite grid that:
- Renders at the **exact same perspective** as the current board (16° X-rotation, 60° FOV)
- Matches the **tile spacing** of the current 9×5 grid (48px base tile size × scale)
- Extends infinitely beyond the board with distance fade
- Renders **on top of floor tiles** as an overlay
- Uses **bright/visible white lines** (not subtle dark gaps)

The technique: **Infinite Grid Raycast** from `docs/sdf_manual/grid-shaders-sdl3.md` Section 6, combined with the **Pristine Grid** anti-aliasing from Section 3.

---

## Architecture

```
┌─────────────────────────────────────────────────────────────────┐
│                        Render Order                             │
├─────────────────────────────────────────────────────────────────┤
│  1. Background (clear)                                          │
│  2. Floor tiles (render_floor_grid)                             │
│  3. ★ SDF Infinite Grid (NEW - this feature)                    │
│  4. Tile highlights (move range, attack range)                  │
│  5. Path overlay                                                │
│  6. Entities                                                    │
│  7. UI                                                          │
└─────────────────────────────────────────────────────────────────┘
```

---

## File Changes Summary

| File | Action | Purpose |
|------|--------|---------|
| `shaders/sdf_grid.vert` | CREATE | Full-screen quad + raycast unproject |
| `shaders/sdf_grid.frag` | CREATE | Pristine grid SDF + distance fade |
| `include/sdf_grid_renderer.hpp` | CREATE | Grid renderer class declaration |
| `src/sdf_grid_renderer.cpp` | CREATE | Grid renderer implementation |
| `src/gpu_renderer.cpp` | MODIFY | Add pipeline creation for SDF grid |
| `include/gpu_renderer.hpp` | MODIFY | Add SDF grid pipeline handle + draw method |
| `src/main.cpp` | MODIFY | Call SDF grid render after floor tiles |

---

## Step 1: Perspective Matrix Derivation

The current system uses CPU-side perspective transform. We must derive matching GPU matrices.

### Current Perspective Parameters (from `perspective.hpp`)

```cpp
constexpr float FOV_DEGREES = 60.0f;
constexpr float BOARD_X_ROTATION = 16.0f;  // degrees
constexpr float DEG_TO_RAD = 3.14159265359f / 180.0f;

float zeye = (window_h * 0.5f) / tan(FOV_DEGREES * 0.5f * DEG_TO_RAD);
float center_x = window_w * 0.5f;
float center_y = window_h * 0.5f;
```

### Transform Breakdown (from `perspective.cpp:26-45`)

The CPU transform does:
1. Translate point relative to screen center
2. Rotate around X-axis by `rotation_deg` (16°)
3. Perspective divide by `(zeye - rotated_z)`
4. Translate back to screen coordinates

### Equivalent GPU Matrices

For the raycast infinite grid, we need `invViewProj` (inverse of view × projection).

**View Matrix** (camera looking down at 16° angle):
```
Camera position: (center_x, center_y, zeye) in screen-space
                 BUT we treat board as Y=0 ground plane

The board sits at Z=0 in world space.
Camera rotates -16° around X-axis (tilt down to look at board).
```

**Projection Matrix** (perspective with 60° FOV):
```
fov = 60°
aspect = window_w / window_h
near = 1.0 (minimum depth clamp from perspective.cpp:38)
far = zeye * 2.0 (enough to see full board)
```

### Matrix Construction (C++ implementation)

```cpp
// sdf_grid_renderer.cpp

Mat4 build_view_matrix(const RenderConfig& config) {
    float zeye = calculate_zeye(config.window_h);
    float angle_rad = BOARD_X_ROTATION * DEG_TO_RAD;

    // Camera at (0, 0, zeye), rotated -16° around X to look at ground plane
    // In OpenGL convention: +Y is up, +Z is toward viewer
    // Board is at Y=0 (ground plane), camera above looking down

    Mat4 rotation = Mat4::rotation_x(-angle_rad);  // Tilt camera down
    Mat4 translation = Mat4::translation(0, 0, -zeye);  // Move camera back

    return translation * rotation;
}

Mat4 build_projection_matrix(const RenderConfig& config) {
    float aspect = (float)config.window_w / (float)config.window_h;
    float fov_rad = FOV_DEGREES * DEG_TO_RAD;
    float zeye = calculate_zeye(config.window_h);
    float near = 1.0f;
    float far = zeye * 4.0f;

    return Mat4::perspective(fov_rad, aspect, near, far);
}

Mat4 build_inv_view_proj(const RenderConfig& config) {
    Mat4 view = build_view_matrix(config);
    Mat4 proj = build_projection_matrix(config);
    Mat4 viewProj = proj * view;
    return viewProj.inverse();
}
```

---

## Step 2: Grid Scale Calculation

The grid must align with tile boundaries. Each tile is `tile_size` pixels in board-space.

### Board-Space to World-Space Mapping

The board occupies:
- Width: `BOARD_COLS * tile_size` = `9 * tile_size` pixels
- Height: `BOARD_ROWS * tile_size` = `5 * tile_size` pixels
- Centered at `(center_x, center_y)` on screen

In world-space (for the shader):
- One tile = `1.0` world unit
- Grid origin = board origin (left-top corner)

### Grid Scale Uniform

```cpp
float gridScale = 1.0f;  // 1 grid cell = 1 tile = 1 world unit
```

The vertex shader must convert screen/board coordinates to world units:
```glsl
// World position where 1 unit = 1 tile
vec3 worldPos = vec3(
    (boardSpaceX - board_origin_x) / tile_size,
    0.0,  // Ground plane at Y=0
    (boardSpaceY - board_origin_y) / tile_size
);
```

---

## Step 3: Shader Implementation

### 3.1 Vertex Shader: `shaders/sdf_grid.vert`

```glsl
#version 450

// Outputs for fragment shader raycast
layout(location = 0) out vec3 v_nearPoint;
layout(location = 1) out vec3 v_farPoint;

// Vertex UBO (set 1 for SDL3)
layout(set = 1, binding = 0, std140) uniform VertexUBO {
    mat4 invViewProj;      // 64 bytes, offset 0
    vec4 cameraPos;        // 16 bytes, offset 64 (world-space camera position)
};

// Unproject NDC point to world space
vec3 unproject(float x, float y, float z) {
    vec4 p = invViewProj * vec4(x, y, z, 1.0);
    return p.xyz / p.w;
}

// Full-screen triangle (no vertex buffer needed)
// Generates 3 vertices that cover the entire screen
const vec2 positions[3] = vec2[](
    vec2(-1.0, -1.0),
    vec2( 3.0, -1.0),
    vec2(-1.0,  3.0)
);

void main() {
    vec2 pos = positions[gl_VertexIndex];
    gl_Position = vec4(pos, 0.0, 1.0);

    // Unproject to world-space ray endpoints
    v_nearPoint = unproject(pos.x, pos.y, 0.0);  // Near plane (NDC z=0)
    v_farPoint = unproject(pos.x, pos.y, 1.0);   // Far plane (NDC z=1)
}
```

### 3.2 Fragment Shader: `shaders/sdf_grid.frag`

```glsl
#version 450

layout(location = 0) in vec3 v_nearPoint;
layout(location = 1) in vec3 v_farPoint;

layout(location = 0) out vec4 fragColor;

// Fragment UBO (set 3 for SDL3)
layout(set = 3, binding = 0, std140) uniform FragmentUBO {
    mat4 viewProj;         // 64 bytes, offset 0
    vec4 lineColor;        // 16 bytes, offset 64 (RGBA)
    vec4 cameraPos;        // 16 bytes, offset 80
    float near;            // 4 bytes, offset 96
    float far;             // 4 bytes, offset 100
    float gridScale;       // 4 bytes, offset 104 (world units per grid cell)
    float lineWidth;       // 4 bytes, offset 108 (0.0-1.0, typically 0.02-0.05)
    float fadeStart;       // 4 bytes, offset 112 (distance where fade begins)
    float fadeEnd;         // 4 bytes, offset 116 (distance where fully faded)
    vec2 boardOrigin;      // 8 bytes, offset 120 (world-space board corner)
};

// Pristine Grid from docs/sdf_manual/grid-shaders-sdl3.md Section 3
float pristineGrid(vec2 uv, float lw) {
    vec4 uvDDXY = vec4(dFdx(uv), dFdy(uv));
    vec2 uvDeriv = vec2(length(uvDDXY.xz), length(uvDDXY.yw));

    vec2 targetWidth = vec2(lw);
    vec2 drawWidth = clamp(targetWidth, uvDeriv, vec2(0.5));
    vec2 lineAA = uvDeriv * 1.5;

    vec2 gridUV = 1.0 - abs(fract(uv) * 2.0 - 1.0);
    vec2 grid2 = smoothstep(drawWidth + lineAA, drawWidth - lineAA, gridUV);

    // Brightness compensation when forced wider
    grid2 *= clamp(lw / drawWidth, vec2(0.0), vec2(1.0));

    // Moiré suppression at high derivatives
    grid2 = mix(grid2, vec2(lw), clamp(uvDeriv * 2.0 - 1.0, vec2(0.0), vec2(1.0)));

    return mix(grid2.x, 1.0, grid2.y);
}

// Compute clip-space depth for depth buffer
float computeDepth(vec3 worldPos) {
    vec4 clip = viewProj * vec4(worldPos, 1.0);
    return clip.z / clip.w;
}

// Linear depth for distance fade
float linearDepth(vec3 worldPos) {
    vec4 clip = viewProj * vec4(worldPos, 1.0);
    float ndc = (clip.z / clip.w);
    // Vulkan NDC: z is 0 to 1, not -1 to 1
    return (2.0 * near * far) / (far + near - ndc * (far - near)) / far;
}

void main() {
    // Ray-plane intersection: find where ray hits Y=0 ground plane
    // Ray: P = nearPoint + t * (farPoint - nearPoint)
    // Plane: Y = 0
    // Solve: nearPoint.y + t * (farPoint.y - nearPoint.y) = 0

    float t = -v_nearPoint.y / (v_farPoint.y - v_nearPoint.y);

    // Discard if plane is behind camera (t < 0) or ray parallel to plane
    if (t < 0.0 || abs(v_farPoint.y - v_nearPoint.y) < 0.0001) {
        discard;
    }

    // World position on ground plane
    vec3 fragPos = v_nearPoint + t * (v_farPoint - v_nearPoint);

    // Convert world position to grid UV
    // Grid origin at boardOrigin, 1 unit = 1 tile
    vec2 worldXZ = fragPos.xz;
    vec2 uv = (worldXZ - boardOrigin) * gridScale;

    // Compute grid intensity using pristine grid
    float g = pristineGrid(uv, lineWidth);

    // Distance-based fade
    float dist = length(fragPos.xz - cameraPos.xz);
    float fade = 1.0 - smoothstep(fadeStart, fadeEnd, dist);

    // Final color with alpha
    vec4 color = lineColor;
    color.a *= g * fade;

    // Early discard for nearly transparent pixels
    if (color.a < 0.001) {
        discard;
    }

    // Write depth for proper occlusion with 3D content
    gl_FragDepth = computeDepth(fragPos);

    fragColor = color;
}
```

---

## Step 4: C++ Structures

### 4.1 Header: `include/sdf_grid_renderer.hpp`

```cpp
#pragma once

#include "types.hpp"
#include "gpu_types.hpp"
#include <SDL3/SDL_gpu.h>

// Must match shader UBO layout exactly (std140)
struct alignas(16) SDFGridVertexUBO {
    float invViewProj[16];  // mat4: 64 bytes
    float cameraPos[4];     // vec4: 16 bytes (xyz + pad)
};

struct alignas(16) SDFGridFragmentUBO {
    float viewProj[16];     // mat4: 64 bytes, offset 0
    float lineColor[4];     // vec4: 16 bytes, offset 64
    float cameraPos[4];     // vec4: 16 bytes, offset 80
    float near;             // float: 4 bytes, offset 96
    float far;              // float: 4 bytes, offset 100
    float gridScale;        // float: 4 bytes, offset 104
    float lineWidth;        // float: 4 bytes, offset 108
    float fadeStart;        // float: 4 bytes, offset 112
    float fadeEnd;          // float: 4 bytes, offset 116
    float boardOrigin[2];   // vec2: 8 bytes, offset 120
};

class SDFGridRenderer {
public:
    bool init(SDL_GPUDevice* device);
    void render(SDL_GPURenderPass* pass, const RenderConfig& config);
    void cleanup();

    // Configurable parameters
    void set_line_color(float r, float g, float b, float a);
    void set_line_width(float width);  // 0.0-1.0, default 0.03
    void set_fade_distance(float start, float end);  // In tile units

private:
    SDL_GPUDevice* device = nullptr;
    SDL_GPUGraphicsPipeline* pipeline = nullptr;
    SDL_GPUShader* vert_shader = nullptr;
    SDL_GPUShader* frag_shader = nullptr;

    // Line appearance
    float line_color[4] = {1.0f, 1.0f, 1.0f, 0.6f};  // White, 60% opacity
    float line_width = 0.03f;   // Thin but visible
    float fade_start = 8.0f;    // Start fading at 8 tiles from camera
    float fade_end = 20.0f;     // Fully faded at 20 tiles

    // Matrix computation
    void compute_matrices(const RenderConfig& config,
                          SDFGridVertexUBO& vert_ubo,
                          SDFGridFragmentUBO& frag_ubo);
};
```

### 4.2 Implementation: `src/sdf_grid_renderer.cpp`

```cpp
#include "sdf_grid_renderer.hpp"
#include "perspective.hpp"
#include <cmath>
#include <cstring>

// Simple 4x4 matrix operations (inline for clarity)
namespace {

struct Mat4 {
    float m[16];

    static Mat4 identity() {
        Mat4 r = {};
        r.m[0] = r.m[5] = r.m[10] = r.m[15] = 1.0f;
        return r;
    }

    static Mat4 translation(float x, float y, float z) {
        Mat4 r = identity();
        r.m[12] = x; r.m[13] = y; r.m[14] = z;
        return r;
    }

    static Mat4 rotation_x(float rad) {
        Mat4 r = identity();
        float c = std::cos(rad), s = std::sin(rad);
        r.m[5] = c;  r.m[6] = s;
        r.m[9] = -s; r.m[10] = c;
        return r;
    }

    static Mat4 perspective(float fov_rad, float aspect, float near, float far) {
        Mat4 r = {};
        float f = 1.0f / std::tan(fov_rad * 0.5f);
        r.m[0] = f / aspect;
        r.m[5] = f;
        // Vulkan-style: Z maps to [0,1] not [-1,1]
        r.m[10] = far / (near - far);
        r.m[11] = -1.0f;
        r.m[14] = (near * far) / (near - far);
        return r;
    }

    Mat4 operator*(const Mat4& b) const {
        Mat4 r = {};
        for (int i = 0; i < 4; i++) {
            for (int j = 0; j < 4; j++) {
                for (int k = 0; k < 4; k++) {
                    r.m[j * 4 + i] += m[k * 4 + i] * b.m[j * 4 + k];
                }
            }
        }
        return r;
    }

    Mat4 inverse() const {
        // 4x4 matrix inversion (Gauss-Jordan)
        Mat4 inv = identity();
        Mat4 tmp = *this;

        for (int i = 0; i < 4; i++) {
            // Find pivot
            int pivot = i;
            for (int j = i + 1; j < 4; j++) {
                if (std::abs(tmp.m[j * 4 + i]) > std::abs(tmp.m[pivot * 4 + i])) {
                    pivot = j;
                }
            }

            // Swap rows
            if (pivot != i) {
                for (int k = 0; k < 4; k++) {
                    std::swap(tmp.m[i * 4 + k], tmp.m[pivot * 4 + k]);
                    std::swap(inv.m[i * 4 + k], inv.m[pivot * 4 + k]);
                }
            }

            // Scale row
            float scale = 1.0f / tmp.m[i * 4 + i];
            for (int k = 0; k < 4; k++) {
                tmp.m[i * 4 + k] *= scale;
                inv.m[i * 4 + k] *= scale;
            }

            // Eliminate column
            for (int j = 0; j < 4; j++) {
                if (j != i) {
                    float factor = tmp.m[j * 4 + i];
                    for (int k = 0; k < 4; k++) {
                        tmp.m[j * 4 + k] -= factor * tmp.m[i * 4 + k];
                        inv.m[j * 4 + k] -= factor * inv.m[i * 4 + k];
                    }
                }
            }
        }

        return inv;
    }

    void to_array(float* out) const {
        std::memcpy(out, m, sizeof(m));
    }
};

} // namespace

bool SDFGridRenderer::init(SDL_GPUDevice* dev) {
    device = dev;

    // Load shaders (compiled SPIR-V)
    size_t vert_size, frag_size;
    void* vert_code = SDL_LoadFile("dist/shaders/sdf_grid.vert.spv", &vert_size);
    void* frag_code = SDL_LoadFile("dist/shaders/sdf_grid.frag.spv", &frag_size);

    if (!vert_code || !frag_code) {
        SDL_Log("Failed to load SDF grid shaders");
        if (vert_code) SDL_free(vert_code);
        if (frag_code) SDL_free(frag_code);
        return false;
    }

    // Create vertex shader
    SDL_GPUShaderCreateInfo vert_info = {
        .code_size = vert_size,
        .code = (const Uint8*)vert_code,
        .entrypoint = "main",
        .format = SDL_GPU_SHADERFORMAT_SPIRV,
        .stage = SDL_GPU_SHADERSTAGE_VERTEX,
        .num_samplers = 0,
        .num_storage_textures = 0,
        .num_storage_buffers = 0,
        .num_uniform_buffers = 1  // VertexUBO
    };
    vert_shader = SDL_CreateGPUShader(device, &vert_info);
    SDL_free(vert_code);

    // Create fragment shader
    SDL_GPUShaderCreateInfo frag_info = {
        .code_size = frag_size,
        .code = (const Uint8*)frag_code,
        .entrypoint = "main",
        .format = SDL_GPU_SHADERFORMAT_SPIRV,
        .stage = SDL_GPU_SHADERSTAGE_FRAGMENT,
        .num_samplers = 0,
        .num_storage_textures = 0,
        .num_storage_buffers = 0,
        .num_uniform_buffers = 1  // FragmentUBO
    };
    frag_shader = SDL_CreateGPUShader(device, &frag_info);
    SDL_free(frag_code);

    if (!vert_shader || !frag_shader) {
        SDL_Log("Failed to create SDF grid shaders");
        cleanup();
        return false;
    }

    // Create graphics pipeline
    SDL_GPUColorTargetDescription color_target = {
        .format = SDL_GPU_TEXTUREFORMAT_B8G8R8A8_UNORM,  // Match swapchain
        .blend_state = {
            .enable_blend = true,
            .src_color_blendfactor = SDL_GPU_BLENDFACTOR_SRC_ALPHA,
            .dst_color_blendfactor = SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_ALPHA,
            .color_blend_op = SDL_GPU_BLENDOP_ADD,
            .src_alpha_blendfactor = SDL_GPU_BLENDFACTOR_ONE,
            .dst_alpha_blendfactor = SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_ALPHA,
            .alpha_blend_op = SDL_GPU_BLENDOP_ADD,
        }
    };

    SDL_GPUGraphicsPipelineCreateInfo pipeline_info = {
        .vertex_shader = vert_shader,
        .fragment_shader = frag_shader,
        .vertex_input_state = {
            .num_vertex_buffers = 0,  // No vertex buffer - positions generated in shader
            .num_vertex_attributes = 0,
        },
        .primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST,
        .rasterizer_state = {
            .fill_mode = SDL_GPU_FILLMODE_FILL,
            .cull_mode = SDL_GPU_CULLMODE_NONE,  // Full-screen quad, no culling
            .front_face = SDL_GPU_FRONTFACE_COUNTER_CLOCKWISE,
        },
        .depth_stencil_state = {
            .enable_depth_test = false,   // Overlay on top, no depth test
            .enable_depth_write = false,
            .compare_op = SDL_GPU_COMPAREOP_ALWAYS,
        },
        .target_info = {
            .num_color_targets = 1,
            .color_target_descriptions = &color_target,
            .has_depth_stencil_target = false,
        },
    };

    pipeline = SDL_CreateGPUGraphicsPipeline(device, &pipeline_info);
    if (!pipeline) {
        SDL_Log("Failed to create SDF grid pipeline: %s", SDL_GetError());
        cleanup();
        return false;
    }

    SDL_Log("SDF grid renderer initialized");
    return true;
}

void SDFGridRenderer::compute_matrices(const RenderConfig& config,
                                        SDFGridVertexUBO& vert_ubo,
                                        SDFGridFragmentUBO& frag_ubo) {
    float zeye = calculate_zeye(static_cast<float>(config.window_h));
    float aspect = static_cast<float>(config.window_w) / static_cast<float>(config.window_h);
    float fov_rad = FOV_DEGREES * DEG_TO_RAD;
    float angle_rad = BOARD_X_ROTATION * DEG_TO_RAD;

    // Near/far planes
    float near = 1.0f;
    float far = zeye * 4.0f;

    // Build view matrix: camera at (0, zeye, 0) looking down at angle
    // In our coordinate system:
    //   - X is right
    //   - Y is up (camera height)
    //   - Z is forward (into screen)
    // The board sits at Y=0 (ground plane).

    // First, position camera above ground
    Mat4 cam_translate = Mat4::translation(0.0f, -zeye, 0.0f);

    // Then rotate to look down at board (rotate around X)
    Mat4 cam_rotate = Mat4::rotation_x(angle_rad);

    // View = rotate then translate (camera transform is inverse of object)
    Mat4 view = cam_translate * cam_rotate;

    // Projection matrix (Vulkan-style, Z in [0,1])
    Mat4 proj = Mat4::perspective(fov_rad, aspect, near, far);

    // Combined view-projection
    Mat4 viewProj = proj * view;
    Mat4 invViewProj = viewProj.inverse();

    // Fill UBO data
    invViewProj.to_array(vert_ubo.invViewProj);
    viewProj.to_array(frag_ubo.viewProj);

    // Camera position in world space (at origin, zeye above ground)
    vert_ubo.cameraPos[0] = 0.0f;
    vert_ubo.cameraPos[1] = zeye;
    vert_ubo.cameraPos[2] = 0.0f;
    vert_ubo.cameraPos[3] = 1.0f;

    frag_ubo.cameraPos[0] = 0.0f;
    frag_ubo.cameraPos[1] = zeye;
    frag_ubo.cameraPos[2] = 0.0f;
    frag_ubo.cameraPos[3] = 1.0f;

    // Fragment UBO parameters
    frag_ubo.near = near;
    frag_ubo.far = far;
    frag_ubo.gridScale = 1.0f;  // 1 world unit = 1 tile
    frag_ubo.lineWidth = line_width;
    frag_ubo.fadeStart = fade_start;
    frag_ubo.fadeEnd = fade_end;

    // Line color
    std::memcpy(frag_ubo.lineColor, line_color, sizeof(line_color));

    // Board origin in world space
    // The board is centered on screen, so world origin = board center
    // Board extends from (-4.5, -2.5) to (4.5, 2.5) in tile units
    frag_ubo.boardOrigin[0] = -static_cast<float>(BOARD_COLS) * 0.5f;
    frag_ubo.boardOrigin[1] = -static_cast<float>(BOARD_ROWS) * 0.5f;
}

void SDFGridRenderer::render(SDL_GPURenderPass* pass, const RenderConfig& config) {
    if (!pipeline) return;

    // Compute matrices and fill UBOs
    SDFGridVertexUBO vert_ubo = {};
    SDFGridFragmentUBO frag_ubo = {};
    compute_matrices(config, vert_ubo, frag_ubo);

    // Bind pipeline
    SDL_BindGPUGraphicsPipeline(pass, pipeline);

    // Push vertex uniforms (set 1, binding 0)
    SDL_PushGPUVertexUniformData(
        SDL_GetGPUCommandBufferFromRenderPass(pass),
        0,  // slot
        &vert_ubo,
        sizeof(vert_ubo)
    );

    // Push fragment uniforms (set 3, binding 0)
    SDL_PushGPUFragmentUniformData(
        SDL_GetGPUCommandBufferFromRenderPass(pass),
        0,  // slot
        &frag_ubo,
        sizeof(frag_ubo)
    );

    // Draw full-screen triangle (3 vertices, generated in shader)
    SDL_DrawGPUPrimitives(pass, 3, 1, 0, 0);
}

void SDFGridRenderer::cleanup() {
    if (pipeline) {
        SDL_ReleaseGPUGraphicsPipeline(device, pipeline);
        pipeline = nullptr;
    }
    if (vert_shader) {
        SDL_ReleaseGPUShader(device, vert_shader);
        vert_shader = nullptr;
    }
    if (frag_shader) {
        SDL_ReleaseGPUShader(device, frag_shader);
        frag_shader = nullptr;
    }
}

void SDFGridRenderer::set_line_color(float r, float g, float b, float a) {
    line_color[0] = r;
    line_color[1] = g;
    line_color[2] = b;
    line_color[3] = a;
}

void SDFGridRenderer::set_line_width(float width) {
    line_width = std::clamp(width, 0.001f, 0.5f);
}

void SDFGridRenderer::set_fade_distance(float start, float end) {
    fade_start = start;
    fade_end = end;
}
```

---

## Step 5: GPU Renderer Integration

### 5.1 Modify `include/gpu_renderer.hpp`

Add to class declaration:

```cpp
// In GPURenderer class, private section:
SDL_GPUGraphicsPipeline* sdf_grid_pipeline = nullptr;

// In GPURenderer class, public section:
void render_sdf_grid(const RenderConfig& config);
```

### 5.2 Modify `src/gpu_renderer.cpp`

Add initialization in `GPURenderer::init()`:

```cpp
// After other pipeline creation...
sdf_grid_renderer.init(device);
```

Add render method:

```cpp
void GPURenderer::render_sdf_grid(const RenderConfig& config) {
    if (!current_render_pass) return;
    sdf_grid_renderer.render(current_render_pass, config);
}
```

Add cleanup in `GPURenderer::cleanup()`:

```cpp
sdf_grid_renderer.cleanup();
```

---

## Step 6: Main.cpp Integration

### 6.1 Add Include

```cpp
#include "sdf_grid_renderer.hpp"
```

### 6.2 Add to Render Loop

After `state.grid_renderer.render_floor_grid(config)` and before tile highlights:

```cpp
// Floor tiles (existing)
state.grid_renderer.render_floor_grid(config);

// SDF infinite grid overlay (NEW)
g_gpu.render_sdf_grid(config);

// Tile highlights (existing)
// ... move range, attack range, etc.
```

---

## Step 7: Build System Integration

### 7.1 Shader Compilation

Add to `build.fish` shader compilation section:

```fish
# SDF Grid shaders
glslangValidator -V shaders/sdf_grid.vert -o dist/shaders/sdf_grid.vert.spv
glslangValidator -V shaders/sdf_grid.frag -o dist/shaders/sdf_grid.frag.spv
```

### 7.2 CMakeLists.txt (if applicable)

Add new source files:

```cmake
set(SOURCES
    # ... existing sources ...
    src/sdf_grid_renderer.cpp
)
```

---

## Step 8: Perspective Alignment Verification

### Critical: Ensuring Exact Match

The SDF grid must align **exactly** with the existing tile boundaries. This requires:

1. **Same coordinate origin**: Board center at screen center
2. **Same projection parameters**: 60° FOV, same aspect ratio
3. **Same rotation**: 16° X-axis tilt
4. **Same scale**: 1 world unit = 1 tile = `tile_size` pixels

### Verification Test

Add a debug mode that renders both:
1. The existing `GridRenderer::render()` white lines (re-enable temporarily)
2. The new SDF grid

They should overlap **exactly**. If there's any offset or scaling mismatch, the matrix construction is wrong.

### Common Alignment Issues

| Issue | Symptom | Fix |
|-------|---------|-----|
| Y-axis flip | Grid upside down | Negate Y in view matrix or flip camera rotation |
| Origin offset | Grid shifted | Adjust `boardOrigin` in fragment UBO |
| Scale mismatch | Grid cells wrong size | Verify `gridScale` matches tile_size relationship |
| Rotation mismatch | Grid at wrong angle | Verify `BOARD_X_ROTATION` used consistently |
| Aspect distortion | Grid cells not square | Ensure projection matrix uses correct aspect ratio |

---

## Step 9: Visual Tuning

### Default Parameters

| Parameter | Value | Description |
|-----------|-------|-------------|
| Line Color | `(1.0, 1.0, 1.0, 0.6)` | White at 60% opacity |
| Line Width | `0.03` | 3% of cell width, thin but visible |
| Fade Start | `8.0` | Begin fading at 8 tiles from camera center |
| Fade End | `20.0` | Fully faded at 20 tiles |

### Adjustments for Aesthetics

If lines too thin at distance:
- Increase `lineWidth` to `0.05`

If grid too dominant:
- Reduce `lineColor.a` to `0.3`

If fade too abrupt:
- Increase gap between `fadeStart` and `fadeEnd`

---

## Step 10: Testing Checklist

- [ ] Shader compilation succeeds (no SPIR-V errors)
- [ ] Pipeline creation succeeds (no SDL_GPU errors)
- [ ] Grid renders without crashes
- [ ] Grid lines align with tile boundaries (verify with debug overlay)
- [ ] Grid extends beyond board edges
- [ ] Fade effect works at distance
- [ ] No z-fighting with floor tiles
- [ ] Performance acceptable (check frame time)
- [ ] Grid renders correctly at different window sizes
- [ ] Grid renders correctly at different scale factors

---

## Appendix A: Coordinate System Reference

```
Screen Space (current CPU perspective):
    Origin: top-left of window
    +X: right
    +Y: down
    Tile (0,0) at board_origin_x, board_origin_y

World Space (new GPU raycast):
    Origin: center of board
    +X: right
    +Y: up (camera height)
    +Z: forward (into screen)
    Tile (0,0) at (-4.5, 0, -2.5) for 9×5 board

The fragment shader converts fragPos.xz to tile coordinates
by subtracting boardOrigin and multiplying by gridScale.
```

---

## Appendix B: UBO Memory Layout Verification

Verify these match between C++ structs and GLSL:

```
SDFGridVertexUBO (80 bytes):
  offset 0:  invViewProj (mat4, 64 bytes)
  offset 64: cameraPos (vec4, 16 bytes)

SDFGridFragmentUBO (128 bytes):
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
```

Use `static_assert(sizeof(SDFGridFragmentUBO) == 128)` to verify.

---

## Appendix C: References

- Ben Golus — The Best Darn Grid Shader (Yet): https://bgolus.medium.com/the-best-darn-grid-shader-yet-727f9278b9d8
- docs/sdf_manual/grid-shaders-sdl3.md — Local reference with SDL3 specifics
- docs/sdf_manual/analytical-antialiasing-manual.md — SDF fundamentals
