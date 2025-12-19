#!/usr/bin/env python3
import curses
import json
import sys

FIELDS = [
    "id", "id_name", "name", "description", "card_type", "faction",
    "faction_id", "mana_cost", "attack", "health", "durability",
    "rarity", "rarity_id", "race", "keywords", "is_general",
    "is_token", "is_hidden", "set_name", "lore_name", "lore_text"
]

def main(stdscr, cards):
    curses.curs_set(0)
    curses.init_pair(1, curses.COLOR_BLACK, curses.COLOR_WHITE)
    curses.init_pair(2, curses.COLOR_GREEN, curses.COLOR_BLACK)
    
    selected = set(["id", "name", "description", "card_type", "faction", "mana_cost", "attack", "health", "rarity"])
    cursor = 0
    
    while True:
        stdscr.clear()
        h, w = stdscr.getmaxyx()
        
        stdscr.addstr(0, 0, "Select fields (SPACE=toggle, a=all, n=none, ENTER=export, q=quit)", curses.A_BOLD)
        stdscr.addstr(1, 0, f"Selected: {len(selected)}/{len(FIELDS)}")
        
        for i, field in enumerate(FIELDS):
            y = i + 3
            if y >= h - 1:
                break
            
            marker = "[x]" if field in selected else "[ ]"
            line = f"{marker} {field}"
            
            if i == cursor:
                stdscr.addstr(y, 0, line, curses.color_pair(1))
            elif field in selected:
                stdscr.addstr(y, 0, line, curses.color_pair(2))
            else:
                stdscr.addstr(y, 0, line)
        
        stdscr.refresh()
        
        key = stdscr.getch()
        
        if key in (ord('q'), ord('Q'), 27):
            return None
        elif key in (ord('\n'), ord('\r')):
            return selected
        elif key == ord(' '):
            field = FIELDS[cursor]
            if field in selected:
                selected.discard(field)
            else:
                selected.add(field)
        elif key == ord('a'):
            selected = set(FIELDS)
        elif key == ord('n'):
            selected.clear()
        elif key in (curses.KEY_UP, ord('k')):
            cursor = max(0, cursor - 1)
        elif key in (curses.KEY_DOWN, ord('j')):
            cursor = min(len(FIELDS) - 1, cursor + 1)

def extract_fields(cards, fields):
    out = []
    for card in cards:
        out.append({f: card.get(f) for f in fields if f in card})
    return out

if __name__ == "__main__":
    if len(sys.argv) < 2:
        print("Usage: python field_picker.py <cards.json> [output.json]", file=sys.stderr)
        sys.exit(1)
    
    with open(sys.argv[1]) as f:
        cards = json.load(f)
    
    selected = curses.wrapper(main, cards)
    
    if selected is None:
        print("Cancelled", file=sys.stderr)
        sys.exit(0)
    
    result = extract_fields(cards, sorted(selected, key=FIELDS.index))
    
    if len(sys.argv) > 2:
        with open(sys.argv[2], 'w') as f:
            json.dump(result, f, indent=2)
        print(f"Wrote {len(result)} cards to {sys.argv[2]}", file=sys.stderr)
    else:
        print(json.dumps(result, indent=2))
