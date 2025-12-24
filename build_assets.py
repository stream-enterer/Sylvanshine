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
import math
import os
import plistlib
import re
import shutil
import sys
from dataclasses import dataclass, field
from pathlib import Path
from typing import Any

import numpy as np
from PIL import Image


# =============================================================================
# SDF Generation using Jump Flooding Algorithm
# =============================================================================

def generate_sdf_jfa(alpha: np.ndarray, max_dist: float = 32.0) -> np.ndarray:
    """
    Generate a Signed Distance Field from an alpha channel using Jump Flooding Algorithm.

    Args:
        alpha: 2D numpy array of alpha values (0-255 or 0.0-1.0)
        max_dist: Maximum distance to compute (pixels)

    Returns:
        2D numpy array of signed distances (negative = inside, positive = outside)
    """
    h, w = alpha.shape

    # Normalize alpha to 0-1 if needed
    if alpha.max() > 1.0:
        alpha = alpha.astype(np.float32) / 255.0

    # Threshold for inside/outside (alpha > 0.5 = inside)
    inside = alpha > 0.5

    # Initialize nearest seed coordinates
    # Use -1 to indicate "no seed found yet"
    nearest_x = np.full((h, w), -1, dtype=np.int32)
    nearest_y = np.full((h, w), -1, dtype=np.int32)

    # Seed: edge pixels (inside pixels adjacent to outside, or vice versa)
    # For simplicity, seed ALL inside pixels initially
    inside_y, inside_x = np.where(inside)
    nearest_x[inside_y, inside_x] = inside_x
    nearest_y[inside_y, inside_x] = inside_y

    # Jump Flooding passes
    max_dim = max(w, h)
    step = max_dim // 2
    if step < 1:
        step = 1

    # Offset directions: 8-connected + self
    offsets = [(-1, -1), (0, -1), (1, -1),
               (-1, 0),           (1, 0),
               (-1, 1),  (0, 1),  (1, 1)]

    while step >= 1:
        # Create coordinate grids
        yy, xx = np.mgrid[0:h, 0:w]

        for ox, oy in offsets:
            # Neighbor coordinates
            nx = xx + ox * step
            ny = yy + oy * step

            # Clamp to bounds
            nx = np.clip(nx, 0, w - 1)
            ny = np.clip(ny, 0, h - 1)

            # Get neighbor's nearest seed
            neighbor_seed_x = nearest_x[ny, nx]
            neighbor_seed_y = nearest_y[ny, nx]

            # Check if neighbor has a valid seed
            valid_neighbor = neighbor_seed_x >= 0

            # Calculate distances
            # Current pixel's distance to its seed
            curr_has_seed = nearest_x >= 0
            curr_dist = np.where(
                curr_has_seed,
                np.sqrt((xx - nearest_x)**2 + (yy - nearest_y)**2),
                np.inf
            )

            # Current pixel's distance to neighbor's seed
            neighbor_dist = np.where(
                valid_neighbor,
                np.sqrt((xx - neighbor_seed_x)**2 + (yy - neighbor_seed_y)**2),
                np.inf
            )

            # Update if neighbor's seed is closer
            update_mask = neighbor_dist < curr_dist
            nearest_x = np.where(update_mask, neighbor_seed_x, nearest_x)
            nearest_y = np.where(update_mask, neighbor_seed_y, nearest_y)

        step //= 2

    # Compute final distances
    yy, xx = np.mgrid[0:h, 0:w]
    has_seed = nearest_x >= 0
    dist = np.where(
        has_seed,
        np.sqrt((xx - nearest_x)**2 + (yy - nearest_y)**2),
        max_dist
    )

    # Sign: negative inside, positive outside
    sdf = np.where(inside, -dist, dist)

    # Clamp to max distance
    sdf = np.clip(sdf, -max_dist, max_dist)

    return sdf.astype(np.float32)


def encode_sdf_to_uint8(sdf: np.ndarray, max_dist: float = 32.0) -> np.ndarray:
    """
    Encode SDF to 8-bit format: (sdf + max_dist) / (2 * max_dist) * 255

    This maps [-max_dist, +max_dist] to [0, 255]
    Decode in shader: sdf = encoded / 255.0 * 2.0 * max_dist - max_dist
    """
    # Normalize to 0-1 range
    normalized = (sdf + max_dist) / (2.0 * max_dist)
    # Clamp and convert to uint8
    encoded = np.clip(normalized * 255.0, 0, 255).astype(np.uint8)
    return encoded


def generate_sdf_atlas(
    spritesheet_path: Path,
    frames: list[dict],
    output_path: Path,
    max_dist: float = 32.0,
    verbose: bool = False
) -> bool:
    """
    Generate an SDF atlas matching the layout of the source spritesheet.

    Args:
        spritesheet_path: Path to source spritesheet PNG
        frames: List of frame dicts with 'x', 'y', 'w', 'h' keys
        output_path: Path to save the SDF atlas PNG
        max_dist: Maximum SDF distance
        verbose: Print debug info

    Returns:
        True if successful, False on error
    """
    try:
        # Load spritesheet
        img = Image.open(spritesheet_path).convert('RGBA')
        img_array = np.array(img)
        atlas_h, atlas_w = img_array.shape[:2]

        # Create output SDF atlas (grayscale)
        sdf_atlas = np.full((atlas_h, atlas_w), 128, dtype=np.uint8)  # 128 = distance 0

        # Process each frame
        for frame in frames:
            x, y, w, h = frame['x'], frame['y'], frame['w'], frame['h']

            # Skip invalid frames
            if w <= 0 or h <= 0:
                continue
            if x < 0 or y < 0 or x + w > atlas_w or y + h > atlas_h:
                continue

            # Extract alpha channel for this frame
            alpha = img_array[y:y+h, x:x+w, 3]

            # Generate SDF for this frame
            sdf = generate_sdf_jfa(alpha, max_dist)

            # Encode to uint8
            encoded = encode_sdf_to_uint8(sdf, max_dist)

            # Place in atlas at same position
            sdf_atlas[y:y+h, x:x+w] = encoded

        # Save as grayscale PNG
        sdf_img = Image.fromarray(sdf_atlas, mode='L')
        sdf_img.save(output_path, optimize=True)

        if verbose:
            print(f"    Generated SDF atlas: {output_path.name} ({len(frames)} frames)")

        return True

    except Exception as e:
        print(f"  Error generating SDF for {spritesheet_path}: {e}")
        return False


# =============================================================================
# Build Stats and Frame classes
# =============================================================================

@dataclass
class BuildStats:
    """Track build statistics."""
    units_found: int = 0
    units_processed: int = 0
    units_skipped: int = 0
    fx_found: int = 0
    fx_processed: int = 0
    fx_skipped: int = 0
    tiles_found: int = 0
    tiles_copied: int = 0
    tiles_skipped: int = 0
    files_copied: int = 0
    files_skipped: int = 0
    sdf_generated: int = 0
    sdf_skipped: int = 0
    sdf_failed: int = 0
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


def needs_regeneration(src: Path, derived: Path) -> bool:
    """Check if a derived file needs regeneration based on source mtime.

    Unlike needs_copy, this only checks timestamps since derived files
    (like SDF atlases) will have different sizes than their sources.
    """
    if not derived.exists():
        return True

    # Regenerate if source is newer than derived file
    return src.stat().st_mtime > derived.stat().st_mtime


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
    (dist_dir / 'resources' / 'tiles').mkdir(parents=True, exist_ok=True)

    # Initialize assets manifest
    assets: dict[str, Any] = {
        'units': {},
        'fx': {},
        'tiles': {},
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

            # Generate SDF atlas for this unit
            sdf_path = dist_dir / 'resources' / 'units' / f'{unit_name}_sdf.png'
            sdf_generated = False

            # Check if SDF needs regeneration based on source PNG timestamp
            # (not dest_png, and using mtime-only check since SDF has different size)
            needs_sdf = force or needs_regeneration(png_path, sdf_path)

            if needs_sdf and animations:
                # Collect all unique frames from all animations
                all_frames = []
                seen_rects = set()
                for anim in animations.values():
                    for frame in anim.frames:
                        rect_key = (frame.x, frame.y, frame.w, frame.h)
                        if rect_key not in seen_rects:
                            seen_rects.add(rect_key)
                            all_frames.append({
                                'x': frame.x, 'y': frame.y,
                                'w': frame.w, 'h': frame.h
                            })

                if all_frames:
                    if generate_sdf_atlas(png_path, all_frames, sdf_path, verbose=verbose):
                        stats.sdf_generated += 1
                        sdf_generated = True
                    else:
                        stats.sdf_failed += 1
            elif sdf_path.exists():
                stats.sdf_skipped += 1
                sdf_generated = True  # Already exists and up-to-date

            # Add to assets manifest
            unit_entry = {
                'spritesheet': f'resources/units/{unit_name}.png',
                'animations': {
                    name: animation_to_dict(anim)
                    for name, anim in animations.items()
                }
            }
            if sdf_generated:
                unit_entry['sdf_atlas'] = f'resources/units/{unit_name}_sdf.png'

            assets['units'][unit_name] = unit_entry

            stats.units_processed += 1

        copied = stats.units_processed - stats.units_skipped
        print(f"  {stats.units_found} units: {copied} copied, {stats.units_skipped} up-to-date")
        print(f"  SDF: {stats.sdf_generated} generated, {stats.sdf_skipped} up-to-date, {stats.sdf_failed} failed")

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

    # ===== Process Tiles =====
    tiles_dir = resources_dir / 'tiles'
    if tiles_dir.exists():
        print("Processing tiles...")
        # Get all PNG files, excluding @2x variants (we use 1x for now)
        png_files = sorted([p for p in tiles_dir.glob('*.png') if '@2x' not in p.name])
        stats.tiles_found = len(png_files)

        # Tiles we need for the grid system
        # Corner sprites: tile_merged_large_* (for move blob)
        # Hover corners: tile_merged_hover_* (for hover state)
        # Path sprites: tile_path_move_* (for movement path)
        # Utility: tile_hover, tile_glow, tile_attack, tile_grid

        for png_path in png_files:
            tile_name = png_path.stem
            dest_png = dist_dir / 'resources' / 'tiles' / f'{tile_name}.png'

            try:
                if copy_if_needed(png_path, dest_png, force):
                    stats.tiles_copied += 1
                    stats.files_copied += 1
                else:
                    stats.tiles_skipped += 1
                    stats.files_skipped += 1
            except Exception as e:
                print(f"  Error copying {png_path}: {e}")
                stats.errors += 1
                continue

            # Add to manifest
            assets['tiles'][tile_name] = f'resources/tiles/{tile_name}.png'

        print(f"  {stats.tiles_found} tiles: {stats.tiles_copied} copied, {stats.tiles_skipped} up-to-date")

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
    print(f"  Tiles: {stats.tiles_found} ({stats.tiles_skipped} unchanged)")
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
