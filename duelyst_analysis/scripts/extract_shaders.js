#!/usr/bin/env node
/**
 * Extract shader definitions from app/shaders/
 * Outputs TSV format for duelyst_analysis
 */

const fs = require('fs');
const path = require('path');

const shadersDir = path.join(__dirname, '../../app/shaders');
const outputPath = path.join(__dirname, '../instances/shaders.tsv');

const shaders = [];

// Read all shader files
const files = fs.readdirSync(shadersDir);

files.forEach(file => {
  const filePath = path.join(shadersDir, file);
  const stat = fs.statSync(filePath);

  if (stat.isDirectory()) {
    // Check subdirectories (like helpers/)
    const subFiles = fs.readdirSync(filePath);
    subFiles.forEach(subFile => {
      processShaderFile(path.join(filePath, subFile), `${file}/${subFile}`);
    });
  } else {
    processShaderFile(filePath, file);
  }
});

function processShaderFile(filePath, relativePath) {
  const ext = path.extname(filePath).toLowerCase();
  if (ext !== '.glsl' && ext !== '.vert' && ext !== '.frag') return;

  const content = fs.readFileSync(filePath, 'utf8');
  const name = path.basename(filePath, ext);

  // Determine shader type
  let shaderType = 'unknown';
  if (ext === '.vert' || name.toLowerCase().includes('vertex')) {
    shaderType = 'vertex';
  } else if (ext === '.frag' || name.toLowerCase().includes('fragment')) {
    shaderType = 'fragment';
  } else if (name.toLowerCase().includes('noise') || name.toLowerCase().includes('helper')) {
    shaderType = 'helper';
  } else {
    shaderType = 'fragment'; // Default for .glsl
  }

  // Extract uniforms
  const uniforms = [];
  const uniformRegex = /uniform\s+(\w+)\s+(\w+)/g;
  let match;
  while ((match = uniformRegex.exec(content)) !== null) {
    uniforms.push(`${match[2]}:${match[1]}`);
  }

  // Extract attributes
  const attributes = [];
  const attrRegex = /attribute\s+(\w+)\s+(\w+)/g;
  while ((match = attrRegex.exec(content)) !== null) {
    attributes.push(`${match[2]}:${match[1]}`);
  }

  // Extract defines
  const defines = [];
  const defineRegex = /#define\s+(\w+)\s+(.+)/g;
  while ((match = defineRegex.exec(content)) !== null) {
    defines.push(`${match[1]}=${match[2].trim()}`);
  }

  shaders.push({
    name,
    file: relativePath,
    type: shaderType,
    uniforms: uniforms.join(';') || '',
    attributes: attributes.join(';') || '',
    defines: defines.join(';') || '',
    lines: content.split('\n').length
  });
}

// Output as TSV
let output = 'name\tfile\ttype\tuniforms\tattributes\tdefines\tlines\n';
shaders.forEach(s => {
  output += [s.name, s.file, s.type, s.uniforms, s.attributes, s.defines, s.lines].join('\t') + '\n';
});

fs.writeFileSync(outputPath, output);
console.log(`Extracted ${shaders.length} shaders to ${outputPath}`);
