#!/usr/bin/env node
/**
 * Extract localization strings from locale files
 * Outputs TSV format for duelyst_analysis
 */

const fs = require('fs');
const path = require('path');

const localesDir = path.join(__dirname, '../../app/localization/locales');
const outputPath = path.join(__dirname, '../instances/localization.tsv');

const strings = [];
const languages = [];

// Process English locale (primary)
const enDir = path.join(localesDir, 'en');
if (fs.existsSync(enDir)) {
  languages.push('en');
  const files = fs.readdirSync(enDir);

  files.forEach(file => {
    if (!file.endsWith('.json')) return;

    const filePath = path.join(enDir, file);
    const content = JSON.parse(fs.readFileSync(filePath, 'utf8'));
    const namespace = path.basename(file, '.json');

    // Flatten nested objects
    function extractKeys(obj, prefix = '') {
      Object.keys(obj).forEach(key => {
        const fullKey = prefix ? `${prefix}.${key}` : key;
        if (typeof obj[key] === 'object' && obj[key] !== null) {
          extractKeys(obj[key], fullKey);
        } else {
          strings.push({
            namespace,
            key: fullKey,
            value_en: String(obj[key]).replace(/\t/g, ' ').replace(/\n/g, ' '),
            has_params: String(obj[key]).includes('{{')
          });
        }
      });
    }

    extractKeys(content);
  });
}

// Check for German locale
const deDir = path.join(localesDir, 'de');
if (fs.existsSync(deDir)) {
  languages.push('de');
}

// Check for Chinese locale
const zhDir = path.join(localesDir, 'zh-tw');
if (fs.existsSync(zhDir)) {
  languages.push('zh-tw');
}

// Output as TSV
let output = 'namespace\tkey\tvalue_en\thas_params\n';
strings.forEach(s => {
  output += [s.namespace, s.key, s.value_en, s.has_params].join('\t') + '\n';
});

fs.writeFileSync(outputPath, output);
console.log(`Extracted ${strings.length} localization strings to ${outputPath}`);
console.log(`Languages found: ${languages.join(', ')}`);
