#!/usr/bin/env python3
"""
Duelyst Unit Viewer with Attack Sequences

View units with coordinated attack/damage animations and FX.
Uses extracted timing data for proper synchronization.

Usage:
    python3 unit_viewer.py <extracted_dir> [--port 8080]
"""

import json
import http.server
import socketserver
import argparse
import urllib.parse
from pathlib import Path
from functools import partial

HTML_TEMPLATE = r'''<!DOCTYPE html>
<html>
<head>
    <meta charset="UTF-8">
    <title>Duelyst Unit Viewer</title>
    <style>
        * { box-sizing: border-box; margin: 0; padding: 0; }
        body {
            font-family: system-ui, sans-serif;
            background: #111;
            color: #eee;
            height: 100vh;
            overflow: hidden;
        }
        
        .container { display: flex; height: 100vh; }
        
        .unit-list {
            width: 280px;
            background: #1a1a1a;
            border-right: 1px solid #333;
            display: flex;
            flex-direction: column;
        }
        .unit-list header {
            padding: 12px;
            border-bottom: 1px solid #333;
        }
        .unit-list h1 { font-size: 14px; margin-bottom: 10px; }
        .unit-list input {
            width: 100%;
            padding: 8px;
            border: 1px solid #333;
            border-radius: 4px;
            background: #222;
            color: #eee;
            font-size: 12px;
        }
        .unit-list input:focus { outline: none; border-color: #e94560; }
        
        .units {
            flex: 1;
            overflow-y: auto;
            padding: 6px;
        }
        .unit-item {
            display: flex;
            align-items: center;
            gap: 12px;
            padding: 8px 10px;
            border-radius: 4px;
            cursor: pointer;
            margin-bottom: 2px;
            font-size: 12px;
        }
        .unit-item:hover { background: #252525; }
        .unit-item.active { background: #e94560; }
        .unit-item.target { background: #3d8a5a; }
        .unit-item .thumb {
            width: 48px;
            height: 48px;
            background: #0a0a0a;
            border-radius: 3px;
            display: flex;
            align-items: center;
            justify-content: center;
            overflow: hidden;
            flex-shrink: 0;
            image-rendering: pixelated;
        }
        .unit-item .thumb canvas {
            image-rendering: pixelated;
        }
        .unit-item .name {
            flex: 1;
            white-space: nowrap;
            overflow: hidden;
            text-overflow: ellipsis;
        }
        .unit-item .badges { display: flex; gap: 3px; }
        .unit-item .badge {
            font-size: 9px;
            padding: 2px 4px;
            border-radius: 2px;
            background: #333;
        }
        .unit-item .badge.has-fx { background: #5a3d8a; }
        .unit-item .badge.faction-fx { background: #3d5a4a; }
        
        .viewer {
            flex: 1;
            display: flex;
            flex-direction: column;
        }
        
        .toolbar {
            background: #1f1f1f;
            padding: 12px 20px;
            border-bottom: 1px solid #333;
            display: flex;
            align-items: center;
            gap: 20px;
        }
        .toolbar .title {
            font-size: 12px;
            color: #888;
            text-transform: uppercase;
        }
        .seq-btn {
            padding: 8px 16px;
            border: 2px solid #e94560;
            background: transparent;
            color: #e94560;
            border-radius: 4px;
            cursor: pointer;
            font-size: 13px;
            font-weight: 600;
            transition: all 0.15s;
        }
        .seq-btn:hover { background: #e94560; color: #fff; }
        .seq-btn.spawn { border-color: #5a8a3d; color: #5a8a3d; }
        .seq-btn.spawn:hover { background: #5a8a3d; color: #fff; }
        .seq-btn.death { border-color: #8a3d3d; color: #8a3d3d; }
        .seq-btn.death:hover { background: #8a3d3d; color: #fff; }
        
        .scale-control {
            display: flex;
            align-items: center;
            gap: 10px;
            margin-left: auto;
        }
        .scale-control label { font-size: 11px; color: #888; }
        .scale-control input[type="range"] { width: 120px; }
        .scale-control .value {
            font-size: 12px;
            color: #eee;
            min-width: 50px;
            text-align: right;
        }
        
        .stage {
            flex: 1;
            display: flex;
            align-items: center;
            justify-content: center;
            gap: 120px;
            background: #0a0a0a;
        }
        .stage-unit {
            display: flex;
            flex-direction: column;
            align-items: center;
            gap: 10px;
        }
        .stage-unit .label {
            font-size: 11px;
            color: #666;
            text-transform: uppercase;
        }
        .stage-unit .label .unit-name {
            color: #aaa;
            margin-left: 6px;
        }
        .stage-unit .canvas-wrap {
            position: relative;
        }
        .stage-unit canvas {
            position: absolute;
            top: 0;
            left: 0;
            image-rendering: pixelated;
        }
        .stage-unit .unit-canvas { z-index: 1; }
        .stage-unit .fx-canvas { z-index: 2; pointer-events: none; }
        
        .controls {
            background: #1a1a1a;
            padding: 12px 16px;
            border-top: 1px solid #333;
            display: flex;
            gap: 20px;
        }
        .control-group { flex: 1; }
        .control-group label {
            display: block;
            font-size: 10px;
            color: #666;
            margin-bottom: 6px;
            text-transform: uppercase;
        }
        .buttons { display: flex; flex-wrap: wrap; gap: 4px; }
        .btn {
            padding: 5px 10px;
            border: 1px solid #333;
            background: #222;
            color: #eee;
            border-radius: 3px;
            cursor: pointer;
            font-size: 11px;
        }
        .btn:hover { background: #333; }
        .btn.active { background: #e94560; border-color: #e94560; }
        .btn.target-anim.active { background: #3d8a5a; border-color: #3d8a5a; }
        .btn.fx { border-color: #5a3d8a; }
        .btn.fx:hover { background: #5a3d8a; }
        
        .info-panel {
            width: 280px;
            background: #1a1a1a;
            border-left: 1px solid #333;
            overflow-y: auto;
            font-size: 11px;
        }
        .info-section {
            padding: 12px;
            border-bottom: 1px solid #333;
        }
        .info-section h3 {
            font-size: 10px;
            color: #666;
            margin-bottom: 8px;
            text-transform: uppercase;
        }
        .info-item {
            padding: 6px 8px;
            background: #222;
            border-radius: 3px;
            margin-bottom: 4px;
            word-break: break-all;
        }
        .info-item.success { border-left: 2px solid #5a8a3d; }
        .info-item.warning { border-left: 2px solid #8a8a3d; }
        .info-item .label { color: #666; font-size: 10px; }
        .info-item .value { margin-top: 2px; }
        
        .fx-type-item {
            padding: 6px 8px;
            background: #222;
            border-radius: 3px;
            margin-bottom: 4px;
            display: flex;
            justify-content: space-between;
        }
        .fx-type-item .count {
            font-size: 10px;
            padding: 2px 6px;
            background: #333;
            border-radius: 2px;
        }
        .fx-type-item.available { border-left: 2px solid #5a8a3d; }
        .fx-type-item.fallback { border-left: 2px solid #8a8a3d; }
        .fx-type-item.missing { border-left: 2px solid #8a3d3d; opacity: 0.5; }
        
        .timing-info {
            font-size: 10px;
            color: #666;
            padding: 8px;
            background: #1a1a1a;
            border-radius: 3px;
            margin-top: 8px;
        }
        .timing-info span { color: #aaa; }
        
        .instructions {
            font-size: 11px;
            color: #666;
            padding: 8px 12px;
            background: #222;
            border-radius: 4px;
            line-height: 1.5;
        }
        
        .empty {
            display: flex;
            align-items: center;
            justify-content: center;
            height: 100%;
            color: #444;
        }
    </style>
</head>
<body>
    <div class="container">
        <aside class="unit-list">
            <header>
                <h1>Unit Viewer</h1>
                <input type="text" id="search" placeholder="Search units...">
            </header>
            <div class="units" id="units"></div>
        </aside>
        
        <main class="viewer">
            <div class="toolbar">
                <span class="title">Sequences</span>
                <button class="seq-btn spawn" onclick="playSpawnSequence()">Spawn</button>
                <button class="seq-btn" onclick="playAttackSequence()">Attack</button>
                <button class="seq-btn death" onclick="playDeathSequence()">Death</button>
                
                <div class="scale-control">
                    <label>Unit Scale</label>
                    <input type="range" id="unit-scale" min="1" max="4" value="1" step="1">
                    <span class="value" id="scale-value">100%</span>
                </div>
            </div>
            
            <div class="stage">
                <div class="stage-unit">
                    <div class="label">Attacker <span class="unit-name" id="source-name">-</span></div>
                    <div class="canvas-wrap" id="source-wrap">
                        <canvas class="unit-canvas" id="source-unit"></canvas>
                        <canvas class="fx-canvas" id="source-fx"></canvas>
                    </div>
                </div>
                <div class="stage-unit">
                    <div class="label">Target <span class="unit-name" id="target-name">-</span></div>
                    <div class="canvas-wrap" id="target-wrap">
                        <canvas class="unit-canvas" id="target-unit"></canvas>
                        <canvas class="fx-canvas" id="target-fx"></canvas>
                    </div>
                </div>
            </div>
            
            <div class="controls">
                <div class="control-group">
                    <label>Attacker Animation</label>
                    <div class="buttons" id="source-anim-btns"></div>
                </div>
                <div class="control-group">
                    <label>Target Animation</label>
                    <div class="buttons" id="target-anim-btns"></div>
                </div>
                <div class="control-group">
                    <label>Manual FX (on Target)</label>
                    <div class="buttons" id="fx-btns"></div>
                </div>
            </div>
        </main>
        
        <aside class="info-panel" id="info">
            <div class="info-section">
                <div class="instructions">
                    <strong>Usage:</strong><br>
                    Click unit = Attacker<br>
                    Shift+click = Target<br>
                    Attack plays attacker anim, then target damage + FX
                </div>
            </div>
            <div class="empty">Select a unit</div>
        </aside>
    </div>

    <script>
    var units = [];
    var fxMappings = {};
    var fxTree = {};
    var fxSprites = {};
    var rsxTiming = {};
    var fxTiming = {};
    
    var sourceUnit = null;
    var targetUnit = null;
    var sourceImg = null;
    var targetImg = null;
    
    var animState = {
        source: null,
        target: null
    };
    
    var unitScale = 1;

    var ALL_FX_TYPES = ['UnitSpawnFX', 'UnitAttackedFX', 'UnitDamagedFX', 'UnitDiedFX', 'UnitHealedFX'];

    async function init() {
        try {
            var responses = await Promise.all([
                fetch('/api/units'),
                fetch('/api/fx_mappings'),
                fetch('/api/fx_tree'),
                fetch('/api/fx_sprites'),
                fetch('/api/rsx_timing'),
                fetch('/api/fx_timing')
            ]);
            
            units = await responses[0].json();
            fxMappings = await responses[1].json();
            fxTree = await responses[2].json();
            fxSprites = await responses[3].json();
            rsxTiming = await responses[4].json();
            fxTiming = await responses[5].json();
            
            console.log('Loaded', units.length, 'units');
            
            renderUnitList(units);
            
            document.getElementById('search').addEventListener('input', function(e) {
                var q = e.target.value.toLowerCase();
                renderUnitList(units.filter(function(u) {
                    return u.name.toLowerCase().includes(q);
                }));
            });
            
            document.getElementById('unit-scale').addEventListener('input', function(e) {
                unitScale = parseInt(e.target.value);
                document.getElementById('scale-value').textContent = (unitScale * 100) + '%';
                updateCanvasSizes();
                if (sourceUnit) playIdleAnim('source');
                if (targetUnit) playIdleAnim('target');
            });
        } catch (err) {
            console.error('Init error:', err);
        }
    }

    function updateCanvasSizes() {
        var baseSize = 200 * unitScale;
        
        ['source', 'target'].forEach(function(which) {
            var wrap = document.getElementById(which + '-wrap');
            var unitCanvas = document.getElementById(which + '-unit');
            var fxCanvas = document.getElementById(which + '-fx');
            
            wrap.style.width = baseSize + 'px';
            wrap.style.height = baseSize + 'px';
            unitCanvas.width = baseSize;
            unitCanvas.height = baseSize;
            fxCanvas.width = baseSize;
            fxCanvas.height = baseSize;
        });
    }

    function renderUnitList(list) {
        var container = document.getElementById('units');
        var html = '';
        
        for (var i = 0; i < list.length; i++) {
            var u = list[i];
            var fxId = findFXId(u.name);
            var cardFX = lookupFX(fxId);
            var faction = getFactionFromUnit(u.name);
            var factionFX = getFactionDefaults(faction);
            
            var hasCardFX = cardFX && ALL_FX_TYPES.some(function(t) { return cardFX[t] && cardFX[t].length > 0; });
            var hasFactionFX = factionFX && ALL_FX_TYPES.some(function(t) { return factionFX[t] && factionFX[t].length > 0; });
            
            var badges = '';
            if (hasCardFX) badges += '<span class="badge has-fx">FX</span>';
            else if (hasFactionFX) badges += '<span class="badge faction-fx">Fac</span>';
            
            html += '<div class="unit-item" data-name="' + u.name + '" data-idx="' + i + '">';
            html += '<div class="thumb"><canvas width="48" height="48" data-idx="' + i + '"></canvas></div>';
            html += '<span class="name">' + u.name + '</span>';
            html += '<div class="badges">' + badges + '</div>';
            html += '</div>';
        }
        
        container.innerHTML = html;
        
        container.querySelectorAll('.unit-item').forEach(function(el) {
            el.addEventListener('click', function(e) {
                selectUnit(e, el.dataset.name);
            });
        });
        
        list.forEach(function(u, idx) {
            var canvas = container.querySelector('canvas[data-idx="' + idx + '"]');
            if (canvas) renderStaticThumb(canvas, u);
        });
        
        updateListHighlights();
    }

    function renderStaticThumb(canvas, unit) {
        if (!unit.spritesheet || !unit.animations) return;
        
        var animNames = Object.keys(unit.animations);
        var defaultAnim = animNames.find(function(n) { return n.includes('idle') || n.includes('breathing'); }) || animNames[0];
        if (!defaultAnim) return;
        
        var anim = unit.animations[defaultAnim];
        var frame = anim.frames && anim.frames[0];
        if (!frame) return;
        
        var img = new Image();
        img.onload = function() {
            var ctx = canvas.getContext('2d');
            var scale = Math.min(48 / frame.w, 48 / frame.h) * 1.2;
            var w = frame.w * scale;
            var h = frame.h * scale;
            var x = (48 - w) / 2;
            var y = (48 - h) / 2;
            
            ctx.imageSmoothingEnabled = false;
            ctx.clearRect(0, 0, 48, 48);
            ctx.drawImage(img, frame.x, frame.y, frame.w, frame.h, x, y, w, h);
        };
        img.src = '/' + unit.spritesheet;
    }

    function getFactionFromUnit(unitName) {
        var fxId = findFXId(unitName);
        if (fxId) {
            var match = fxId.match(/Faction(\d)/);
            if (match) return 'Faction' + match[1];
        }
        return 'Neutral';
    }

    function findFXId(unitName) {
        var normalized = unitName.toLowerCase().replace(/[^a-z0-9]/g, '');
        var cards = fxMappings.cards || {};
        
        for (var cardId in cards) {
            var cardName = cardId.split('.').pop().toLowerCase().replace(/[^a-z0-9]/g, '');
            if (cardName === normalized) return cards[cardId];
        }
        for (var cardId in cards) {
            var cardName = cardId.split('.').pop().toLowerCase().replace(/[^a-z0-9]/g, '');
            if (normalized.includes(cardName) || cardName.includes(normalized)) return cards[cardId];
        }
        return null;
    }

    function lookupFX(fxId) {
        if (!fxId) return null;
        var parts = fxId.replace('FX.', '').split('.');
        var data = fxTree;
        for (var i = 0; i < parts.length; i++) {
            if (data && data[parts[i]]) data = data[parts[i]];
            else return null;
        }
        return data;
    }

    function getFactionDefaults(faction) {
        return (fxTree.Factions && fxTree.Factions[faction]) || (fxTree.Factions && fxTree.Factions.Neutral) || null;
    }

    function getEffectsForUnit(unit, fxType) {
        if (!unit) return { effects: [], source: 'none' };
        var fxId = findFXId(unit.name);
        var cardFX = lookupFX(fxId);
        var faction = getFactionFromUnit(unit.name);
        var factionFX = getFactionDefaults(faction);
        
        if (cardFX && cardFX[fxType] && cardFX[fxType].length > 0) return { effects: cardFX[fxType], source: 'card' };
        if (factionFX && factionFX[fxType] && factionFX[fxType].length > 0) return { effects: factionFX[fxType], source: 'faction' };
        return { effects: [], source: 'none' };
    }

    function updateListHighlights() {
        document.querySelectorAll('.unit-item').forEach(function(el) {
            el.classList.remove('active', 'target');
            if (sourceUnit && el.dataset.name === sourceUnit.name) el.classList.add('active');
            if (targetUnit && el.dataset.name === targetUnit.name) el.classList.add('target');
        });
    }

    function selectUnit(event, name) {
        var unit = units.find(function(u) { return u.name === name; });
        if (!unit) return;
        
        if (event.shiftKey && sourceUnit) {
            targetUnit = unit;
            loadUnitImage(unit, 'target');
        } else {
            sourceUnit = unit;
            if (!targetUnit) targetUnit = unit;
            loadUnitImage(unit, 'source');
            if (targetUnit.name === unit.name) loadUnitImage(unit, 'target');
        }
        
        updateListHighlights();
        renderControls();
        renderInfo();
    }

    function loadUnitImage(unit, which) {
        var img = new Image();
        img.onload = function() {
            if (which === 'source') {
                sourceImg = img;
                document.getElementById('source-name').textContent = unit.name;
            } else {
                targetImg = img;
                document.getElementById('target-name').textContent = unit.name;
            }
            updateCanvasSizes();
            playIdleAnim(which);
        };
        img.src = '/' + unit.spritesheet;
    }

    function playIdleAnim(which) {
        var unit = which === 'source' ? sourceUnit : targetUnit;
        if (!unit) return;
        var animNames = Object.keys(unit.animations || {});
        var idle = animNames.find(function(n) { return n.includes('idle') || n.includes('breathing'); }) || animNames[0];
        if (idle) playAnim(which, idle, true);
    }

    function renderControls() {
        var sourceAnims = Object.keys(sourceUnit ? sourceUnit.animations || {} : {});
        var targetAnims = Object.keys(targetUnit ? targetUnit.animations || {} : {});
        
        document.getElementById('source-anim-btns').innerHTML = sourceAnims.map(function(name) {
            return '<button class="btn" onclick="playAnim(\'source\', \'' + name + '\', true)">' + name + '</button>';
        }).join('');
        
        document.getElementById('target-anim-btns').innerHTML = targetAnims.map(function(name) {
            return '<button class="btn target-anim" onclick="playAnim(\'target\', \'' + name + '\', true)">' + name + '</button>';
        }).join('');
        
        document.getElementById('fx-btns').innerHTML = ALL_FX_TYPES.map(function(t) {
            var result = getEffectsForUnit(targetUnit, t);
            var shortName = t.replace('Unit', '').replace('FX', '');
            var disabled = result.effects.length === 0;
            return '<button class="btn fx" onclick="triggerFX(\'' + t + '\')"' + (disabled ? ' disabled style="opacity:0.3"' : '') + '>' + shortName + '</button>';
        }).join('');
    }

    function renderInfo() {
        var infoEl = document.getElementById('info');
        
        if (!sourceUnit) {
            infoEl.innerHTML = '<div class="info-section"><div class="instructions"><strong>Usage:</strong><br>Click unit = Attacker<br>Shift+click = Target<br>All FX play on Target</div></div><div class="empty">Select a unit</div>';
            return;
        }
        
        var html = '<div class="info-section"><div class="instructions"><strong>FX Info:</strong><br>UnitAttackedFX = attack impact on target<br>UnitDamagedFX = damage reaction on target<br>Both play on TARGET, not attacker</div></div>';
        
        html += renderUnitInfo(sourceUnit, 'Attacker');
        if (targetUnit && targetUnit.name !== sourceUnit.name) {
            html += renderUnitInfo(targetUnit, 'Target');
        }
        
        infoEl.innerHTML = html;
    }

    function renderUnitInfo(unit, label) {
        var fxId = findFXId(unit.name);
        var faction = getFactionFromUnit(unit.name);
        
        var fxTypesHtml = '';
        for (var i = 0; i < ALL_FX_TYPES.length; i++) {
            var fxType = ALL_FX_TYPES[i];
            var result = getEffectsForUnit(unit, fxType);
            var cls = result.source === 'card' ? 'available' : result.source === 'faction' ? 'fallback' : 'missing';
            var countLabel = result.source === 'none' ? '0' : result.effects.length + ' (' + result.source + ')';
            fxTypesHtml += '<div class="fx-type-item ' + cls + '"><span>' + fxType.replace('Unit','').replace('FX','') + '</span><span class="count">' + countLabel + '</span></div>';
        }
        
        var attackAnim = unit.animations && unit.animations.attack;
        var timingHtml = '';
        if (attackAnim && attackAnim.frames) {
            var duration = attackAnim.frames.length / (attackAnim.fps || 12);
            timingHtml = '<div class="timing-info">Attack duration: <span>' + duration.toFixed(2) + 's</span> (' + attackAnim.frames.length + ' frames @ ' + (attackAnim.fps || 12) + 'fps)</div>';
        }
        
        return '<div class="info-section"><h3>' + label + '</h3>' +
            '<div class="info-item"><div class="label">Name</div><div class="value">' + unit.name + '</div></div>' +
            '<div class="info-item ' + (fxId ? 'success' : 'warning') + '"><div class="label">FX ID</div><div class="value">' + (fxId || 'None (using faction defaults)') + '</div></div>' +
            '<div class="info-item"><div class="label">Faction</div><div class="value">' + faction + '</div></div>' +
            timingHtml +
            '</div><div class="info-section"><h3>' + unit.name + ' FX Types</h3>' + fxTypesHtml + '</div>';
    }

    function playAnim(which, animName, loop) {
        var unit = which === 'source' ? sourceUnit : targetUnit;
        var img = which === 'source' ? sourceImg : targetImg;
        var canvas = document.getElementById(which + '-unit');
        
        if (!unit || !img) return null;
        
        var btnContainer = which === 'source' ? 'source-anim-btns' : 'target-anim-btns';
        document.querySelectorAll('#' + btnContainer + ' .btn').forEach(function(b) {
            b.classList.toggle('active', b.textContent === animName);
        });
        
        if (animState[which]) {
            cancelAnimationFrame(animState[which]);
            animState[which] = null;
        }
        
        var anim = unit.animations[animName];
        if (!anim || !anim.frames || !anim.frames.length) return null;
        
        var ctx = canvas.getContext('2d');
        var frames = anim.frames;
        var fps = anim.fps || 12;
        var frameDuration = 1000 / fps;
        var totalDuration = (frames.length / fps) * 1000;
        
        var frameIdx = 0;
        var lastTime = 0;
        
        return new Promise(function(resolve) {
            function draw(time) {
                if (time - lastTime >= frameDuration) {
                    var frame = frames[frameIdx % frames.length];
                    
                    ctx.clearRect(0, 0, canvas.width, canvas.height);
                    ctx.imageSmoothingEnabled = false;
                    
                    var x = (canvas.width - frame.w * unitScale) / 2;
                    var y = (canvas.height - frame.h * unitScale) / 2;
                    ctx.drawImage(img, frame.x, frame.y, frame.w, frame.h, x, y, frame.w * unitScale, frame.h * unitScale);
                    
                    frameIdx++;
                    lastTime = time;
                    
                    if (!loop && frameIdx >= frames.length) {
                        animState[which] = null;
                        resolve({ duration: totalDuration });
                        playIdleAnim(which);
                        return;
                    }
                }
                
                animState[which] = requestAnimationFrame(draw);
            }
            
            animState[which] = requestAnimationFrame(draw);
            
            if (loop) resolve({ duration: totalDuration });
        });
    }

    function getAnimDuration(unit, animName) {
        var anim = unit && unit.animations && unit.animations[animName];
        if (!anim || !anim.frames) return 0;
        return (anim.frames.length / (anim.fps || 12)) * 1000;
    }

    function playSpawnSequence() {
        if (!sourceUnit) return;
        triggerFX('UnitSpawnFX');
        playIdleAnim('source');
    }

    function playAttackSequence() {
        if (!sourceUnit || !targetUnit) return;
        
        var sourceAnims = Object.keys(sourceUnit.animations);
        var targetAnims = Object.keys(targetUnit.animations);
        
        var attackAnimName = sourceAnims.find(function(n) { return n === 'attack' || n.includes('attack'); });
        var damageAnimName = targetAnims.find(function(n) { return n === 'damage' || n === 'hit' || n.includes('damage') || n.includes('hit'); });
        
        if (!attackAnimName) return;
        
        var attackDuration = getAnimDuration(sourceUnit, attackAnimName);
        var hitTime = attackDuration * 0.5;
        
        playAnim('source', attackAnimName, false);
        
        setTimeout(function() {
            triggerFX('UnitAttackedFX');
            
            setTimeout(function() {
                if (damageAnimName) playAnim('target', damageAnimName, false);
                triggerFX('UnitDamagedFX');
            }, 50);
        }, hitTime);
    }

    function playDeathSequence() {
        if (!sourceUnit) return;
        
        var anims = Object.keys(sourceUnit.animations);
        var deathAnimName = anims.find(function(n) { return n === 'death' || n.includes('death'); });
        
        if (deathAnimName) playAnim('source', deathAnimName, false);
        triggerFX('UnitDiedFX');
    }

    function triggerFX(fxType) {
        var result = getEffectsForUnit(targetUnit, fxType);
        if (!result.effects.length) return;
        
        var canvas = document.getElementById('target-fx');
        var ctx = canvas.getContext('2d');
        
        for (var i = 0; i < result.effects.length; i++) {
            playFXEffect(ctx, canvas, result.effects[i]);
        }
    }

    function playFXEffect(ctx, canvas, effect) {
        return new Promise(function(resolve) {
            var spriteId = effect.spriteIdentifier;
            if (Array.isArray(spriteId)) spriteId = spriteId[Math.floor(Math.random() * spriteId.length)];
            
            if (!spriteId) {
                if (effect.type === 'Light' || effect.type === 'Shockwave') {
                    drawPseudoEffect(ctx, canvas, effect);
                    setTimeout(resolve, 500);
                    return;
                }
                resolve();
                return;
            }
            
            var spriteName = spriteId.replace('.name', '').replace('.img', '');
            var sprite = fxSprites[spriteName];
            if (!sprite) { resolve(); return; }
            
            var img = new Image();
            img.onload = function() {
                var frames = sprite.frames || [];
                var offset = effect.offset || { x: 0, y: 0 };
                
                if (!frames.length) {
                    var scale = unitScale * 1.5;
                    var x = (canvas.width - img.width * scale) / 2 + (offset.x || 0) * unitScale;
                    var y = (canvas.height - img.height * scale) / 2 - (offset.y || 0) * unitScale;
                    ctx.drawImage(img, x, y, img.width * scale, img.height * scale);
                    setTimeout(function() { ctx.clearRect(0, 0, canvas.width, canvas.height); resolve(); }, 400);
                    return;
                }
                
                var fps = sprite.fps || 12;
                var frameDuration = 1000 / fps;
                var frameIdx = 0;
                var lastTime = 0;
                
                function draw(time) {
                    if (frameIdx >= frames.length) {
                        ctx.clearRect(0, 0, canvas.width, canvas.height);
                        resolve();
                        return;
                    }
                    
                    if (time - lastTime >= frameDuration) {
                        var frame = frames[frameIdx];
                        var scale = unitScale * 2;
                        var x = (canvas.width - frame.w * scale) / 2 + (offset.x || 0) * unitScale;
                        var y = (canvas.height - frame.h * scale) / 2 - (offset.y || 0) * unitScale;
                        
                        ctx.clearRect(0, 0, canvas.width, canvas.height);
                        ctx.imageSmoothingEnabled = false;
                        
                        if (effect.flippedX) {
                            ctx.save();
                            ctx.scale(-1, 1);
                            ctx.drawImage(img, frame.x, frame.y, frame.w, frame.h, -x - frame.w * scale, y, frame.w * scale, frame.h * scale);
                            ctx.restore();
                        } else {
                            ctx.drawImage(img, frame.x, frame.y, frame.w, frame.h, x, y, frame.w * scale, frame.h * scale);
                        }
                        
                        frameIdx++;
                        lastTime = time;
                    }
                    requestAnimationFrame(draw);
                }
                requestAnimationFrame(draw);
            };
            img.onerror = resolve;
            img.src = '/' + sprite.spritesheet;
        });
    }

    function drawPseudoEffect(ctx, canvas, effect) {
        var color = effect.color || { r: 255, g: 255, b: 255 };
        var offset = effect.offset || { x: 0, y: 0 };
        var cx = canvas.width / 2 + (offset.x || 0) * unitScale;
        var cy = canvas.height / 2 - (offset.y || 0) * unitScale;
        
        ctx.save();
        if (effect.type === 'Light') {
            var gradient = ctx.createRadialGradient(cx, cy, 0, cx, cy, (effect.radius || 80) * unitScale);
            gradient.addColorStop(0, 'rgba(' + color.r + ',' + color.g + ',' + color.b + ',0.4)');
            gradient.addColorStop(1, 'rgba(0,0,0,0)');
            ctx.fillStyle = gradient;
            ctx.fillRect(0, 0, canvas.width, canvas.height);
        } else if (effect.type === 'Shockwave') {
            ctx.strokeStyle = 'rgba(' + (color.r || 255) + ',' + (color.g || 255) + ',' + (color.b || 255) + ',0.6)';
            ctx.lineWidth = 3;
            ctx.beginPath();
            ctx.arc(cx, cy, (effect.radius || 40) * unitScale, 0, Math.PI * 2);
            ctx.stroke();
        }
        ctx.restore();
        setTimeout(function() { ctx.clearRect(0, 0, canvas.width, canvas.height); }, 500);
    }

    init();
    </script>
</body>
</html>
'''


class ViewerHandler(http.server.SimpleHTTPRequestHandler):
    def __init__(self, *args, asset_dir: Path, **kwargs):
        self.asset_dir = asset_dir
        super().__init__(*args, directory=str(asset_dir), **kwargs)

    def do_GET(self):
        path = urllib.parse.urlparse(self.path).path
        routes = {
            '/': self.serve_html,
            '/api/units': lambda: self.send_json(self.load_units()),
            '/api/fx_mappings': lambda: self.send_json(self.load_fx_mappings()),
            '/api/fx_tree': lambda: self.send_json(self.load_fx_tree()),
            '/api/fx_sprites': lambda: self.send_json(self.load_fx_sprites()),
            '/api/rsx_timing': lambda: self.send_json(self.load_rsx_timing()),
            '/api/fx_timing': lambda: self.send_json(self.load_fx_timing()),
        }
        if path in routes:
            routes[path]()
        else:
            super().do_GET()

    def serve_html(self):
        self.send_response(200)
        self.send_header('Content-Type', 'text/html')
        self.end_headers()
        self.wfile.write(HTML_TEMPLATE.encode())

    def send_json(self, data):
        self.send_response(200)
        self.send_header('Content-Type', 'application/json')
        self.end_headers()
        self.wfile.write(json.dumps(data).encode())

    def load_units(self) -> list:
        units = []
        units_dir = self.asset_dir / 'units'
        if not units_dir.exists():
            print(f"Units dir not found: {units_dir}")
            return units

        for item_dir in sorted(units_dir.iterdir()):
            if not item_dir.is_dir():
                continue
            spritesheet = self.find_spritesheet(item_dir)
            if not spritesheet:
                continue
            animations = {}
            anim_file = item_dir / 'animations.txt'
            if anim_file.exists():
                animations = self.parse_animations(anim_file)
            units.append({'name': item_dir.name, 'spritesheet': spritesheet, 'animations': animations})
        
        print(f"Loaded {len(units)} units")
        return units

    def find_spritesheet(self, item_dir: Path) -> str:
        for name in ['spritesheet.png', f'{item_dir.name}.png']:
            if (item_dir / name).exists():
                return f'units/{item_dir.name}/{name}'
        pngs = list(item_dir.glob('*.png'))
        return f'units/{item_dir.name}/{pngs[0].name}' if pngs else None

    def parse_animations(self, path: Path) -> dict:
        animations = {}
        for line in path.read_text().strip().split('\n'):
            if not line.strip():
                continue
            parts = line.split('\t')
            if len(parts) < 3:
                continue
            name = parts[0]
            try:
                fps = int(parts[1])
            except ValueError:
                fps = 12
            frame_data = parts[2].split(',')
            frames = []
            for i in range(0, len(frame_data) - 4, 5):
                try:
                    frames.append({
                        'idx': int(frame_data[i]),
                        'x': int(frame_data[i + 1]),
                        'y': int(frame_data[i + 2]),
                        'w': int(frame_data[i + 3]),
                        'h': int(frame_data[i + 4])
                    })
                except (ValueError, IndexError):
                    continue
            if frames:
                animations[name] = {'fps': fps, 'frames': frames}
        return animations

    def load_fx_mappings(self) -> dict:
        result = {'cards': {}}
        card_map = self.asset_dir / 'fx_compositions' / 'card_fx_mapping.tsv'
        if card_map.exists():
            for line in card_map.read_text().strip().split('\n')[1:]:
                parts = line.split('\t')
                if len(parts) >= 2:
                    result['cards'][parts[0]] = parts[1]
        return result

    def load_fx_tree(self) -> dict:
        tree_file = self.asset_dir / 'fx_compositions' / 'fx_tree.json'
        return json.loads(tree_file.read_text()) if tree_file.exists() else {}

    def load_fx_sprites(self) -> dict:
        result = {}
        fx_dir = self.asset_dir / 'fx'
        rsx_map = fx_dir / 'rsx_mapping.tsv'
        if not rsx_map.exists():
            return result
        lines = rsx_map.read_text().strip().split('\n')
        if len(lines) < 2:
            return result
        for line in lines[1:]:
            parts = line.split('\t')
            if len(parts) < 2:
                continue
            rsx_name, folder = parts[0], parts[1]
            sprite_data = {'spritesheet': f'fx/{folder}/spritesheet.png', 'frames': [], 'fps': 12}
            anim_file = fx_dir / folder / 'animations.txt'
            if anim_file.exists():
                anims = self.parse_animations(anim_file)
                if rsx_name in anims:
                    sprite_data['frames'] = anims[rsx_name].get('frames', [])
                    sprite_data['fps'] = anims[rsx_name].get('fps', 12)
                elif anims:
                    first = list(anims.values())[0]
                    sprite_data['frames'] = first.get('frames', [])
                    sprite_data['fps'] = first.get('fps', 12)
            result[rsx_name] = sprite_data
        return result

    def load_rsx_timing(self) -> dict:
        timing_file = self.asset_dir / 'fx_compositions' / 'rsx_timing.json'
        return json.loads(timing_file.read_text()) if timing_file.exists() else {}

    def load_fx_timing(self) -> dict:
        timing_file = self.asset_dir / 'fx_compositions' / 'fx_timing.json'
        return json.loads(timing_file.read_text()) if timing_file.exists() else {}

    def log_message(self, *args):
        pass


def main():
    parser = argparse.ArgumentParser(description='Duelyst Unit Viewer')
    parser.add_argument('directory', type=Path)
    parser.add_argument('--port', type=int, default=8080)
    args = parser.parse_args()

    asset_dir = args.directory.expanduser().resolve()
    if not asset_dir.exists():
        print(f"Error: {asset_dir} not found")
        return 1

    print(f"\n  Duelyst Unit Viewer")
    print(f"  http://localhost:{args.port}")
    print(f"  Asset dir: {asset_dir}\n")

    handler = partial(ViewerHandler, asset_dir=asset_dir)
    with socketserver.TCPServer(("", args.port), handler) as httpd:
        try:
            httpd.serve_forever()
        except KeyboardInterrupt:
            print("\nStopped")


if __name__ == '__main__':
    main()
