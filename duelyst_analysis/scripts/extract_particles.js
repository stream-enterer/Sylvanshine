#!/usr/bin/env node
/**
 * Extract particle system configurations from plist files
 * Outputs TSV format for duelyst_analysis
 */

const fs = require('fs');
const path = require('path');

const particlesDir = path.join(__dirname, '../../app/resources/particles');
const outputPath = path.join(__dirname, '../instances/particles.tsv');

const particles = [];

// Read all plist files
const files = fs.readdirSync(particlesDir);

files.forEach(file => {
  if (!file.endsWith('.plist')) return;

  const filePath = path.join(particlesDir, file);
  const content = fs.readFileSync(filePath, 'utf8');
  const name = path.basename(file, '.plist');

  // Parse key-value pairs from plist XML
  const data = {};

  // Simple XML parsing for plist format
  const keyValueRegex = /<key>([^<]+)<\/key>\s*<(?:real|integer|string)>([^<]*)<\/(?:real|integer|string)>/g;
  let match;
  while ((match = keyValueRegex.exec(content)) !== null) {
    data[match[1]] = match[2];
  }

  particles.push({
    name,
    file,
    maxParticles: data.maxParticles || '',
    particleLifespan: data.particleLifespan || '',
    particleLifespanVariance: data.particleLifespanVariance || '',
    startParticleSize: data.startParticleSize || '',
    finishParticleSize: data.finishParticleSize || '',
    speed: data.speed || '',
    speedVariance: data.speedVariance || '',
    angle: data.angle || '',
    angleVariance: data.angleVariance || '',
    emitterType: data.emitterType || '',
    duration: data.duration || '',
    gravityX: data.gravityx || '',
    gravityY: data.gravityy || '',
    radialAcceleration: data.radialAcceleration || '',
    tangentialAcceleration: data.tangentialAcceleration || '',
    startColorRed: data.startColorRed || '',
    startColorGreen: data.startColorGreen || '',
    startColorBlue: data.startColorBlue || '',
    startColorAlpha: data.startColorAlpha || '',
    finishColorAlpha: data.finishColorAlpha || '',
    blendFuncSource: data.blendFuncSource || '',
    blendFuncDestination: data.blendFuncDestination || '',
    textureFileName: data.textureFileName || ''
  });
});

// Output as TSV
const headers = [
  'name', 'file', 'max_particles', 'lifespan', 'lifespan_variance',
  'start_size', 'finish_size', 'speed', 'speed_variance',
  'angle', 'angle_variance', 'emitter_type', 'duration',
  'gravity_x', 'gravity_y', 'radial_accel', 'tangent_accel',
  'start_r', 'start_g', 'start_b', 'start_a', 'finish_a',
  'blend_src', 'blend_dst', 'texture'
];

let output = headers.join('\t') + '\n';
particles.forEach(p => {
  output += [
    p.name, p.file, p.maxParticles, p.particleLifespan, p.particleLifespanVariance,
    p.startParticleSize, p.finishParticleSize, p.speed, p.speedVariance,
    p.angle, p.angleVariance, p.emitterType, p.duration,
    p.gravityX, p.gravityY, p.radialAcceleration, p.tangentialAcceleration,
    p.startColorRed, p.startColorGreen, p.startColorBlue, p.startColorAlpha, p.finishColorAlpha,
    p.blendFuncSource, p.blendFuncDestination, p.textureFileName
  ].join('\t') + '\n';
});

fs.writeFileSync(outputPath, output);
console.log(`Extracted ${particles.length} particle systems to ${outputPath}`);
