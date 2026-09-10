# WorldMan themes

```text
themes/
  index.html          # demo: ui / web / token / erp scenes
  demo.js / demo-data.js
  catalog/            bilingual Meaning (en + 中文)
  minor|vibe|country/ ui|token|web|erp-palettes.mjs
  scripts/generate-theme-css.mjs
  lib/color-utils.mjs
  catalog.tsv
```

- `000` seeds `$PROJECT/sop/themes/` as a **fork** (palettes + generate script)
- `011` generates app CSS from the fork; recommended web styleclass names
- Country packs cover **zfr L2 + L3** (Tier I–III) locales

Generate (in a project):

```bash
node sop/themes/scripts/generate-theme-css.mjs
# open http://127.0.0.1:8765/ for the suite demo:
cd suite/worldman/themes && python3 -m http.server 8765
```

Rebuild suite pack:

```bash
node tools/rebuild-worldman-themes.mjs [/path/to/minor-themes]
node tools/check-theme-contrast.mjs
```
