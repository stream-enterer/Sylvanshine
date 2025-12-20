#!/usr/bin/env python3
# /// script
# requires-python = ">=3.11"
# dependencies = ["polars", "rich"]
# ///
"""Find constant/class usages across the codebase.

Usage:
    uv run find_references.py <identifier>

Example:
    uv run find_references.py CONFIG.FADE_SLOW_DURATION
    uv run find_references.py AttackAction
    uv run find_references.py ModifierProvoke
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


def load_constants_tsv() -> pl.DataFrame:
    """Load the constants TSV file."""
    constants_path = get_data_dir() / "semantic" / "constants.tsv"
    return pl.read_csv(constants_path, separator="\t")


def load_references_to_tsv() -> pl.DataFrame:
    """Load the references_to TSV file."""
    refs_path = get_data_dir() / "references_to.tsv"
    return pl.read_csv(refs_path, separator="\t")


def load_referenced_by_tsv() -> pl.DataFrame:
    """Load the referenced_by TSV file."""
    refs_path = get_data_dir() / "referenced_by.tsv"
    return pl.read_csv(refs_path, separator="\t")


def load_classes_tsv() -> pl.DataFrame:
    """Load the classes TSV file."""
    classes_path = get_data_dir() / "semantic" / "classes.tsv"
    return pl.read_csv(classes_path, separator="\t")


def load_functions_tsv() -> pl.DataFrame:
    """Load the functions TSV file."""
    functions_path = get_data_dir() / "semantic" / "functions.tsv"
    return pl.read_csv(functions_path, separator="\t")


def find_constant(constants_df: pl.DataFrame, identifier: str) -> pl.DataFrame | None:
    """Find constants matching the identifier."""
    # Exact match first
    matches = constants_df.filter(
        pl.col("constant_name") == identifier
    )

    if matches.height == 0:
        # Partial match
        matches = constants_df.filter(
            pl.col("constant_name").str.contains(identifier)
        )

    return matches if matches.height > 0 else None


def find_references(referenced_by_df: pl.DataFrame, identifier: str) -> list[str]:
    """Find files that reference the identifier."""
    # Search in file column for the identifier
    matches = referenced_by_df.filter(
        pl.col("file").str.contains(identifier)
    )

    if matches.height > 0:
        # Get all files that reference this
        refs = []
        for row in matches.iter_rows(named=True):
            referenced_by = row.get("referenced_by", "")
            if referenced_by:
                refs.extend(referenced_by.split("|"))
        return refs

    return []


def find_in_references_to(refs_to_df: pl.DataFrame, identifier: str) -> list[tuple[str, str]]:
    """Find files that reference the identifier in references_to format."""
    results = []

    for row in refs_to_df.iter_rows(named=True):
        file = row.get("file", "")
        references = row.get("references", "")

        if references and identifier.lower() in references.lower():
            # Find the specific reference
            for ref in references.split("|"):
                if identifier.lower() in ref.lower():
                    results.append((file, ref))

    return results


def find_class(classes_df: pl.DataFrame, identifier: str) -> pl.DataFrame | None:
    """Find classes matching the identifier."""
    matches = classes_df.filter(
        pl.col("class_name").str.contains(identifier)
    )
    return matches if matches.height > 0 else None


def main() -> int:
    parser = argparse.ArgumentParser(
        description="Find constant/class usages across the codebase",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog=__doc__
    )
    parser.add_argument(
        "identifier",
        help="Identifier to search for (e.g., 'CONFIG.FADE_SLOW_DURATION', 'AttackAction')"
    )
    parser.add_argument(
        "--type", "-t",
        choices=["all", "constant", "class", "function"],
        default="all",
        help="Type of identifier to search for (default: all)"
    )

    args = parser.parse_args()
    console = Console(stderr=True)
    stdout = Console()

    found_something = False

    # Load data
    try:
        constants_df = load_constants_tsv()
    except Exception as e:
        console.print(f"[yellow]Warning: Could not load constants.tsv: {e}[/yellow]")
        constants_df = None

    try:
        referenced_by_df = load_referenced_by_tsv()
    except Exception as e:
        console.print(f"[yellow]Warning: Could not load referenced_by.tsv: {e}[/yellow]")
        referenced_by_df = None

    try:
        refs_to_df = load_references_to_tsv()
    except Exception as e:
        console.print(f"[yellow]Warning: Could not load references_to.tsv: {e}[/yellow]")
        refs_to_df = None

    try:
        classes_df = load_classes_tsv()
    except Exception as e:
        console.print(f"[yellow]Warning: Could not load classes.tsv: {e}[/yellow]")
        classes_df = None

    stdout.print(Panel(f"[bold cyan]Searching for: {args.identifier}[/bold cyan]", expand=False))

    # Search constants
    if args.type in ["all", "constant"] and constants_df is not None:
        const_matches = find_constant(constants_df, args.identifier)
        if const_matches is not None and const_matches.height > 0:
            found_something = True
            stdout.print("\n[bold green]Constants Found:[/bold green]")

            const_table = Table(show_header=True)
            const_table.add_column("Constant", style="cyan")
            const_table.add_column("Value", style="green")
            const_table.add_column("File", style="yellow")

            for row in const_matches.iter_rows(named=True):
                const_table.add_row(
                    row.get("constant_name", ""),
                    str(row.get("value", ""))[:50],
                    row.get("file", "")
                )

            stdout.print(const_table)

    # Search classes
    if args.type in ["all", "class"] and classes_df is not None:
        class_matches = find_class(classes_df, args.identifier)
        if class_matches is not None and class_matches.height > 0:
            found_something = True
            stdout.print("\n[bold green]Classes Found:[/bold green]")

            class_table = Table(show_header=True)
            class_table.add_column("Class", style="cyan")
            class_table.add_column("File", style="yellow")
            class_table.add_column("Parent", style="magenta")

            for row in class_matches.iter_rows(named=True):
                class_table.add_row(
                    row.get("class_name", ""),
                    row.get("file", ""),
                    row.get("parent_class", "N/A")
                )

            stdout.print(class_table)

    # Search references
    if referenced_by_df is not None:
        refs = find_references(referenced_by_df, args.identifier)
        if refs:
            found_something = True
            stdout.print(f"\n[bold green]Referenced By ({len(refs)} files):[/bold green]")

            refs_table = Table(show_header=True)
            refs_table.add_column("File", style="yellow")

            for ref in sorted(set(refs))[:30]:  # Limit to 30
                refs_table.add_row(ref)

            if len(refs) > 30:
                refs_table.add_row(f"... and {len(refs) - 30} more files")

            stdout.print(refs_table)

    # Search in references_to
    if refs_to_df is not None:
        ref_locations = find_in_references_to(refs_to_df, args.identifier)
        if ref_locations:
            found_something = True
            stdout.print(f"\n[bold green]Found in References ({len(ref_locations)} locations):[/bold green]")

            loc_table = Table(show_header=True)
            loc_table.add_column("File", style="yellow")
            loc_table.add_column("Reference", style="cyan")

            for file, ref in ref_locations[:20]:  # Limit to 20
                loc_table.add_row(file, ref[:60])

            if len(ref_locations) > 20:
                loc_table.add_row("...", f"and {len(ref_locations) - 20} more")

            stdout.print(loc_table)

    if not found_something:
        console.print(f"[red]No references found for '{args.identifier}'[/red]")
        return 1

    return 0


if __name__ == "__main__":
    sys.exit(main())
