#!/usr/bin/env python3
# /// script
# requires-python = ">=3.11"
# dependencies = ["polars", "rich"]
# ///
"""Get unit attack/animation timing information.

Usage:
    uv run get_unit_timing.py <unit_folder>

Example:
    uv run get_unit_timing.py neutral_sai
    uv run get_unit_timing.py f1_general
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


def load_units_tsv() -> pl.DataFrame:
    """Load the units TSV file."""
    units_path = get_data_dir() / "instances" / "units.tsv"
    return pl.read_csv(units_path, separator="\t")


def load_constants_tsv() -> pl.DataFrame:
    """Load the constants TSV file."""
    constants_path = get_data_dir() / "semantic" / "constants.tsv"
    return pl.read_csv(constants_path, separator="\t")


def find_unit_by_folder(units_df: pl.DataFrame, unit_folder: str) -> pl.DataFrame | None:
    """Find unit(s) matching the folder name pattern."""
    # The sprite_resource column contains animation names like "f1GeneralBreathing|f1GeneralIdle|..."
    # We look for patterns matching the unit folder
    pattern = unit_folder.lower().replace("_", "")

    # Filter units where sprite_resource contains the pattern (case-insensitive)
    matches = units_df.filter(
        pl.col("sprite_resource").str.to_lowercase().str.contains(pattern)
    )

    if matches.height == 0:
        # Also try matching on id column
        matches = units_df.filter(
            pl.col("id").str.to_lowercase().str.contains(pattern.replace("neutral", "neutral.").replace("f", "faction"))
        )

    return matches if matches.height > 0 else None


def get_timing_constants(constants_df: pl.DataFrame) -> dict[str, Any]:
    """Extract animation timing constants."""
    timing_constants = {}

    timing_patterns = [
        "ATTACK", "ANIM", "MOVE", "FADE", "DELAY", "DURATION"
    ]

    for pattern in timing_patterns:
        matches = constants_df.filter(
            pl.col("constant_name").str.contains(pattern)
        )
        for row in matches.iter_rows(named=True):
            timing_constants[row["constant_name"]] = row["value"]

    return timing_constants


def parse_sprite_resources(sprite_resource: str) -> dict[str, str]:
    """Parse sprite resource string into animation types."""
    if not sprite_resource or sprite_resource == "null":
        return {}

    animations = sprite_resource.split("|")
    result = {}

    anim_types = [
        "Breathing", "Idle", "Run", "Attack", "Hit", "Damage", "Death",
        "CastStart", "CastEnd", "CastLoop", "Cast"
    ]

    for anim in animations:
        for anim_type in anim_types:
            if anim_type.lower() in anim.lower():
                result[anim_type] = anim
                break
        else:
            result["Other"] = result.get("Other", "") + anim + " "

    return result


def main() -> int:
    parser = argparse.ArgumentParser(
        description="Get unit attack/animation timing information",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog=__doc__
    )
    parser.add_argument(
        "unit_folder",
        help="Unit folder name (e.g., 'neutral_sai', 'f1_general')"
    )

    args = parser.parse_args()
    console = Console(stderr=True)
    stdout = Console()

    # Load data
    try:
        units_df = load_units_tsv()
        constants_df = load_constants_tsv()
    except Exception as e:
        console.print(f"[red]Error loading TSV files: {e}[/red]")
        return 1

    # Find matching units
    matches = find_unit_by_folder(units_df, args.unit_folder)

    if matches is None:
        console.print(f"[red]No unit found matching '{args.unit_folder}'[/red]")
        return 1

    # Get timing constants
    timing_constants = get_timing_constants(constants_df)

    # Output results
    for row in matches.iter_rows(named=True):
        stdout.print(Panel(f"[bold cyan]Unit: {row['id']}[/bold cyan]", expand=False))

        # Basic stats
        stats_table = Table(title="Unit Stats", show_header=True)
        stats_table.add_column("Property", style="cyan")
        stats_table.add_column("Value", style="green")

        stats_table.add_row("Name", row.get("name", "N/A"))
        stats_table.add_row("Attack", str(row.get("attack", "N/A")))
        stats_table.add_row("HP", str(row.get("hp", "N/A")))
        stats_table.add_row("Cost", str(row.get("cost", "N/A")))
        stats_table.add_row("Is General", str(row.get("is_general", "N/A")))

        stdout.print(stats_table)
        stdout.print()

        # Animation resources
        sprite_resource = row.get("sprite_resource", "")
        animations = parse_sprite_resources(sprite_resource)

        if animations:
            anim_table = Table(title="Animation Resources", show_header=True)
            anim_table.add_column("Animation Type", style="cyan")
            anim_table.add_column("Resource", style="yellow")

            for anim_type, resource in animations.items():
                anim_table.add_row(anim_type, resource.strip())

            stdout.print(anim_table)
            stdout.print()

        # Sound resources
        sound_resource = row.get("sound_resource", "")
        if sound_resource and sound_resource != "null":
            sounds = sound_resource.split("|")
            sound_table = Table(title="Sound Resources", show_header=True)
            sound_table.add_column("Index", style="cyan")
            sound_table.add_column("Sound", style="magenta")

            sound_types = ["Deploy", "Walk", "Attack Swing", "Hit", "Attack Impact", "Death"]
            for i, sound in enumerate(sounds):
                label = sound_types[i] if i < len(sound_types) else f"Sound {i}"
                sound_table.add_row(label, sound)

            stdout.print(sound_table)
            stdout.print()

    # Timing constants
    timing_table = Table(title="Relevant Timing Constants", show_header=True)
    timing_table.add_column("Constant", style="cyan")
    timing_table.add_column("Value", style="green")

    key_constants = [
        "CONFIG.ANIMATE_FAST_DURATION",
        "CONFIG.ANIMATE_MEDIUM_DURATION",
        "CONFIG.ANIMATE_SLOW_DURATION",
        "CONFIG.ACTION_DELAY",
        "CONFIG.FADE_SLOW_DURATION",
        "CONFIG.FADE_MEDIUM_DURATION",
        "CONFIG.FADE_FAST_DURATION",
        "CONFIG.ENTITY_ATTACK_DELAY",
        "CONFIG.ENTITY_ATTACK_RELEASE_DELAY",
    ]

    for const in key_constants:
        if const in timing_constants:
            timing_table.add_row(const, str(timing_constants[const]))

    if timing_table.row_count > 0:
        stdout.print(timing_table)

    return 0


if __name__ == "__main__":
    sys.exit(main())
