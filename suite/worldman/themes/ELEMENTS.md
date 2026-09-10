# WorldMan theme elements

Color values live in grouped `.mjs` palettes (like minor-themes).

```text
themes/
  catalog/          # bilingual color-language prose (en + 中文)
  minor/            # soft / pride
  vibe/             # media / retro
  country/          # cultural country palettes (zfr L2 + extended)
  lib/color-utils.mjs
  catalog.tsv       # includes locales column
```

Each of `minor/`, `vibe/`, `country/`:

| File | Purpose |
|------|---------|
| `ui-palettes.mjs` | UiTheme-style roles |
| `token-palettes.mjs` | Syntax tokens |
| `web-palettes.mjs` | WorldMan web styleclasses (SOP 011) |
| `erp-palettes.mjs` | ERP semantic colors |

## Web primaries

background, foreground, primary, primary-foreground, card, card-foreground,
accent, accent-foreground, muted-foreground, border, ring, destructive, success, warning

## ERP primaries

page, page-foreground, panel, panel-foreground, header, toolbar, link,
button, button-foreground, danger, danger-foreground, success, success-foreground,
warning, warning-foreground, table-header, table-border, row-alt, form-border,
form-focus, status-neutral, status-active, status-done, status-error,
amount-in, amount-out

## Contrast / 色彩可分

Semantic foreground/background pairs **must** meet contrast ≥ 4.5:1
(WCAG-style ratio; same method as minor-themes `ensureContrast`).

Checked pairs (see `lib/color-utils.mjs`):

- UI: windowFg/windowBg, windowFg/surfaceBg, listFg/windowBg, promptFg/promptBg, …
- Web: foreground/background, card-foreground/card, primary-foreground/primary, …
- ERP: page-foreground/page, panel-foreground/panel, button-foreground/button, …

```bash
node tools/rebuild-worldman-themes.mjs
node tools/check-theme-contrast.mjs
```

## zfr L2 country coverage

Tier I + II locales map to country families:

- `fr` → France
- `it` → Italy
- `de` → German
- `es_MX` → Mexico
- `zh_CN` → China
- `ja` → Japan
- `ko` → Korea
- `ar` → Arab
- `id` → Indonesia
- `vi` → Viet
- `th` → Thai
- `pt_BR` → Brasil
- `nl` → Netherlands
- `pl` → Poland
- `tr` → Turkey
- `ru` → Russia
- `hi` → India
- `sv` → Sweden
- `no` → Norway
- `da` → Denmark
- `fi` → Finland
- `cs` → Czech
- `ro` → Romania
- `el` → Greece
- `hu` → Hungary
- `bg` → Bulgaria
- `uk` → Ukraine
- `kk` → Kazakhstan
- `fil_PH` → Philippines
- `bn` → Bengal
- `mr` → Marathi
- `ta` → Tamil
- `te` → Telugu
- `he` → Hebrew
- `sw` → Swahili

Missing after rebuild: (none).
