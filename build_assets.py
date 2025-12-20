#!/usr/bin/env python3
"""
Build Assets Pipeline for Sylvanshine

Scans ./app/resources/ for game assets, parses .plist animation data,
copies files to dist/, and generates a unified assets.json manifest.

Supports incremental builds - only copies changed files.

Usage:
    uv run build_assets.py [--clean] [--verbose] [--force]
"""

import argparse
import json
import os
import plistlib
import re
import shutil
import sys
from dataclasses import dataclass, field
from pathlib import Path
from typing import Any


@dataclass
class BuildStats:
    """Track build statistics."""
    units_found: int = 0
    units_processed: int = 0
    units_skipped: int = 0
    fx_found: int = 0
    fx_processed: int = 0
    fx_skipped: int = 0
    files_copied: int = 0
    files_skipped: int = 0
    manifest_updated: bool = False
    warnings: int = 0
    errors: int = 0


@dataclass
class Frame:
    """A single animation frame."""
    x: int
    y: int
    w: int
    h: int
    offset_x: int = 0
    offset_y: int = 0
    rotated: bool = False


@dataclass
class Animation:
    """An animation sequence."""
    fps: int = 12
    frames: list[Frame] = field(default_factory=list)


def needs_copy(src: Path, dest: Path) -> bool:
    """Check if src needs to be copied to dest.

    Uses mtime + size for fast comparison.
    Returns True if copy is needed.
    """
    if not dest.exists():
        return True

    src_stat = src.stat()
    dest_stat = dest.stat()

    # If sizes differ, need to copy
    if src_stat.st_size != dest_stat.st_size:
        return True

    # If src is newer than dest, need to copy
    if src_stat.st_mtime > dest_stat.st_mtime:
        return True

    return False


def copy_if_needed(src: Path, dest: Path, force: bool = False) -> bool:
    """Copy file only if needed. Returns True if copied."""
    if force or needs_copy(src, dest):
        shutil.copy2(src, dest)
        return True
    return False


def write_json_if_changed(path: Path, data: dict) -> bool:
    """Write JSON only if content changed. Returns True if written."""
    # Use sort_keys for deterministic output
    new_content = json.dumps(data, indent=2, sort_keys=True)

    if path.exists():
        try:
            old_content = path.read_text()
            if old_content == new_content:
                return False
        except Exception:
            pass  # If we can't read, just write

    path.write_text(new_content)
    return True


def parse_rect_string(rect_str: str) -> tuple[int, int, int, int]:
    """Parse plist rect format '{{x,y},{w,h}}' into (x, y, w, h)."""
    match = re.match(r'\{\{(-?\d+),(-?\d+)\},\{(\d+),(\d+)\}\}', rect_str.replace(' ', ''))
    if match:
        return int(match.group(1)), int(match.group(2)), int(match.group(3)), int(match.group(4))
    return 0, 0, 0, 0


def parse_offset_string(offset_str: str) -> tuple[int, int]:
    """Parse plist offset format '{x,y}' into (x, y)."""
    match = re.match(r'\{(-?\d+),(-?\d+)\}', offset_str.replace(' ', ''))
    if match:
        return int(match.group(1)), int(match.group(2))
    return 0, 0


def parse_plist(plist_path: Path, verbose: bool = False) -> dict[str, Animation]:
    """Parse a plist file and extract animation data.

    Returns a dict mapping animation names to Animation objects.
    Animation names are derived from frame names by removing the unit prefix and _XXX.png suffix.
    E.g., "f1_general_idle_000.png" -> "idle"
    """
    animations: dict[str, Animation] = {}

    try:
        with open(plist_path, 'rb') as f:
            plist = plistlib.load(f)
    except Exception as e:
        if verbose:
            print(f"  Warning: Failed to parse {plist_path}: {e}")
        return animations

    frames_dict = plist.get('frames', {})
    unit_name = plist_path.stem

    animation_frames: dict[str, list[tuple[int, Frame]]] = {}

    for frame_name, frame_data in frames_dict.items():
        match = re.match(r'(.+)_(\d+)\.png$', frame_name)
        if not match:
            continue

        full_anim_name = match.group(1)
        frame_num = int(match.group(2))

        # Strip the unit prefix to get just the action name
        if full_anim_name.startswith(unit_name + '_'):
            anim_name = full_anim_name[len(unit_name) + 1:]
        else:
            anim_name = full_anim_name

        frame_str = frame_data.get('frame', '{{0,0},{0,0}}')
        x, y, w, h = parse_rect_string(frame_str)

        offset_str = frame_data.get('offset', '{0,0}')
        offset_x, offset_y = parse_offset_string(offset_str)

        rotated = frame_data.get('rotated', False)

        frame = Frame(x=x, y=y, w=w, h=h, offset_x=offset_x, offset_y=offset_y, rotated=rotated)

        if anim_name not in animation_frames:
            animation_frames[anim_name] = []
        animation_frames[anim_name].append((frame_num, frame))

    for anim_name, frames in animation_frames.items():
        frames.sort(key=lambda x: x[0])
        animations[anim_name] = Animation(
            fps=12,
            frames=[f for _, f in frames]
        )

    return animations


def load_timing_data(timing_path: Path, verbose: bool = False) -> dict[str, dict]:
    """Load unit timing data from TSV file."""
    timing = {}

    if not timing_path.exists():
        if verbose:
            print(f"Warning: Timing file not found: {timing_path}")
        return timing

    with open(timing_path, 'r') as f:
        lines = f.readlines()

    if not lines:
        return timing

    header = lines[0].strip().split('\t')

    for line in lines[1:]:
        line = line.strip()
        if not line:
            continue
        parts = line.split('\t')
        if len(parts) < 4:
            continue

        row = dict(zip(header, parts))
        unit_folder = row.get('unit_folder', '')

        if unit_folder == 'unit_folder':
            continue

        if unit_folder:
            try:
                timing[unit_folder] = {
                    'attack_delay': float(row.get('attack_delay', '0.5')),
                    'attack_release_delay': float(row.get('attack_release_delay', '0.0'))
                }
            except ValueError as e:
                if verbose:
                    print(f"  Warning: Invalid timing data for {unit_folder}: {e}")
                continue

    return timing


def load_fx_mapping(manifest_path: Path, rsx_mapping_path: Path, verbose: bool = False) -> tuple[dict, dict]:
    """Load FX manifest and RSX mapping from TSV files."""
    fx_manifest = {}
    rsx_mapping = {}

    if manifest_path.exists():
        with open(manifest_path, 'r') as f:
            lines = f.readlines()

        if lines:
            header = lines[0].strip().split('\t')
            for line in lines[1:]:
                parts = line.strip().split('\t')
                if len(parts) >= 2:
                    row = dict(zip(header, parts))
                    folder = row.get('folder', '')
                    if folder:
                        fx_manifest[folder] = {
                            'spritesheet': row.get('spritesheet', ''),
                            'animations': row.get('animations', '').split(',') if row.get('animations') else []
                        }

    if rsx_mapping_path.exists():
        with open(rsx_mapping_path, 'r') as f:
            lines = f.readlines()

        if lines:
            header = lines[0].strip().split('\t')
            for line in lines[1:]:
                parts = line.strip().split('\t')
                if len(parts) >= 2:
                    row = dict(zip(header, parts))
                    rsx_name = row.get('rsx_name', '')
                    folder = row.get('folder', '')
                    prefix = row.get('prefix', '')
                    if rsx_name and folder:
                        # Animation name is prefix without trailing underscore
                        anim_name = prefix.rstrip('_') if prefix else ''
                        rsx_mapping[rsx_name] = {
                            'folder': folder,
                            'anim': anim_name
                        }

    return fx_manifest, rsx_mapping


def animation_to_dict(anim: Animation) -> dict:
    """Convert Animation to JSON-serializable dict."""
    return {
        'fps': anim.fps,
        'frames': [
            {'x': f.x, 'y': f.y, 'w': f.w, 'h': f.h}
            for f in anim.frames
        ]
    }


def build_assets(
    source_dir: Path,
    dist_dir: Path,
    clean: bool = False,
    force: bool = False,
    verbose: bool = False
) -> BuildStats:
    """Build all game assets into dist/ directory."""
    stats = BuildStats()

    resources_dir = source_dir / 'resources'

    if not resources_dir.exists():
        print(f"Error: Source resources directory not found: {resources_dir}")
        stats.errors += 1
        return stats

    # Clean dist directory if requested
    if clean and dist_dir.exists():
        print("Cleaning dist directory...")
        shutil.rmtree(dist_dir)

    # Create output directories
    dist_dir.mkdir(parents=True, exist_ok=True)
    (dist_dir / 'resources' / 'units').mkdir(parents=True, exist_ok=True)
    (dist_dir / 'resources' / 'fx').mkdir(parents=True, exist_ok=True)

    # Initialize assets manifest
    assets: dict[str, Any] = {
        'units': {},
        'fx': {},
        'timing': {},
        'fx_mapping': {}
    }

    # ===== Load timing data =====
    print("Loading timing data...")
    timing_path = source_dir / 'timing' / 'unit_timing.tsv'
    assets['timing'] = load_timing_data(timing_path, verbose)
    print(f"  Loaded {len(assets['timing'])} timing entries")

    # ===== Load FX mapping =====
    print("Loading FX mappings...")
    manifest_path = source_dir / 'fx' / 'manifest.tsv'
    rsx_mapping_path = source_dir / 'fx' / 'rsx_mapping.tsv'
    fx_manifest, rsx_mapping = load_fx_mapping(manifest_path, rsx_mapping_path, verbose)
    assets['fx_mapping'] = rsx_mapping
    print(f"  Loaded {len(rsx_mapping)} RSX mappings")

    # ===== Process Units =====
    units_dir = resources_dir / 'units'
    if units_dir.exists():
        print("Processing units...")
        png_files = sorted(units_dir.glob('*.png'))
        stats.units_found = len(png_files)

        for png_path in png_files:
            unit_name = png_path.stem
            plist_path = units_dir / f'{unit_name}.plist'

            # Parse animations from plist
            animations = {}
            if plist_path.exists():
                animations = parse_plist(plist_path, verbose)
            else:
                if verbose:
                    print(f"  Warning: No plist for {unit_name}")
                stats.warnings += 1

            # Copy spritesheet to dist (only if needed)
            dest_png = dist_dir / 'resources' / 'units' / f'{unit_name}.png'
            try:
                if copy_if_needed(png_path, dest_png, force):
                    stats.files_copied += 1
                else:
                    stats.files_skipped += 1
                    stats.units_skipped += 1
            except Exception as e:
                print(f"  Error copying {png_path}: {e}")
                stats.errors += 1
                continue

            # Add to assets manifest
            assets['units'][unit_name] = {
                'spritesheet': f'resources/units/{unit_name}.png',
                'animations': {
                    name: animation_to_dict(anim)
                    for name, anim in animations.items()
                }
            }

            stats.units_processed += 1

        copied = stats.units_processed - stats.units_skipped
        print(f"  {stats.units_found} units: {copied} copied, {stats.units_skipped} up-to-date")

    # ===== Process FX =====
    fx_dir = resources_dir / 'fx'
    if fx_dir.exists():
        print("Processing FX...")
        png_files = sorted(fx_dir.glob('*.png'))
        png_files = [p for p in png_files if '@2x' not in p.name]
        stats.fx_found = len(png_files)

        for png_path in png_files:
            fx_name = png_path.stem
            plist_path = fx_dir / f'{fx_name}.plist'

            animations = {}
            if plist_path.exists():
                animations = parse_plist(plist_path, verbose)
            else:
                if verbose:
                    print(f"  Warning: No plist for FX {fx_name}")
                stats.warnings += 1

            dest_png = dist_dir / 'resources' / 'fx' / f'{fx_name}.png'
            try:
                if copy_if_needed(png_path, dest_png, force):
                    stats.files_copied += 1
                else:
                    stats.files_skipped += 1
                    stats.fx_skipped += 1
            except Exception as e:
                print(f"  Error copying {png_path}: {e}")
                stats.errors += 1
                continue

            assets['fx'][fx_name] = {
                'spritesheet': f'resources/fx/{fx_name}.png',
                'animations': {
                    name: animation_to_dict(anim)
                    for name, anim in animations.items()
                }
            }

            stats.fx_processed += 1

        copied = stats.fx_processed - stats.fx_skipped
        print(f"  {stats.fx_found} FX: {copied} copied, {stats.fx_skipped} up-to-date")

    # ===== Copy shadow texture =====
    shadow_src = source_dir / 'resources' / 'unit_shadow.png'
    if shadow_src.exists():
        shadow_dest = dist_dir / 'resources' / 'unit_shadow.png'
        if copy_if_needed(shadow_src, shadow_dest, force):
            stats.files_copied += 1
            print("Copied unit_shadow.png")
        else:
            stats.files_skipped += 1

    # ===== Write assets.json (only if changed) =====
    assets_path = dist_dir / 'assets.json'
    if write_json_if_changed(assets_path, assets):
        stats.manifest_updated = True
        print("Updated assets.json")
    else:
        print("assets.json unchanged")

    # ===== Summary =====
    print(f"\nBuild complete!")
    print(f"  Units: {stats.units_processed} ({stats.units_skipped} unchanged)")
    print(f"  FX: {stats.fx_processed} ({stats.fx_skipped} unchanged)")
    print(f"  Files: {stats.files_copied} copied, {stats.files_skipped} skipped")
    if stats.warnings > 0:
        print(f"  Warnings: {stats.warnings}")
    if stats.errors > 0:
        print(f"  Errors: {stats.errors}")

    return stats


def main():
    parser = argparse.ArgumentParser(description='Build Sylvanshine game assets')
    parser.add_argument('--clean', action='store_true', help='Clean dist directory before building')
    parser.add_argument('--force', '-f', action='store_true', help='Force copy all files (ignore cache)')
    parser.add_argument('--verbose', '-v', action='store_true', help='Verbose output')
    args = parser.parse_args()

    script_dir = Path(__file__).parent.resolve()
    source_dir = script_dir / 'app'
    dist_dir = script_dir / 'dist'

    print(f"Building assets from: {source_dir}")
    print(f"Output to: {dist_dir}")
    print()

    stats = build_assets(
        source_dir=source_dir,
        dist_dir=dist_dir,
        clean=args.clean,
        force=args.force,
        verbose=args.verbose
    )

    if stats.errors > 0:
        print(f"\nBuild completed with {stats.errors} errors")
        sys.exit(1)

    sys.exit(0)


if __name__ == '__main__':
    main()
