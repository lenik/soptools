# Theme attachment schema

- `catalog/*.md` — family color-language prose
- `catalog.tsv` — id, label, type, group, family, paletteKey, catalog, locales
- `{minor,vibe,country}/{ui,token,web,erp}-palettes.mjs` — grouped HSL defs
- `scripts/generate-theme-css.mjs` — project-forkable CSS generator (seeded by SOP 000)

Project `sop/themes/` is a **fork** of this attachment: evolve palettes and the
generator for the product. App CSS should be **generated** from the fork, not
hand-duplicated.

Adapted from minor-themes@1.0.8; web + erp are WorldMan additions.
