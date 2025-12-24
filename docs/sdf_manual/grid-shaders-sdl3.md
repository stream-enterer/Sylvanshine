# Grid Shaders for SDL3

Complete reference for implementing anti-aliased procedural grids in SDL3 GPU API.

---

## 1. SDL3 GPU Shader Setup

SDL3 GPU API requires SPIR-V shaders. Write in GLSL 450, compile with `glslangValidator` or `glslc`.

### 1.1 GLSL Layout Requirements

```glsl
#version 450

// VERTEX SHADER bindings
layout(set = 0, binding = 0) uniform sampler2D textures[];     // sampled textures
layout(set = 1, binding = 0) uniform UBO { ... };              // uniforms

// FRAGMENT SHADER bindings  
layout(set = 2, binding = 0) uniform sampler2D textures[];     // sampled textures
layout(set = 3, binding = 0) uniform UBO { ... };              // uniforms
```

**Critical:** Use `std140` layout. `vec3` occupies 16 bytes (padded to `vec4`). Pack manually or use `vec4`.

### 1.2 Compilation

```bash
glslangValidator -V grid.vert -o grid.vert.spv
glslangValidator -V grid.frag -o grid.frag.spv
```

Or with `glslc`:

```bash
glslc -fshader-stage=vertex grid.vert -o grid.vert.spv
glslc -fshader-stage=fragment grid.frag -o grid.frag.spv
```

### 1.3 C++ Shader Loading

```cpp
SDL_GPUShader* load_shader(SDL_GPUDevice* device, const char* path, 
                           SDL_GPUShaderStage stage, 
                           Uint32 num_samplers, Uint32 num_uniform_buffers) {
    size_t size;
    void* code = SDL_LoadFile(path, &size);
    
    SDL_GPUShaderCreateInfo info = {
        .code_size = size,
        .code = (const Uint8*)code,
        .entrypoint = "main",
        .format = SDL_GPU_SHADERFORMAT_SPIRV,
        .stage = stage,
        .num_samplers = num_samplers,
        .num_storage_textures = 0,
        .num_storage_buffers = 0,
        .num_uniform_buffers = num_uniform_buffers
    };
    
    SDL_GPUShader* shader = SDL_CreateGPUShader(device, &info);
    SDL_free(code);
    return shader;
}
```

---

## 2. Screen-Space Derivatives

GPUs compute derivatives across 2×2 pixel quads via finite differences.

| Function | Formula | Use |
|----------|---------|-----|
| `dFdx(v)` | `v[x+1] - v[x]` | Horizontal change per pixel |
| `dFdy(v)` | `v[y+1] - v[y]` | Vertical change per pixel |
| `fwidth(v)` | `abs(dFdx(v)) + abs(dFdy(v))` | Total change (L1 norm) |

**For grid shaders**, compute per-axis derivative lengths:

```glsl
vec4 uvDDXY = vec4(dFdx(uv), dFdy(uv));
vec2 uvDeriv = vec2(length(uvDDXY.xz), length(uvDDXY.yw));
```

This gives the rate of UV change per pixel for X and Y lines independently. More accurate than `fwidth(uv)` for grids because X and Y lines need different AA widths when the surface is angled.

---

## 3. The Pristine Grid

The highest-quality fixed-scale grid. Handles all distances without aliasing or moiré.

### 3.1 Algorithm

```glsl
float pristineGrid(vec2 uv, vec2 lineWidth) {
    lineWidth = clamp(lineWidth, vec2(0.0), vec2(1.0));
    
    vec4 uvDDXY = vec4(dFdx(uv), dFdy(uv));
    vec2 uvDeriv = vec2(length(uvDDXY.xz), length(uvDDXY.yw));
    
    bvec2 invertLine = greaterThan(lineWidth, vec2(0.5));
    vec2 targetWidth = mix(lineWidth, 1.0 - lineWidth, vec2(invertLine));
    vec2 drawWidth = clamp(targetWidth, uvDeriv, vec2(0.5));
    vec2 lineAA = max(uvDeriv, vec2(0.000001)) * 1.5;
    
    vec2 gridUV = abs(fract(uv) * 2.0 - 1.0);
    gridUV = mix(1.0 - gridUV, gridUV, vec2(invertLine));
    
    vec2 grid2 = smoothstep(drawWidth + lineAA, drawWidth - lineAA, gridUV);
    grid2 *= clamp(targetWidth / drawWidth, vec2(0.0), vec2(1.0));
    grid2 = mix(grid2, targetWidth, clamp(uvDeriv * 2.0 - 1.0, vec2(0.0), vec2(1.0)));
    grid2 = mix(grid2, 1.0 - grid2, vec2(invertLine));
    
    return mix(grid2.x, 1.0, grid2.y);
}
```

### 3.2 Key Techniques Explained

**Triangle wave UV:**
```glsl
vec2 gridUV = abs(fract(uv) * 2.0 - 1.0);
```
Converts sawtooth `fract(uv)` (0→1) to triangle wave (0→1→0). Centers the line at cell boundaries where `fract(uv) = 0` or `1`.

**Line width clamping:**
```glsl
vec2 drawWidth = clamp(targetWidth, uvDeriv, 0.5);
```
- Lower bound `uvDeriv`: line can't be thinner than 1 pixel
- Upper bound `0.5`: prevents overlap with adjacent cells (UV goes 0→1→0, so max drawable width is 0.5)

**Brightness compensation:**
```glsl
grid2 *= clamp(targetWidth / drawWidth, 0.0, 1.0);
```
When lines are forced wider than intended (at distance), reduce brightness proportionally. A 0.01-width line drawn at 0.1 width should be 10% brightness.

**Moiré suppression:**
```glsl
grid2 = mix(grid2, targetWidth, clamp(uvDeriv * 2.0 - 1.0, 0.0, 1.0));
```
When `uvDeriv > 0.5`, AA regions overlap (lines start merging). Fade to solid `targetWidth` gray. Complete fade at `uvDeriv = 1.0` (cells smaller than 1 pixel).

**Line inversion:**
```glsl
bvec2 invertLine = greaterThan(lineWidth, vec2(0.5));
vec2 targetWidth = mix(lineWidth, 1.0 - lineWidth, vec2(invertLine));
```
For `lineWidth > 0.5`, draw the gap instead of the line. Enables smooth transition: thin lines → thick lines → nearly solid.

**Final combine:**
```glsl
return mix(grid2.x, 1.0, grid2.y);
```
If either X or Y has a line, show it. Equivalent to `max(grid2.x, grid2.y)` but smoother blending at intersections.

### 3.3 Complete Shader

**grid_pristine.vert:**
```glsl
#version 450

layout(location = 0) in vec3 a_position;
layout(location = 1) in vec2 a_uv;

layout(location = 0) out vec2 v_uv;

layout(set = 1, binding = 0, std140) uniform UBO {
    mat4 mvp;
    vec4 cameraPos;
    float gridScale;
    float _pad0, _pad1, _pad2;
};

void main() {
    gl_Position = mvp * vec4(a_position, 1.0);
    
    vec3 worldPos = a_position;
    vec3 offset = floor(cameraPos.xyz * gridScale);
    v_uv = (worldPos.xz * gridScale) - offset.xz;
}
```

**grid_pristine.frag:**
```glsl
#version 450

layout(location = 0) in vec2 v_uv;
layout(location = 0) out vec4 fragColor;

layout(set = 3, binding = 0, std140) uniform UBO {
    vec4 lineColor;
    vec4 baseColor;
    vec2 lineWidth;
    vec2 _pad;
};

float pristineGrid(vec2 uv, vec2 lw) {
    lw = clamp(lw, vec2(0.0), vec2(1.0));
    
    vec4 uvDDXY = vec4(dFdx(uv), dFdy(uv));
    vec2 uvDeriv = vec2(length(uvDDXY.xz), length(uvDDXY.yw));
    
    bvec2 invertLine = greaterThan(lw, vec2(0.5));
    vec2 targetWidth = mix(lw, 1.0 - lw, vec2(invertLine));
    vec2 drawWidth = clamp(targetWidth, uvDeriv, vec2(0.5));
    vec2 lineAA = max(uvDeriv, vec2(0.000001)) * 1.5;
    
    vec2 gridUV = abs(fract(uv) * 2.0 - 1.0);
    gridUV = mix(1.0 - gridUV, gridUV, vec2(invertLine));
    
    vec2 grid2 = smoothstep(drawWidth + lineAA, drawWidth - lineAA, gridUV);
    grid2 *= clamp(targetWidth / drawWidth, vec2(0.0), vec2(1.0));
    grid2 = mix(grid2, targetWidth, clamp(uvDeriv * 2.0 - 1.0, vec2(0.0), vec2(1.0)));
    grid2 = mix(grid2, 1.0 - grid2, vec2(invertLine));
    
    return mix(grid2.x, 1.0, grid2.y);
}

void main() {
    float g = pristineGrid(v_uv, lineWidth);
    fragColor = mix(baseColor, lineColor, g * lineColor.a);
}
```

---

## 4. Multi-Scale Grid (LOD Grid)

Shows different grid densities based on camera distance. Grid lines fade in/out as you zoom.

### 4.1 Algorithm

```glsl
float lodGrid(vec2 uv, vec3 viewPos, float divisions, float bias,
              float minorWidth, float majorWidth) {
    float logDiv = log(max(divisions, 2.0));
    float logLen = (0.5 * log(dot(viewPos, viewPos))) / logDiv - bias;
    
    float logA = floor(logLen);
    float logB = logA + 1.0;
    float logC = logA + 2.0;
    float blend = fract(logLen);
    
    float scaleA = pow(divisions, logA);
    float scaleB = pow(divisions, logB);
    float scaleC = pow(divisions, logC);
    
    vec2 uvA = uv / scaleA;
    vec2 uvB = uv / scaleB;
    vec2 uvC = uv / scaleC;
    
    vec2 gridA = 1.0 - abs(fract(uvA) * 2.0 - 1.0);
    vec2 gridB = 1.0 - abs(fract(uvB) * 2.0 - 1.0);
    vec2 gridC = 1.0 - abs(fract(uvC) * 2.0 - 1.0);
    
    float widthA = minorWidth * (1.0 - blend);
    float widthB = mix(majorWidth, minorWidth, blend);
    float widthC = majorWidth * blend;
    
    float fadeA = clamp(widthA, 0.0, 1.0);
    float fadeB = clamp(widthB, 0.0, 1.0);
    float fadeC = clamp(widthC, 0.0, 1.0);
    
    vec2 uvLen = max(vec2(length(vec2(dFdx(uv.x), dFdy(uv.x))),
                         length(vec2(dFdx(uv.y), dFdy(uv.y)))), 0.00001);
    
    vec2 aaA = uvLen / scaleA;
    vec2 aaB = uvLen / scaleB;
    vec2 aaC = uvLen / scaleC;
    
    vec2 g2A = smoothstep((widthA + 1.5) * aaA, (widthA - 1.5) * aaA, gridA);
    vec2 g2B = smoothstep((widthB + 1.5) * aaB, (widthB - 1.5) * aaB, gridB);
    vec2 g2C = smoothstep((widthC + 1.5) * aaC, (widthC - 1.5) * aaC, gridC);
    
    float gA = clamp(g2A.x + g2A.y, 0.0, 1.0) * fadeA;
    float gB = clamp(g2B.x + g2B.y, 0.0, 1.0) * fadeB;
    float gC = clamp(g2C.x + g2C.y, 0.0, 1.0) * fadeC;
    
    return clamp(gA + max(gB, gC), 0.0, 1.0);
}
```

### 4.2 Parameters

| Parameter | Purpose | Typical Value |
|-----------|---------|---------------|
| `divisions` | Subdivision ratio between LOD levels | 10.0 |
| `bias` | Shifts LOD transition distance | 0.5 |
| `minorWidth` | Minor line width in pixels | 0.33 |
| `majorWidth` | Major line width in pixels | 2.0 |

### 4.3 How It Works

1. **Log-scale distance:** `logLen = log(distance) / log(divisions)` gives which power-of-N scale we're at
2. **Three LOD levels:** Current (`A`), next (`B`), and major (`C`) scales
3. **Blend factor:** `fract(logLen)` smoothly interpolates between levels
4. **Width animation:** Minor lines fade out, major lines fade in during transition

---

## 5. Simple Grid (Constant Pixel Width)

Lines stay constant pixel width regardless of distance. Good for UI, debug overlays.

```glsl
float simpleGrid(vec2 uv, float lineWidth) {
    vec2 grid = abs(fract(uv - 0.5) - 0.5);
    vec2 deriv = fwidth(uv);
    vec2 lines = smoothstep(vec2(0.0), deriv * lineWidth, grid);
    return 1.0 - min(lines.x, lines.y);
}
```

At distance, lines converge to solid color (each cell < 1 pixel).

---

## 6. Infinite Grid (Full-Screen Raycast)

Render a full-screen quad and raycast to find grid intersection. True infinite extent.

### 6.1 Vertex Shader

```glsl
#version 450

layout(location = 0) out vec3 v_nearPoint;
layout(location = 1) out vec3 v_farPoint;

layout(set = 1, binding = 0, std140) uniform UBO {
    mat4 invViewProj;
    vec4 cameraPos;
    float near;
    float far;
    vec2 _pad;
};

vec3 unproject(float x, float y, float z) {
    vec4 p = invViewProj * vec4(x, y, z, 1.0);
    return p.xyz / p.w;
}

const vec2 positions[6] = vec2[](
    vec2(-1, -1), vec2(1, -1), vec2(1, 1),
    vec2(-1, -1), vec2(1, 1), vec2(-1, 1)
);

void main() {
    vec2 p = positions[gl_VertexIndex];
    gl_Position = vec4(p, 0.0, 1.0);
    v_nearPoint = unproject(p.x, p.y, 0.0);
    v_farPoint = unproject(p.x, p.y, 1.0);
}
```

### 6.2 Fragment Shader

```glsl
#version 450

layout(location = 0) in vec3 v_nearPoint;
layout(location = 1) in vec3 v_farPoint;
layout(location = 0) out vec4 fragColor;

layout(set = 3, binding = 0, std140) uniform UBO {
    mat4 viewProj;
    vec4 cameraPos;
    vec4 lineColor;
    vec4 axisColorX;
    vec4 axisColorZ;
    float near;
    float far;
    float gridScale;
    float _pad;
};

float pristineGrid(vec2 uv, float lineWidth) {
    vec4 uvDDXY = vec4(dFdx(uv), dFdy(uv));
    vec2 uvDeriv = vec2(length(uvDDXY.xz), length(uvDDXY.yw));
    vec2 drawWidth = clamp(vec2(lineWidth), uvDeriv, vec2(0.5));
    vec2 lineAA = uvDeriv * 1.5;
    vec2 gridUV = 1.0 - abs(fract(uv) * 2.0 - 1.0);
    vec2 grid2 = smoothstep(drawWidth + lineAA, drawWidth - lineAA, gridUV);
    grid2 *= clamp(lineWidth / drawWidth, vec2(0.0), vec2(1.0));
    grid2 = mix(grid2, vec2(lineWidth), clamp(uvDeriv * 2.0 - 1.0, vec2(0.0), vec2(1.0)));
    return mix(grid2.x, 1.0, grid2.y);
}

float computeDepth(vec3 pos) {
    vec4 clip = viewProj * vec4(pos, 1.0);
    return clip.z / clip.w;
}

float linearDepth(vec3 pos) {
    vec4 clip = viewProj * vec4(pos, 1.0);
    float ndc = (clip.z / clip.w) * 2.0 - 1.0;
    return (2.0 * near * far) / (far + near - ndc * (far - near)) / far;
}

void main() {
    float t = -v_nearPoint.y / (v_farPoint.y - v_nearPoint.y);
    
    if (t < 0.0) {
        discard;
    }
    
    vec3 fragPos = v_nearPoint + t * (v_farPoint - v_nearPoint);
    vec2 uv = fragPos.xz * gridScale;
    
    float g = pristineGrid(uv, 0.02);
    
    vec4 color = lineColor;
    
    // Axis highlighting
    if (abs(fragPos.x) < 0.1 / gridScale) color = axisColorZ;
    if (abs(fragPos.z) < 0.1 / gridScale) color = axisColorX;
    color.a = g;
    
    // Distance fade
    float depth = linearDepth(fragPos);
    float fade = max(0.0, 1.0 - depth);
    color.a *= fade;
    
    gl_FragDepth = computeDepth(fragPos);
    fragColor = color;
}
```

### 6.3 C++ Setup

```cpp
// No vertex buffer needed - positions generated in shader
SDL_GPUGraphicsPipelineCreateInfo pipeline_info = {
    .vertex_shader = vert_shader,
    .fragment_shader = frag_shader,
    .primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST,
    .rasterizer_state = {
        .fill_mode = SDL_GPU_FILLMODE_FILL,
        .cull_mode = SDL_GPU_CULLMODE_NONE,
    },
    .depth_stencil_state = {
        .enable_depth_test = true,
        .enable_depth_write = true,
        .compare_op = SDL_GPU_COMPAREOP_LESS_OR_EQUAL,
    },
    .target_info = {
        .num_color_targets = 1,
        .color_target_descriptions = &(SDL_GPUColorTargetDescription){
            .format = swapchain_format,
            .blend_state = {
                .enable_blend = true,
                .src_color_blendfactor = SDL_GPU_BLENDFACTOR_SRC_ALPHA,
                .dst_color_blendfactor = SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_ALPHA,
            }
        },
        .has_depth_stencil_target = true,
        .depth_stencil_format = SDL_GPU_TEXTUREFORMAT_D32_FLOAT,
    },
};

// Draw call
SDL_DrawGPUPrimitives(render_pass, 6, 1, 0, 0);
```

---

## 7. World-Space Precision

Floating-point precision degrades far from origin. Fix by snapping UVs relative to camera.

### 7.1 Vertex Shader Fix

```glsl
vec3 worldPos = a_position;
vec3 offset = floor(cameraPos.xyz * gridScale);
v_uv = (worldPos.xz * gridScale) - offset.xz;
```

UVs stay small regardless of world position.

### 7.2 Why It Works

At world position `(10000, 0, 10000)` with `gridScale = 1`:
- Without fix: `uv = (10000, 10000)` → precision loss
- With fix: `offset = (10000, 10000)`, `uv = (0, 0)` → full precision

The grid pattern repeats, so the offset is invisible.

---

## 8. Common Patterns

### 8.1 Axis Highlighting

```glsl
vec2 deriv = fwidth(uv);
float axisWidth = 0.1;

vec4 color = gridColor;
if (abs(uv.x) < axisWidth * deriv.x) color = vec4(0, 0, 1, 1); // Z axis (blue)
if (abs(uv.y) < axisWidth * deriv.y) color = vec4(1, 0, 0, 1); // X axis (red)
```

### 8.2 Distance Fade

```glsl
float dist = length(fragPos - cameraPos.xyz);
float fade = 1.0 - smoothstep(fadeStart, fadeEnd, dist);
color.a *= fade;
```

### 8.3 Height-Based Fade

```glsl
float camHeight = abs(cameraPos.y);
float fadeRadius = camHeight * 25.0;
fadeRadius = clamp(fadeRadius, minFade, maxFade);
float dist = length(fragPos.xz - cameraPos.xz);
float fade = 1.0 - smoothstep(0.0, fadeRadius, dist);
```

### 8.4 Major/Minor Lines

```glsl
float majorScale = 10.0;
float minor = pristineGrid(uv, 0.01);
float major = pristineGrid(uv / majorScale, 0.02);
float grid = max(minor * 0.3, major);
```

---

## 9. Performance Notes

| Technique | Cost | When to Use |
|-----------|------|-------------|
| Simple grid | Low | UI, debug, constant-width lines |
| Pristine grid | Medium | Quality ground planes, editors |
| LOD grid | Medium-High | Large terrains, infinite zoom |
| Raycast infinite | Medium | Full-screen ground, CAD apps |

**Optimizations:**
- Early discard fragments with `alpha < threshold`
- Use `discard` for fully transparent regions
- LOD grid: reduce divisions for distant cameras
- Raycast: skip if camera facing away from plane

---

## 10. Uniform Buffer Layout

SDL3 requires `std140` layout. Watch alignment:

```glsl
layout(std140) uniform UBO {
    mat4 mvp;           // 64 bytes, offset 0
    vec4 color;         // 16 bytes, offset 64
    vec2 lineWidth;     // 8 bytes, offset 80
    vec2 _pad;          // 8 bytes, offset 88 (pad to 16-byte boundary)
    float scale;        // 4 bytes, offset 96
    float _pad2[3];     // 12 bytes, offset 100 (pad to 16)
};
```

C++ struct must match:

```cpp
struct alignas(16) GridUniforms {
    float mvp[16];
    float color[4];
    float lineWidth[2];
    float _pad[2];
    float scale;
    float _pad2[3];
};
```

---

## 11. Quick Reference

### Grid Functions

```glsl
// Pristine (quality, fixed scale)
float pristineGrid(vec2 uv, vec2 lineWidth);

// Simple (constant pixel width)
float simpleGrid(vec2 uv, float pixelWidth);

// LOD (multi-scale, zoom-adaptive)
float lodGrid(vec2 uv, vec3 viewPos, float divisions, float bias, float minorW, float majorW);
```

### Derivative Helpers

```glsl
// Per-axis derivative length (for grids)
vec4 uvDDXY = vec4(dFdx(uv), dFdy(uv));
vec2 uvDeriv = vec2(length(uvDDXY.xz), length(uvDDXY.yw));

// Total derivative (for SDFs)
float w = fwidth(d);        // L1 norm
float w = length(vec2(dFdx(d), dFdy(d)));  // L2 norm
```

### SDL3 Shader Bindings

| Stage | Resource | Set |
|-------|----------|-----|
| Vertex | Samplers/textures | 0 |
| Vertex | Uniforms | 1 |
| Fragment | Samplers/textures | 2 |
| Fragment | Uniforms | 3 |

---

## 12. References

- Ben Golus — The Best Darn Grid Shader (Yet): https://bgolus.medium.com/the-best-darn-grid-shader-yet-727f9278b9d8
- Ben Golus — Pristine Grid Gist: https://gist.github.com/bgolus/d49651f52b1dcf82f70421ba922ed064
- Ben Golus — Infinite Grid Gist: https://gist.github.com/bgolus/455a3666188f12cf13189839480e7120
- SDL3 GPU API: https://wiki.libsdl.org/SDL3/CategoryGPU
- SDL_shadercross: https://moonside.games/posts/introducing-sdl-gpu-shadercross/
- Inigo Quilez — Filterable Procedurals: https://iquilezles.org/articles/filterableprocedurals/
