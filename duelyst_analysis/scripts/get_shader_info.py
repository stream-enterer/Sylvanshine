#!/usr/bin/env python3
# /// script
# requires-python = ">=3.11"
# dependencies = ["polars", "rich"]
# ///
"""Get shader uniforms, attributes, and configuration.

Usage:
    uv run get_shader_info.py <shader_name>

Arguments:
    shader_name:  Shader name or pattern (e.g., 'Glow', 'Dissolve', 'Fire')

Examples:
    uv run get_shader_info.py Glow
    uv run get_shader_info.py Dissolve
    uv run get_shader_info.py Fire
    uv run get_shader_info.py fragment
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


def load_shaders_tsv() -> pl.DataFrame:
    """Load the shaders TSV file."""
    shaders_path = get_data_dir() / "instances" / "shaders.tsv"
    return pl.read_csv(shaders_path, separator="\t")


def load_fx_tsv() -> pl.DataFrame:
    """Load the FX TSV file for cross-reference."""
    fx_path = get_data_dir() / "instances" / "fx.tsv"
    return pl.read_csv(fx_path, separator="\t")


def find_shaders(df: pl.DataFrame, pattern: str) -> pl.DataFrame | None:
    """Find shaders matching the name pattern."""
    matches = df.filter(
        pl.col("name").str.to_lowercase().str.contains(pattern.lower())
    )
    return matches if matches.height > 0 else None


def parse_uniforms(uniform_str: str) -> list[dict[str, str]]:
    """Parse uniform string into list of uniform definitions."""
    if not uniform_str or uniform_str == "null":
        return []

    uniforms = []
    for uniform in uniform_str.split(";"):
        uniform = uniform.strip()
        if ":" in uniform:
            name, type_str = uniform.split(":", 1)
            uniforms.append({"name": name.strip(), "type": type_str.strip()})

    return uniforms


def parse_attributes(attr_str: str) -> list[dict[str, str]]:
    """Parse attribute string into list of attribute definitions."""
    if not attr_str or attr_str == "null":
        return []

    attributes = []
    for attr in attr_str.split(";"):
        attr = attr.strip()
        if ":" in attr:
            name, type_str = attr.split(":", 1)
            attributes.append({"name": name.strip(), "type": type_str.strip()})

    return attributes


def parse_defines(define_str: str) -> list[dict[str, str]]:
    """Parse defines string into list of preprocessor definitions."""
    if not define_str or define_str == "null":
        return []

    defines = []
    for define in define_str.split(";"):
        define = define.strip()
        if "=" in define:
            name, value = define.split("=", 1)
            defines.append({"name": name.strip(), "value": value.strip()})
        elif define:
            defines.append({"name": define, "value": "(flag)"})

    return defines


def get_uniform_description(uniform_name: str) -> str:
    """Get a description for common uniform types."""
    descriptions = {
        "u_time": "Animation time (seconds)",
        "u_phase": "Effect phase (0.0-1.0)",
        "u_resolution": "Screen resolution",
        "u_texResolution": "Texture resolution",
        "u_color": "Base color (RGB)",
        "u_intensity": "Effect intensity",
        "u_frequency": "Oscillation frequency",
        "u_amplitude": "Wave amplitude",
        "u_threshold": "Cutoff threshold",
        "u_brightness": "Brightness multiplier",
        "u_gamma": "Gamma correction",
        "u_size": "Effect size",
        "u_seed": "Random seed",
        "u_depthMap": "Depth buffer texture",
        "u_noiseMap": "Noise texture",
        "u_cubeMap": "Cube map texture",
        "u_previousBloom": "Previous bloom pass",
        "u_transition": "Transition value (0.0-1.0)",
    }

    for key, desc in descriptions.items():
        if key in uniform_name:
            return desc

    if uniform_name.startswith("u_"):
        return "Custom uniform"
    return ""


def find_related_shaders(df: pl.DataFrame, shader_name: str) -> list[str]:
    """Find related shaders (vertex/fragment pairs)."""
    # Extract base name
    base_name = shader_name.replace("Fragment", "").replace("Vertex", "")

    related = df.filter(
        (pl.col("name").str.contains(base_name)) &
        (pl.col("name") != shader_name)
    )

    return related["name"].to_list()


def find_fx_using_shader(fx_df: pl.DataFrame, shader_name: str) -> list[str]:
    """Find FX that might use effects related to this shader."""
    # Map shader patterns to FX patterns
    shader_lower = shader_name.lower()
    fx_patterns = []

    if "dissolve" in shader_lower:
        fx_patterns.extend(["dissolve", "death", "spawn"])
    if "glow" in shader_lower:
        fx_patterns.extend(["glow", "buff", "aura"])
    if "fire" in shader_lower:
        fx_patterns.extend(["fire", "burn", "flame"])
    if "distortion" in shader_lower:
        fx_patterns.extend(["distort", "warp", "ripple"])
    if "blur" in shader_lower:
        fx_patterns.extend(["blur", "motion"])

    if not fx_patterns:
        # Use shader name directly
        base = shader_name.replace("Fragment", "").replace("Vertex", "").lower()
        fx_patterns = [base]

    results = []
    for pattern in fx_patterns:
        matches = fx_df.filter(
            pl.col("rsx_name").str.to_lowercase().str.contains(pattern)
        )
        results.extend(matches["rsx_name"].to_list())

    return list(set(results))[:10]  # Deduplicate and limit


def main() -> int:
    parser = argparse.ArgumentParser(
        description="Get shader uniforms, attributes, and configuration",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog=__doc__
    )
    parser.add_argument(
        "shader_name",
        help="Shader name or pattern (e.g., 'Glow', 'Dissolve')"
    )

    args = parser.parse_args()
    console = Console(stderr=True)
    stdout = Console()

    # Load data
    try:
        shaders_df = load_shaders_tsv()
        fx_df = load_fx_tsv()
    except Exception as e:
        console.print(f"[red]Error loading TSV files: {e}[/red]")
        return 1

    # Find matching shaders
    matches = find_shaders(shaders_df, args.shader_name)

    if matches is None:
        console.print(f"[red]No shaders found matching '{args.shader_name}'[/red]")
        return 1

    stdout.print(f"\n[bold]Found {matches.height} shader(s)[/bold]\n")

    for row in matches.iter_rows(named=True):
        shader_name = row.get("name", "N/A")
        shader_type = row.get("type", "N/A")
        type_color = "green" if shader_type == "fragment" else "blue" if shader_type == "vertex" else "yellow"

        stdout.print(Panel(
            f"[bold cyan]{shader_name}[/bold cyan] ([{type_color}]{shader_type}[/{type_color}])",
            expand=False
        ))

        # Basic info
        info_table = Table(title="Shader Info", show_header=True)
        info_table.add_column("Property", style="cyan")
        info_table.add_column("Value", style="green")

        info_table.add_row("File", row.get("file", "N/A"))
        info_table.add_row("Type", shader_type)
        info_table.add_row("Lines", str(row.get("lines", "N/A")))

        stdout.print(info_table)
        stdout.print()

        # Uniforms
        uniforms = parse_uniforms(row.get("uniforms", ""))
        if uniforms:
            uniform_table = Table(title="Uniforms", show_header=True)
            uniform_table.add_column("Name", style="cyan")
            uniform_table.add_column("Type", style="yellow")
            uniform_table.add_column("Description", style="dim")

            for u in uniforms:
                desc = get_uniform_description(u["name"])
                uniform_table.add_row(u["name"], u["type"], desc)

            stdout.print(uniform_table)
            stdout.print()

        # Attributes
        attributes = parse_attributes(row.get("attributes", ""))
        if attributes:
            attr_table = Table(title="Attributes", show_header=True)
            attr_table.add_column("Name", style="cyan")
            attr_table.add_column("Type", style="yellow")

            for a in attributes:
                attr_table.add_row(a["name"], a["type"])

            stdout.print(attr_table)
            stdout.print()

        # Defines
        defines = parse_defines(row.get("defines", ""))
        if defines:
            define_table = Table(title="Preprocessor Defines", show_header=True)
            define_table.add_column("Name", style="cyan")
            define_table.add_column("Value", style="magenta")

            for d in defines:
                define_table.add_row(d["name"], d["value"])

            stdout.print(define_table)
            stdout.print()

        # Related shaders
        related = find_related_shaders(shaders_df, shader_name)
        if related:
            related_tree = Tree("[bold]Related Shaders[/bold]")
            for rel in related:
                rel_type = "vertex" if "Vertex" in rel else "fragment" if "Fragment" in rel else "helper"
                color = "blue" if rel_type == "vertex" else "green" if rel_type == "fragment" else "yellow"
                related_tree.add(f"[{color}]{rel}[/{color}] ({rel_type})")
            stdout.print(related_tree)
            stdout.print()

        # FX that might use this shader
        fx_list = find_fx_using_shader(fx_df, shader_name)
        if fx_list:
            fx_tree = Tree("[bold]Potentially Related FX[/bold]")
            for fx_name in fx_list[:5]:
                fx_tree.add(f"[magenta]{fx_name}[/magenta]")
            if len(fx_list) > 5:
                fx_tree.add(f"[dim]... and {len(fx_list) - 5} more[/dim]")
            stdout.print(fx_tree)
            stdout.print()

    return 0


if __name__ == "__main__":
    sys.exit(main())
