#!/usr/bin/env python3
import json
import re
import sys
import time
from urllib.request import urlopen, Request
from urllib.parse import quote

WIKI_BASE = "https://duelyst.fandom.com/wiki/"

def fetch_page(url: str) -> str:
    req = Request(url, headers={"User-Agent": "Mozilla/5.0"})
    try:
        with urlopen(req, timeout=10) as resp:
            return resp.read().decode("utf-8")
    except Exception as e:
        print(f"Failed to fetch {url}: {e}", file=sys.stderr)
        return ""

def get_card_lore_page() -> str:
    return fetch_page(WIKI_BASE + "Card_Lore")

def extract_lore_links(html: str) -> list:
    pattern = r'<a[^>]*href="(/wiki/[^"]*_Lore)"[^>]*>([^<]+)</a>'
    return [(match.group(2), WIKI_BASE[:-1] + match.group(1)) 
            for match in re.finditer(pattern, html)]

def extract_lore_from_page(html: str) -> dict:
    lore = {}
    card_sections = re.split(r'<h[23][^>]*>', html)
    
    for section in card_sections:
        title_match = re.match(r'[^<]*<[^>]*>([^<]+)', section)
        if not title_match:
            continue
        
        card_name = re.sub(r'<[^>]+>', '', title_match.group(1)).strip()
        if not card_name or card_name in ["Contents", "Navigation"]:
            continue
        
        text_match = re.search(r'<p>(.*?)</p>', section, re.DOTALL)
        if text_match:
            text = re.sub(r'<[^>]+>', '', text_match.group(1))
            text = text.strip()
            if text and len(text) > 50:
                lore[card_name] = text
    
    return lore

def scrape_all_lore() -> dict:
    print("Fetching Card Lore index...", file=sys.stderr)
    index_html = get_card_lore_page()
    
    all_lore = {}
    lore_pages = extract_lore_links(index_html)
    
    if not lore_pages:
        print("Extracting lore from main page...", file=sys.stderr)
        all_lore = extract_lore_from_page(index_html)
    else:
        for name, url in lore_pages:
            print(f"Fetching {name}...", file=sys.stderr)
            html = fetch_page(url)
            page_lore = extract_lore_from_page(html)
            all_lore.update(page_lore)
            time.sleep(0.5)
    
    return all_lore

def merge_lore_with_cards(cards_file: str, lore: dict) -> list:
    with open(cards_file) as f:
        cards = json.load(f)
    
    for card in cards:
        name = card.get("name", "")
        if name in lore and not card.get("lore_text"):
            card["lore_text"] = lore[name]
    
    return cards

def main():
    if len(sys.argv) > 1 and sys.argv[1] == "--merge":
        if len(sys.argv) < 3:
            print("Usage: python scrape_lore.py --merge <cards.json>")
            sys.exit(1)
        lore = scrape_all_lore()
        cards = merge_lore_with_cards(sys.argv[2], lore)
        print(json.dumps(cards, indent=2))
    else:
        lore = scrape_all_lore()
        print(json.dumps(lore, indent=2))

if __name__ == "__main__":
    main()
