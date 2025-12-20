#!/usr/bin/env python3
# /// script
# requires-python = ">=3.11"
# dependencies = ["polars", "rich"]
# ///
"""Analyze card stat distributions and flag balance outliers.

Usage:
    uv run validate_card_balance.py [--faction FACTION] [--rarity RARITY]

Example:
    uv run validate_card_balance.py
    uv run validate_card_balance.py --faction Faction1
    uv run validate_card_balance.py --rarity Legendary
    uv run validate_card_balance.py --check-efficiency
"""

import argparse
import sys
from pathlib import Path
from dataclasses import dataclass

import polars as pl
from rich.console import Console
from rich.table import Table
from rich.panel import Panel


@dataclass
class StatSummary:
    """Summary statistics for a numeric column."""
    mean: float
    std: float
    min: float
    max: float
    median: float


def get_data_dir() -> Path:
    """Get the duelyst_analysis directory path."""
    return Path(__file__).parent.parent


def load_units_tsv() -> pl.DataFrame:
    """Load the units TSV file."""
    units_path = get_data_dir() / "instances" / "units.tsv"
    return pl.read_csv(units_path, separator="\t")


def load_cards_tsv() -> pl.DataFrame:
    """Load the cards TSV file."""
    cards_path = get_data_dir() / "instances" / "cards.tsv"
    return pl.read_csv(cards_path, separator="\t")


def get_stat_summary(df: pl.DataFrame, column: str) -> StatSummary:
    """Get summary statistics for a column."""
    col = df[column].cast(pl.Float64)
    return StatSummary(
        mean=col.mean() or 0,
        std=col.std() or 0,
        min=col.min() or 0,
        max=col.max() or 0,
        median=col.median() or 0
    )


def find_outliers(df: pl.DataFrame, column: str, std_threshold: float = 2.0) -> pl.DataFrame:
    """Find rows where column value is > std_threshold standard deviations from mean."""
    col = df[column].cast(pl.Float64)
    mean = col.mean() or 0
    std = col.std() or 1

    return df.filter(
        ((pl.col(column).cast(pl.Float64) - mean).abs() / std) > std_threshold
    )


def calculate_efficiency(row: dict) -> float:
    """Calculate stats-to-cost efficiency ratio."""
    attack = float(row.get("attack", 0) or 0)
    hp = float(row.get("hp", 0) or 0)
    cost = float(row.get("cost", 1) or 1)

    # Simple efficiency: (attack + hp) / cost
    # Higher = more efficient
    if cost <= 0:
        cost = 1
    return (attack + hp) / cost


def get_mana_curve(df: pl.DataFrame) -> dict[int, int]:
    """Get count of cards at each mana cost."""
    curve = {}
    for cost in range(10):
        count = df.filter(pl.col("cost").cast(pl.Int64) == cost).height
        if count > 0:
            curve[cost] = count
    return curve


def main() -> int:
    parser = argparse.ArgumentParser(
        description="Analyze card stat distributions and flag balance outliers",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog=__doc__
    )
    parser.add_argument(
        "--faction", "-f",
        help="Filter by faction (e.g., 'Faction1', 'Neutral')"
    )
    parser.add_argument(
        "--rarity", "-r",
        help="Filter by rarity (e.g., 'Legendary', 'Common')"
    )
    parser.add_argument(
        "--std-threshold", "-t",
        type=float,
        default=2.0,
        help="Standard deviation threshold for outliers (default: 2.0)"
    )
    parser.add_argument(
        "--check-efficiency", "-e",
        action="store_true",
        help="Show efficiency analysis (stats-to-cost ratio)"
    )
    parser.add_argument(
        "--mana-curve", "-m",
        action="store_true",
        help="Show mana curve analysis"
    )
    parser.add_argument(
        "--show-all", "-a",
        action="store_true",
        help="Show all analyses"
    )

    args = parser.parse_args()
    console = Console(stderr=True)
    stdout = Console()

    # Load data
    try:
        units_df = load_units_tsv()
    except Exception as e:
        console.print(f"[red]Error loading units.tsv: {e}[/red]")
        return 1

    # Filter out generals and token units
    units_df = units_df.filter(
        (pl.col("is_general") != "true") &
        (pl.col("is_general") != "TRUE") &
        (~pl.col("rarity_id").str.contains("Token"))
    )

    original_count = units_df.height

    # Apply filters
    if args.faction:
        faction_pattern = args.faction.lower()
        units_df = units_df.filter(
            pl.col("faction_id").str.to_lowercase().str.contains(faction_pattern)
        )

    if args.rarity:
        rarity_pattern = args.rarity.lower()
        units_df = units_df.filter(
            pl.col("rarity_id").str.to_lowercase().str.contains(rarity_pattern)
        )

    filtered_count = units_df.height

    # Header
    filter_desc = ""
    if args.faction:
        filter_desc += f" [Faction: {args.faction}]"
    if args.rarity:
        filter_desc += f" [Rarity: {args.rarity}]"

    stdout.print(Panel(f"[bold cyan]Card Balance Analysis{filter_desc}[/bold cyan]", expand=False))
    stdout.print(f"[dim]Analyzing {filtered_count} units (of {original_count} total, excluding generals/tokens)[/dim]\n")

    if filtered_count == 0:
        console.print("[red]No units match the filter criteria[/red]")
        return 1

    # Stat distributions
    stdout.print("[bold]Stat Distributions:[/bold]\n")

    stats_table = Table(show_header=True)
    stats_table.add_column("Stat", style="cyan")
    stats_table.add_column("Mean", style="green")
    stats_table.add_column("Std Dev", style="yellow")
    stats_table.add_column("Min", style="blue")
    stats_table.add_column("Max", style="red")
    stats_table.add_column("Median", style="magenta")

    for stat in ["attack", "hp", "cost"]:
        summary = get_stat_summary(units_df, stat)
        stats_table.add_row(
            stat.capitalize(),
            f"{summary.mean:.2f}",
            f"{summary.std:.2f}",
            f"{summary.min:.0f}",
            f"{summary.max:.0f}",
            f"{summary.median:.0f}"
        )

    stdout.print(stats_table)

    # Outliers
    stdout.print(f"\n[bold]Stat Outliers (>{args.std_threshold} std devs from mean):[/bold]\n")

    for stat in ["attack", "hp", "cost"]:
        outliers = find_outliers(units_df, stat, args.std_threshold)

        if outliers.height > 0:
            stdout.print(f"[yellow]{stat.capitalize()} Outliers ({outliers.height}):[/yellow]")

            outlier_table = Table(show_header=True)
            outlier_table.add_column("Card ID", style="cyan", width=40)
            outlier_table.add_column("ATK", style="green")
            outlier_table.add_column("HP", style="green")
            outlier_table.add_column("Cost", style="yellow")
            outlier_table.add_column("Faction", style="blue")

            for row in outliers.head(10).iter_rows(named=True):
                outlier_table.add_row(
                    str(row.get("id", ""))[:40],
                    str(row.get("attack", "")),
                    str(row.get("hp", "")),
                    str(row.get("cost", "")),
                    str(row.get("faction_id", "")).replace("Factions.", "")
                )

            stdout.print(outlier_table)

            if outliers.height > 10:
                stdout.print(f"[dim]... and {outliers.height - 10} more[/dim]")
            stdout.print()

    # Efficiency analysis
    if args.check_efficiency or args.show_all:
        stdout.print("[bold]Efficiency Analysis (Stats-to-Cost Ratio):[/bold]\n")

        # Calculate efficiency for all units
        efficiencies = []
        for row in units_df.iter_rows(named=True):
            eff = calculate_efficiency(row)
            efficiencies.append({
                "id": row.get("id", ""),
                "attack": row.get("attack", 0),
                "hp": row.get("hp", 0),
                "cost": row.get("cost", 0),
                "faction_id": row.get("faction_id", ""),
                "efficiency": eff
            })

        # Sort by efficiency
        efficiencies.sort(key=lambda x: x["efficiency"], reverse=True)

        # Top 10 most efficient
        stdout.print("[green]Top 10 Most Efficient (ATK+HP)/Cost:[/green]")
        top_table = Table(show_header=True)
        top_table.add_column("Card ID", style="cyan", width=40)
        top_table.add_column("ATK", style="green")
        top_table.add_column("HP", style="green")
        top_table.add_column("Cost", style="yellow")
        top_table.add_column("Efficiency", style="magenta")

        for e in efficiencies[:10]:
            top_table.add_row(
                str(e["id"])[:40],
                str(e["attack"]),
                str(e["hp"]),
                str(e["cost"]),
                f"{e['efficiency']:.2f}"
            )

        stdout.print(top_table)

        # Bottom 10 least efficient
        stdout.print("\n[red]Bottom 10 Least Efficient:[/red]")
        bottom_table = Table(show_header=True)
        bottom_table.add_column("Card ID", style="cyan", width=40)
        bottom_table.add_column("ATK", style="green")
        bottom_table.add_column("HP", style="green")
        bottom_table.add_column("Cost", style="yellow")
        bottom_table.add_column("Efficiency", style="magenta")

        for e in efficiencies[-10:]:
            bottom_table.add_row(
                str(e["id"])[:40],
                str(e["attack"]),
                str(e["hp"]),
                str(e["cost"]),
                f"{e['efficiency']:.2f}"
            )

        stdout.print(bottom_table)
        stdout.print("\n[dim]Note: Low efficiency often indicates powerful abilities/keywords[/dim]")

    # Mana curve
    if args.mana_curve or args.show_all:
        stdout.print("\n[bold]Mana Curve Analysis:[/bold]\n")

        curve = get_mana_curve(units_df)

        curve_table = Table(show_header=True)
        curve_table.add_column("Cost", style="cyan")
        curve_table.add_column("Count", style="green")
        curve_table.add_column("Bar", style="yellow")

        max_count = max(curve.values()) if curve else 1

        for cost in range(10):
            count = curve.get(cost, 0)
            bar_len = int((count / max_count) * 30) if max_count > 0 else 0
            bar = "█" * bar_len
            curve_table.add_row(str(cost), str(count), bar)

        stdout.print(curve_table)

        # Curve analysis
        total = sum(curve.values())
        low_cost = sum(curve.get(c, 0) for c in range(3))
        mid_cost = sum(curve.get(c, 0) for c in range(3, 6))
        high_cost = sum(curve.get(c, 0) for c in range(6, 10))

        stdout.print(f"\n[dim]Distribution: Low (0-2): {low_cost} ({100*low_cost/total:.1f}%) | "
                     f"Mid (3-5): {mid_cost} ({100*mid_cost/total:.1f}%) | "
                     f"High (6+): {high_cost} ({100*high_cost/total:.1f}%)[/dim]")

    # Faction comparison
    if args.show_all and not args.faction:
        stdout.print("\n[bold]Faction Comparison:[/bold]\n")

        faction_table = Table(show_header=True)
        faction_table.add_column("Faction", style="cyan")
        faction_table.add_column("Units", style="green")
        faction_table.add_column("Avg ATK", style="yellow")
        faction_table.add_column("Avg HP", style="yellow")
        faction_table.add_column("Avg Cost", style="yellow")
        faction_table.add_column("Avg Eff", style="magenta")

        # Load original unfiltered data
        full_units = load_units_tsv()
        full_units = full_units.filter(
            (pl.col("is_general") != "true") &
            (pl.col("is_general") != "TRUE") &
            (~pl.col("rarity_id").str.contains("Token"))
        )

        factions = sorted(full_units["faction_id"].unique().to_list())

        for faction in factions:
            if not faction:
                continue

            faction_df = full_units.filter(pl.col("faction_id") == faction)
            count = faction_df.height

            if count == 0:
                continue

            avg_atk = faction_df["attack"].cast(pl.Float64).mean() or 0
            avg_hp = faction_df["hp"].cast(pl.Float64).mean() or 0
            avg_cost = faction_df["cost"].cast(pl.Float64).mean() or 0

            # Calculate average efficiency
            total_eff = 0
            for row in faction_df.iter_rows(named=True):
                total_eff += calculate_efficiency(row)
            avg_eff = total_eff / count if count > 0 else 0

            faction_table.add_row(
                faction.replace("Factions.", ""),
                str(count),
                f"{avg_atk:.2f}",
                f"{avg_hp:.2f}",
                f"{avg_cost:.2f}",
                f"{avg_eff:.2f}"
            )

        stdout.print(faction_table)

    return 0


if __name__ == "__main__":
    sys.exit(main())
