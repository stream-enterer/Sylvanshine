# Analytical Anti-Aliasing: Technical Manual

## 1. Principle

Analytical anti-aliasing computes mathematically exact pixel coverage using **Signed Distance Fields (SDFs)** combined with **screen-space partial derivatives**. Rather than sampling multiple points per pixel, we calculate precisely how much of each pixel the shape occupies.

**Core algorithm:**
```glsl
float d = sdf(p);                              // signed distance to shape boundary
float w = fwidth(d);                           // rate of change per screen pixel
float alpha = smoothstep(w, -w, d);            // 1-pixel anti-aliased blend
```

This produces mathematically perfect 1-pixel-wide anti-aliasing at any resolution, zoom level, rotation, or perspective transformation.

---

## 2. Signed Distance Fields

### 2.1 Definition

An SDF is a function returning the shortest distance from point `p` to the nearest surface of a shape:

- **Zero** = exactly on the boundary
- **Positive/Negative** = outside/inside (convention varies)

**Conventions:**
| Convention | Inside | Outside | Used By |
|------------|--------|---------|---------|
| Inigo Quilez | negative | positive | Most shader tutorials |
| Wikipedia | positive | negative | Some academic sources |

Formulas differ only by sign. This manual uses **Inigo Quilez convention** (positive outside).

### 2.2 Critical Property: Unit Gradient

A proper SDF has **gradient magnitude ≈ 1 everywhere**:

```
|∇sdf| = 1
```

This means the distance value changes at a constant rate of 1 unit per 1 unit of space traveled. This property is essential because:

1. It enables direct mapping between distance and pixel coverage
2. The `fwidth(d)` result directly represents pixel width in distance units
3. Anti-aliasing width remains consistent across the entire shape

**Operations that break this property:**
- Smooth boolean operations (smooth-min, smooth-max)
- Non-uniform scaling of coordinates
- Stretching a circle instead of using proper ellipse SDF

---

## 3. Screen-Space Partial Derivatives

### 3.1 Available Functions

| Function | Definition | Description |
|----------|------------|-------------|
| `dFdx(v)` | ∂v/∂x | Rate of change along screen X |
| `dFdy(v)` | ∂v/∂y | Rate of change along screen Y |
| `fwidth(v)` | `abs(dFdx(v)) + abs(dFdy(v))` | Total rate of change (L1 norm) |

**HLSL equivalents:** `ddx()`, `ddy()`, `fwidth()`

### 3.2 How GPUs Compute Derivatives

GPUs execute fragment shaders in 2×2 pixel quads. Each fragment has access to its horizontal and vertical neighbors, enabling finite difference computation:

```
dFdx(v) = v[x+1, y] - v[x, y]
dFdy(v) = v[x, y+1] - v[x, y]
```

This is essentially free—the same mechanism used internally for mipmap level selection.

### 3.3 Enabling in WebGL 1 / GLSL ES 2.0

```glsl
#extension GL_OES_standard_derivatives : enable
```

Available by default in WebGL 2, GLSL ES 3.0+, and all desktop GL versions.

### 3.4 Why fwidth(d) Works

When you call `fwidth(d)` on a signed distance value:

- In 2D at native resolution: returns how many distance units fit in one pixel
- In 3D with perspective: automatically accounts for foreshortening
- Objects farther away have higher `fwidth(d)` (distance changes faster per pixel)
- Objects closer have lower `fwidth(d)` (distance changes slower per pixel)

The derivative measures distance change **as seen from the flat screen**, automatically handling all transformations.

---

## 4. Anti-Aliasing Formulas

### 4.1 Standard Method (Resolution-Independent)

Works for 2D, 3D, any transformation:

```glsl
float d = sdf(p);
float w = fwidth(d);
float alpha = 1.0 - smoothstep(-w, w, d);
```

**Simplified equivalent:**
```glsl
float alpha = clamp(0.5 - d / fwidth(d), 0.0, 1.0);
```

For **Wikipedia convention** (positive inside):
```glsl
float alpha = clamp(0.5 + d / fwidth(d), 0.0, 1.0);
```

### 4.2 Known Resolution (2D Only)

When coordinate space is fixed and known:

```glsl
// Assuming p normalized to [-1, 1] on shortest axis
vec2 p = (2.0 * gl_FragCoord.xy - resolution) / min(resolution.x, resolution.y);
float pixelWidth = 2.0 / min(resolution.x, resolution.y);

float d = sdf(p);
float alpha = smoothstep(pixelWidth, -pixelWidth, d);
```

### 4.3 L2-Norm Alternative

More geometrically accurate than `fwidth` (L1 norm), slightly more expensive:

```glsl
float w = length(vec2(dFdx(d), dFdy(d)));
float alpha = smoothstep(w, -w, d);
```

Comparison:
- **L1 (fwidth):** Slightly wider AA at 45° angles
- **L2 (length):** Uniform AA width at all angles

Difference is subtle; L1 is usually sufficient.

### 4.4 Transition Position Control

Control where the AA gradient appears relative to the mathematical edge:

```glsl
// k = 0.0: transition inside shape
// k = 0.5: transition centered on edge (default)
// k = 1.0: transition outside shape
float alpha = clamp(k - d / w, 0.0, 1.0);
```

### 4.5 Unknown SDF Scale (Texture-Based SDFs)

When SDF texture scale is unknown, compute it dynamically:

```glsl
float d = cutoff - texture(sdfTex, uv).r;

// SDF gradient in screen space
vec2 gradient = vec2(dFdx(d), dFdy(d));

// Distance to edge in pixels
float pixelDist = d / length(gradient);

float alpha = clamp(0.5 - pixelDist, 0.0, 1.0);
```

This handles any SDF texture regardless of how it was generated.

---

## 5. Smoothstep vs Linear Interpolation

### 5.1 Linear Step

```glsl
float linearstep(float a, float b, float x) {
    return clamp((x - a) / (b - a), 0.0, 1.0);
}
```

### 5.2 Smooth Step (Hermite)

```glsl
// Built-in: smoothstep(edge0, edge1, x)
// Hermite interpolation: 3t² - 2t³
```

### 5.3 Which to Use

**For 1-pixel anti-aliasing:** Either works; difference is imperceptible.

**For blur/soft edges:** Use `smoothstep`. Linear interpolation creates visible discontinuity artifacts (Mach banding) at the transition boundaries. The human visual system detects the abrupt change in derivative.

**Performance:** `smoothstep` is a GPU intrinsic—no performance difference.

---

## 6. 2D SDF Primitives

All functions return positive values outside, negative inside. Shapes centered at origin; transform `p` for positioning.

### 6.1 Circle

```glsl
float sdCircle(vec2 p, float r) {
    return length(p) - r;
}
```

### 6.2 Box (Axis-Aligned)

```glsl
float sdBox(vec2 p, vec2 b) {
    vec2 d = abs(p) - b;
    return length(max(d, 0.0)) + min(max(d.x, d.y), 0.0);
}
```

### 6.3 Rounded Box

```glsl
float sdRoundedBox(vec2 p, vec2 b, vec4 r) {
    // r.x = top-right, r.y = bottom-right, r.z = top-left, r.w = bottom-left
    r.xy = (p.x > 0.0) ? r.xy : r.zw;
    r.x  = (p.y > 0.0) ? r.x  : r.y;
    vec2 q = abs(p) - b + r.x;
    return min(max(q.x, q.y), 0.0) + length(max(q, 0.0)) - r.x;
}
```

### 6.4 Line Segment

```glsl
float sdSegment(vec2 p, vec2 a, vec2 b) {
    vec2 pa = p - a, ba = b - a;
    float h = clamp(dot(pa, ba) / dot(ba, ba), 0.0, 1.0);
    return length(pa - ba * h);
}
```

### 6.5 Capsule / Stadium

```glsl
float sdCapsule(vec2 p, vec2 a, vec2 b, float r) {
    return sdSegment(p, a, b) - r;
}
```

### 6.6 Equilateral Triangle

```glsl
float sdEquilateralTriangle(vec2 p, float r) {
    const float k = sqrt(3.0);
    p.x = abs(p.x) - r;
    p.y = p.y + r / k;
    if (p.x + k * p.y > 0.0)
        p = vec2(p.x - k * p.y, -k * p.x - p.y) / 2.0;
    p.x -= clamp(p.x, -2.0 * r, 0.0);
    return -length(p) * sign(p.y);
}
```

### 6.7 Isosceles Triangle

```glsl
float sdTriangleIsosceles(vec2 p, vec2 q) {
    p.x = abs(p.x);
    vec2 a = p - q * clamp(dot(p, q) / dot(q, q), 0.0, 1.0);
    vec2 b = p - q * vec2(clamp(p.x / q.x, 0.0, 1.0), 1.0);
    float s = -sign(q.y);
    vec2 d = min(vec2(dot(a, a), s * (p.x * q.y - p.y * q.x)),
                 vec2(dot(b, b), s * (p.y - q.y)));
    return -sqrt(d.x) * sign(d.y);
}
```

### 6.8 Regular Pentagon

```glsl
float sdPentagon(vec2 p, float r) {
    const vec3 k = vec3(0.809016994, 0.587785252, 0.726542528);
    p.x = abs(p.x);
    p -= 2.0 * min(dot(vec2(-k.x, k.y), p), 0.0) * vec2(-k.x, k.y);
    p -= 2.0 * min(dot(vec2(k.x, k.y), p), 0.0) * vec2(k.x, k.y);
    p -= vec2(clamp(p.x, -r * k.z, r * k.z), r);
    return length(p) * sign(p.y);
}
```

### 6.9 Regular Hexagon

```glsl
float sdHexagon(vec2 p, float r) {
    const vec3 k = vec3(-0.866025404, 0.5, 0.577350269);
    p = abs(p);
    p -= 2.0 * min(dot(k.xy, p), 0.0) * k.xy;
    p -= vec2(clamp(p.x, -k.z * r, k.z * r), r);
    return length(p) * sign(p.y);
}
```

### 6.10 Regular Octagon

```glsl
float sdOctagon(vec2 p, float r) {
    const vec3 k = vec3(-0.9238795325, 0.3826834323, 0.4142135623);
    p = abs(p);
    p -= 2.0 * min(dot(vec2(k.x, k.y), p), 0.0) * vec2(k.x, k.y);
    p -= 2.0 * min(dot(vec2(-k.x, k.y), p), 0.0) * vec2(-k.x, k.y);
    p -= vec2(clamp(p.x, -k.z * r, k.z * r), r);
    return length(p) * sign(p.y);
}
```

### 6.11 N-Star Polygon

```glsl
float sdStar(vec2 p, float r, int n, float m) {
    // m = 2 to n: star pointiness
    float an = 3.141593 / float(n);
    float en = 3.141593 / m;
    vec2 acs = vec2(cos(an), sin(an));
    vec2 ecs = vec2(cos(en), sin(en));
    float bn = mod(atan(p.x, p.y), 2.0 * an) - an;
    p = length(p) * vec2(cos(bn), abs(sin(bn)));
    p -= r * acs;
    p += ecs * clamp(-dot(p, ecs), 0.0, r * acs.y / ecs.y);
    return length(p) * sign(p.x);
}
```

### 6.12 Pie / Pacman

```glsl
float sdPie(vec2 p, vec2 c, float r) {
    // c = vec2(sin(aperture), cos(aperture))
    p.x = abs(p.x);
    float l = length(p) - r;
    float m = length(p - c * clamp(dot(p, c), 0.0, r));
    return max(l, m * sign(c.y * p.x - c.x * p.y));
}
```

### 6.13 Arc

```glsl
float sdArc(vec2 p, vec2 sc, float ra, float rb) {
    // sc = vec2(sin(aperture), cos(aperture))
    // ra = arc radius, rb = arc thickness
    p.x = abs(p.x);
    return ((sc.y * p.x > sc.x * p.y) 
        ? length(p - sc * ra) 
        : abs(length(p) - ra)) - rb;
}
```

### 6.14 Ring / Annulus

```glsl
float sdRing(vec2 p, vec2 n, float r, float th) {
    // n = vec2(sin(aperture), cos(aperture))
    p.x = abs(p.x);
    p = mat2(n.x, n.y, -n.y, n.x) * p;
    return max(abs(length(p) - r) - th * 0.5,
               length(vec2(p.x, max(0.0, abs(r - p.y) - th * 0.5))) * sign(p.x));
}
```

### 6.15 Ellipse (Exact)

```glsl
float sdEllipse(vec2 p, vec2 ab) {
    p = abs(p);
    if (p.x > p.y) { p = p.yx; ab = ab.yx; }
    float l = ab.y * ab.y - ab.x * ab.x;
    float m = ab.x * p.x / l;
    float m2 = m * m;
    float n = ab.y * p.y / l;
    float n2 = n * n;
    float c = (m2 + n2 - 1.0) / 3.0;
    float c3 = c * c * c;
    float q = c3 + m2 * n2 * 2.0;
    float d = c3 + m2 * n2;
    float g = m + m * n2;
    float co;
    if (d < 0.0) {
        float h = acos(q / c3) / 3.0;
        float s = cos(h);
        float t = sin(h) * sqrt(3.0);
        float rx = sqrt(-c * (s + t + 2.0) + m2);
        float ry = sqrt(-c * (s - t + 2.0) + m2);
        co = (ry + sign(l) * rx + abs(g) / (rx * ry) - m) / 2.0;
    } else {
        float h = 2.0 * m * n * sqrt(d);
        float s = sign(q + h) * pow(abs(q + h), 1.0 / 3.0);
        float u = sign(q - h) * pow(abs(q - h), 1.0 / 3.0);
        float rx = -s - u - c * 4.0 + 2.0 * m2;
        float ry = (s - u) * sqrt(3.0);
        float rm = sqrt(rx * rx + ry * ry);
        co = (ry / sqrt(rm - rx) + 2.0 * g / rm - m) / 2.0;
    }
    vec2 r = ab * vec2(co, sqrt(1.0 - co * co));
    return length(r - p) * sign(p.y - r.y);
}
```

### 6.16 Parabola

```glsl
float sdParabola(vec2 pos, float k) {
    pos.x = abs(pos.x);
    float ik = 1.0 / k;
    float p = ik * (pos.y - 0.5 * ik) / 3.0;
    float q = 0.25 * ik * ik * pos.x;
    float h = q * q - p * p * p;
    float r = sqrt(abs(h));
    float x = (h > 0.0)
        ? pow(q + r, 1.0 / 3.0) - pow(abs(q - r), 1.0 / 3.0) * sign(r - q)
        : 2.0 * cos(atan(r, q) / 3.0) * sqrt(p);
    return length(pos - vec2(x, k * x * x)) * sign(pos.x - x);
}
```

### 6.17 Cross

```glsl
float sdCross(vec2 p, vec2 b, float r) {
    p = abs(p);
    p = (p.y > p.x) ? p.yx : p.xy;
    vec2 q = p - b;
    float k = max(q.y, q.x);
    vec2 w = (k > 0.0) ? q : vec2(b.y - p.x, -k);
    return sign(k) * length(max(w, 0.0)) + r;
}
```

### 6.18 Vesica

```glsl
float sdVesica(vec2 p, float r, float d) {
    p = abs(p);
    float b = sqrt(r * r - d * d);
    return ((p.y - b) * d > p.x * b)
        ? length(p - vec2(0.0, b))
        : length(p - vec2(-d, 0.0)) - r;
}
```

### 6.19 Moon

```glsl
float sdMoon(vec2 p, float d, float ra, float rb) {
    p.y = abs(p.y);
    float a = (ra * ra - rb * rb + d * d) / (2.0 * d);
    float b = sqrt(max(ra * ra - a * a, 0.0));
    if (d * (p.x * b - p.y * a) > d * d * max(b - p.y, 0.0))
        return length(p - vec2(a, b));
    return max((length(p) - ra), -(length(p - vec2(d, 0.0)) - rb));
}
```

### 6.20 Egg

```glsl
float sdEgg(vec2 p, float ra, float rb) {
    const float k = sqrt(3.0);
    p.x = abs(p.x);
    float r = ra - rb;
    return ((p.y < 0.0) ? length(vec2(p.x, p.y)) - r
         : (k * (p.x + r) < p.y) ? length(vec2(p.x, p.y - k * r))
         : length(vec2(p.x + r, p.y)) - 2.0 * r) - rb;
}
```

### 6.21 Heart

```glsl
float sdHeart(vec2 p) {
    p.x = abs(p.x);
    if (p.y + p.x > 1.0)
        return sqrt(dot2(p - vec2(0.25, 0.75))) - sqrt(2.0) / 4.0;
    return sqrt(min(dot2(p - vec2(0.0, 1.0)),
                    dot2(p - 0.5 * max(p.x + p.y, 0.0)))) * sign(p.x - p.y);
}
```

### 6.22 Bezier Curve (Quadratic)

```glsl
float sdBezier(vec2 pos, vec2 A, vec2 B, vec2 C) {
    vec2 a = B - A;
    vec2 b = A - 2.0 * B + C;
    vec2 c = a * 2.0;
    vec2 d = A - pos;
    float kk = 1.0 / dot(b, b);
    float kx = kk * dot(a, b);
    float ky = kk * (2.0 * dot(a, a) + dot(d, b)) / 3.0;
    float kz = kk * dot(d, a);
    float res = 0.0;
    float p = ky - kx * kx;
    float p3 = p * p * p;
    float q = kx * (2.0 * kx * kx - 3.0 * ky) + kz;
    float h = q * q + 4.0 * p3;
    if (h >= 0.0) {
        h = sqrt(h);
        vec2 x = (vec2(h, -h) - q) / 2.0;
        vec2 uv = sign(x) * pow(abs(x), vec2(1.0 / 3.0));
        float t = clamp(uv.x + uv.y - kx, 0.0, 1.0);
        res = dot2(d + (c + b * t) * t);
    } else {
        float z = sqrt(-p);
        float v = acos(q / (p * z * 2.0)) / 3.0;
        float m = cos(v);
        float n = sin(v) * 1.732050808;
        vec3 t = clamp(vec3(m + m, -n - m, n - m) * z - kx, 0.0, 1.0);
        res = min(dot2(d + (c + b * t.x) * t.x),
                  dot2(d + (c + b * t.y) * t.y));
    }
    return sqrt(res);
}
```

---

## 7. SDF Operations

### 7.1 Boolean Operations

```glsl
// Union (min) — exact for exterior distances
float opUnion(float d1, float d2) {
    return min(d1, d2);
}

// Subtraction — bound only, not exact distance
float opSubtraction(float d1, float d2) {
    return max(-d1, d2);
}

// Intersection — bound only, not exact distance
float opIntersection(float d1, float d2) {
    return max(d1, d2);
}

// XOR — exact
float opXor(float d1, float d2) {
    return max(min(d1, d2), -max(d1, d2));
}
```

### 7.2 Smooth Boolean Operations

**Warning:** These break the unit gradient property. Anti-aliasing will be inconsistent near blend regions.

```glsl
// Smooth Union
float opSmoothUnion(float d1, float d2, float k) {
    float h = clamp(0.5 + 0.5 * (d2 - d1) / k, 0.0, 1.0);
    return mix(d2, d1, h) - k * h * (1.0 - h);
}

// Smooth Subtraction
float opSmoothSubtraction(float d1, float d2, float k) {
    float h = clamp(0.5 - 0.5 * (d2 + d1) / k, 0.0, 1.0);
    return mix(d2, -d1, h) + k * h * (1.0 - h);
}

// Smooth Intersection
float opSmoothIntersection(float d1, float d2, float k) {
    float h = clamp(0.5 - 0.5 * (d2 - d1) / k, 0.0, 1.0);
    return mix(d2, d1, h) + k * h * (1.0 - h);
}
```

### 7.3 Shape Modifications (Preserve SDF Properties)

```glsl
// Rounding — add radius to any shape
float opRound(float d, float r) {
    return d - r;
}

// Onion / Shell — creates hollow ring
float opOnion(float d, float r) {
    return abs(d) - r;
}
```

### 7.4 Domain Operations

```glsl
// Infinite repetition
vec2 opRepeat(vec2 p, vec2 spacing) {
    return mod(p + 0.5 * spacing, spacing) - 0.5 * spacing;
}

// Limited repetition
vec2 opRepeatLimited(vec2 p, float spacing, vec2 limit) {
    return p - spacing * clamp(round(p / spacing), -limit, limit);
}

// Symmetry across Y-axis
vec2 opSymX(vec2 p) {
    return vec2(abs(p.x), p.y);
}

// Symmetry across both axes
vec2 opSymXY(vec2 p) {
    return abs(p);
}
```

---

## 8. Complete Implementation Examples

### 8.1 Basic Anti-Aliased Circle

```glsl
#version 300 es
precision highp float;

uniform vec2 resolution;
out vec4 fragColor;

float sdCircle(vec2 p, float r) {
    return length(p) - r;
}

void main() {
    vec2 p = (2.0 * gl_FragCoord.xy - resolution) / min(resolution.x, resolution.y);
    
    float d = sdCircle(p, 0.5);
    float w = fwidth(d);
    float alpha = 1.0 - smoothstep(-w, w, d);
    
    vec3 col = vec3(0.1) + vec3(0.2, 0.5, 0.8) * alpha;
    fragColor = vec4(col, 1.0);
}
```

### 8.2 Shape with Anti-Aliased Outline

```glsl
float d = sdCircle(p, 0.5);
float w = fwidth(d);

// Fill
float fillAlpha = 1.0 - smoothstep(-w, w, d);

// Outline (0.02 units thick)
float outlineDist = abs(d) - 0.02;
float outlineAlpha = 1.0 - smoothstep(-w, w, outlineDist);

vec3 col = vec3(0.1);
col = mix(col, vec3(1.0, 0.5, 0.0), outlineAlpha);  // Orange outline
col = mix(col, vec3(0.2, 0.5, 0.8), fillAlpha);     // Blue fill
```

### 8.3 Soft Glow Effect

```glsl
float d = sdCircle(p, 0.3);
float w = fwidth(d);

// Exponential glow falloff
float glow = exp(-max(d, 0.0) * 3.0);

// Sharp fill with AA
float fill = 1.0 - smoothstep(-w, w, d);

vec3 col = vec3(glow * 0.2, glow * 0.4, glow * 0.8);
col += vec3(0.9) * fill;
```

### 8.4 Multiple Combined Shapes

```glsl
float d1 = sdCircle(p - vec2(-0.3, 0.0), 0.25);
float d2 = sdCircle(p - vec2(0.3, 0.0), 0.25);
float d3 = sdBox(p, vec2(0.3, 0.1));

// Smooth union of circles, then subtract box
float d = opSmoothUnion(d1, d2, 0.1);
d = opSubtraction(d3, d);

float w = fwidth(d);
float alpha = 1.0 - smoothstep(-w, w, d);
```

### 8.5 Grid Shader

```glsl
float grid(vec2 p, float spacing, float thickness) {
    vec2 q = abs(fract(p / spacing - 0.5) - 0.5) * spacing;
    float d = min(q.x, q.y) - thickness * 0.5;
    return 1.0 - smoothstep(-fwidth(d), fwidth(d), d);
}

void main() {
    vec2 p = gl_FragCoord.xy / resolution.y;
    float g = grid(p, 0.1, 0.002);
    fragColor = vec4(vec3(g), 1.0);
}
```

### 8.6 Anti-Aliased Step Function (Procedural Effects)

```glsl
// Replace step() with smooth AA version
float aaStep(float threshold, float value) {
    float w = fwidth(value);
    return smoothstep(threshold - w, threshold + w, value);
}

// For wider transitions (e.g., fire effect)
float aaStepWide(float threshold, float value) {
    float w = fwidth(value);  // Full pixel width, not half
    return smoothstep(threshold - w, threshold + w, value);
}

// Usage example: procedural fire
float fireGradient = (1.0 - uv.y) * (1.0 - uv.y);
float noise = texture(noiseTex, uv - vec2(0.0, time)).r;

float outline = aaStep(noise, fireGradient);
float edge1 = aaStep(noise, fireGradient - 0.25);
float edge2 = aaStep(noise, fireGradient - 0.5);

vec3 col = color1 * outline;
col = mix(col, color2, edge1);
col = mix(col, color3, edge2);
```

### 8.7 SDF Texture with Unknown Scale

```glsl
uniform sampler2D sdfTexture;
uniform float cutoff;  // Typically 0.5

void main() {
    float sdfValue = texture(sdfTexture, uv).r;
    float d = cutoff - sdfValue;
    
    // Compute pixel distance using SDF gradient
    vec2 gradient = vec2(dFdx(d), dFdy(d));
    float pixelDist = d / length(gradient);
    
    float alpha = clamp(0.5 - pixelDist, 0.0, 1.0);
    fragColor = vec4(color.rgb, color.a * alpha);
}
```

---

## 9. MSAA + Alpha-to-Coverage

Combine analytical AA with hardware MSAA for overlapping transparent geometry:

```cpp
// OpenGL setup
glEnable(GL_MULTISAMPLE);
glEnable(GL_SAMPLE_ALPHA_TO_COVERAGE);
```

```glsl
// Fragment shader
float d = sdf(p);
float w = fwidth(d);
gl_FragColor = vec4(color.rgb, smoothstep(w, -w, d));
// Hardware converts alpha to MSAA coverage mask
```

This provides sub-pixel accuracy where geometry overlaps.

---

## 10. Multi-Channel SDF (MSDF)

Standard SDFs lose sharp corners when scaled. MSDF stores distances to different edge contours in RGB channels:

```glsl
float median(float r, float g, float b) {
    return max(min(r, g), min(max(r, g), b));
}

void main() {
    vec3 msd = texture(msdfTexture, uv).rgb;
    float sd = median(msd.r, msd.g, msd.b);
    float w = fwidth(sd);
    float alpha = smoothstep(0.5 - w, 0.5 + w, sd);
    fragColor = vec4(color.rgb, color.a * alpha);
}
```

**Generation tool:** [msdfgen](https://github.com/Chlumsky/msdfgen)

---

## 11. Common Mistakes

### 11.1 Wrong fwidth Target

```glsl
// WRONG — measures coordinate change, not distance change
float w = fwidth(uv.x);
float alpha = smoothstep(w, -w, d);

// CORRECT — measures how fast distance changes per pixel
float w = fwidth(d);
float alpha = smoothstep(w, -w, d);
```

The SDF value `d` already incorporates all coordinate transformations. Taking `fwidth(d)` gives you exactly what you need.

### 11.2 Using length(fwidth(uv))

```glsl
// WRONG — common mistake
float w = length(fwidth(uv));

// This measures UV coordinate change, not distance change.
// Won't work correctly for shapes other than centered circles.

// CORRECT
float w = fwidth(d);
```

### 11.3 Non-Uniform Scaling

```glsl
// WRONG — breaks SDF, creates ellipse with wrong distances
float d = sdCircle(p * vec2(2.0, 1.0), 1.0);

// CORRECT — use proper ellipse SDF
float d = sdEllipse(p, vec2(0.5, 1.0));
```

### 11.4 Forgetting Aspect Ratio

```glsl
// WRONG for non-square viewports
vec2 p = gl_FragCoord.xy / resolution;

// CORRECT — maintain aspect ratio
vec2 p = (2.0 * gl_FragCoord.xy - resolution) / min(resolution.x, resolution.y);
```

### 11.5 Smooth Operations Near Edges

Smooth boolean operations distort the distance field gradient. Anti-aliasing will appear thicker or thinner in blend regions:

```glsl
float d = opSmoothUnion(d1, d2, 0.2);
// Gradient magnitude ≠ 1 near the blend
// fwidth(d) will give incorrect pixel width
```

**Mitigations:**
- Accept the artifact (often not visible)
- Use sharp booleans for final edge
- Apply smooth ops only to distant/interior regions

---

## 12. Quick Reference

### Formulas (Positive Outside SDF)

| Operation | Formula |
|-----------|---------|
| AA alpha | `clamp(0.5 - d/fwidth(d), 0.0, 1.0)` |
| Smooth AA | `1.0 - smoothstep(-w, w, d)` where `w = fwidth(d)` |
| Outline | `1.0 - smoothstep(-w, w, abs(d) - thickness)` |
| Glow | `exp(-max(d, 0.0) * falloff)` |
| L2 width | `length(vec2(dFdx(d), dFdy(d)))` |

### Formulas (Positive Inside SDF)

| Operation | Formula |
|-----------|---------|
| AA alpha | `clamp(0.5 + d/fwidth(d), 0.0, 1.0)` |
| Smooth AA | `smoothstep(-w, w, d)` where `w = fwidth(d)` |

### Derivative Functions

| GLSL | HLSL | Description |
|------|------|-------------|
| `dFdx(v)` | `ddx(v)` | Horizontal rate of change |
| `dFdy(v)` | `ddy(v)` | Vertical rate of change |
| `fwidth(v)` | `fwidth(v)` | `abs(dFdx(v)) + abs(dFdy(v))` |

---

## 13. References

- Inigo Quilez — 2D Distance Functions: https://iquilezles.org/articles/distfunctions2d/
- Inigo Quilez — 3D Distance Functions: https://iquilezles.org/articles/distfunctions/
- FrostKiwi — Analytical Anti-Aliasing: https://blog.frost.kiwi/analytical-anti-aliasing/
- pkh.me — Perfecting AA on SDFs: https://blog.pkh.me/p/44-perfecting-anti-aliasing-on-signed-distance-functions.html
- Drew Cassidy — SDF Texture Antialiasing: https://drewcassidy.me/2020/06/26/sdf-antialiasing/
- Ronja — Partial Derivatives (fwidth): https://www.ronja-tutorials.com/post/046-fwidth/
- Evan Wallace — Anti-Aliased Grid Shader: https://madebyevan.com/shaders/grid/
- Valve — Alpha-Tested Magnification (SIGGRAPH 2007)
- msdfgen: https://github.com/Chlumsky/msdfgen
