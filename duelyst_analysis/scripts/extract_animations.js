#!/usr/bin/env node
/**
 * Extract animation definitions from resources.js
 * Outputs TSV format for duelyst_analysis
 */

const fs = require('fs');
const path = require('path');

const resourcesPath = path.join(__dirname, '../../app/data/resources.js');
const outputPath = path.join(__dirname, '../instances/animations.tsv');

const content = fs.readFileSync(resourcesPath, 'utf8');

// Parse entries using regex to find animation-like entries (those with framePrefix)
const animations = [];
const animRegex = /(\w+):\s*\{\s*name:\s*['"]([^'"]+)['"],\s*framePrefix:\s*['"]([^'"]+)['"],\s*frameDelay:\s*([\d.]+)/g;

let match;
while ((match = animRegex.exec(content)) !== null) {
  const [_, key, name, framePrefix, frameDelay] = match;

  // Extract img and plist if present
  const entryStart = match.index;
  const entryEnd = content.indexOf('},', entryStart);
  const entryText = content.substring(entryStart, entryEnd + 1);

  const imgMatch = entryText.match(/img:\s*['"]([^'"]+)['"]/);
  const plistMatch = entryText.match(/plist:\s*['"]([^'"]+)['"]/);
  const is16BitMatch = entryText.match(/is16Bit:\s*(true|false)/);

  animations.push({
    key,
    name,
    framePrefix,
    frameDelay: parseFloat(frameDelay),
    img: imgMatch ? imgMatch[1] : '',
    plist: plistMatch ? plistMatch[1] : '',
    is16Bit: is16BitMatch ? is16BitMatch[1] : 'false'
  });
}

// Output as TSV
let output = 'key\tname\tframe_prefix\tframe_delay\timg\tplist\tis_16bit\n';
animations.forEach(a => {
  output += [a.key, a.name, a.framePrefix, a.frameDelay, a.img, a.plist, a.is16Bit].join('\t') + '\n';
});

fs.writeFileSync(outputPath, output);
console.log(`Extracted ${animations.length} animations to ${outputPath}`);
