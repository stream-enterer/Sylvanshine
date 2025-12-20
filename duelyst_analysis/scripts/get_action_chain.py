#!/usr/bin/env python3
# /// script
# requires-python = ">=3.11"
# dependencies = ["polars", "rich"]
# ///
"""Get action execution sequence - parent actions, subactions, delays.

Usage:
    uv run get_action_chain.py <action_class>

Example:
    uv run get_action_chain.py ApplyCardToBoardAction
    uv run get_action_chain.py AttackAction
    uv run get_action_chain.py DamageAction
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


def load_actions_tsv() -> pl.DataFrame:
    """Load the actions TSV file."""
    actions_path = get_data_dir() / "instances" / "actions.tsv"
    return pl.read_csv(actions_path, separator="\t")


def load_inheritance_tsv() -> pl.DataFrame:
    """Load the inheritance TSV file."""
    inheritance_path = get_data_dir() / "semantic" / "inheritance.tsv"
    return pl.read_csv(inheritance_path, separator="\t")


def find_action(actions_df: pl.DataFrame, action_class: str) -> pl.DataFrame | None:
    """Find actions matching the class name."""
    # Exact match first
    matches = actions_df.filter(
        pl.col("class_name") == action_class
    )

    if matches.height == 0:
        # Partial match
        pattern = action_class.lower()
        matches = actions_df.filter(
            pl.col("class_name").str.to_lowercase().str.contains(pattern)
        )

    return matches if matches.height > 0 else None


def build_inheritance_tree(inheritance_df: pl.DataFrame) -> tuple[dict[str, str], dict[str, list[str]]]:
    """Build parent->child and child->parent mappings."""
    parent_of: dict[str, str] = {}
    children_of: dict[str, list[str]] = defaultdict(list)

    for row in inheritance_df.iter_rows(named=True):
        child = row.get("child_class", "")
        parent = row.get("parent_class", "")

        if child and parent:
            parent_of[child] = parent
            children_of[parent].append(child)

    return parent_of, children_of


def get_parent_chain(action_class: str, parent_of: dict[str, str]) -> list[str]:
    """Get the full parent chain for an action."""
    chain = []
    current = action_class

    while current in parent_of:
        parent = parent_of[current]
        chain.append(parent)
        current = parent
        if len(chain) > 20:  # Prevent infinite loops
            break

    return chain


def get_children_tree(
    action_class: str,
    children_of: dict[str, list[str]],
    depth: int = 0,
    max_depth: int = 3
) -> dict[str, list]:
    """Get the children tree for an action."""
    if depth > max_depth:
        return {}

    result = {}

    if action_class in children_of:
        for child in children_of[action_class]:
            result[child] = get_children_tree(child, children_of, depth + 1, max_depth)

    return result


def add_children_to_tree(
    tree: Tree,
    children_dict: dict[str, dict],
    actions_df: pl.DataFrame
) -> None:
    """Add children to a rich Tree."""
    for child_name, grandchildren in children_dict.items():
        # Get action info if available
        action_info = actions_df.filter(pl.col("class_name") == child_name)

        if action_info.height > 0:
            row = action_info.row(0, named=True)
            desc = row.get("description", "")[:40]
            label = f"[cyan]{child_name}[/cyan] [dim]- {desc}[/dim]"
        else:
            label = f"[cyan]{child_name}[/cyan]"

        branch = tree.add(label)

        if grandchildren:
            add_children_to_tree(branch, grandchildren, actions_df)


def main() -> int:
    parser = argparse.ArgumentParser(
        description="Get action execution sequence",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog=__doc__
    )
    parser.add_argument(
        "action_class",
        help="Action class name (e.g., 'ApplyCardToBoardAction', 'AttackAction')"
    )
    parser.add_argument(
        "--depth", "-d",
        type=int,
        default=3,
        help="Maximum depth for child actions (default: 3)"
    )

    args = parser.parse_args()
    console = Console(stderr=True)
    stdout = Console()

    # Load data
    try:
        actions_df = load_actions_tsv()
    except Exception as e:
        console.print(f"[red]Error loading actions.tsv: {e}[/red]")
        return 1

    try:
        inheritance_df = load_inheritance_tsv()
    except Exception as e:
        console.print(f"[yellow]Warning: Could not load inheritance.tsv: {e}[/yellow]")
        inheritance_df = None

    # Find the action
    matches = find_action(actions_df, args.action_class)

    if matches is None:
        console.print(f"[red]No action found matching '{args.action_class}'[/red]")

        # Show available actions
        stdout.print("\n[yellow]Available actions:[/yellow]")
        for row in actions_df.head(20).iter_rows(named=True):
            stdout.print(f"  {row.get('class_name', '')}")
        return 1

    # Build inheritance mappings
    parent_of: dict[str, str] = {}
    children_of: dict[str, list[str]] = defaultdict(list)

    if inheritance_df is not None:
        parent_of, children_of = build_inheritance_tree(inheritance_df)

    for row in matches.iter_rows(named=True):
        action_name = row.get("class_name", "Unknown")

        stdout.print(Panel(f"[bold cyan]{action_name}[/bold cyan]", expand=False))

        # Basic info
        info_table = Table(title="Action Information", show_header=True)
        info_table.add_column("Property", style="cyan")
        info_table.add_column("Value", style="green")

        info_table.add_row("Class Name", action_name)
        info_table.add_row("File", row.get("file_name", "N/A"))
        info_table.add_row("Parent Class", row.get("parent_class", "N/A"))
        info_table.add_row("Type", row.get("type", "N/A"))
        info_table.add_row("Has Target", row.get("has_target", "N/A"))
        info_table.add_row("Has Source", row.get("has_source", "N/A"))

        stdout.print(info_table)
        stdout.print()

        # Description
        description = row.get("description", "")
        if description:
            stdout.print(Panel(description, title="Description", expand=False))
            stdout.print()

        # Key methods
        key_methods = row.get("key_methods", "")
        if key_methods:
            methods_table = Table(title="Key Methods", show_header=True)
            methods_table.add_column("Method", style="yellow")

            for method in key_methods.split(", "):
                methods_table.add_row(method.strip())

            stdout.print(methods_table)
            stdout.print()

        # Triggers events
        triggers = row.get("triggers_events", "")
        if triggers and triggers != "None":
            triggers_table = Table(title="Triggers/Subactions", show_header=True)
            triggers_table.add_column("Event/Action", style="magenta")

            for trigger in triggers.split(", "):
                triggers_table.add_row(trigger.strip())

            stdout.print(triggers_table)
            stdout.print()

        # Parent chain
        parent_chain = get_parent_chain(action_name, parent_of)
        if parent_chain:
            stdout.print("[bold]Inheritance Chain (parents):[/bold]")
            chain_tree = Tree(f"[bold cyan]{action_name}[/bold cyan]")
            current = chain_tree
            for parent in parent_chain:
                current = current.add(f"[green]extends[/green] [cyan]{parent}[/cyan]")
            stdout.print(chain_tree)
            stdout.print()

        # Child actions
        children = get_children_tree(action_name, children_of, max_depth=args.depth)
        if children:
            stdout.print("[bold]Derived Actions (children):[/bold]")
            children_tree = Tree(f"[bold cyan]{action_name}[/bold cyan]")
            add_children_to_tree(children_tree, children, actions_df)
            stdout.print(children_tree)
            stdout.print()

        # Related actions (from same parent)
        parent_class = row.get("parent_class", "")
        if parent_class and parent_class in children_of:
            siblings = [c for c in children_of[parent_class] if c != action_name]
            if siblings:
                stdout.print(f"[bold]Sibling Actions (also extend {parent_class}):[/bold]")
                siblings_table = Table(show_header=True)
                siblings_table.add_column("Action", style="cyan")

                for sibling in siblings[:10]:
                    siblings_table.add_row(sibling)

                if len(siblings) > 10:
                    siblings_table.add_row(f"... and {len(siblings) - 10} more")

                stdout.print(siblings_table)

        stdout.print()

    # Execution flow note
    stdout.print(Panel(
        "[bold]Action Execution Flow:[/bold]\n\n"
        "1. [cyan]_modifyForExecution()[/cyan] - Pre-execution modifications\n"
        "2. [cyan]_execute()[/cyan] - Main action logic\n"
        "3. Subactions are created and executed\n"
        "4. Modifiers react via [yellow]onBeforeAction[/yellow], [yellow]onAfterAction[/yellow]\n"
        "5. View layer animates via [green]GameLayer.showAction()[/green]",
        title="Execution Notes",
        expand=False
    ))

    return 0


if __name__ == "__main__":
    sys.exit(main())
