#!/usr/bin/env node
/**
 * Extract AI agent actions from challenge files
 * Outputs TSV format for duelyst_analysis
 */

const fs = require('fs');
const path = require('path');

const challengesDir = path.join(__dirname, '../../app/sdk/challenges');
const agentsDir = path.join(__dirname, '../../app/sdk/agents');
const outputPath = path.join(__dirname, '../instances/agent_actions.tsv');

const actions = [];

// Extract agent action types from agentActions.coffee
const agentActionsPath = path.join(agentsDir, 'agentActions.coffee');
if (fs.existsSync(agentActionsPath)) {
  const content = fs.readFileSync(agentActionsPath, 'utf8');

  // Find all createAgent* methods
  const methodRegex = /(@\w+):\s*\(([^)]*)\)\s*->/g;
  let match;
  while ((match = methodRegex.exec(content)) !== null) {
    const methodName = match[1].replace('@', '');
    const params = match[2].split(',').map(p => p.trim()).filter(p => p);

    actions.push({
      type: 'action_type',
      name: methodName,
      params: params.join(';'),
      source: 'agentActions.coffee',
      turn: '',
      description: ''
    });
  }
}

// Extract agent action usages from challenge files
const files = fs.readdirSync(challengesDir);
files.forEach(file => {
  if (!file.endsWith('.coffee')) return;

  const filePath = path.join(challengesDir, file);
  const content = fs.readFileSync(filePath, 'utf8');
  const challengeName = path.basename(file, '.coffee');

  // Find addActionForTurn calls
  const actionRegex = /addActionForTurn\s*\(\s*(\d+)\s*,\s*AgentActions\.(\w+)\s*\(([^)]*)\)/g;
  let match;
  while ((match = actionRegex.exec(content)) !== null) {
    const turn = match[1];
    const actionType = match[2];
    const params = match[3].trim();

    actions.push({
      type: 'action_usage',
      name: actionType,
      params: params.replace(/\n/g, ' ').replace(/\t/g, ' ').substring(0, 100),
      source: challengeName,
      turn: turn,
      description: ''
    });
  }
});

// Output as TSV
let output = 'type\tname\tparams\tsource\tturn\tdescription\n';
actions.forEach(a => {
  output += [a.type, a.name, a.params, a.source, a.turn, a.description].join('\t') + '\n';
});

fs.writeFileSync(outputPath, output);
console.log(`Extracted ${actions.length} agent actions to ${outputPath}`);
