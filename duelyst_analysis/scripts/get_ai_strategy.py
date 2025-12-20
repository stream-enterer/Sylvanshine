#!/usr/bin/env python3
# /// script
# requires-python = ">=3.11"
# dependencies = ["polars", "rich"]
# ///
"""Get AI agent information and scripted challenge behaviors.

The Duelyst AI system uses StaticAgent for scripted challenges/tutorials.
Actions are pre-defined per turn using agent action factories.

Usage:
    uv run get_ai_strategy.py <query>
    uv run get_ai_strategy.py --list-actions
    uv run get_ai_strategy.py --list-agents

Arguments:
    query:  Agent name, action type, or challenge category

Examples:
    uv run get_ai_strategy.py StaticAgent
    uv run get_ai_strategy.py move
    uv run get_ai_strategy.py --list-actions
    uv run get_ai_strategy.py tutorial
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


def load_agents_tsv() -> pl.DataFrame:
    """Load the agents TSV file."""
    agents_path = get_data_dir() / "instances" / "agents.tsv"
    return pl.read_csv(agents_path, separator="\t")


def load_agent_actions_tsv() -> pl.DataFrame:
    """Load the agent_actions TSV file."""
    actions_path = get_data_dir() / "instances" / "agent_actions.tsv"
    return pl.read_csv(actions_path, separator="\t")


def load_challenges_tsv() -> pl.DataFrame:
    """Load the challenges TSV file for cross-reference."""
    challenges_path = get_data_dir() / "instances" / "challenges.tsv"
    return pl.read_csv(challenges_path, separator="\t")


def find_agents(df: pl.DataFrame, pattern: str) -> pl.DataFrame | None:
    """Find agents matching the name pattern."""
    matches = df.filter(
        pl.col("class_name").str.to_lowercase().str.contains(pattern.lower()) |
        pl.col("purpose").str.to_lowercase().str.contains(pattern.lower())
    )
    return matches if matches.height > 0 else None


def find_actions(df: pl.DataFrame, pattern: str) -> pl.DataFrame | None:
    """Find agent actions matching the pattern."""
    matches = df.filter(
        pl.col("name").str.to_lowercase().str.contains(pattern.lower()) |
        pl.col("type").str.to_lowercase().str.contains(pattern.lower())
    )
    return matches if matches.height > 0 else None


def find_challenges(df: pl.DataFrame, pattern: str) -> pl.DataFrame | None:
    """Find challenges matching category or difficulty."""
    matches = df.filter(
        pl.col("category").str.to_lowercase().str.contains(pattern.lower()) |
        pl.col("difficulty").str.to_lowercase().str.contains(pattern.lower()) |
        pl.col("name").str.to_lowercase().str.contains(pattern.lower())
    )
    return matches if matches.height > 0 else None


def get_action_description(name: str, params: str) -> str:
    """Get human-readable description for an action."""
    descriptions = {
        "createAgentActionMoveUnit": f"Move unit by delta position ({params})",
        "createAgentActionAttackWithUnit": f"Attack with unit ({params})",
        "createAgentActionPlayCard": f"Play card from hand ({params})",
        "createAgentActionPlayCardFindPosition": f"Play card finding optimal position ({params})",
        "createAgentActionPlayFollowup": f"Play followup effect ({params})",
        "createAgentSoftActionTagUnitAtPosition": f"Tag unit for reference ({params})",
        "createAgentSoftActionShowInstructionLabels": f"Show tutorial labels ({params})",
        "createSDKActionFromAgentAction": "Convert agent action to SDK action",
        "executeSoftActionForAgent": "Execute non-game-affecting action",
    }

    for key, desc in descriptions.items():
        if key in name:
            return desc

    return f"Agent action ({params})"


def main() -> int:
    parser = argparse.ArgumentParser(
        description="Get AI agent information and scripted challenge behaviors",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog=__doc__
    )
    parser.add_argument(
        "query",
        nargs="?",
        default=None,
        help="Agent name, action type, or challenge category"
    )
    parser.add_argument(
        "--list-actions",
        action="store_true",
        help="List all available agent action types"
    )
    parser.add_argument(
        "--list-agents",
        action="store_true",
        help="List all agent types"
    )

    args = parser.parse_args()
    console = Console(stderr=True)
    stdout = Console()

    # Load data
    try:
        agents_df = load_agents_tsv()
        actions_df = load_agent_actions_tsv()
        challenges_df = load_challenges_tsv()
    except Exception as e:
        console.print(f"[red]Error loading TSV files: {e}[/red]")
        return 1

    # List all actions
    if args.list_actions:
        stdout.print(Panel("[bold]Agent Action Types[/bold]", expand=False))

        # Filter to action_type entries
        action_types = actions_df.filter(pl.col("type") == "action_type")

        action_table = Table(show_header=True)
        action_table.add_column("Action Factory", style="cyan")
        action_table.add_column("Parameters", style="yellow")
        action_table.add_column("Category", style="green")

        for row in action_types.iter_rows(named=True):
            name = row.get("name", "N/A")
            params = row.get("params", "")

            # Categorize
            if "Soft" in name:
                category = "soft (UI/tutorial)"
            elif "Move" in name:
                category = "hard (movement)"
            elif "Attack" in name:
                category = "hard (combat)"
            elif "PlayCard" in name or "Followup" in name:
                category = "hard (card play)"
            else:
                category = "utility"

            action_table.add_row(name, params, category)

        stdout.print(action_table)
        stdout.print()

        # Explanation
        explain_tree = Tree("[bold]Action Categories[/bold]")
        hard = explain_tree.add("[green]Hard Actions[/green] - Affect game state")
        hard.add("Movement: Move units on the board")
        hard.add("Combat: Attack with units")
        hard.add("Card Play: Play cards from hand")
        soft = explain_tree.add("[yellow]Soft Actions[/yellow] - UI/tutorial only")
        soft.add("Tag units for reference in scripts")
        soft.add("Show instruction labels")
        stdout.print(explain_tree)

        return 0

    # List all agents
    if args.list_agents:
        stdout.print(Panel("[bold]AI Agent Types[/bold]", expand=False))

        for row in agents_df.iter_rows(named=True):
            agent_name = row.get("class_name", "N/A")
            purpose = row.get("purpose", "N/A")
            scripted = row.get("scripted_behavior", "No")

            stdout.print(Panel(f"[bold cyan]{agent_name}[/bold cyan]", expand=False))

            agent_table = Table(show_header=True)
            agent_table.add_column("Property", style="cyan")
            agent_table.add_column("Value", style="green")

            agent_table.add_row("File", row.get("file_name", "N/A"))
            agent_table.add_row("Parent Class", row.get("parent_class", "N/A"))
            agent_table.add_row("Purpose", purpose)
            agent_table.add_row("Scripted", scripted)
            agent_table.add_row("Key Methods", row.get("key_methods", "N/A"))

            stdout.print(agent_table)
            stdout.print()

            # Description
            desc = row.get("description", "")
            if desc:
                stdout.print(f"[dim]{desc}[/dim]\n")

        return 0

    # Need a query for search
    if not args.query:
        console.print("[yellow]Please provide a query or use --list-actions/--list-agents[/yellow]")
        return 1

    # Search agents
    agent_matches = find_agents(agents_df, args.query)
    if agent_matches is not None:
        stdout.print(f"\n[bold]Found {agent_matches.height} agent(s)[/bold]\n")

        for row in agent_matches.iter_rows(named=True):
            agent_name = row.get("class_name", "N/A")
            stdout.print(Panel(f"[bold cyan]{agent_name}[/bold cyan]", expand=False))

            agent_table = Table(title="Agent Details", show_header=True)
            agent_table.add_column("Property", style="cyan")
            agent_table.add_column("Value", style="green")

            agent_table.add_row("File", row.get("file_name", "N/A"))
            agent_table.add_row("Parent", row.get("parent_class", "N/A"))
            agent_table.add_row("Purpose", row.get("purpose", "N/A"))
            agent_table.add_row("Difficulty", row.get("difficulty", "N/A"))
            agent_table.add_row("Scripted", row.get("scripted_behavior", "N/A"))

            stdout.print(agent_table)
            stdout.print()

            # Key methods
            methods = row.get("key_methods", "")
            if methods:
                method_tree = Tree("[bold]Key Methods[/bold]")
                for method in methods.split(", "):
                    method_tree.add(f"[yellow]{method}[/yellow]")
                stdout.print(method_tree)
                stdout.print()

            # Description
            desc = row.get("description", "")
            if desc:
                stdout.print(Panel(desc, title="Description", expand=False))
                stdout.print()

    # Search actions
    action_matches = find_actions(actions_df, args.query)
    if action_matches is not None:
        stdout.print(f"\n[bold]Found {action_matches.height} action(s)[/bold]\n")

        for row in action_matches.iter_rows(named=True):
            action_name = row.get("name", "N/A")
            action_type = row.get("type", "N/A")
            params = row.get("params", "")

            type_color = "green" if action_type == "action_type" else "yellow"
            stdout.print(Panel(
                f"[bold cyan]{action_name}[/bold cyan] ([{type_color}]{action_type}[/{type_color}])",
                expand=False
            ))

            action_table = Table(show_header=True)
            action_table.add_column("Property", style="cyan")
            action_table.add_column("Value", style="green")

            action_table.add_row("Parameters", params if params else "None")
            action_table.add_row("Source", row.get("source", "N/A"))
            action_table.add_row("Description", get_action_description(action_name, params))

            stdout.print(action_table)
            stdout.print()

    # Search challenges
    challenge_matches = find_challenges(challenges_df, args.query)
    if challenge_matches is not None:
        stdout.print(f"\n[bold]Found {challenge_matches.height} challenge(s)[/bold]\n")

        challenge_table = Table(title="Challenges Using AI", show_header=True)
        challenge_table.add_column("Challenge ID", style="cyan")
        challenge_table.add_column("Name", style="white")
        challenge_table.add_column("Category", style="yellow")
        challenge_table.add_column("Difficulty", style="green")
        challenge_table.add_column("Faction", style="magenta")

        for row in challenge_matches.iter_rows(named=True):
            challenge_table.add_row(
                row.get("challenge_id", "N/A"),
                row.get("name", "N/A"),
                row.get("category", "N/A"),
                row.get("difficulty", "N/A"),
                row.get("faction", "N/A")
            )

        stdout.print(challenge_table)
        stdout.print()

        # Note about AI usage
        stdout.print(Panel(
            "All challenges use StaticAgent with pre-scripted actions.\n"
            "Actions are defined per-turn in challenge setup scripts.",
            title="AI System Note",
            expand=False
        ))

    # No matches at all
    if agent_matches is None and action_matches is None and challenge_matches is None:
        console.print(f"[red]No results found for '{args.query}'[/red]")
        return 1

    return 0


if __name__ == "__main__":
    sys.exit(main())
