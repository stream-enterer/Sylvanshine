#!/usr/bin/env node
/**
 * Extract replay/serialization format from codebase
 * Outputs TSV format for duelyst_analysis
 */

const fs = require('fs');
const path = require('path');

const sdkDir = path.join(__dirname, '../../app/sdk');
const outputPath = path.join(__dirname, '../instances/serialization_fields.tsv');

const fields = [];

// Files to analyze for serialization
const filesToAnalyze = [
  'gameSession.coffee',
  'gameTurn.coffee',
  'step.coffee',
  'actions/action.coffee',
  'player.coffee',
  'cards/card.coffee',
  'entities/entity.coffee',
  'modifiers/modifier.coffee'
];

filesToAnalyze.forEach(file => {
  const filePath = path.join(sdkDir, file);
  if (!fs.existsSync(filePath)) return;

  const content = fs.readFileSync(filePath, 'utf8');
  const className = path.basename(file, '.coffee');

  // Extract serializable properties from deserialize methods
  const deserializeMatch = content.match(/deserialize\w*:\s*\([^)]*\)\s*->([\s\S]*?)(?=\n\s{2}\w+:|$)/);
  if (deserializeMatch) {
    const body = deserializeMatch[1];

    // Find property assignments from data
    const propRegex = /@(\w+)\s*=\s*(?:data\.|json\.)?(\w+)/g;
    let match;
    while ((match = propRegex.exec(body)) !== null) {
      fields.push({
        class: className,
        property: match[1],
        source: match[2],
        direction: 'deserialize',
        file
      });
    }
  }

  // Extract properties from serialize methods
  const serializeMatch = content.match(/serializeToJSON:\s*\([^)]*\)\s*->([\s\S]*?)(?=\n\s{2}\w+:|$)/);
  if (serializeMatch) {
    const body = serializeMatch[1];

    // Find property exports
    const propRegex = /(\w+):\s*@(\w+)/g;
    let match;
    while ((match = propRegex.exec(body)) !== null) {
      fields.push({
        class: className,
        property: match[2],
        source: match[1],
        direction: 'serialize',
        file
      });
    }
  }

  // Also look for getters that return serialized data
  const getterRegex = /get(\w+):\s*\(\)\s*->\s*(?:return\s+)?@(\w+)/g;
  let match;
  while ((match = getterRegex.exec(content)) !== null) {
    // Skip duplicates
    const existing = fields.find(f => f.class === className && f.property === match[2]);
    if (!existing) {
      fields.push({
        class: className,
        property: match[2],
        source: match[1],
        direction: 'getter',
        file
      });
    }
  }
});

// Output as TSV
let output = 'class\tproperty\tserialized_name\tdirection\tfile\n';
fields.forEach(f => {
  output += [f.class, f.property, f.source, f.direction, f.file].join('\t') + '\n';
});

fs.writeFileSync(outputPath, output);
console.log(`Extracted ${fields.length} serialization fields to ${outputPath}`);
