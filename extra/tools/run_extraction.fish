#!/usr/bin/env fish

set repo ~/.local/git/duelyst
set outdir .
set scripts (dirname (status filename))

mkdir -p $outdir

echo "=== Extracting cards from source ===" >&2
python3 $scripts/extract_cards.py $repo > $outdir/cards.json
or begin; echo "Extraction failed"; exit 1; end

echo "=== Verifying extraction ===" >&2
python3 $scripts/verify_cards.py $outdir/cards.json

echo "=== Output written to $outdir/cards.json ===" >&2

read -P "Scrape wiki for additional lore? [y/N] " -n 1 confirm
if test "$confirm" = y
    echo "=== Scraping wiki lore ===" >&2
    python3 $scripts/scrape_lore.py --merge $outdir/cards.json > $outdir/cards_with_lore.json
    mv $outdir/cards_with_lore.json $outdir/cards.json
    echo "=== Lore merged ===" >&2
end

echo "=== Done ===" >&2
echo $outdir/cards.json
