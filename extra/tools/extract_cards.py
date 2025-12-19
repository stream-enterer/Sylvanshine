#!/usr/bin/env python3
import json
import re
import sys
from pathlib import Path
from dataclasses import dataclass, field, asdict
from typing import Optional

@dataclass
class Card:
    id: int = 0
    id_name: str = ""
    name: str = ""
    description: str = ""
    card_type: str = ""
    faction: str = ""
    faction_id: int = 0
    mana_cost: int = 0
    attack: Optional[int] = None
    health: Optional[int] = None
    durability: Optional[int] = None
    rarity: str = ""
    rarity_id: int = 0
    race: str = ""
    keywords: list = field(default_factory=list)
    is_general: bool = False
    is_token: bool = False
    is_hidden: bool = False
    set_name: str = ""
    lore_name: str = ""
    lore_text: str = ""

FACTIONS = {
    1: "Lyonar", 2: "Songhai", 3: "Vetruvian", 
    4: "Abyssian", 5: "Magmar", 6: "Vanar",
    100: "Neutral", 200: "Tutorial", 300: "Boss"
}

RARITIES = {
    0: "Fixed", 1: "Common", 2: "Rare", 
    3: "Epic", 4: "Legendary", 5: "Token", 6: "Mythron"
}

def parse_cards_lookup(content: str) -> dict:
    """Parse cardsLookup.coffee to get card ID mappings."""
    cards = {}
    current_group = None
    
    for line in content.split('\n'):
        group_match = re.match(r'\s*@(\w+):\s*\{', line)
        if group_match:
            current_group = group_match.group(1)
            continue
        
        if current_group and ':' in line and '{' not in line:
            match = re.match(r'\s*(\w+):\s*(\d+)', line)
            if match:
                name, id_val = match.groups()
                full_name = f"{current_group}.{name}"
                cards[int(id_val)] = full_name
                cards[full_name] = int(id_val)
    
    return cards

def parse_i18n(content: str) -> dict:
    """Parse cards.json for localized strings."""
    return json.loads(content)

def parse_lore(content: str) -> dict:
    """Parse cardLore.coffee for lore entries."""
    lore = {}
    pattern = r'l\[Cards\.(\w+)\.(\w+)\]\s*=\s*\{[^}]*name:\s*"([^"]*)"[^}]*text:\s*\[(.*?)\]\.join'
    
    for match in re.finditer(pattern, content, re.DOTALL):
        group, name, lore_name, text_parts = match.groups()
        card_key = f"{group}.{name}"
        text = re.sub(r'"\s*,\s*"', '\n\n', text_parts)
        text = text.strip().strip('"')
        lore[card_key] = {"name": lore_name, "text": text}
    
    return lore

def extract_card_block(block: str, i18n: dict, set_name: str) -> Optional[Card]:
    """Extract card data from a single card definition block."""
    card = Card(set_name=set_name)
    
    id_match = re.search(r'identifier\s*==\s*Cards\.(\w+)\.(\w+)', block)
    if not id_match:
        return None
    
    group, name = id_match.groups()
    card.id_name = f"{group}.{name}"
    
    type_match = re.search(r'card\s*=\s*new\s+(\w+)\(', block)
    if type_match:
        t = type_match.group(1)
        if t == "Unit":
            card.card_type = "Unit"
        elif t == "Artifact":
            card.card_type = "Artifact"
        elif "Spell" in t:
            card.card_type = "Spell"
        else:
            card.card_type = t
    
    faction_match = re.search(r'card\.factionId\s*=\s*Factions\.(\w+)', block)
    if faction_match:
        f = faction_match.group(1)
        faction_map = {
            "Faction1": (1, "Lyonar"), "Lyonar": (1, "Lyonar"),
            "Faction2": (2, "Songhai"), "Songhai": (2, "Songhai"),
            "Faction3": (3, "Vetruvian"), "Vetruvian": (3, "Vetruvian"),
            "Faction4": (4, "Abyssian"), "Abyssian": (4, "Abyssian"),
            "Faction5": (5, "Magmar"), "Magmar": (5, "Magmar"),
            "Faction6": (6, "Vanar"), "Vanar": (6, "Vanar"),
            "Neutral": (100, "Neutral"),
            "Tutorial": (200, "Tutorial"),
            "Boss": (300, "Boss"),
        }
        if f in faction_map:
            card.faction_id, card.faction = faction_map[f]
    
    name_match = re.search(r'card\.name\s*=\s*i18next\.t\(["\']cards\.([^"\']+)["\']', block)
    if name_match:
        key = name_match.group(1)
        card.name = i18n.get(key, key)
    
    desc_match = re.search(r'card\.setDescription\(i18next\.t\(["\']cards\.([^"\']+)["\']', block)
    if desc_match:
        key = desc_match.group(1)
        card.description = i18n.get(key, "")
    
    if not card.description:
        alt_desc_match = re.search(r'card\.setDescription\(i18next\.t\(["\']([^"\']+)["\']', block)
        if alt_desc_match:
            key = alt_desc_match.group(1)
            card.description = i18n.get(key, "")
    
    mana_match = re.search(r'card\.manaCost\s*=\s*(\d+)', block)
    if mana_match:
        card.mana_cost = int(mana_match.group(1))
    
    atk_match = re.search(r'card\.atk\s*=\s*(\d+)', block)
    if atk_match:
        card.attack = int(atk_match.group(1))
    
    hp_match = re.search(r'card\.maxHP\s*=\s*(\d+)', block)
    if hp_match:
        card.health = int(hp_match.group(1))
    
    durability_match = re.search(r'card\.durability\s*=\s*(\d+)', block)
    if durability_match:
        card.durability = int(durability_match.group(1))
    
    rarity_match = re.search(r'card\.rarityId\s*=\s*Rarity\.(\w+)', block)
    if rarity_match:
        r = rarity_match.group(1)
        for rid, rname in RARITIES.items():
            if rname == r:
                card.rarity = rname
                card.rarity_id = rid
                break
    
    race_match = re.search(r'card\.setRaceId\(Races\.(\w+)\)', block)
    if race_match:
        card.race = race_match.group(1)
    
    if re.search(r'card\.setIsGeneral\(true\)', block):
        card.is_general = True
    
    if re.search(r'card\.setIsHiddenInCollection\(true\)', block):
        card.is_hidden = True
    
    if re.search(r'ModifierToken', block):
        card.is_token = True
    
    keywords = []
    keyword_patterns = [
        (r'ModifierProvoke', 'Provoke'),
        (r'ModifierRanged|Ranged', 'Ranged'),
        (r'ModifierFlying|Flying', 'Flying'),
        (r'ModifierFirstStrike|Rush', 'Rush'),
        (r'ModifierTranscendance|Celerity', 'Celerity'),
        (r'ModifierBlastAttack|Blast', 'Blast'),
        (r'ModifierFrenzy|Frenzy', 'Frenzy'),
        (r'ModifierAirdrop', 'Airdrop'),
        (r'ModifierGrow', 'Grow'),
        (r'ModifierRebirth', 'Rebirth'),
        (r'ModifierForcefield', 'Forcefield'),
        (r'ModifierOpeningGambit', 'Opening Gambit'),
        (r'ModifierDyingWish', 'Dying Wish'),
        (r'ModifierBackstab', 'Backstab'),
        (r'ModifierDeathWatch', 'Deathwatch'),
        (r'ModifierInfiltrate', 'Infiltrate'),
        (r'ModifierBanding|Zeal', 'Zeal'),
    ]
    for pattern, kw in keyword_patterns:
        if re.search(pattern, block):
            keywords.append(kw)
    card.keywords = list(set(keywords))
    
    return card

def parse_factory_file(content: str, i18n: dict, set_name: str) -> list:
    """Parse a factory .coffee file and extract all cards."""
    cards = []
    blocks = re.split(r'\n\s*if\s*\(identifier\s*==', content)
    
    for block in blocks[1:]:
        block = "if (identifier ==" + block
        end_match = re.search(r'\n\s*if\s*\(identifier\s*==|\n\s*return\s+card', block)
        if end_match:
            block = block[:end_match.start()]
        
        card = extract_card_block(block, i18n, set_name)
        if card and card.name:
            cards.append(card)
    
    return cards

def main():
    if len(sys.argv) < 2:
        print("Usage: python extract_cards.py <duelyst_repo_path>")
        sys.exit(1)
    
    repo = Path(sys.argv[1])
    sdk = repo / "app" / "sdk"
    
    lookup_file = sdk / "cards" / "cardsLookup.coffee"
    i18n_file = repo / "app" / "localization" / "locales" / "en" / "cards.json"
    lore_file = sdk / "cards" / "cardLore.coffee"
    factory_dir = sdk / "cards" / "factory"
    
    print("Loading card ID mappings...", file=sys.stderr)
    card_ids = parse_cards_lookup(lookup_file.read_text())
    
    print("Loading i18n strings...", file=sys.stderr)
    i18n = parse_i18n(i18n_file.read_text())
    
    print("Loading lore...", file=sys.stderr)
    lore = parse_lore(lore_file.read_text())
    
    all_cards = []
    
    for factory_file in sorted(factory_dir.rglob("*.coffee")):
        set_name = factory_file.parent.name
        print(f"Processing {factory_file.relative_to(repo)}...", file=sys.stderr)
        
        content = factory_file.read_text()
        cards = parse_factory_file(content, i18n, set_name)
        
        for card in cards:
            if card.id_name in card_ids:
                card.id = card_ids[card.id_name]
            if card.id_name in lore:
                card.lore_name = lore[card.id_name]["name"]
                card.lore_text = lore[card.id_name]["text"]
        
        all_cards.extend(cards)
    
    seen_ids = set()
    unique_cards = []
    for card in all_cards:
        if card.id not in seen_ids:
            seen_ids.add(card.id)
            unique_cards.append(card)
    
    unique_cards.sort(key=lambda c: (c.faction_id, c.card_type, c.mana_cost, c.name))
    
    print(f"\nExtracted {len(unique_cards)} unique cards", file=sys.stderr)
    
    output = [asdict(c) for c in unique_cards]
    print(json.dumps(output, indent=2))

if __name__ == "__main__":
    main()
