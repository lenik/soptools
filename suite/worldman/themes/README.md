# WorldMan themes

```text
themes/
  index.html          # demo: ui / web / token / erp scenes
  demo.js / demo-data.js
  catalog/            bilingual Meaning (en + 中文)
  minor|vibe|country/ ui|token|web|erp-palettes.mjs
  lib/color-utils.mjs
  catalog.tsv
```

- `000` → `$PROJECT/sop/themes/`
- `011` may use **all** class sets (ui / web / token / erp) by real semantics
- Country packs cover **zfr L2 + L3** (Tier I–III) locales

Preview:

```bash
cd suite/worldman/themes && python3 -m http.server 8765
# open http://127.0.0.1:8765/
```

Rebuild:

```bash
node tools/rebuild-worldman-themes.mjs [/path/to/minor-themes]
node tools/check-theme-contrast.mjs
```
