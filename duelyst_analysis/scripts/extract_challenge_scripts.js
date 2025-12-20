#!/usr/bin/env node
/**
 * Extract challenge/tutorial scripts with step sequences
 * Outputs TSV format for duelyst_analysis
 */

const fs = require('fs');
const path = require('path');

const challengesDir = path.join(__dirname, '../../app/sdk/challenges');
const outputPath = path.join(__dirname, '../instances/challenge_scripts.tsv');

const scripts = [];

// Read all challenge files
const challengeDirs = ['tutorial', 'beginner', 'medium', 'advanced', 'expert', 'gate', ''];

challengeDirs.forEach(subdir => {
  const dir = subdir ? path.join(challengesDir, subdir) : challengesDir;
  if (!fs.existsSync(dir)) return;

  const files = fs.readdirSync(dir);
  files.forEach(file => {
    if (!file.endsWith('.coffee')) return;

    const filePath = path.join(dir, file);
    const content = fs.readFileSync(filePath, 'utf8');
    const challengeName = path.basename(file, '.coffee');

    // Extract class name
    const classMatch = content.match(/class\s+(\w+)\s+extends/);
    const className = classMatch ? classMatch[1] : challengeName;

    // Extract type from path
    const type = subdir || 'base';

    // Count agent actions per turn
    const turnActions = {};
    const actionRegex = /addActionForTurn\s*\(\s*(\d+)/g;
    let match;
    while ((match = actionRegex.exec(content)) !== null) {
      const turn = match[1];
      turnActions[turn] = (turnActions[turn] || 0) + 1;
    }

    // Extract instruction labels
    const instructions = [];
    const instructionRegex = /label:\s*["']([^"']+)["']/g;
    while ((match = instructionRegex.exec(content)) !== null) {
      instructions.push(match[1].substring(0, 50));
    }

    // Extract setupBoard calls
    const hasBoardSetup = content.includes('setupBoard');
    const hasOpponentAgent = content.includes('setupOpponentAgent');

    // Count cards in setupBoard
    const playCardCount = (content.match(/playCardByIdToBoard/g) || []).length;
    const applyCardCount = (content.match(/applyCardToBoard/g) || []).length;

    scripts.push({
      name: challengeName,
      className,
      type,
      totalTurns: Object.keys(turnActions).length,
      totalActions: Object.values(turnActions).reduce((a, b) => a + b, 0),
      actionsPerTurn: JSON.stringify(turnActions),
      instructionCount: instructions.length,
      hasBoardSetup,
      hasOpponentAgent,
      cardSetupCount: playCardCount + applyCardCount
    });
  });
});

// Output as TSV
let output = 'name\tclass\ttype\ttotal_turns\ttotal_actions\tactions_per_turn\tinstruction_count\thas_board_setup\thas_opponent_agent\tcard_setup_count\n';
scripts.forEach(s => {
  output += [
    s.name, s.className, s.type, s.totalTurns, s.totalActions,
    s.actionsPerTurn, s.instructionCount, s.hasBoardSetup, s.hasOpponentAgent, s.cardSetupCount
  ].join('\t') + '\n';
});

fs.writeFileSync(outputPath, output);
console.log(`Extracted ${scripts.length} challenge scripts to ${outputPath}`);
