#!/usr/bin/env python3
# /// script
# requires-python = ">=3.11"
# dependencies = ["rich"]
# ///
"""Generate Mermaid diagrams from flow documentation.

Usage:
    uv run generate_flow_diagram.py <flow_name>

Example:
    uv run generate_flow_diagram.py unit_attack
    uv run generate_flow_diagram.py spell_cast
    uv run generate_flow_diagram.py --list
"""

import argparse
import re
import sys
from pathlib import Path

from rich.console import Console


def get_data_dir() -> Path:
    """Get the duelyst_analysis directory path."""
    return Path(__file__).parent.parent


def get_flows_dir() -> Path:
    """Get the flows directory path."""
    return get_data_dir() / "flows"


def list_flows() -> list[str]:
    """List all available flow files."""
    flows_dir = get_flows_dir()
    return sorted([f.stem for f in flows_dir.glob("*_flow.md")])


def parse_timeline_table(content: str) -> list[dict]:
    """Parse the Timeline section table."""
    events = []

    # Find the Timeline section
    timeline_match = re.search(r'## Timeline\s*(.*?)(?=\n##|\Z)', content, re.DOTALL)
    if not timeline_match:
        return events

    timeline_section = timeline_match.group(1)

    # Find table rows (skip header and separator)
    table_rows = re.findall(r'^\|([^|]+)\|([^|]+)\|([^|]+)\|([^|]+)\|', timeline_section, re.MULTILINE)

    for row in table_rows[2:]:  # Skip header and separator
        time, event, location, action = [col.strip() for col in row]
        if time and event and not time.startswith('-'):
            events.append({
                'time': time,
                'event': event,
                'location': location,
                'action': action
            })

    return events


def parse_system_interactions(content: str) -> list[dict]:
    """Parse the System Interactions section table."""
    interactions = []

    # Find the System Interactions section
    section_match = re.search(r'## System Interactions\s*(.*?)(?=\n##|\Z)', content, re.DOTALL)
    if not section_match:
        return interactions

    section = section_match.group(1)

    # Find table rows
    table_rows = re.findall(r'^\|([^|]+)\|([^|]+)\|([^|]+)\|([^|]+)\|', section, re.MULTILINE)

    for row in table_rows[2:]:  # Skip header and separator
        phase, system, action, code = [col.strip() for col in row]
        if phase and system and not phase.startswith('-'):
            interactions.append({
                'phase': phase,
                'system': system,
                'action': action,
                'code': code
            })

    return interactions


def generate_mermaid_flowchart(flow_name: str, events: list[dict], interactions: list[dict]) -> str:
    """Generate a Mermaid flowchart from parsed data."""
    lines = [
        f"flowchart TD",
        f"    subgraph {flow_name.replace('_', ' ').title()}",
    ]

    # Create nodes from events
    node_ids = {}
    for i, event in enumerate(events):
        node_id = f"E{i}"
        label = event['event'][:40].replace('"', "'")
        node_ids[i] = node_id
        lines.append(f'    {node_id}["{label}"]')

    # Connect nodes sequentially
    for i in range(len(events) - 1):
        lines.append(f"    {node_ids[i]} --> {node_ids[i+1]}")

    lines.append("    end")

    # Add system subgraphs
    if interactions:
        # Group by system
        systems = {}
        for interaction in interactions:
            system = interaction['system']
            if system not in systems:
                systems[system] = []
            systems[system].append(interaction)

        lines.append("")
        lines.append("    subgraph Systems")

        for system, items in list(systems.items())[:5]:  # Limit to 5 systems
            safe_system = re.sub(r'[^a-zA-Z0-9]', '', system)
            lines.append(f"        subgraph {safe_system}")
            for j, item in enumerate(items[:3]):  # Limit actions per system
                action = item['action'][:30].replace('"', "'")
                lines.append(f'            {safe_system}{j}["{action}"]')
            lines.append("        end")

        lines.append("    end")

    return "\n".join(lines)


def generate_mermaid_sequence(flow_name: str, interactions: list[dict]) -> str:
    """Generate a Mermaid sequence diagram from interactions."""
    lines = [
        "sequenceDiagram",
        f"    participant User",
    ]

    # Collect unique systems
    systems = []
    for interaction in interactions:
        system = interaction['system']
        if system not in systems and system.lower() != 'player':
            systems.append(system)

    # Add system participants
    for system in systems[:6]:  # Limit participants
        safe_system = re.sub(r'[^a-zA-Z0-9]', '', system)
        lines.append(f"    participant {safe_system} as {system}")

    lines.append("")

    # Create interaction arrows
    prev_system = "User"
    for interaction in interactions:
        system = interaction['system']
        safe_system = re.sub(r'[^a-zA-Z0-9]', '', system)
        action = interaction['action'][:50].replace('"', "'")

        if system.lower() == 'player':
            lines.append(f"    User->>+{systems[0] if systems else 'System'}: {action}")
        else:
            safe_prev = re.sub(r'[^a-zA-Z0-9]', '', prev_system)
            if safe_prev != safe_system:
                lines.append(f"    {safe_prev}->>+{safe_system}: {action}")
            else:
                lines.append(f"    Note over {safe_system}: {action}")
            prev_system = system

    return "\n".join(lines)


def generate_ascii_diagram(flow_name: str, events: list[dict]) -> str:
    """Generate ASCII art diagram from events."""
    lines = [
        f"╔{'═' * 50}╗",
        f"║ {flow_name.replace('_', ' ').upper().center(48)} ║",
        f"╠{'═' * 50}╣",
    ]

    for i, event in enumerate(events):
        time = event['time'][:10]
        label = event['event'][:35]

        lines.append(f"║ {time:10} │ {label:35} ║")

        if i < len(events) - 1:
            lines.append(f"║ {'':10} │ {'↓':35} ║")

    lines.append(f"╚{'═' * 50}╝")

    return "\n".join(lines)


def main() -> int:
    parser = argparse.ArgumentParser(
        description="Generate Mermaid diagrams from flow documentation",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog=__doc__
    )
    parser.add_argument(
        "flow_name",
        nargs="?",
        help="Flow name (e.g., 'unit_attack', 'spell_cast')"
    )
    parser.add_argument(
        "--list", "-l",
        action="store_true",
        dest="list_flows",
        help="List all available flows"
    )
    parser.add_argument(
        "--format", "-f",
        choices=["flowchart", "sequence", "ascii"],
        default="flowchart",
        help="Output format (default: flowchart)"
    )
    parser.add_argument(
        "--output", "-o",
        help="Output file (default: stdout)"
    )

    args = parser.parse_args()
    console = Console(stderr=True)
    stdout = Console()

    # List flows
    if args.list_flows or not args.flow_name:
        flows = list_flows()
        stdout.print("[bold cyan]Available Flows:[/bold cyan]")
        for flow in flows:
            stdout.print(f"  - {flow}")
        stdout.print(f"\n[dim]Usage: uv run generate_flow_diagram.py <flow_name>[/dim]")
        return 0

    # Find flow file
    flow_name = args.flow_name.replace("_flow", "").replace(".md", "")
    flow_file = get_flows_dir() / f"{flow_name}_flow.md"

    if not flow_file.exists():
        # Try with _flow suffix
        flow_file = get_flows_dir() / f"{flow_name}.md"

    if not flow_file.exists():
        console.print(f"[red]Flow '{flow_name}' not found[/red]")
        console.print("\nAvailable flows:")
        for flow in list_flows():
            console.print(f"  - {flow}")
        return 1

    # Read flow file
    content = flow_file.read_text()

    # Parse content
    events = parse_timeline_table(content)
    interactions = parse_system_interactions(content)

    if not events and not interactions:
        console.print(f"[yellow]Warning: No Timeline or System Interactions found in {flow_file.name}[/yellow]")

    # Generate diagram
    if args.format == "flowchart":
        diagram = generate_mermaid_flowchart(flow_name, events, interactions)
    elif args.format == "sequence":
        diagram = generate_mermaid_sequence(flow_name, interactions)
    else:
        diagram = generate_ascii_diagram(flow_name, events)

    # Output
    if args.output:
        output_path = Path(args.output)
        output_path.write_text(diagram)
        console.print(f"[green]Diagram written to {output_path}[/green]")
    else:
        # Print to stdout
        print(diagram)

    # Print usage hint
    if args.format in ["flowchart", "sequence"]:
        console.print("\n[dim]Paste output into: https://mermaid.live/ or a markdown file[/dim]")

    return 0


if __name__ == "__main__":
    sys.exit(main())
