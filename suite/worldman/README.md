# worldman suite

See [../README.md](../README.md) for layout and branch selection.

Each language branch (`default`, `zh_CN`, `ja`) contains the full WorldMan
workflow step set; see that branch’s `README.md` for naming conventions, GPT
headers, and architecture notes.

Shared attachment (not language-specific):

- `themes/` — `catalog/*.md` (en + 中文), `index.html` demo (ui/web/token/erp),
  and `minor|vibe|country/{ui,token,web,erp}-palettes.mjs` with contrast-checked
  fg/bg pairs. Country packs cover zfr L2+L3 (Tier I–III). Step `000` copies
  into `sop/themes/`; step `011` may use **all** class sets by semantics.
