#!/usr/bin/env python3
# /// script
# requires-python = ">=3.11"
# dependencies = ["polars", "rich"]
# ///
"""Look up localized text strings by key, namespace, or content search.

Searches the localization database and cross-references with cards/units.

Usage:
    uv run lookup_localization.py <query> [--namespace NAMESPACE]
    uv run lookup_localization.py --list-namespaces

Arguments:
    query:      Key name, partial key, or text to search for

Examples:
    uv run lookup_localization.py sundrop_elixir
    uv run lookup_localization.py "Holy Immolation" --namespace cards
    uv run lookup_localization.py zeal
    uv run lookup_localization.py --list-namespaces
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


def load_localization_tsv() -> pl.DataFrame:
    """Load the localization TSV file."""
    loc_path = get_data_dir() / "instances" / "localization.tsv"
    return pl.read_csv(loc_path, separator="\t")


def load_cards_tsv() -> pl.DataFrame:
    """Load the cards TSV file for cross-reference."""
    cards_path = get_data_dir() / "instances" / "cards.tsv"
    return pl.read_csv(cards_path, separator="\t")


def load_units_tsv() -> pl.DataFrame:
    """Load the units TSV file for cross-reference."""
    units_path = get_data_dir() / "instances" / "units.tsv"
    return pl.read_csv(units_path, separator="\t")


def find_localization(
    df: pl.DataFrame,
    query: str,
    namespace: str | None = None
) -> pl.DataFrame | None:
    """Find localization entries matching the query."""
    query_lower = query.lower()

    # Build filter conditions
    key_match = pl.col("key").str.to_lowercase().str.contains(query_lower)
    value_match = pl.col("value_en").str.to_lowercase().str.contains(query_lower)

    if namespace:
        matches = df.filter(
            (pl.col("namespace") == namespace) &
            (key_match | value_match)
        )
    else:
        matches = df.filter(key_match | value_match)

    return matches if matches.height > 0 else None


def get_namespace_stats(df: pl.DataFrame) -> dict[str, int]:
    """Get count of entries per namespace."""
    counts = df.group_by("namespace").count().sort("count", descending=True)
    return {row["namespace"]: row["count"] for row in counts.iter_rows(named=True)}


def find_card_by_localization_key(
    cards_df: pl.DataFrame,
    units_df: pl.DataFrame,
    key: str
) -> list[dict]:
    """Find cards that might use this localization key."""
    results = []

    # Extract card ID pattern from key
    # Keys often look like: card_name_12345, card_description_12345
    key_lower = key.lower()

    # Check if this looks like a card key
    if "card_" in key_lower or key_lower.startswith("spell_") or key_lower.startswith("artifact_"):
        # Try to find matching card
        pattern = key_lower.replace("card_name_", "").replace("card_description_", "")
        pattern = pattern.replace("spell_", "").replace("artifact_", "")

        # Search cards
        card_matches = cards_df.filter(
            pl.col("id").str.to_lowercase().str.contains(pattern)
        )
        for row in card_matches.iter_rows(named=True):
            results.append({
                "type": "card",
                "id": row.get("id", "N/A"),
                "name": row.get("name", "N/A")
            })

        # Search units
        unit_matches = units_df.filter(
            pl.col("id").str.to_lowercase().str.contains(pattern)
        )
        for row in unit_matches.iter_rows(named=True):
            results.append({
                "type": "unit",
                "id": row.get("id", "N/A"),
                "name": row.get("name", "N/A")
            })

    return results[:5]  # Limit results


def main() -> int:
    parser = argparse.ArgumentParser(
        description="Look up localized text strings",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog=__doc__
    )
    parser.add_argument(
        "query",
        nargs="?",
        default=None,
        help="Key name, partial key, or text to search for"
    )
    parser.add_argument(
        "--namespace", "-n",
        default=None,
        help="Filter by namespace (e.g., 'cards', 'modifiers')"
    )
    parser.add_argument(
        "--list-namespaces",
        action="store_true",
        help="List all available namespaces"
    )
    parser.add_argument(
        "--exact",
        action="store_true",
        help="Match key exactly instead of partial match"
    )

    args = parser.parse_args()
    console = Console(stderr=True)
    stdout = Console()

    # Load data
    try:
        loc_df = load_localization_tsv()
        cards_df = load_cards_tsv()
        units_df = load_units_tsv()
    except Exception as e:
        console.print(f"[red]Error loading TSV files: {e}[/red]")
        return 1

    # List namespaces
    if args.list_namespaces:
        stdout.print(Panel("[bold]Localization Namespaces[/bold]", expand=False))

        namespace_stats = get_namespace_stats(loc_df)

        ns_table = Table(show_header=True)
        ns_table.add_column("Namespace", style="cyan")
        ns_table.add_column("Entry Count", style="green", justify="right")
        ns_table.add_column("Description", style="dim")

        descriptions = {
            "cards": "Card names and descriptions",
            "cosmetics": "Skins, emotes, and cosmetic items",
            "modifiers": "Card ability and modifier text",
            "challenges": "Challenge mode text",
            "boss_battles": "Boss battle descriptions",
            "factions": "Faction names and lore",
            "shop": "Shop and purchase text",
            "tutorial": "Tutorial instructions",
            "common": "Common UI text",
            "game_tips": "Loading screen tips",
            "achievements": "Achievement names and descriptions",
            "collection": "Collection screen text",
            "settings": "Settings menu text",
            "main_menu": "Main menu text",
            "buddy_list": "Friends list text",
            "battle": "In-game battle text",
            "quests": "Quest descriptions",
            "login": "Login screen text",
            "profile": "Player profile text",
            "rift": "Rift mode text",
        }

        for ns, count in namespace_stats.items():
            desc = descriptions.get(ns, "")
            ns_table.add_row(ns, str(count), desc)

        stdout.print(ns_table)
        stdout.print()

        total = sum(namespace_stats.values())
        stdout.print(f"[dim]Total entries: {total}[/dim]")
        return 0

    # Need a query for search
    if not args.query:
        console.print("[yellow]Please provide a query or use --list-namespaces[/yellow]")
        return 1

    # Find matches
    if args.exact:
        # Exact key match
        matches = loc_df.filter(pl.col("key") == args.query)
        if matches.height == 0:
            matches = None
    else:
        matches = find_localization(loc_df, args.query, args.namespace)

    if matches is None:
        filter_desc = f"'{args.query}'"
        if args.namespace:
            filter_desc += f" in namespace '{args.namespace}'"
        console.print(f"[red]No localization entries found for {filter_desc}[/red]")
        return 1

    stdout.print(f"\n[bold]Found {matches.height} localization entries[/bold]\n")

    # Group by namespace for display
    namespaces = matches["namespace"].unique().to_list()

    for namespace in namespaces:
        ns_matches = matches.filter(pl.col("namespace") == namespace)
        stdout.print(Panel(f"[bold cyan]{namespace}[/bold cyan] ({ns_matches.height} entries)", expand=False))

        for row in ns_matches.iter_rows(named=True):
            key = row.get("key", "N/A")
            value = row.get("value_en", "N/A")
            has_params = row.get("has_params", False)

            # Format the entry
            loc_table = Table(show_header=False, box=None, padding=(0, 1))
            loc_table.add_column("Field", style="cyan", width=12)
            loc_table.add_column("Value", style="white")

            loc_table.add_row("Key:", key)

            # Truncate long values for display
            if len(value) > 200:
                display_value = value[:200] + "..."
            else:
                display_value = value

            loc_table.add_row("Text:", display_value)

            if has_params:
                loc_table.add_row("Params:", "[yellow]Yes (contains placeholders)[/yellow]")

            stdout.print(loc_table)

            # Cross-reference with cards
            related = find_card_by_localization_key(cards_df, units_df, key)
            if related:
                for item in related:
                    stdout.print(
                        f"  [dim]-> {item['type']}: {item['id']} ({item['name']})[/dim]"
                    )

            stdout.print()

    # Show namespace filter hint if many results
    if matches.height > 20 and not args.namespace:
        stdout.print(
            "[dim]Tip: Use --namespace to filter results "
            "(e.g., --namespace cards)[/dim]"
        )

    return 0


if __name__ == "__main__":
    sys.exit(main())
