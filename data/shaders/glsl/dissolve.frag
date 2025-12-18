#version 450

layout(set = 2, binding = 0) uniform sampler2D u_texture;

layout(set = 3, binding = 0) uniform Uniforms {
    float u_opacity;
    float u_time;
    float u_seed;
    float u_padding;
};

layout(location = 0) in vec2 v_texCoord;
layout(location = 0) out vec4 fragColor;

// Duelyst dissolve parameters
const float u_frequency = 8.0;
const float u_amplitude = 1.0;
const float u_vignetteStrength = 1.0;
const float u_edgeFalloff = 0.15;

// Inlined from helpers/NoiseFBM.glsl
float getHash(in vec2 coord) {
    return fract(sin(dot(coord, vec2(234.1235, 123.752))) * 34702.0);
}

float getNoise(in vec2 coord) {
    vec2 coordWhole = floor(coord);
    vec2 coordFraction = fract(coord);
    vec2 smoothFraction = coordFraction * coordFraction * (3.0 - 2.0 * coordFraction);
    float bl = getHash(coordWhole);
    float br = getHash(vec2(coordWhole.x + 1.0, coordWhole.y));
    float tl = getHash(vec2(coordWhole.x, coordWhole.y + 1.0));
    float tr = getHash(vec2(coordWhole.x + 1.0, coordWhole.y + 1.0));
    float b = mix(bl, br, smoothFraction.x);
    float t = mix(tl, tr, smoothFraction.x);
    return mix(b, t, smoothFraction.y);
}

float getNoiseFBM(in vec2 coord, in float time, in float frequency, in float amplitude) {
    float fractalSum = 0.0;
    time = time * 3.141592653589793 * 2.0 + 3.141592653589793;
    for (int i = 0; i < 2; i++) {
        vec2 offset = vec2(cos(time + frequency), sin(time + frequency)) * (frequency * 0.1);
        float noise = getNoise(coord * frequency + offset);
        fractalSum += noise * amplitude;
        amplitude *= 0.5;
        frequency *= 2.0;
    }
    return fractalSum;
}

void main() {
    vec4 color = texture(u_texture, v_texCoord);
    
    // Vignette
    vec2 noiseCenterDiff = v_texCoord - vec2(0.5);
    float noiseCenterOffset = u_time + 0.25;
    noiseCenterOffset = (noiseCenterOffset * noiseCenterOffset * noiseCenterOffset * noiseCenterOffset) + 0.25;
    float noiseCenterDist = length(noiseCenterDiff) * 5.0 * noiseCenterOffset + noiseCenterOffset;
    noiseCenterDist = noiseCenterDist * noiseCenterDist * noiseCenterDist * noiseCenterDist * noiseCenterDist * noiseCenterDist;
    float noiseVignette = max(0.0, 1.0 - noiseCenterDist) * u_vignetteStrength;
    
    // Noise
    float noise = getNoiseFBM(vec2(u_seed) + v_texCoord, 0.0, u_frequency, max(0.0, u_amplitude + u_time - noiseVignette));
    float noiseRangeMin = 0.99 - u_time;
    float noiseRangeMax = 1.0 - u_time;
    float noiseSmooth = 1.0 - smoothstep(noiseRangeMin, noiseRangeMax, noise);
    float noiseEdge = fract(1.0 - smoothstep(noiseRangeMin, noiseRangeMax + u_edgeFalloff, noise));
    
    float alpha = color.a * (noiseSmooth + noiseEdge) * u_opacity;
    vec3 edgeGlow = vec3(smoothstep(0.1, 0.9, noiseEdge));
    fragColor = vec4(mix(color.rgb, vec3(1.0), edgeGlow), alpha);
}
