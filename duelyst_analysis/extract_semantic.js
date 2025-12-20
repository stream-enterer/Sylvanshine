#!/usr/bin/env node
const fs = require('fs');
const path = require('path');

const APP_DIR = path.join(__dirname, '..', 'app');
const OUTPUT_DIR = path.join(__dirname, 'semantic');

// Ensure output directory exists
if (!fs.existsSync(OUTPUT_DIR)) {
  fs.mkdirSync(OUTPUT_DIR, { recursive: true });
}

// Data structures
const classes = [];
const functions = [];
const constants = [];
const events = [];
const dataShapes = [];
const inheritance = [];

// Track event definitions and usages
const eventDefinitions = {};
const eventEmitters = {};
const eventListeners = {};

// Helper to get relative path from app/
function getRelPath(filePath) {
  return path.relative(path.join(__dirname, '..'), filePath);
}

// Escape TSV field
function escapeTsv(value) {
  if (value === null || value === undefined) return '';
  const str = String(value);
  if (str.includes('\t') || str.includes('\n') || str.includes('\r')) {
    return str.replace(/[\t\n\r]/g, ' ');
  }
  return str;
}

// Parse CoffeeScript file
function parseCoffeeScript(filePath, content) {
  const relPath = getRelPath(filePath);
  const lines = content.split('\n');

  // Track current class for methods
  let currentClass = null;
  let currentClassIndent = 0;
  let classMethods = {};

  lines.forEach((line, idx) => {
    const lineNum = idx + 1;
    const trimmed = line.trim();
    const indent = line.search(/\S/);

    // Class definition: class X extends Y or class X
    const classMatch = trimmed.match(/^class\s+(\w+)(?:\s+extends\s+(\w+))?/);
    if (classMatch) {
      const className = classMatch[1];
      const parentClass = classMatch[2] || '';
      currentClass = className;
      currentClassIndent = indent >= 0 ? indent : 0;
      classMethods[className] = [];

      classes.push({
        class_name: className,
        parent_class: parentClass,
        file: relPath,
        methods: '',
        line: lineNum
      });

      if (parentClass) {
        inheritance.push({
          child_class: className,
          parent_class: parentClass,
          file: relPath
        });
      }
      return;
    }

    // Exit class scope
    if (currentClass && indent >= 0 && indent <= currentClassIndent && trimmed && !trimmed.startsWith('#')) {
      // Update methods for previous class
      const classEntry = classes.find(c => c.class_name === currentClass && c.file === relPath);
      if (classEntry && classMethods[currentClass]) {
        classEntry.methods = classMethods[currentClass].join(';');
      }

      if (!classMatch) {
        currentClass = null;
      }
    }

    // Class static property (data shape): @property: value
    if (currentClass && trimmed.match(/^@\w+:\s*/)) {
      const propMatch = trimmed.match(/^@(\w+):\s*(.*)$/);
      if (propMatch) {
        const field = propMatch[1];
        let type = 'unknown';
        const value = propMatch[2].trim();

        if (value === 'null' || value === 'undefined') type = 'null';
        else if (value === 'true' || value === 'false') type = 'boolean';
        else if (/^-?\d+$/.test(value)) type = 'number';
        else if (/^-?\d*\.\d+$/.test(value)) type = 'number';
        else if (/^['"]/.test(value)) type = 'string';
        else if (/^\[/.test(value)) type = 'array';
        else if (/^\{/.test(value)) type = 'object';
        else if (/^->|^=>/.test(value)) type = 'function';

        dataShapes.push({
          class_name: currentClass,
          field: field,
          type: type
        });
      }
    }

    // Instance property: property: value (inside class)
    if (currentClass && indent > currentClassIndent && trimmed.match(/^\w+:\s*/) && !trimmed.match(/^@/)) {
      const propMatch = trimmed.match(/^(\w+):\s*(.*)$/);
      if (propMatch && !propMatch[2].match(/^\(|^->|^=>/)) {
        const field = propMatch[1];
        let type = 'unknown';
        const value = propMatch[2].trim();

        if (value === 'null' || value === 'undefined') type = 'null';
        else if (value === 'true' || value === 'false') type = 'boolean';
        else if (/^-?\d+$/.test(value)) type = 'number';
        else if (/^-?\d*\.\d+$/.test(value)) type = 'number';
        else if (/^['"]/.test(value)) type = 'string';
        else if (/^\[/.test(value)) type = 'array';
        else if (/^\{/.test(value)) type = 'object';

        if (type !== 'unknown') {
          dataShapes.push({
            class_name: currentClass,
            field: field,
            type: type
          });
        }
      }
    }

    // Method definitions in class
    if (currentClass && indent > currentClassIndent) {
      // Static method: @methodName: (params) ->
      const staticMethodMatch = trimmed.match(/^@(\w+):\s*\(([^)]*)\)\s*[-=]>/);
      if (staticMethodMatch) {
        const methodName = staticMethodMatch[1];
        const params = staticMethodMatch[2];
        classMethods[currentClass].push(methodName);
        functions.push({
          function_name: `${currentClass}.${methodName}`,
          file: relPath,
          parameters: params,
          line_number: lineNum
        });
        return;
      }

      // Instance method: methodName: (params) ->
      const instanceMethodMatch = trimmed.match(/^(\w+):\s*\(([^)]*)\)\s*[-=]>/);
      if (instanceMethodMatch) {
        const methodName = instanceMethodMatch[1];
        const params = instanceMethodMatch[2];
        classMethods[currentClass].push(methodName);
        functions.push({
          function_name: `${currentClass}#${methodName}`,
          file: relPath,
          parameters: params,
          line_number: lineNum
        });
        return;
      }

      // Short method: methodName: ->
      const shortMethodMatch = trimmed.match(/^@?(\w+):\s*[-=]>$/);
      if (shortMethodMatch) {
        const methodName = shortMethodMatch[1];
        const isStatic = trimmed.startsWith('@');
        classMethods[currentClass].push(methodName);
        functions.push({
          function_name: isStatic ? `${currentClass}.${methodName}` : `${currentClass}#${methodName}`,
          file: relPath,
          parameters: '',
          line_number: lineNum
        });
        return;
      }
    }

    // Standalone function: functionName = (params) ->
    if (!currentClass) {
      const funcMatch = trimmed.match(/^(\w+)\s*=\s*\(([^)]*)\)\s*[-=]>/);
      if (funcMatch) {
        functions.push({
          function_name: funcMatch[1],
          file: relPath,
          parameters: funcMatch[2],
          line_number: lineNum
        });
      }
    }

    // Event emit patterns
    const emitMatch = line.match(/\.emit\s*\(\s*['"]([^'"]+)['"]/);
    if (emitMatch) {
      const eventName = emitMatch[1];
      if (!eventEmitters[eventName]) eventEmitters[eventName] = new Set();
      eventEmitters[eventName].add(relPath);
    }

    const triggerMatch = line.match(/\.trigger\s*\(\s*['"]([^'"]+)['"]/);
    if (triggerMatch) {
      const eventName = triggerMatch[1];
      if (!eventEmitters[eventName]) eventEmitters[eventName] = new Set();
      eventEmitters[eventName].add(relPath);
    }

    // Event listener patterns
    const onMatch = line.match(/\.on\s*\(\s*['"]([^'"]+)['"]/);
    if (onMatch) {
      const eventName = onMatch[1];
      if (!eventListeners[eventName]) eventListeners[eventName] = new Set();
      eventListeners[eventName].add(relPath);
    }

    const listenToMatch = line.match(/\.listenTo\s*\([^,]+,\s*['"]([^'"]+)['"]/);
    if (listenToMatch) {
      const eventName = listenToMatch[1];
      if (!eventListeners[eventName]) eventListeners[eventName] = new Set();
      eventListeners[eventName].add(relPath);
    }

    // EVENTS.* usage
    const eventsUsage = line.match(/EVENTS\.(\w+)/g);
    if (eventsUsage) {
      eventsUsage.forEach(match => {
        const eventName = match.replace('EVENTS.', '');
        if (line.includes('.emit') || line.includes('.trigger')) {
          if (!eventEmitters[eventName]) eventEmitters[eventName] = new Set();
          eventEmitters[eventName].add(relPath);
        }
        if (line.includes('.on(') || line.includes('.listenTo')) {
          if (!eventListeners[eventName]) eventListeners[eventName] = new Set();
          eventListeners[eventName].add(relPath);
        }
      });
    }
  });

  // Finalize last class
  if (currentClass) {
    const classEntry = classes.find(c => c.class_name === currentClass && c.file === relPath);
    if (classEntry && classMethods[currentClass]) {
      classEntry.methods = classMethods[currentClass].join(';');
    }
  }
}

// Parse JavaScript file
function parseJavaScript(filePath, content) {
  const relPath = getRelPath(filePath);
  const lines = content.split('\n');

  let currentClass = null;
  let braceDepth = 0;
  let classStartBrace = 0;
  let classMethods = {};

  // First pass: find CONFIG, RSX, EVENTS objects
  const configMatch = content.match(/const\s+CONFIG\s*=\s*\{\s*\}/);
  const rsxMatch = content.match(/const\s+RSX\s*=\s*\{/);
  const eventsMatch = content.match(/const\s+EVENTS\s*=\s*\{/);

  lines.forEach((line, idx) => {
    const lineNum = idx + 1;
    const trimmed = line.trim();

    // Track brace depth
    const openBraces = (line.match(/\{/g) || []).length;
    const closeBraces = (line.match(/\}/g) || []).length;
    braceDepth += openBraces - closeBraces;

    // Class definition
    const classMatch = trimmed.match(/^class\s+(\w+)(?:\s+extends\s+(\w+))?/);
    if (classMatch) {
      const className = classMatch[1];
      const parentClass = classMatch[2] || '';
      currentClass = className;
      classStartBrace = braceDepth;
      classMethods[className] = [];

      classes.push({
        class_name: className,
        parent_class: parentClass,
        file: relPath,
        methods: '',
        line: lineNum
      });

      if (parentClass) {
        inheritance.push({
          child_class: className,
          parent_class: parentClass,
          file: relPath
        });
      }
      return;
    }

    // Exit class
    if (currentClass && braceDepth < classStartBrace) {
      const classEntry = classes.find(c => c.class_name === currentClass && c.file === relPath);
      if (classEntry && classMethods[currentClass]) {
        classEntry.methods = classMethods[currentClass].join(';');
      }
      currentClass = null;
    }

    // CONFIG.PROPERTY = value
    const configPropMatch = trimmed.match(/^CONFIG\.([A-Z_][A-Z0-9_]*)\s*=\s*(.+?);?\s*$/);
    if (configPropMatch) {
      let value = configPropMatch[2];
      if (value.length > 100) value = value.substring(0, 100) + '...';
      constants.push({
        constant_name: `CONFIG.${configPropMatch[1]}`,
        value: value.replace(/;$/, ''),
        file: relPath
      });
    }

    // RSX property: key: { name: value, ... }
    if (relPath.includes('resources.js')) {
      const rsxMatch = trimmed.match(/^(\w+):\s*\{\s*name:\s*['"]([^'"]+)['"]/);
      if (rsxMatch) {
        constants.push({
          constant_name: `RSX.${rsxMatch[1]}`,
          value: rsxMatch[2],
          file: relPath
        });
      }
    }

    // EVENTS property: key: 'value'
    if (relPath.includes('event_types.js')) {
      const evMatch = trimmed.match(/^(\w+):\s*['"]([^'"]+)['"]/);
      if (evMatch) {
        eventDefinitions[evMatch[1]] = evMatch[2];
        constants.push({
          constant_name: `EVENTS.${evMatch[1]}`,
          value: evMatch[2],
          file: relPath
        });
      }
    }

    // const CONSTANT = value
    const constMatch = trimmed.match(/^const\s+([A-Z_][A-Z0-9_]*)\s*=\s*(.+?);?\s*$/);
    if (constMatch && !constMatch[1].match(/^(CONFIG|RSX|EVENTS|_)$/)) {
      let value = constMatch[2];
      if (value.length > 100) value = value.substring(0, 100) + '...';
      constants.push({
        constant_name: constMatch[1],
        value: value.replace(/;$/, ''),
        file: relPath
      });
    }

    // Method in class
    if (currentClass) {
      // methodName(params) {
      const methodMatch = trimmed.match(/^(\w+)\s*\(([^)]*)\)\s*\{/);
      if (methodMatch && !methodMatch[1].match(/^(if|for|while|switch|catch|function)$/)) {
        classMethods[currentClass].push(methodMatch[1]);
        functions.push({
          function_name: `${currentClass}#${methodMatch[1]}`,
          file: relPath,
          parameters: methodMatch[2],
          line_number: lineNum
        });
        return;
      }

      // static methodName(params) {
      const staticMatch = trimmed.match(/^static\s+(\w+)\s*\(([^)]*)\)\s*\{/);
      if (staticMatch) {
        classMethods[currentClass].push(staticMatch[1]);
        functions.push({
          function_name: `${currentClass}.${staticMatch[1]}`,
          file: relPath,
          parameters: staticMatch[2],
          line_number: lineNum
        });
        return;
      }

      // get/set propertyName() {
      const getSetMatch = trimmed.match(/^(get|set)\s+(\w+)\s*\(([^)]*)\)\s*\{/);
      if (getSetMatch) {
        classMethods[currentClass].push(`${getSetMatch[1]} ${getSetMatch[2]}`);
        return;
      }
    }

    // Standalone function
    if (!currentClass) {
      // function name(params) {
      const funcMatch = trimmed.match(/^function\s+(\w+)\s*\(([^)]*)\)/);
      if (funcMatch) {
        functions.push({
          function_name: funcMatch[1],
          file: relPath,
          parameters: funcMatch[2],
          line_number: lineNum
        });
        return;
      }

      // const name = function(params) or const name = (params) =>
      const constFuncMatch = trimmed.match(/^(?:const|let|var)\s+(\w+)\s*=\s*(?:function\s*)?\(([^)]*)\)\s*(?:=>|{)/);
      if (constFuncMatch) {
        functions.push({
          function_name: constFuncMatch[1],
          file: relPath,
          parameters: constFuncMatch[2],
          line_number: lineNum
        });
        return;
      }

      // module.exports.name = function
      const exportFuncMatch = trimmed.match(/^(?:module\.)?exports\.(\w+)\s*=\s*(?:function\s*)?\(([^)]*)\)/);
      if (exportFuncMatch) {
        functions.push({
          function_name: exportFuncMatch[1],
          file: relPath,
          parameters: exportFuncMatch[2],
          line_number: lineNum
        });
      }
    }

    // Event emit patterns
    const emitMatch = line.match(/\.emit\s*\(\s*['"]([^'"]+)['"]/);
    if (emitMatch) {
      const eventName = emitMatch[1];
      if (!eventEmitters[eventName]) eventEmitters[eventName] = new Set();
      eventEmitters[eventName].add(relPath);
    }

    const triggerMatch = line.match(/\.trigger\s*\(\s*['"]([^'"]+)['"]/);
    if (triggerMatch) {
      const eventName = triggerMatch[1];
      if (!eventEmitters[eventName]) eventEmitters[eventName] = new Set();
      eventEmitters[eventName].add(relPath);
    }

    // Event listener patterns
    const onMatch = line.match(/\.on\s*\(\s*['"]([^'"]+)['"]/);
    if (onMatch) {
      const eventName = onMatch[1];
      if (!eventListeners[eventName]) eventListeners[eventName] = new Set();
      eventListeners[eventName].add(relPath);
    }

    const listenToMatch = line.match(/\.listenTo\s*\([^,]+,\s*['"]([^'"]+)['"]/);
    if (listenToMatch) {
      const eventName = listenToMatch[1];
      if (!eventListeners[eventName]) eventListeners[eventName] = new Set();
      eventListeners[eventName].add(relPath);
    }

    // EVENTS.* usage
    const eventsUsage = line.match(/EVENTS\.(\w+)/g);
    if (eventsUsage) {
      eventsUsage.forEach(match => {
        const eventName = match.replace('EVENTS.', '');
        if (line.includes('.emit') || line.includes('.trigger')) {
          if (!eventEmitters[eventName]) eventEmitters[eventName] = new Set();
          eventEmitters[eventName].add(relPath);
        }
        if (line.includes('.on(') || line.includes('.listenTo') || line.includes('addEventListener')) {
          if (!eventListeners[eventName]) eventListeners[eventName] = new Set();
          eventListeners[eventName].add(relPath);
        }
      });
    }
  });
}

// Recursively find all files
function findFiles(dir, exts, files = []) {
  const entries = fs.readdirSync(dir, { withFileTypes: true });
  for (const entry of entries) {
    const fullPath = path.join(dir, entry.name);
    if (entry.isDirectory()) {
      findFiles(fullPath, exts, files);
    } else if (exts.some(ext => entry.name.endsWith(ext))) {
      files.push(fullPath);
    }
  }
  return files;
}

// Main processing
console.log('Finding files...');
const files = findFiles(APP_DIR, ['.js', '.coffee']);
console.log(`Found ${files.length} files`);

console.log('Parsing files...');
let processed = 0;
for (const filePath of files) {
  try {
    const content = fs.readFileSync(filePath, 'utf8');
    if (filePath.endsWith('.coffee')) {
      parseCoffeeScript(filePath, content);
    } else {
      parseJavaScript(filePath, content);
    }
    processed++;
    if (processed % 200 === 0) {
      console.log(`  Processed ${processed}/${files.length}`);
    }
  } catch (err) {
    console.error(`Error parsing ${filePath}: ${err.message}`);
  }
}

// Build events data
console.log('Building events data...');
const allEventNames = new Set([
  ...Object.keys(eventDefinitions),
  ...Object.keys(eventEmitters),
  ...Object.keys(eventListeners)
]);

for (const eventName of allEventNames) {
  const emitterFiles = eventEmitters[eventName] ? Array.from(eventEmitters[eventName]).join(';') : '';
  const listenerFiles = eventListeners[eventName] ? Array.from(eventListeners[eventName]).join(';') : '';
  events.push({
    event_name: eventName,
    emitter_file: emitterFiles,
    listener_files: listenerFiles
  });
}

// Write TSV files
console.log('Writing output files...');

// Classes TSV
const classesTsv = 'class_name\tparent_class\tfile\tmethods\n' +
  classes.map(c => `${escapeTsv(c.class_name)}\t${escapeTsv(c.parent_class)}\t${escapeTsv(c.file)}\t${escapeTsv(c.methods)}`).join('\n');
fs.writeFileSync(path.join(OUTPUT_DIR, 'classes.tsv'), classesTsv);
console.log(`  classes.tsv: ${classes.length} entries`);

// Functions TSV
const functionsTsv = 'function_name\tfile\tparameters\tline_number\n' +
  functions.map(f => `${escapeTsv(f.function_name)}\t${escapeTsv(f.file)}\t${escapeTsv(f.parameters)}\t${f.line_number}`).join('\n');
fs.writeFileSync(path.join(OUTPUT_DIR, 'functions.tsv'), functionsTsv);
console.log(`  functions.tsv: ${functions.length} entries`);

// Constants TSV (prioritize CONFIG, RSX, EVENTS)
const sortedConstants = constants.sort((a, b) => {
  const aScore = a.constant_name.startsWith('CONFIG.') ? 0 :
                 a.constant_name.startsWith('RSX.') ? 1 :
                 a.constant_name.startsWith('EVENTS.') ? 2 : 3;
  const bScore = b.constant_name.startsWith('CONFIG.') ? 0 :
                 b.constant_name.startsWith('RSX.') ? 1 :
                 b.constant_name.startsWith('EVENTS.') ? 2 : 3;
  if (aScore !== bScore) return aScore - bScore;
  return a.constant_name.localeCompare(b.constant_name);
});
const constantsTsv = 'constant_name\tvalue\tfile\n' +
  sortedConstants.map(c => `${escapeTsv(c.constant_name)}\t${escapeTsv(c.value)}\t${escapeTsv(c.file)}`).join('\n');
fs.writeFileSync(path.join(OUTPUT_DIR, 'constants.tsv'), constantsTsv);
console.log(`  constants.tsv: ${constants.length} entries`);

// Events TSV
const eventsTsv = 'event_name\temitter_file\tlistener_files\n' +
  events.map(e => `${escapeTsv(e.event_name)}\t${escapeTsv(e.emitter_file)}\t${escapeTsv(e.listener_files)}`).join('\n');
fs.writeFileSync(path.join(OUTPUT_DIR, 'events.tsv'), eventsTsv);
console.log(`  events.tsv: ${events.length} entries`);

// Data shapes TSV
const dataShapesTsv = 'class_name\tfield\ttype\n' +
  dataShapes.map(d => `${escapeTsv(d.class_name)}\t${escapeTsv(d.field)}\t${escapeTsv(d.type)}`).join('\n');
fs.writeFileSync(path.join(OUTPUT_DIR, 'data_shapes.tsv'), dataShapesTsv);
console.log(`  data_shapes.tsv: ${dataShapes.length} entries`);

// Inheritance TSV
const inheritanceTsv = 'child_class\tparent_class\tfile\n' +
  inheritance.map(i => `${escapeTsv(i.child_class)}\t${escapeTsv(i.parent_class)}\t${escapeTsv(i.file)}`).join('\n');
fs.writeFileSync(path.join(OUTPUT_DIR, 'inheritance.tsv'), inheritanceTsv);
console.log(`  inheritance.tsv: ${inheritance.length} entries`);

console.log('Done!');
