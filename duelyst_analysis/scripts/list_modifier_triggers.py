#!/usr/bin/env python3
# /// script
# requires-python = ">=3.11"
# dependencies = ["polars", "rich"]
# ///
"""List all modifiers with a specific trigger type.

Usage:
    uv run list_modifier_triggers.py <trigger_type>

Example:
    uv run list_modifier_triggers.py ModifierStartTurnWatch
    uv run list_modifier_triggers.py onStartTurn
    uv run list_modifier_triggers.py DyingWish
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


def load_modifiers_tsv() -> pl.DataFrame:
    """Load the modifiers TSV file."""
    modifiers_path = get_data_dir() / "instances" / "modifiers.tsv"
    return pl.read_csv(modifiers_path, separator="\t")


def find_modifiers_by_trigger(modifiers_df: pl.DataFrame, trigger_type: str) -> pl.DataFrame:
    """Find modifiers with the specified trigger type."""
    pattern = trigger_type.lower()

    # Search in triggers column and class_name column
    matches = modifiers_df.filter(
        pl.col("triggers").fill_null("").str.to_lowercase().str.contains(pattern) |
        pl.col("class_name").str.to_lowercase().str.contains(pattern) |
        pl.col("type").str.to_lowercase().str.contains(pattern)
    )

    return matches


def get_trigger_types(modifiers_df: pl.DataFrame) -> list[str]:
    """Get all unique trigger types."""
    triggers = set()

    for row in modifiers_df.iter_rows(named=True):
        trigger_str = row.get("triggers", "")
        if trigger_str and trigger_str != "null":
            for trigger in trigger_str.split(", "):
                triggers.add(trigger.strip())

    return sorted(triggers)


def main() -> int:
    parser = argparse.ArgumentParser(
        description="List all modifiers with a specific trigger type",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog=__doc__
    )
    parser.add_argument(
        "trigger_type",
        nargs="?",
        help="Trigger type to search for (e.g., 'onStartTurn', 'DyingWish'). If omitted, lists all trigger types."
    )
    parser.add_argument(
        "--list-triggers", "-l",
        action="store_true",
        help="List all available trigger types"
    )
    parser.add_argument(
        "--keywords-only", "-k",
        action="store_true",
        help="Only show keyword modifiers"
    )
    parser.add_argument(
        "--watch-only", "-w",
        action="store_true",
        help="Only show watch modifiers"
    )

    args = parser.parse_args()
    console = Console(stderr=True)
    stdout = Console()

    # Load data
    try:
        modifiers_df = load_modifiers_tsv()
    except Exception as e:
        console.print(f"[red]Error loading modifiers.tsv: {e}[/red]")
        return 1

    # List all trigger types if requested
    if args.list_triggers or args.trigger_type is None:
        triggers = get_trigger_types(modifiers_df)

        stdout.print(Panel("[bold cyan]Available Trigger Types[/bold cyan]", expand=False))

        triggers_table = Table(show_header=True)
        triggers_table.add_column("Trigger", style="cyan")
        triggers_table.add_column("Count", style="green")

        for trigger in triggers:
            count = modifiers_df.filter(
                pl.col("triggers").fill_null("").str.contains(trigger)
            ).height
            triggers_table.add_row(trigger, str(count))

        stdout.print(triggers_table)

        # Also show common modifier patterns
        stdout.print("\n[bold]Common Modifier Patterns:[/bold]")
        patterns = [
            ("Watch modifiers", "Watch"),
            ("Opening Gambit", "OpeningGambit"),
            ("Dying Wish", "DyingWish"),
            ("Keywords", "is_keyword = TRUE"),
        ]

        for name, pattern in patterns:
            if pattern == "is_keyword = TRUE":
                count = modifiers_df.filter(pl.col("is_keyword") == "TRUE").height
            else:
                count = modifiers_df.filter(
                    pl.col("class_name").str.contains(pattern)
                ).height
            stdout.print(f"  {name}: {count}")

        return 0

    # Find modifiers with the trigger type
    matches = find_modifiers_by_trigger(modifiers_df, args.trigger_type)

    # Apply filters
    if args.keywords_only:
        matches = matches.filter(pl.col("is_keyword") == "TRUE")

    if args.watch_only:
        matches = matches.filter(pl.col("is_watch") == "TRUE")

    if matches.height == 0:
        console.print(f"[red]No modifiers found with trigger type '{args.trigger_type}'[/red]")
        return 1

    stdout.print(Panel(f"[bold cyan]Modifiers with trigger: {args.trigger_type}[/bold cyan]", expand=False))
    stdout.print(f"[dim]Found {matches.height} modifiers[/dim]\n")

    # Main results table
    results_table = Table(show_header=True, show_lines=True)
    results_table.add_column("Class Name", style="cyan", width=35)
    results_table.add_column("Type", style="green", width=15)
    results_table.add_column("Triggers", style="yellow", width=30)
    results_table.add_column("Keyword", style="magenta", width=8)
    results_table.add_column("Watch", style="blue", width=8)

    for row in matches.iter_rows(named=True):
        results_table.add_row(
            row.get("class_name", ""),
            row.get("type", ""),
            row.get("triggers", "N/A"),
            "Yes" if row.get("is_keyword") == "TRUE" else "No",
            "Yes" if row.get("is_watch") == "TRUE" else "No"
        )

    stdout.print(results_table)

    # Details for first few
    stdout.print("\n[bold]Modifier Details (first 5):[/bold]\n")

    for i, row in enumerate(matches.head(5).iter_rows(named=True)):
        detail_table = Table(title=row.get("class_name", "Unknown"), show_header=True)
        detail_table.add_column("Property", style="cyan")
        detail_table.add_column("Value", style="green")

        detail_table.add_row("File", row.get("file_name", "N/A"))
        detail_table.add_row("Parent Class", row.get("parent_class", "N/A"))
        detail_table.add_row("Applied Name", str(row.get("applied_name", "N/A")))
        detail_table.add_row("Description", str(row.get("applied_description", "N/A"))[:60])
        detail_table.add_row("Effects", str(row.get("effects", "N/A")))
        detail_table.add_row("Stack Type", str(row.get("stack_type", "N/A")))

        stdout.print(detail_table)
        stdout.print()

    return 0


if __name__ == "__main__":
    sys.exit(main())
