#!/usr/bin/env python3
# /// script
# requires-python = ">=3.11"
# dependencies = ["polars", "rich"]
# ///
"""Get FX composition resolution - sprites, particles, sounds, timing.

Usage:
    uv run get_fx_composition.py <fx_identifier>

Example:
    uv run get_fx_composition.py Faction1.UnitSpawnFX
    uv run get_fx_composition.py fx_f1_divinebond
"""

import argparse
import sys
from pathlib import Path

import polars as pl
from rich.console import Console
from rich.table import Table
from rich.panel import Panel
from rich.tree import Tree


def get_data_dir() -> Path:
    """Get the duelyst_analysis directory path."""
    return Path(__file__).parent.parent


def load_fx_tsv() -> pl.DataFrame:
    """Load the FX TSV file."""
    fx_path = get_data_dir() / "instances" / "fx.tsv"
    return pl.read_csv(fx_path, separator="\t")


def load_units_tsv() -> pl.DataFrame:
    """Load the units TSV file for cross-reference."""
    units_path = get_data_dir() / "instances" / "units.tsv"
    return pl.read_csv(units_path, separator="\t")


def find_fx(fx_df: pl.DataFrame, fx_identifier: str) -> pl.DataFrame | None:
    """Find FX entries matching the identifier."""
    # Normalize the search pattern
    pattern = fx_identifier.lower().replace(".", "_").replace("fx_", "")

    # Search in rsx_name column
    matches = fx_df.filter(
        pl.col("rsx_name").str.to_lowercase().str.contains(pattern)
    )

    if matches.height == 0:
        # Try searching in category or usage_context
        matches = fx_df.filter(
            pl.col("category").str.to_lowercase().str.contains(pattern) |
            pl.col("usage_context").fill_null("").str.to_lowercase().str.contains(pattern)
        )

    return matches if matches.height > 0 else None


def find_cards_using_fx(units_df: pl.DataFrame, fx_name: str) -> list[str]:
    """Find cards that use a particular FX."""
    matches = units_df.filter(
        pl.col("fx_resource").fill_null("").str.contains(fx_name)
    )
    return matches["id"].to_list()


def main() -> int:
    parser = argparse.ArgumentParser(
        description="Get FX composition resolution",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog=__doc__
    )
    parser.add_argument(
        "fx_identifier",
        help="FX identifier (e.g., 'Faction1.UnitSpawnFX', 'fx_f1_divinebond')"
    )

    args = parser.parse_args()
    console = Console(stderr=True)
    stdout = Console()

    # Load data
    try:
        fx_df = load_fx_tsv()
        units_df = load_units_tsv()
    except Exception as e:
        console.print(f"[red]Error loading TSV files: {e}[/red]")
        return 1

    # Find matching FX
    matches = find_fx(fx_df, args.fx_identifier)

    if matches is None:
        console.print(f"[red]No FX found matching '{args.fx_identifier}'[/red]")
        return 1

    stdout.print(f"\n[bold]Found {matches.height} FX entries matching '{args.fx_identifier}'[/bold]\n")

    for row in matches.iter_rows(named=True):
        # Create FX panel
        stdout.print(Panel(f"[bold cyan]{row['rsx_name']}[/bold cyan]", expand=False))

        # Basic info table
        info_table = Table(title="FX Information", show_header=True)
        info_table.add_column("Property", style="cyan")
        info_table.add_column("Value", style="green")

        info_table.add_row("RSX Name", row.get("rsx_name", "N/A"))
        info_table.add_row("FX Type", row.get("fx_type", "N/A"))
        info_table.add_row("Category", row.get("category", "N/A"))
        info_table.add_row("File Path", row.get("file_path", "N/A"))

        stdout.print(info_table)
        stdout.print()

        # Sprite details
        sprite_table = Table(title="Sprite Details", show_header=True)
        sprite_table.add_column("Property", style="cyan")
        sprite_table.add_column("Value", style="yellow")

        sprite_table.add_row("Frame Prefix", row.get("frame_prefix", "N/A"))
        sprite_table.add_row("Frame Delay", str(row.get("frame_delay", "N/A")))
        sprite_table.add_row("Is 16-bit", str(row.get("is_16bit", "N/A")))
        sprite_table.add_row("Has Plist", row.get("has_plist", "N/A"))

        stdout.print(sprite_table)
        stdout.print()

        # Related sounds
        sounds = row.get("related_sounds", "")
        if sounds and sounds != "null" and str(sounds).strip():
            sound_table = Table(title="Related Sounds", show_header=True)
            sound_table.add_column("Sound", style="magenta")

            for sound in str(sounds).split("|"):
                if sound.strip():
                    sound_table.add_row(sound.strip())

            stdout.print(sound_table)
            stdout.print()

        # Usage context
        usage = row.get("usage_context", "")
        if usage and usage != "null" and str(usage).strip():
            usage_table = Table(title="Usage Context", show_header=True)
            usage_table.add_column("Context", style="blue")

            for ctx in str(usage).split(";"):
                if ctx.strip():
                    usage_table.add_row(ctx.strip())

            stdout.print(usage_table)
            stdout.print()

        # Cards using this FX
        fx_name = row.get("rsx_name", "")
        if fx_name:
            cards = find_cards_using_fx(units_df, fx_name)
            if cards:
                cards_table = Table(title="Cards Using This FX", show_header=True)
                cards_table.add_column("Card ID", style="green")

                for card in cards[:10]:  # Limit to 10
                    cards_table.add_row(card)

                if len(cards) > 10:
                    cards_table.add_row(f"... and {len(cards) - 10} more")

                stdout.print(cards_table)

        stdout.print()

    # Timing information
    stdout.print(Panel("[bold]FX Timing Notes[/bold]", expand=False))
    timing_tree = Tree("[cyan]Frame Timing Calculation[/cyan]")
    timing_tree.add("frame_delay: Seconds between animation frames")
    timing_tree.add("Total duration = frame_count * frame_delay")
    timing_tree.add("Standard frame_delay values: 0.04, 0.08, 0.1, 0.2")
    stdout.print(timing_tree)

    return 0


if __name__ == "__main__":
    sys.exit(main())
