#!/usr/bin/env python3
# /// script
# requires-python = ">=3.11"
# dependencies = ["polars", "rich"]
# ///
"""Get animation frame data and timing for units, FX, or specific animations.

Usage:
    uv run get_animation_frames.py <entity_type> <entity_name> [animation_name]

Arguments:
    entity_type:     'unit', 'fx', or 'all' (search all animations)
    entity_name:     Unit/FX identifier (e.g., 'neutral_sai', 'f1_general')
    animation_name:  Optional animation type (e.g., 'attack', 'idle', 'death')

Examples:
    uv run get_animation_frames.py unit neutral_sai attack
    uv run get_animation_frames.py unit f1_general
    uv run get_animation_frames.py all bloodtear
    uv run get_animation_frames.py fx fireexplosion
"""

import argparse
import sys
from pathlib import Path
from typing import Any

import polars as pl
from rich.console import Console
from rich.table import Table
from rich.panel import Panel


def get_data_dir() -> Path:
    """Get the duelyst_analysis directory path."""
    return Path(__file__).parent.parent


def load_animations_tsv() -> pl.DataFrame:
    """Load the animations TSV file."""
    anim_path = get_data_dir() / "instances" / "animations.tsv"
    return pl.read_csv(anim_path, separator="\t")


def load_units_tsv() -> pl.DataFrame:
    """Load the units TSV file for cross-reference."""
    units_path = get_data_dir() / "instances" / "units.tsv"
    return pl.read_csv(units_path, separator="\t")


def find_animations(
    anim_df: pl.DataFrame,
    entity_type: str,
    entity_name: str,
    animation_name: str | None = None
) -> pl.DataFrame | None:
    """Find animations matching the entity and optional animation type."""
    # Normalize search pattern
    pattern = entity_name.lower().replace("_", "")

    # Base filter on key/name columns
    matches = anim_df.filter(
        pl.col("key").str.to_lowercase().str.contains(pattern) |
        pl.col("name").str.to_lowercase().str.contains(pattern)
    )

    # Apply entity type filter
    if entity_type == "unit":
        # Units typically have patterns like f1General, neutralSai
        matches = matches.filter(
            ~pl.col("key").str.to_lowercase().str.starts_with("fx_")
        )
    elif entity_type == "fx":
        # FX animations start with fx_
        matches = matches.filter(
            pl.col("key").str.to_lowercase().str.starts_with("fx_") |
            pl.col("name").str.to_lowercase().str.starts_with("fx_")
        )

    # Filter by animation name if provided
    if animation_name:
        anim_pattern = animation_name.lower()
        matches = matches.filter(
            pl.col("key").str.to_lowercase().str.contains(anim_pattern) |
            pl.col("name").str.to_lowercase().str.contains(anim_pattern)
        )

    return matches if matches.height > 0 else None


def find_unit_using_animation(units_df: pl.DataFrame, anim_key: str) -> list[dict[str, Any]]:
    """Find units that use a particular animation."""
    matches = units_df.filter(
        pl.col("sprite_resource").fill_null("").str.contains(anim_key)
    )

    results = []
    for row in matches.iter_rows(named=True):
        results.append({
            "id": row.get("id", "N/A"),
            "name": row.get("name", "N/A"),
            "faction": row.get("faction_id", "N/A")
        })
    return results


def calculate_animation_stats(frame_delay: float) -> dict[str, Any]:
    """Calculate animation timing statistics."""
    fps = 1.0 / frame_delay if frame_delay > 0 else 0

    # Common frame counts for different animation types
    typical_frames = {
        "idle": (4, 8),
        "breathing": (6, 12),
        "attack": (8, 16),
        "run": (6, 10),
        "death": (10, 20),
        "cast": (8, 12),
        "hit": (4, 8),
    }

    return {
        "fps": fps,
        "typical_frames": typical_frames,
        "frame_delay_ms": frame_delay * 1000
    }


def main() -> int:
    parser = argparse.ArgumentParser(
        description="Get animation frame data and timing",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog=__doc__
    )
    parser.add_argument(
        "entity_type",
        choices=["unit", "fx", "all"],
        help="Entity type to search (unit, fx, or all)"
    )
    parser.add_argument(
        "entity_name",
        help="Entity identifier (e.g., 'neutral_sai', 'f1_general')"
    )
    parser.add_argument(
        "animation_name",
        nargs="?",
        default=None,
        help="Optional animation type (e.g., 'attack', 'idle')"
    )

    args = parser.parse_args()
    console = Console(stderr=True)
    stdout = Console()

    # Load data
    try:
        anim_df = load_animations_tsv()
        units_df = load_units_tsv()
    except Exception as e:
        console.print(f"[red]Error loading TSV files: {e}[/red]")
        return 1

    # Find matching animations
    matches = find_animations(
        anim_df,
        args.entity_type,
        args.entity_name,
        args.animation_name
    )

    if matches is None:
        filter_desc = f"'{args.entity_name}'"
        if args.animation_name:
            filter_desc += f" with animation '{args.animation_name}'"
        console.print(f"[red]No animations found for {args.entity_type} {filter_desc}[/red]")
        return 1

    stdout.print(f"\n[bold]Found {matches.height} animation(s)[/bold]\n")

    for row in matches.iter_rows(named=True):
        anim_key = row.get("key", "N/A")
        stdout.print(Panel(f"[bold cyan]{anim_key}[/bold cyan]", expand=False))

        # Frame data table
        frame_table = Table(title="Frame Configuration", show_header=True)
        frame_table.add_column("Property", style="cyan")
        frame_table.add_column("Value", style="green")

        frame_delay = float(row.get("frame_delay", 0))
        stats = calculate_animation_stats(frame_delay)

        frame_table.add_row("Animation Name", row.get("name", "N/A"))
        frame_table.add_row("Frame Prefix", row.get("frame_prefix", "N/A"))
        frame_table.add_row("Frame Delay (s)", f"{frame_delay:.3f}")
        frame_table.add_row("Frame Delay (ms)", f"{stats['frame_delay_ms']:.1f}")
        frame_table.add_row("FPS", f"{stats['fps']:.1f}")
        frame_table.add_row("Is 16-bit", str(row.get("is_16bit", "N/A")))

        stdout.print(frame_table)
        stdout.print()

        # Asset paths
        asset_table = Table(title="Asset Paths", show_header=True)
        asset_table.add_column("Asset Type", style="cyan")
        asset_table.add_column("Path", style="yellow")

        asset_table.add_row("Sprite Sheet", row.get("img", "N/A"))
        asset_table.add_row("Plist File", row.get("plist", "N/A"))

        stdout.print(asset_table)
        stdout.print()

        # Find units using this animation
        units = find_unit_using_animation(units_df, anim_key)
        if units:
            units_table = Table(title="Units Using This Animation", show_header=True)
            units_table.add_column("Unit ID", style="green")
            units_table.add_column("Name", style="white")
            units_table.add_column("Faction", style="blue")

            for unit in units[:5]:  # Limit display
                units_table.add_row(
                    unit["id"],
                    unit["name"],
                    str(unit["faction"])
                )

            if len(units) > 5:
                units_table.add_row("...", f"({len(units) - 5} more)", "")

            stdout.print(units_table)
            stdout.print()

        # Timing estimation
        timing_table = Table(title="Duration Estimates", show_header=True)
        timing_table.add_column("Frame Count", style="cyan")
        timing_table.add_column("Duration (s)", style="yellow")
        timing_table.add_column("Duration (ms)", style="yellow")

        for frames in [4, 6, 8, 10, 12, 16, 20]:
            duration = frames * frame_delay
            timing_table.add_row(
                str(frames),
                f"{duration:.3f}",
                f"{duration * 1000:.0f}"
            )

        stdout.print(timing_table)
        stdout.print()

    return 0


if __name__ == "__main__":
    sys.exit(main())
