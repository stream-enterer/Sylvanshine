#!/usr/bin/env python3
# /// script
# requires-python = ">=3.11"
# dependencies = ["polars", "rich"]
# ///
"""Trace full dependency tree of imports (direct + transitive).

Usage:
    uv run trace_imports.py <file_path>

Example:
    uv run trace_imports.py app/sdk/actions/attackAction.coffee
    uv run trace_imports.py app/application.coffee
"""

import argparse
import sys
from pathlib import Path
from collections import defaultdict

import polars as pl
from rich.console import Console
from rich.tree import Tree
from rich.panel import Panel


def get_data_dir() -> Path:
    """Get the duelyst_analysis directory path."""
    return Path(__file__).parent.parent


def load_imports_tsv() -> pl.DataFrame:
    """Load the imports TSV file."""
    imports_path = get_data_dir() / "imports.tsv"
    return pl.read_csv(imports_path, separator="\t")


def build_import_graph(imports_df: pl.DataFrame) -> dict[str, list[tuple[str, str, str]]]:
    """Build a graph of file -> [(import_type, module, resolved)]."""
    graph: dict[str, list[tuple[str, str, str]]] = defaultdict(list)

    for row in imports_df.iter_rows(named=True):
        file = row["file"]
        import_type = row.get("import_type", "require")
        module = row.get("module", "")
        resolved = row.get("resolved", module)
        graph[file].append((import_type, module, resolved))

    return graph


def normalize_path(path: str) -> str:
    """Normalize file path for matching."""
    # Remove leading ./ or /
    path = path.lstrip("./")
    # Handle paths without extensions
    if not any(path.endswith(ext) for ext in [".coffee", ".js", ".ts"]):
        # Try common extensions
        for ext in [".coffee", ".js"]:
            if f"{path}{ext}" in path:
                return f"{path}{ext}"
    return path


def find_file_in_graph(graph: dict[str, list[tuple[str, str, str]]], search_path: str) -> str | None:
    """Find a file in the graph matching the search path."""
    normalized = normalize_path(search_path)

    # Exact match
    if normalized in graph:
        return normalized

    # Partial match
    for file in graph.keys():
        if file.endswith(normalized) or normalized in file:
            return file

    return None


def trace_imports(
    graph: dict[str, list[tuple[str, str, str]]],
    file: str,
    visited: set[str] | None = None,
    depth: int = 0,
    max_depth: int = 10
) -> dict[str, list[tuple[str, str, str]]]:
    """Recursively trace all imports."""
    if visited is None:
        visited = set()

    if file in visited or depth > max_depth:
        return {}

    visited.add(file)
    result: dict[str, list[tuple[str, str, str]]] = {}

    if file in graph:
        result[file] = graph[file]

        for import_type, module, resolved in graph[file]:
            if resolved and resolved not in visited:
                # Check if resolved file exists in graph
                resolved_file = find_file_in_graph(graph, resolved)
                if resolved_file:
                    child_imports = trace_imports(graph, resolved_file, visited, depth + 1, max_depth)
                    result.update(child_imports)

    return result


def build_tree(
    tree: Tree,
    graph: dict[str, list[tuple[str, str, str]]],
    file: str,
    visited: set[str],
    max_depth: int = 5,
    depth: int = 0
) -> None:
    """Build a rich Tree of imports."""
    if depth > max_depth or file in visited:
        if file in visited:
            tree.add(f"[dim]{file} (circular)[/dim]")
        return

    visited.add(file)

    if file in graph:
        for import_type, module, resolved in graph[file]:
            # Determine style based on import type
            if import_type == "require":
                style = "green"
            elif import_type == "import":
                style = "cyan"
            else:
                style = "yellow"

            # Check if it's a local file or npm package
            if resolved.startswith("app/"):
                icon = "[file]"
                label = f"{icon} [{style}]{resolved}[/{style}]"
            else:
                icon = "[package]"
                label = f"{icon} [{style}]{module}[/{style}] [dim]({import_type})[/dim]"

            branch = tree.add(label)

            # Recurse for local files
            if resolved.startswith("app/"):
                resolved_file = find_file_in_graph(graph, resolved)
                if resolved_file and resolved_file not in visited:
                    build_tree(branch, graph, resolved_file, visited.copy(), max_depth, depth + 1)


def main() -> int:
    parser = argparse.ArgumentParser(
        description="Trace full dependency tree of imports",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog=__doc__
    )
    parser.add_argument(
        "file_path",
        help="File path to trace imports for (e.g., 'app/sdk/actions/attackAction.coffee')"
    )
    parser.add_argument(
        "--depth", "-d",
        type=int,
        default=5,
        help="Maximum depth to trace (default: 5)"
    )
    parser.add_argument(
        "--flat", "-f",
        action="store_true",
        help="Show flat list instead of tree"
    )

    args = parser.parse_args()
    console = Console(stderr=True)
    stdout = Console()

    # Load data
    try:
        imports_df = load_imports_tsv()
    except Exception as e:
        console.print(f"[red]Error loading imports.tsv: {e}[/red]")
        return 1

    # Build import graph
    graph = build_import_graph(imports_df)

    # Find the file
    file = find_file_in_graph(graph, args.file_path)

    if file is None:
        console.print(f"[red]File not found in imports: '{args.file_path}'[/red]")
        console.print("[yellow]Available files matching pattern:[/yellow]")
        pattern = args.file_path.split("/")[-1].replace(".coffee", "").replace(".js", "")
        for f in list(graph.keys())[:10]:
            if pattern.lower() in f.lower():
                console.print(f"  {f}")
        return 1

    stdout.print(Panel(f"[bold cyan]Import Tree for: {file}[/bold cyan]", expand=False))

    if args.flat:
        # Flat list of all dependencies
        all_imports = trace_imports(graph, file, max_depth=args.depth)

        # Collect unique dependencies
        deps: set[str] = set()
        npm_deps: set[str] = set()

        for f, imports in all_imports.items():
            for import_type, module, resolved in imports:
                if resolved.startswith("app/"):
                    deps.add(resolved)
                else:
                    npm_deps.add(module)

        stdout.print(f"\n[bold]Local Dependencies ({len(deps)}):[/bold]")
        for dep in sorted(deps):
            stdout.print(f"  [green]{dep}[/green]")

        stdout.print(f"\n[bold]NPM Packages ({len(npm_deps)}):[/bold]")
        for dep in sorted(npm_deps):
            stdout.print(f"  [cyan]{dep}[/cyan]")
    else:
        # Tree view
        tree = Tree(f"[bold]{file}[/bold]")
        build_tree(tree, graph, file, set(), max_depth=args.depth)
        stdout.print(tree)

    # Summary
    if file in graph:
        direct_imports = graph[file]
        local_count = sum(1 for _, _, r in direct_imports if r.startswith("app/"))
        npm_count = len(direct_imports) - local_count

        stdout.print(f"\n[dim]Direct imports: {len(direct_imports)} ({local_count} local, {npm_count} npm)[/dim]")

    return 0


if __name__ == "__main__":
    sys.exit(main())
