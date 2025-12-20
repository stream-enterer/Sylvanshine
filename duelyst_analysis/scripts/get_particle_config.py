#!/usr/bin/env python3
# /// script
# requires-python = ">=3.11"
# dependencies = ["polars", "rich"]
# ///
"""Get particle emitter configuration and properties.

Usage:
    uv run get_particle_config.py <particle_name>

Arguments:
    particle_name:  Particle system name (e.g., 'fireexplosion', 'manaswirl')

Examples:
    uv run get_particle_config.py fireexplosion
    uv run get_particle_config.py manaspring
    uv run get_particle_config.py cloud
    uv run get_particle_config.py dot
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


def load_particles_tsv() -> pl.DataFrame:
    """Load the particles TSV file."""
    particles_path = get_data_dir() / "instances" / "particles.tsv"
    return pl.read_csv(particles_path, separator="\t")


def load_fx_tsv() -> pl.DataFrame:
    """Load the FX TSV file for cross-reference."""
    fx_path = get_data_dir() / "instances" / "fx.tsv"
    return pl.read_csv(fx_path, separator="\t")


def find_particles(df: pl.DataFrame, pattern: str) -> pl.DataFrame | None:
    """Find particles matching the name pattern."""
    matches = df.filter(
        pl.col("name").str.to_lowercase().str.contains(pattern.lower())
    )
    return matches if matches.height > 0 else None


def find_fx_using_particle(fx_df: pl.DataFrame, particle_name: str) -> list[str]:
    """Find FX that might use this particle system."""
    # Check if any FX name contains the particle name pattern
    pattern = particle_name.lower().replace("_", "")
    matches = fx_df.filter(
        pl.col("rsx_name").str.to_lowercase().str.contains(pattern)
    )
    return matches["rsx_name"].to_list()


def classify_particle(row: dict) -> str:
    """Classify particle type based on properties."""
    name = row.get("name", "").lower()
    duration = float(row.get("duration", -1))
    gravity_y = float(row.get("gravity_y", 0))
    speed = float(row.get("speed", 0))

    if duration > 0:
        return "burst"  # One-shot effect
    elif gravity_y < -50:
        return "falling"
    elif gravity_y > 50:
        return "rising"
    elif speed > 200:
        return "projectile"
    elif "cloud" in name or "dust" in name:
        return "ambient"
    elif "dot" in name:
        return "indicator"
    else:
        return "continuous"


def rgba_to_hex(r: float, g: float, b: float, a: float = 1.0) -> str:
    """Convert RGBA floats to hex color string."""
    r_int = min(255, max(0, int(r * 255)))
    g_int = min(255, max(0, int(g * 255)))
    b_int = min(255, max(0, int(b * 255)))
    return f"#{r_int:02x}{g_int:02x}{b_int:02x}"


def main() -> int:
    parser = argparse.ArgumentParser(
        description="Get particle emitter configuration and properties",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog=__doc__
    )
    parser.add_argument(
        "particle_name",
        help="Particle system name or pattern (e.g., 'fireexplosion', 'cloud')"
    )

    args = parser.parse_args()
    console = Console(stderr=True)
    stdout = Console()

    # Load data
    try:
        particles_df = load_particles_tsv()
        fx_df = load_fx_tsv()
    except Exception as e:
        console.print(f"[red]Error loading TSV files: {e}[/red]")
        return 1

    # Find matching particles
    matches = find_particles(particles_df, args.particle_name)

    if matches is None:
        console.print(f"[red]No particles found matching '{args.particle_name}'[/red]")
        return 1

    stdout.print(f"\n[bold]Found {matches.height} particle system(s)[/bold]\n")

    for row in matches.iter_rows(named=True):
        particle_name = row.get("name", "N/A")
        particle_type = classify_particle(row)
        stdout.print(Panel(
            f"[bold cyan]{particle_name}[/bold cyan] ([yellow]{particle_type}[/yellow])",
            expand=False
        ))

        # Emitter properties
        emitter_table = Table(title="Emitter Properties", show_header=True)
        emitter_table.add_column("Property", style="cyan")
        emitter_table.add_column("Value", style="green")

        emitter_table.add_row("File", row.get("file", "N/A"))
        emitter_table.add_row("Max Particles", str(row.get("max_particles", "N/A")))

        duration = float(row.get("duration", -1))
        if duration < 0:
            emitter_table.add_row("Duration", "Continuous (-1)")
        else:
            emitter_table.add_row("Duration (s)", f"{duration:.2f}")

        emitter_type = int(row.get("emitter_type", 0))
        emitter_type_name = "Gravity" if emitter_type == 0 else "Radial"
        emitter_table.add_row("Emitter Type", f"{emitter_type} ({emitter_type_name})")

        stdout.print(emitter_table)
        stdout.print()

        # Lifetime properties
        lifetime_table = Table(title="Particle Lifetime", show_header=True)
        lifetime_table.add_column("Property", style="cyan")
        lifetime_table.add_column("Value", style="yellow")

        lifespan = float(row.get("lifespan", 0))
        lifespan_var = float(row.get("lifespan_variance", 0))
        lifetime_table.add_row("Lifespan (s)", f"{lifespan:.2f}")
        lifetime_table.add_row("Lifespan Variance", f"{lifespan_var:.2f}")
        lifetime_table.add_row("Lifespan Range", f"{lifespan - lifespan_var:.2f} - {lifespan + lifespan_var:.2f}")

        stdout.print(lifetime_table)
        stdout.print()

        # Size properties
        size_table = Table(title="Particle Size", show_header=True)
        size_table.add_column("Property", style="cyan")
        size_table.add_column("Value", style="yellow")

        size_table.add_row("Start Size", str(row.get("start_size", "N/A")))
        size_table.add_row("Finish Size", str(row.get("finish_size", "N/A")))

        stdout.print(size_table)
        stdout.print()

        # Motion properties
        motion_table = Table(title="Motion Properties", show_header=True)
        motion_table.add_column("Property", style="cyan")
        motion_table.add_column("Value", style="magenta")

        speed = float(row.get("speed", 0))
        speed_var = float(row.get("speed_variance", 0))
        motion_table.add_row("Speed", str(speed))
        motion_table.add_row("Speed Variance", str(speed_var))
        motion_table.add_row("Angle", str(row.get("angle", "N/A")))
        motion_table.add_row("Angle Variance", str(row.get("angle_variance", "N/A")))
        motion_table.add_row("Gravity X", str(row.get("gravity_x", "N/A")))
        motion_table.add_row("Gravity Y", str(row.get("gravity_y", "N/A")))
        motion_table.add_row("Radial Accel", str(row.get("radial_accel", "N/A")))
        motion_table.add_row("Tangent Accel", str(row.get("tangent_accel", "N/A")))

        stdout.print(motion_table)
        stdout.print()

        # Color properties
        color_table = Table(title="Color Properties", show_header=True)
        color_table.add_column("Property", style="cyan")
        color_table.add_column("RGBA", style="green")
        color_table.add_column("Hex", style="yellow")

        start_r = float(row.get("start_r", 1))
        start_g = float(row.get("start_g", 1))
        start_b = float(row.get("start_b", 1))
        start_a = float(row.get("start_a", 1))
        finish_a = float(row.get("finish_a", 0))

        start_hex = rgba_to_hex(start_r, start_g, start_b)
        color_table.add_row(
            "Start Color",
            f"({start_r:.2f}, {start_g:.2f}, {start_b:.2f}, {start_a:.2f})",
            start_hex
        )
        color_table.add_row("Finish Alpha", str(finish_a), "N/A")

        stdout.print(color_table)
        stdout.print()

        # Blend mode
        blend_table = Table(title="Blending", show_header=True)
        blend_table.add_column("Property", style="cyan")
        blend_table.add_column("Value", style="yellow")
        blend_table.add_column("OpenGL Mode", style="white")

        blend_modes = {
            0: "GL_ZERO",
            1: "GL_ONE",
            770: "GL_SRC_ALPHA",
            771: "GL_ONE_MINUS_SRC_ALPHA",
            772: "GL_DST_ALPHA",
            773: "GL_ONE_MINUS_DST_ALPHA",
        }

        blend_src = int(row.get("blend_src", 770))
        blend_dst = int(row.get("blend_dst", 771))
        blend_table.add_row("Blend Src", str(blend_src), blend_modes.get(blend_src, "Unknown"))
        blend_table.add_row("Blend Dst", str(blend_dst), blend_modes.get(blend_dst, "Unknown"))

        # Determine blend effect
        if blend_src == 770 and blend_dst == 1:
            blend_effect = "Additive (glowing)"
        elif blend_src == 770 and blend_dst == 771:
            blend_effect = "Normal (alpha blend)"
        elif blend_src == 1 and blend_dst == 771:
            blend_effect = "Premultiplied alpha"
        else:
            blend_effect = "Custom"

        blend_table.add_row("Effect", blend_effect, "")

        stdout.print(blend_table)
        stdout.print()

        # Texture
        texture = row.get("texture", "N/A")
        if texture and texture != "N/A":
            stdout.print(f"[dim]Texture: {texture}[/dim]\n")

        # Cross-reference with FX
        related_fx = find_fx_using_particle(fx_df, particle_name)
        if related_fx:
            fx_tree = Tree("[bold]Related FX[/bold]")
            for fx_name in related_fx[:5]:
                fx_tree.add(f"[green]{fx_name}[/green]")
            if len(related_fx) > 5:
                fx_tree.add(f"[dim]... and {len(related_fx) - 5} more[/dim]")
            stdout.print(fx_tree)
            stdout.print()

    return 0


if __name__ == "__main__":
    sys.exit(main())
