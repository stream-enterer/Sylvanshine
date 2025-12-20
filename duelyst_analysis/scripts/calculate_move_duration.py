#!/usr/bin/env python3
# /// script
# requires-python = ">=3.11"
# dependencies = ["polars", "rich"]
# ///
"""Calculate movement timing for a unit.

Uses the movement formula from system documentation:
- Base duration = walk_animation_duration * tile_count * ENTITY_MOVE_DURATION_MODIFIER
- Correction = base_duration * ENTITY_MOVE_CORRECTION
- Final duration = max(0, base_duration - correction)
- Flying units cap at ENTITY_MOVE_FLYING_FIXED_TILE_COUNT tiles for animation

Usage:
    uv run calculate_move_duration.py <unit_folder> <tile_count>

Example:
    uv run calculate_move_duration.py f1_general 3
    uv run calculate_move_duration.py neutral_sai 2
"""

import argparse
import sys
from pathlib import Path

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


def get_timing_constants(constants_df: pl.DataFrame) -> dict[str, float]:
    """Extract movement timing constants."""
    constants = {
        "ENTITY_MOVE_DURATION_MODIFIER": 1.0,
        "ENTITY_MOVE_MODIFIER_MAX": 1.0,
        "ENTITY_MOVE_MODIFIER_MIN": 0.75,
        "ENTITY_MOVE_CORRECTION": 0.2,
        "ENTITY_MOVE_FLYING_FIXED_TILE_COUNT": 3.0,
        "PATH_MOVE_DURATION": 1.5,
        "MOVE_FAST_DURATION": 0.15,
        "MOVE_MEDIUM_DURATION": 0.35,
    }

    for row in constants_df.iter_rows(named=True):
        name = row.get("constant_name", "")
        value = row.get("value", "")

        for key in constants.keys():
            if key in name:
                try:
                    constants[key] = float(value)
                except (ValueError, TypeError):
                    pass

    return constants


def find_unit_by_folder(units_df: pl.DataFrame, unit_folder: str) -> pl.DataFrame | None:
    """Find unit(s) matching the folder name pattern."""
    pattern = unit_folder.lower().replace("_", "")

    matches = units_df.filter(
        pl.col("sprite_resource").str.to_lowercase().str.contains(pattern)
    )

    if matches.height == 0:
        matches = units_df.filter(
            pl.col("id").str.to_lowercase().str.contains(pattern)
        )

    return matches if matches.height > 0 else None


def has_flying(unit_row: dict) -> bool:
    """Check if unit has flying modifier."""
    abilities = unit_row.get("abilities", "") or ""
    return "ModifierFlying" in abilities or "Transcendance" in abilities


def calculate_movement_duration(
    tile_count: int,
    walk_anim_duration: float,
    constants: dict[str, float],
    is_flying: bool = False
) -> dict[str, float]:
    """
    Calculate movement duration using the formula from EntityNode.js:394-414.

    getMoveDuration(moveTileCount) {
      const animMove = this.getAnimationActionFromAnimResource('walk');
      if (animMove != null) {
        let animMoveDuration = animMove.getDuration() * CONFIG.ENTITY_MOVE_DURATION_MODIFIER;
        let animMoveCorrection = animMoveDuration * CONFIG.ENTITY_MOVE_CORRECTION;
        for (let i = 0; i < moveTileCount; i++) {
          movementDuration += animMoveDuration;
        }
        movementDuration = Math.max(0.0, movementDuration - animMoveCorrection);
      }
      return movementDuration;
    }
    """
    effective_tile_count = tile_count

    # Flying units cap at fixed tile count for animation
    if is_flying:
        flying_cap = constants["ENTITY_MOVE_FLYING_FIXED_TILE_COUNT"]
        effective_tile_count = min(tile_count, int(flying_cap))

    # Calculate duration
    modifier = constants["ENTITY_MOVE_DURATION_MODIFIER"]
    correction_factor = constants["ENTITY_MOVE_CORRECTION"]

    anim_move_duration = walk_anim_duration * modifier
    anim_move_correction = anim_move_duration * correction_factor

    # Sum up duration for each tile
    movement_duration = anim_move_duration * effective_tile_count

    # Apply correction
    final_duration = max(0.0, movement_duration - anim_move_correction)

    return {
        "tile_count": tile_count,
        "effective_tile_count": effective_tile_count,
        "walk_anim_duration": walk_anim_duration,
        "modifier": modifier,
        "correction_factor": correction_factor,
        "anim_move_duration": anim_move_duration,
        "anim_move_correction": anim_move_correction,
        "base_duration": movement_duration,
        "final_duration": final_duration,
        "is_flying": is_flying
    }


def main() -> int:
    parser = argparse.ArgumentParser(
        description="Calculate movement timing for a unit",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog=__doc__
    )
    parser.add_argument(
        "unit_folder",
        help="Unit folder name (e.g., 'f1_general', 'neutral_sai')"
    )
    parser.add_argument(
        "tile_count",
        type=int,
        help="Number of tiles to move"
    )
    parser.add_argument(
        "--walk-duration", "-w",
        type=float,
        default=0.5,
        help="Walk animation duration in seconds (default: 0.5)"
    )
    parser.add_argument(
        "--flying", "-f",
        action="store_true",
        help="Force flying unit calculation"
    )

    args = parser.parse_args()
    console = Console(stderr=True)
    stdout = Console()

    if args.tile_count < 1:
        console.print("[red]Tile count must be at least 1[/red]")
        return 1

    # Load data
    try:
        units_df = load_units_tsv()
        constants_df = load_constants_tsv()
    except Exception as e:
        console.print(f"[red]Error loading TSV files: {e}[/red]")
        return 1

    # Get timing constants
    constants = get_timing_constants(constants_df)

    # Find the unit
    matches = find_unit_by_folder(units_df, args.unit_folder)

    unit_name = args.unit_folder
    is_flying = args.flying

    if matches is not None and matches.height > 0:
        unit_row = matches.row(0, named=True)
        unit_name = unit_row.get("id", args.unit_folder)
        is_flying = is_flying or has_flying(unit_row)
    else:
        console.print(f"[yellow]Unit '{args.unit_folder}' not found, using default values[/yellow]")

    # Calculate movement duration
    result = calculate_movement_duration(
        args.tile_count,
        args.walk_duration,
        constants,
        is_flying
    )

    stdout.print(Panel(f"[bold cyan]Movement Duration: {unit_name}[/bold cyan]", expand=False))

    # Input parameters
    input_table = Table(title="Input Parameters", show_header=True)
    input_table.add_column("Parameter", style="cyan")
    input_table.add_column("Value", style="green")

    input_table.add_row("Unit", unit_name)
    input_table.add_row("Tiles to Move", str(args.tile_count))
    input_table.add_row("Walk Animation Duration", f"{args.walk_duration}s")
    input_table.add_row("Is Flying", "Yes" if is_flying else "No")

    stdout.print(input_table)
    stdout.print()

    # Constants used
    const_table = Table(title="Timing Constants", show_header=True)
    const_table.add_column("Constant", style="cyan")
    const_table.add_column("Value", style="yellow")

    const_table.add_row("ENTITY_MOVE_DURATION_MODIFIER", str(constants["ENTITY_MOVE_DURATION_MODIFIER"]))
    const_table.add_row("ENTITY_MOVE_CORRECTION", str(constants["ENTITY_MOVE_CORRECTION"]))
    if is_flying:
        const_table.add_row("ENTITY_MOVE_FLYING_FIXED_TILE_COUNT", str(constants["ENTITY_MOVE_FLYING_FIXED_TILE_COUNT"]))

    stdout.print(const_table)
    stdout.print()

    # Calculation breakdown
    calc_table = Table(title="Calculation Breakdown", show_header=True)
    calc_table.add_column("Step", style="cyan")
    calc_table.add_column("Formula", style="yellow")
    calc_table.add_column("Result", style="green")

    calc_table.add_row(
        "1. Effective Tiles",
        f"min({args.tile_count}, flying_cap)" if is_flying else str(args.tile_count),
        str(result["effective_tile_count"])
    )
    calc_table.add_row(
        "2. Anim Duration",
        f"{args.walk_duration} * {result['modifier']}",
        f"{result['anim_move_duration']:.3f}s"
    )
    calc_table.add_row(
        "3. Correction",
        f"{result['anim_move_duration']:.3f} * {result['correction_factor']}",
        f"{result['anim_move_correction']:.3f}s"
    )
    calc_table.add_row(
        "4. Base Duration",
        f"{result['anim_move_duration']:.3f} * {result['effective_tile_count']}",
        f"{result['base_duration']:.3f}s"
    )
    calc_table.add_row(
        "5. Final Duration",
        f"max(0, {result['base_duration']:.3f} - {result['anim_move_correction']:.3f})",
        f"{result['final_duration']:.3f}s"
    )

    stdout.print(calc_table)
    stdout.print()

    # Final result
    stdout.print(Panel(
        f"[bold green]Movement Duration: {result['final_duration']:.3f} seconds[/bold green]\n"
        f"[dim]({result['final_duration'] * 1000:.0f} milliseconds)[/dim]",
        title="Result",
        expand=False
    ))

    # Formula reference
    stdout.print("\n[dim]Formula from EntityNode.js:394-414:[/dim]")
    stdout.print("[dim]duration = max(0, (walk_anim * modifier * tiles) - (walk_anim * modifier * correction))[/dim]")

    return 0


if __name__ == "__main__":
    sys.exit(main())
