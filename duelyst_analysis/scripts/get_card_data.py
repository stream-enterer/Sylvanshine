#!/usr/bin/env python3
# /// script
# requires-python = ">=3.11"
# dependencies = ["polars", "rich"]
# ///
"""Get complete card information including cost, stats, abilities, sounds, FX.

Usage:
    uv run get_card_data.py <card_id>

Example:
    uv run get_card_data.py Cards.Faction1.SilverguardKnight
    uv run get_card_data.py SilverguardKnight
    uv run get_card_data.py Provoke
"""

import argparse
import sys
from pathlib import Path

import polars as pl
from rich.console import Console
from rich.table import Table
from rich.panel import Panel
from rich.columns import Columns


def get_data_dir() -> Path:
    """Get the duelyst_analysis directory path."""
    return Path(__file__).parent.parent


def load_cards_tsv() -> pl.DataFrame:
    """Load the cards TSV file."""
    cards_path = get_data_dir() / "instances" / "cards.tsv"
    return pl.read_csv(cards_path, separator="\t")


def load_units_tsv() -> pl.DataFrame:
    """Load the units TSV file."""
    units_path = get_data_dir() / "instances" / "units.tsv"
    return pl.read_csv(units_path, separator="\t")


def load_spells_tsv() -> pl.DataFrame:
    """Load the spells TSV file."""
    spells_path = get_data_dir() / "instances" / "spells.tsv"
    return pl.read_csv(spells_path, separator="\t")


def load_artifacts_tsv() -> pl.DataFrame:
    """Load the artifacts TSV file."""
    artifacts_path = get_data_dir() / "instances" / "artifacts.tsv"
    return pl.read_csv(artifacts_path, separator="\t")


def load_modifiers_tsv() -> pl.DataFrame:
    """Load the modifiers TSV file."""
    modifiers_path = get_data_dir() / "instances" / "modifiers.tsv"
    return pl.read_csv(modifiers_path, separator="\t")


def find_card(cards_df: pl.DataFrame, card_id: str) -> pl.DataFrame | None:
    """Find cards matching the identifier."""
    # Exact match on identifier
    matches = cards_df.filter(
        pl.col("identifier") == card_id
    )

    if matches.height == 0:
        # Partial match on identifier or name
        pattern = card_id.lower()
        matches = cards_df.filter(
            pl.col("identifier").str.to_lowercase().str.contains(pattern) |
            pl.col("name").str.to_lowercase().str.contains(pattern)
        )

    return matches if matches.height > 0 else None


def find_unit(units_df: pl.DataFrame, card_id: str) -> pl.DataFrame | None:
    """Find unit data matching the card ID."""
    matches = units_df.filter(
        pl.col("id").str.contains(card_id)
    )

    if matches.height == 0:
        pattern = card_id.lower().replace("cards.", "")
        matches = units_df.filter(
            pl.col("id").str.to_lowercase().str.contains(pattern)
        )

    return matches if matches.height > 0 else None


def find_modifiers_by_name(modifiers_df: pl.DataFrame, ability_names: list[str]) -> pl.DataFrame:
    """Find modifiers by their class names."""
    if not ability_names:
        return pl.DataFrame()

    pattern = "|".join(ability_names)
    return modifiers_df.filter(
        pl.col("class_name").str.contains(pattern)
    )


def main() -> int:
    parser = argparse.ArgumentParser(
        description="Get complete card information",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog=__doc__
    )
    parser.add_argument(
        "card_id",
        help="Card ID or name (e.g., 'Cards.Faction1.SilverguardKnight', 'SilverguardKnight')"
    )

    args = parser.parse_args()
    console = Console(stderr=True)
    stdout = Console()

    # Load data
    try:
        cards_df = load_cards_tsv()
        units_df = load_units_tsv()
        modifiers_df = load_modifiers_tsv()
    except Exception as e:
        console.print(f"[red]Error loading TSV files: {e}[/red]")
        return 1

    # Try to load optional data
    try:
        spells_df = load_spells_tsv()
    except Exception:
        spells_df = None

    try:
        artifacts_df = load_artifacts_tsv()
    except Exception:
        artifacts_df = None

    # Find the card
    card_matches = find_card(cards_df, args.card_id)

    if card_matches is None:
        console.print(f"[red]No card found matching '{args.card_id}'[/red]")
        return 1

    for card_row in card_matches.iter_rows(named=True):
        card_identifier = card_row.get("identifier", "Unknown")
        stdout.print(Panel(f"[bold cyan]{card_identifier}[/bold cyan]", expand=False))

        # Basic card info from cards.tsv
        info_table = Table(title="Card Info", show_header=True)
        info_table.add_column("Property", style="cyan")
        info_table.add_column("Value", style="green")

        info_table.add_row("ID", str(card_row.get("id", "N/A")))
        info_table.add_row("Identifier", card_identifier)
        info_table.add_row("Name", card_row.get("name", "N/A"))
        info_table.add_row("Card Group", card_row.get("card_group", "N/A"))
        info_table.add_row("Card Type", card_row.get("card_type", "N/A"))

        stdout.print(info_table)
        stdout.print()

        # Find unit data
        unit_matches = find_unit(units_df, card_identifier)

        if unit_matches is not None and unit_matches.height > 0:
            unit_row = unit_matches.row(0, named=True)

            # Stats table
            stats_table = Table(title="Unit Stats", show_header=True)
            stats_table.add_column("Stat", style="cyan")
            stats_table.add_column("Value", style="green")

            stats_table.add_row("Attack", str(unit_row.get("attack", "N/A")))
            stats_table.add_row("HP", str(unit_row.get("hp", "N/A")))
            stats_table.add_row("Cost", str(unit_row.get("cost", "N/A")))
            stats_table.add_row("Faction", str(unit_row.get("faction_id", "N/A")))
            stats_table.add_row("Rarity", str(unit_row.get("rarity_id", "N/A")))
            stats_table.add_row("Race", str(unit_row.get("race_id", "N/A")))
            stats_table.add_row("Is General", str(unit_row.get("is_general", "N/A")))

            stdout.print(stats_table)
            stdout.print()

            # Abilities
            abilities = unit_row.get("abilities", "")
            if abilities and abilities != "null":
                ability_list = abilities.split("|")

                abilities_table = Table(title="Abilities", show_header=True)
                abilities_table.add_column("Ability", style="magenta")

                for ability in ability_list:
                    abilities_table.add_row(ability)

                stdout.print(abilities_table)
                stdout.print()

                # Get modifier details
                modifier_matches = find_modifiers_by_name(modifiers_df, ability_list)
                if modifier_matches.height > 0:
                    mod_table = Table(title="Ability Details", show_header=True)
                    mod_table.add_column("Modifier", style="cyan", width=30)
                    mod_table.add_column("Description", style="yellow", width=40)
                    mod_table.add_column("Triggers", style="green")

                    for mod_row in modifier_matches.iter_rows(named=True):
                        mod_table.add_row(
                            mod_row.get("class_name", ""),
                            str(mod_row.get("applied_description", "N/A"))[:40],
                            str(mod_row.get("triggers", "N/A"))
                        )

                    stdout.print(mod_table)
                    stdout.print()

            # Sounds
            sound_resource = unit_row.get("sound_resource", "")
            if sound_resource and sound_resource != "null":
                sounds = sound_resource.split("|")
                sound_labels = ["Deploy", "Walk", "Attack Swing", "Hit", "Attack Impact", "Death"]

                sounds_table = Table(title="Sound Resources", show_header=True)
                sounds_table.add_column("Event", style="cyan")
                sounds_table.add_column("Sound", style="magenta")

                for i, sound in enumerate(sounds):
                    label = sound_labels[i] if i < len(sound_labels) else f"Sound {i}"
                    sounds_table.add_row(label, sound)

                stdout.print(sounds_table)
                stdout.print()

            # FX
            fx_resource = unit_row.get("fx_resource", "")
            if fx_resource and fx_resource != "null":
                fx_table = Table(title="FX Resource", show_header=True)
                fx_table.add_column("FX", style="yellow")
                fx_table.add_row(fx_resource)
                stdout.print(fx_table)
                stdout.print()

            # Sprite resources
            sprite_resource = unit_row.get("sprite_resource", "")
            if sprite_resource and sprite_resource != "null":
                sprites = sprite_resource.split("|")
                anim_types = ["Breathing", "Idle", "Run", "Attack", "Hit/Damage", "Death",
                             "CastStart", "CastEnd", "CastLoop", "Cast"]

                sprites_table = Table(title="Sprite Resources", show_header=True)
                sprites_table.add_column("Animation", style="cyan")
                sprites_table.add_column("Resource", style="green")

                for i, sprite in enumerate(sprites):
                    label = anim_types[i] if i < len(anim_types) else f"Anim {i}"
                    sprites_table.add_row(label, sprite)

                stdout.print(sprites_table)

        stdout.print()

    return 0


if __name__ == "__main__":
    sys.exit(main())
