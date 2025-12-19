#!/usr/bin/env python3
import json
import sys
from collections import Counter

def verify_cards(cards_file: str):
    with open(cards_file) as f:
        cards = json.load(f)
    
    print(f"Total cards: {len(cards)}\n")
    
    by_type = Counter(c["card_type"] for c in cards)
    print("By Type:")
    for t, count in sorted(by_type.items()):
        print(f"  {t}: {count}")
    
    print("\nBy Faction:")
    by_faction = Counter(c["faction"] for c in cards)
    for f, count in sorted(by_faction.items(), key=lambda x: -x[1]):
        print(f"  {f}: {count}")
    
    print("\nBy Rarity:")
    by_rarity = Counter(c["rarity"] for c in cards if c["rarity"])
    for r, count in sorted(by_rarity.items()):
        print(f"  {r}: {count}")
    
    print("\nBy Set:")
    by_set = Counter(c["set_name"] for c in cards)
    for s, count in sorted(by_set.items()):
        print(f"  {s}: {count}")
    
    generals = [c for c in cards if c["is_general"]]
    print(f"\nGenerals: {len(generals)}")
    for g in sorted(generals, key=lambda x: x["faction"]):
        print(f"  {g['faction']}: {g['name']}")
    
    with_lore = sum(1 for c in cards if c.get("lore_text"))
    print(f"\nCards with lore: {with_lore}")
    
    missing_name = [c for c in cards if not c["name"]]
    missing_stats = [c for c in cards if c["card_type"] == "Unit" and (c["attack"] is None or c["health"] is None)]
    missing_durability = [c for c in cards if c["card_type"] == "Artifact" and c.get("durability") is None]
    
    print("\nData Quality:")
    print(f"  Missing name: {len(missing_name)}")
    print(f"  Units missing stats: {len(missing_stats)}")
    print(f"  Artifacts missing durability: {len(missing_durability)}")
    
    if missing_stats:
        print("\n  Units without stats:")
        for c in missing_stats[:10]:
            print(f"    {c['id_name']}: {c['name']}")
    
    playable = [c for c in cards 
                if not c["is_hidden"] 
                and not c["is_token"]
                and c["faction"] not in ["Tutorial", "Boss"]
                and c["card_type"] in ["Unit", "Spell", "Artifact"]]
    
    print(f"\nPlayable (non-hidden, non-token, non-boss): {len(playable)}")
    
    expected_ranges = {
        "Unit": (300, 500),
        "Spell": (150, 300),
        "Artifact": (40, 80),
    }
    
    print("\nExpected vs Actual (playable only):")
    playable_by_type = Counter(c["card_type"] for c in playable)
    for t, (lo, hi) in expected_ranges.items():
        actual = playable_by_type.get(t, 0)
        status = "✓" if lo <= actual <= hi else "?"
        print(f"  {t}: {actual} (expected {lo}-{hi}) {status}")

if __name__ == "__main__":
    if len(sys.argv) < 2:
        print("Usage: python verify_cards.py <cards.json>")
        sys.exit(1)
    verify_cards(sys.argv[1])
