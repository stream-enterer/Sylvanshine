#!/usr/bin/env python3
# /// script
# requires-python = ">=3.11"
# dependencies = ["polars", "rich"]
# ///
"""Trace modifier interactions and stacking for a given modifier.

Usage:
    uv run get_modifier_chain.py <modifier_id>

Example:
    uv run get_modifier_chain.py ModifierFlying
    uv run get_modifier_chain.py ModifierProvoke
    uv run get_modifier_chain.py ModifierDeathWatch
"""

import argparse
import sys
from pathlib import Path
from collections import defaultdict

import polars as pl
from rich.console import Console
from rich.table import Table
from rich.panel import Panel
from rich.tree import Tree


def get_data_dir() -> Path:
    """Get the duelyst_analysis directory path."""
    return Path(__file__).parent.parent


def load_modifiers_tsv() -> pl.DataFrame:
    """Load the modifiers TSV file."""
    modifiers_path = get_data_dir() / "instances" / "modifiers.tsv"
    return pl.read_csv(modifiers_path, separator="\t")


def load_inheritance_tsv() -> pl.DataFrame:
    """Load the inheritance TSV file."""
    inheritance_path = get_data_dir() / "semantic" / "inheritance.tsv"
    return pl.read_csv(inheritance_path, separator="\t")


def load_events_tsv() -> pl.DataFrame:
    """Load the events TSV file."""
    events_path = get_data_dir() / "semantic" / "events.tsv"
    return pl.read_csv(events_path, separator="\t")


def find_modifier(modifiers_df: pl.DataFrame, modifier_id: str) -> dict | None:
    """Find a modifier by class name or type."""
    pattern = modifier_id.lower()

    # Exact match first
    exact = modifiers_df.filter(
        pl.col("class_name").str.to_lowercase() == pattern
    )
    if exact.height > 0:
        return exact.row(0, named=True)

    # Partial match
    partial = modifiers_df.filter(
        pl.col("class_name").str.to_lowercase().str.contains(pattern)
    )
    if partial.height > 0:
        return partial.row(0, named=True)

    return None


def get_inheritance_chain(modifiers_df: pl.DataFrame, inheritance_df: pl.DataFrame, class_name: str) -> list[str]:
    """Get the full inheritance chain for a modifier."""
    chain = [class_name]
    current = class_name

    while True:
        # Find in modifiers.tsv first
        mod = modifiers_df.filter(pl.col("class_name") == current)
        if mod.height > 0:
            parent = mod.row(0, named=True).get("parent_class", "")
            if parent and parent != "null" and parent != current:
                chain.append(parent)
                current = parent
                continue

        # Check inheritance.tsv
        inh = inheritance_df.filter(pl.col("child_class") == current)
        if inh.height > 0:
            parent = inh.row(0, named=True).get("parent_class", "")
            if parent and parent != current:
                chain.append(parent)
                current = parent
                continue

        break

    return chain


def get_child_modifiers(modifiers_df: pl.DataFrame, inheritance_df: pl.DataFrame, class_name: str) -> list[str]:
    """Get all direct child modifiers."""
    children = []

    # From modifiers.tsv
    mods = modifiers_df.filter(pl.col("parent_class") == class_name)
    for row in mods.iter_rows(named=True):
        children.append(row["class_name"])

    # From inheritance.tsv (may include non-modifier classes)
    inh = inheritance_df.filter(pl.col("parent_class") == class_name)
    for row in inh.iter_rows(named=True):
        child = row["child_class"]
        if child not in children and "Modifier" in child:
            children.append(child)

    return sorted(set(children))


def get_modifiers_with_same_trigger(modifiers_df: pl.DataFrame, trigger: str) -> list[dict]:
    """Get all modifiers that use the same trigger."""
    if not trigger:
        return []

    triggers = [t.strip() for t in trigger.split(",")]
    matches = []

    for t in triggers:
        filtered = modifiers_df.filter(
            pl.col("triggers").fill_null("").str.contains(t)
        )
        for row in filtered.iter_rows(named=True):
            matches.append(row)

    return matches


def build_hierarchy_tree(modifiers_df: pl.DataFrame, inheritance_df: pl.DataFrame, class_name: str, depth: int = 0, max_depth: int = 3) -> Tree:
    """Build a tree showing class hierarchy."""
    tree = Tree(f"[cyan]{class_name}[/cyan]")

    if depth >= max_depth:
        tree.add("[dim]...[/dim]")
        return tree

    children = get_child_modifiers(modifiers_df, inheritance_df, class_name)
    for child in children[:10]:  # Limit to 10 children
        subtree = build_hierarchy_tree(modifiers_df, inheritance_df, child, depth + 1, max_depth)
        tree.add(subtree)

    if len(children) > 10:
        tree.add(f"[dim]... and {len(children) - 10} more[/dim]")

    return tree


def main() -> int:
    parser = argparse.ArgumentParser(
        description="Trace modifier interactions and stacking",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog=__doc__
    )
    parser.add_argument(
        "modifier_id",
        help="Modifier class name to analyze (e.g., 'ModifierFlying')"
    )
    parser.add_argument(
        "--show-children", "-c",
        action="store_true",
        help="Show child modifiers that extend this one"
    )
    parser.add_argument(
        "--show-conflicts", "-x",
        action="store_true",
        help="Show modifiers with same trigger type (potential conflicts)"
    )
    parser.add_argument(
        "--hierarchy-depth", "-d",
        type=int,
        default=3,
        help="Max depth for hierarchy tree (default: 3)"
    )

    args = parser.parse_args()
    console = Console(stderr=True)
    stdout = Console()

    # Load data
    try:
        modifiers_df = load_modifiers_tsv()
        inheritance_df = load_inheritance_tsv()
    except Exception as e:
        console.print(f"[red]Error loading data: {e}[/red]")
        return 1

    # Find the modifier
    modifier = find_modifier(modifiers_df, args.modifier_id)

    if not modifier:
        console.print(f"[red]Modifier '{args.modifier_id}' not found[/red]")
        console.print("\n[dim]Try searching with: uv run list_modifier_triggers.py <partial_name>[/dim]")
        return 1

    class_name = modifier["class_name"]

    # Header
    stdout.print(Panel(f"[bold cyan]Modifier Chain: {class_name}[/bold cyan]", expand=False))

    # Basic info
    info_table = Table(title="Modifier Info", show_header=True)
    info_table.add_column("Property", style="cyan", width=20)
    info_table.add_column("Value", style="green")

    info_table.add_row("Class Name", class_name)
    info_table.add_row("File", str(modifier.get("file_name", "N/A")))
    info_table.add_row("Type", str(modifier.get("type", "N/A")))
    info_table.add_row("Parent Class", str(modifier.get("parent_class", "N/A")))
    info_table.add_row("Is Keyword", str(modifier.get("is_keyword", "FALSE")))
    info_table.add_row("Is Watch", str(modifier.get("is_watch", "FALSE")))
    info_table.add_row("Applied Name", str(modifier.get("applied_name", "N/A")))
    info_table.add_row("Description", str(modifier.get("applied_description", "N/A"))[:80])
    info_table.add_row("Triggers", str(modifier.get("triggers", "N/A")))
    info_table.add_row("Effects", str(modifier.get("effects", "N/A")))
    info_table.add_row("Stack Type", str(modifier.get("stack_type", "N/A")))

    stdout.print(info_table)

    # Inheritance chain
    stdout.print("\n[bold]Inheritance Chain (Parent → Child):[/bold]")
    chain = get_inheritance_chain(modifiers_df, inheritance_df, class_name)
    chain.reverse()  # Root first

    chain_str = " → ".join([f"[cyan]{c}[/cyan]" for c in chain])
    stdout.print(f"  {chain_str}")

    # Parent modifiers with details
    if len(chain) > 1:
        stdout.print("\n[bold]Parent Modifiers:[/bold]")
        parents_table = Table(show_header=True)
        parents_table.add_column("Class", style="cyan")
        parents_table.add_column("Type", style="green")
        parents_table.add_column("Triggers", style="yellow")

        for parent in chain[:-1]:  # Exclude the modifier itself
            parent_mod = find_modifier(modifiers_df, parent)
            if parent_mod:
                parents_table.add_row(
                    parent,
                    str(parent_mod.get("type", "N/A")),
                    str(parent_mod.get("triggers", "N/A"))[:40]
                )
            else:
                parents_table.add_row(parent, "[dim]External/SDK[/dim]", "")

        stdout.print(parents_table)

    # Child modifiers
    if args.show_children:
        stdout.print("\n[bold]Child Modifiers (extending this):[/bold]")
        children = get_child_modifiers(modifiers_df, inheritance_df, class_name)

        if children:
            children_table = Table(show_header=True)
            children_table.add_column("Class", style="cyan")
            children_table.add_column("Description", style="green")

            for child in children[:20]:
                child_mod = find_modifier(modifiers_df, child)
                if child_mod:
                    desc = str(child_mod.get("applied_description", ""))[:50]
                    children_table.add_row(child, desc)
                else:
                    children_table.add_row(child, "")

            if len(children) > 20:
                children_table.add_row(f"[dim]... and {len(children) - 20} more[/dim]", "")

            stdout.print(children_table)
        else:
            stdout.print("  [dim]No child modifiers found[/dim]")

        # Hierarchy tree
        stdout.print("\n[bold]Hierarchy Tree:[/bold]")
        tree = build_hierarchy_tree(modifiers_df, inheritance_df, class_name, 0, args.hierarchy_depth)
        stdout.print(tree)

    # Modifiers with same trigger (potential conflicts)
    if args.show_conflicts:
        triggers = modifier.get("triggers", "")
        if triggers and triggers != "null":
            stdout.print(f"\n[bold]Modifiers with Same Trigger ({triggers}):[/bold]")
            same_trigger = get_modifiers_with_same_trigger(modifiers_df, triggers)

            if len(same_trigger) > 1:  # More than just this one
                conflict_table = Table(show_header=True)
                conflict_table.add_column("Class", style="cyan")
                conflict_table.add_column("Effects", style="yellow")
                conflict_table.add_column("Stack Type", style="magenta")

                for mod in same_trigger[:15]:
                    if mod["class_name"] != class_name:
                        conflict_table.add_row(
                            mod["class_name"],
                            str(mod.get("effects", ""))[:30],
                            str(mod.get("stack_type", ""))
                        )

                stdout.print(conflict_table)
                stdout.print(f"\n[dim]Total modifiers with this trigger: {len(same_trigger)}[/dim]")
            else:
                stdout.print("  [dim]No other modifiers use this trigger[/dim]")

    # Common stacking scenarios
    stdout.print("\n[bold]Stacking Info:[/bold]")
    stack_type = modifier.get("stack_type", "")
    if stack_type and stack_type != "null":
        stdout.print(f"  Stack Type: [cyan]{stack_type}[/cyan]")
        if "maxStacks=" in str(stack_type):
            stdout.print("  [dim]This modifier has a max stack limit[/dim]")
    else:
        stdout.print("  [dim]No explicit stack limit (stacks infinitely)[/dim]")

    # Keyword-specific info
    if modifier.get("is_keyword") == "TRUE":
        stdout.print("\n[bold yellow]Keyword Modifier[/bold yellow]")
        stdout.print("  - Displayed in card text")
        stdout.print("  - Typically cannot stack (maxStacks=1)")
        stdout.print("  - Has defined game behavior")

    # Watch-specific info
    if modifier.get("is_watch") == "TRUE":
        stdout.print("\n[bold blue]Watch Modifier[/bold blue]")
        stdout.print("  - Triggers on specific game events")
        stdout.print("  - Multiple watches of same type can coexist")
        stdout.print("  - Execution order determined by application order")

    return 0


if __name__ == "__main__":
    sys.exit(main())
