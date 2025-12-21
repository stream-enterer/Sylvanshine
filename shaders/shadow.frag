#version 450

// Shadow fragment shader with progressive blur (penumbra effect)
// Port of Duelyst's ShadowHighQualityFragment.glsl using exact formulas

layout(set = 2, binding = 0) uniform sampler2D u_texture;

layout(set = 3, binding = 0) uniform ShadowUniforms {
    float u_opacity;              // External opacity control
    float u_intensity;            // Duelyst default: 0.15
    float u_blurShiftModifier;    // Duelyst default: 1.0
    float u_blurIntensityModifier;// Duelyst default: 3.0
    vec2 u_size;                  // Sprite content size in pixels (for distance calc)
    vec2 u_anchor;                // (anchorX * width, shadowOffset) in pixels
    vec2 u_uvMin;                 // UV bounds min (for clamping blur samples to sprite region)
    vec2 u_uvMax;                 // UV bounds max
    float u_renderScale;          // Render scale for scale-aware blur
    float u_lightDistPctInv;      // Light distance attenuation: 1 at light center, 0 at radius edge
    float _pad1, _pad2;           // Alignment padding
};

layout(location = 0) in vec2 v_texCoord;
layout(location = 1) in vec2 v_position;  // Local sprite position in pixels

layout(location = 0) out vec4 fragColor;

// Sample texture with edge clamping (mimics Duelyst's per-sprite texture behavior)
// Clamping to bounds instead of returning 0 preserves silhouette during blur
float sampleAlpha(vec2 uv) {
    vec2 clampedUV = clamp(uv, u_uvMin, u_uvMax);
    return texture(u_texture, clampedUV).a;
}

void main() {
    // Distance from anchor (feet) - matches Duelyst exactly
    vec2 anchorDiff = v_position - u_anchor;
    float sizeRadius = length(u_size) * 0.5;

    // Blur by distance from anchor point
    // occluderDistPctX: horizontal distance from center (0 at center, 1 at edge)
    // occluderDistPctY: vertical distance from feet (0 at feet, 1 at head)
    float occluderDistPctX = min(abs(anchorDiff.x) / sizeRadius, 1.0);
    float occluderDistPctY = min(max(anchorDiff.y, 0.0) / u_size.x, 1.0);

    // Blur modifier: 0 at feet (sharp!), increases toward head
    // This is the KEY formula - pow(0, anything) = 0, so blur = 0 at feet
    float occluderDistPctBlurModifier = pow(occluderDistPctY, u_blurShiftModifier);

    // Blur in texture coordinate space
    // Divide by renderScale to get consistent screen-space blur at any scale:
    // - At scale 2: blur is half as many texels, but each texel is 2 screen pixels = same blur
    // - At scale 0.5: blur is twice as many texels, but each texel is 0.5 screen pixels = same blur
    vec2 uvSpan = u_uvMax - u_uvMin;  // Sprite's UV range in atlas
    float scaleCompensation = max(u_renderScale, 0.25);  // Clamp to avoid division issues at tiny scales
    float blurX = (uvSpan.x / u_size.x * u_blurIntensityModifier / scaleCompensation) * occluderDistPctBlurModifier;
    float blurY = (uvSpan.y / u_size.y * u_blurIntensityModifier / scaleCompensation) * occluderDistPctBlurModifier;

    // Intensity falloff - exact Duelyst exponents (1.25 for X, 1.5 for Y)
    // Now that we have u_lightDistPctInv, we can use the original values
    float intensityFadeX = pow(1.0 - occluderDistPctX, 1.25);
    float intensityFadeY = pow(1.0 - occluderDistPctY, 1.5);
    float intensity = intensityFadeX * intensityFadeY * u_intensity;

    // 7x7 box blur (only effective away from feet where blur > 0)
    float alpha = 0.0;
    const float boxWeight = 1.0 / 49.0;

    // Row offsets (matching Duelyst's unrolled loop structure)
    float xn3 = -3.0 * blurX;
    float xn2 = -2.0 * blurX;
    float xn1 = -1.0 * blurX;
    float x0  =  0.0;
    float xp1 =  1.0 * blurX;
    float xp2 =  2.0 * blurX;
    float xp3 =  3.0 * blurX;

    float yn3 = -3.0 * blurY;
    float yn2 = -2.0 * blurY;
    float yn1 = -1.0 * blurY;
    float y0  =  0.0;
    float yp1 =  1.0 * blurY;
    float yp2 =  2.0 * blurY;
    float yp3 =  3.0 * blurY;

    // Row -3
    alpha += sampleAlpha(v_texCoord + vec2(xn3, yn3)) * boxWeight;
    alpha += sampleAlpha(v_texCoord + vec2(xn2, yn3)) * boxWeight;
    alpha += sampleAlpha(v_texCoord + vec2(xn1, yn3)) * boxWeight;
    alpha += sampleAlpha(v_texCoord + vec2(x0,  yn3)) * boxWeight;
    alpha += sampleAlpha(v_texCoord + vec2(xp1, yn3)) * boxWeight;
    alpha += sampleAlpha(v_texCoord + vec2(xp2, yn3)) * boxWeight;
    alpha += sampleAlpha(v_texCoord + vec2(xp3, yn3)) * boxWeight;

    // Row -2
    alpha += sampleAlpha(v_texCoord + vec2(xn3, yn2)) * boxWeight;
    alpha += sampleAlpha(v_texCoord + vec2(xn2, yn2)) * boxWeight;
    alpha += sampleAlpha(v_texCoord + vec2(xn1, yn2)) * boxWeight;
    alpha += sampleAlpha(v_texCoord + vec2(x0,  yn2)) * boxWeight;
    alpha += sampleAlpha(v_texCoord + vec2(xp1, yn2)) * boxWeight;
    alpha += sampleAlpha(v_texCoord + vec2(xp2, yn2)) * boxWeight;
    alpha += sampleAlpha(v_texCoord + vec2(xp3, yn2)) * boxWeight;

    // Row -1
    alpha += sampleAlpha(v_texCoord + vec2(xn3, yn1)) * boxWeight;
    alpha += sampleAlpha(v_texCoord + vec2(xn2, yn1)) * boxWeight;
    alpha += sampleAlpha(v_texCoord + vec2(xn1, yn1)) * boxWeight;
    alpha += sampleAlpha(v_texCoord + vec2(x0,  yn1)) * boxWeight;
    alpha += sampleAlpha(v_texCoord + vec2(xp1, yn1)) * boxWeight;
    alpha += sampleAlpha(v_texCoord + vec2(xp2, yn1)) * boxWeight;
    alpha += sampleAlpha(v_texCoord + vec2(xp3, yn1)) * boxWeight;

    // Row 0 (center)
    alpha += sampleAlpha(v_texCoord + vec2(xn3, y0)) * boxWeight;
    alpha += sampleAlpha(v_texCoord + vec2(xn2, y0)) * boxWeight;
    alpha += sampleAlpha(v_texCoord + vec2(xn1, y0)) * boxWeight;
    alpha += sampleAlpha(v_texCoord + vec2(x0,  y0)) * boxWeight;
    alpha += sampleAlpha(v_texCoord + vec2(xp1, y0)) * boxWeight;
    alpha += sampleAlpha(v_texCoord + vec2(xp2, y0)) * boxWeight;
    alpha += sampleAlpha(v_texCoord + vec2(xp3, y0)) * boxWeight;

    // Row +1
    alpha += sampleAlpha(v_texCoord + vec2(xn3, yp1)) * boxWeight;
    alpha += sampleAlpha(v_texCoord + vec2(xn2, yp1)) * boxWeight;
    alpha += sampleAlpha(v_texCoord + vec2(xn1, yp1)) * boxWeight;
    alpha += sampleAlpha(v_texCoord + vec2(x0,  yp1)) * boxWeight;
    alpha += sampleAlpha(v_texCoord + vec2(xp1, yp1)) * boxWeight;
    alpha += sampleAlpha(v_texCoord + vec2(xp2, yp1)) * boxWeight;
    alpha += sampleAlpha(v_texCoord + vec2(xp3, yp1)) * boxWeight;

    // Row +2
    alpha += sampleAlpha(v_texCoord + vec2(xn3, yp2)) * boxWeight;
    alpha += sampleAlpha(v_texCoord + vec2(xn2, yp2)) * boxWeight;
    alpha += sampleAlpha(v_texCoord + vec2(xn1, yp2)) * boxWeight;
    alpha += sampleAlpha(v_texCoord + vec2(x0,  yp2)) * boxWeight;
    alpha += sampleAlpha(v_texCoord + vec2(xp1, yp2)) * boxWeight;
    alpha += sampleAlpha(v_texCoord + vec2(xp2, yp2)) * boxWeight;
    alpha += sampleAlpha(v_texCoord + vec2(xp3, yp2)) * boxWeight;

    // Row +3
    alpha += sampleAlpha(v_texCoord + vec2(xn3, yp3)) * boxWeight;
    alpha += sampleAlpha(v_texCoord + vec2(xn2, yp3)) * boxWeight;
    alpha += sampleAlpha(v_texCoord + vec2(xn1, yp3)) * boxWeight;
    alpha += sampleAlpha(v_texCoord + vec2(x0,  yp3)) * boxWeight;
    alpha += sampleAlpha(v_texCoord + vec2(xp1, yp3)) * boxWeight;
    alpha += sampleAlpha(v_texCoord + vec2(xp2, yp3)) * boxWeight;
    alpha += sampleAlpha(v_texCoord + vec2(xp3, yp3)) * boxWeight;

    // Final color - now using u_lightDistPctInv for proper light attenuation
    // Duelyst formula: gl_FragColor = vec4(0.0, 0.0, 0.0, min(1.0, lightDistPctInv * alpha * intensity * v_fragmentColor.a));
    fragColor = vec4(0.0, 0.0, 0.0, min(1.0, u_lightDistPctInv * alpha * intensity * u_opacity));
}
