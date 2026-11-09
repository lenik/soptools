# SOP suites

Shipped SOP packs live under `suite/<name>/<branch>/`.

Each suite root carries:

```text
suite/<name>/
    VERSION          # suite semver shown in sopwin status bar (e.g. 1.1)
    default/ …       # language branches
```

Suite icons (Tabler outline SVGs) live in `assets/icons/suite/<name>.svg`;
the Meson build rasterizes 32×32 PNGs into the build dir / install prefix
(`…/soptools/icons/suite/`). sopwin’s status bar shows `[icon] <name> <version>`
on the right (e.g. worldman 1.1).

## worldman

Guided construction of a **WorldMan** module (web + backend + optional mobile)
from a Figma prototype export.

```text
suite/worldman/
    default/     # English prompts and scripts
    zh_CN/       # Simplified Chinese
    ja/          # Japanese
    themes/      # catalog/ + minor|vibe|country/*.mjs → $PROJECT/sop/themes/ in 000
```

Rough coverage (same step ids across branches):

| Seq | Role | Purpose |
|-----|------|---------|
| 000 | shell | Refactor Figma export into WorldMan layout; archive scaffolding; delete `.git` (no revert to export); seed `sop/themes/`; `[sop000]` commit |
| 010 | codex | Freeze `sop/design-inventory.md`; refactor web; root `package.json` `db:*` / `dev:*` scripts; localize demo assets (Google* via `:8118`); evolving `[sop010]` commits |
| 011 | codex | Optional theming (`extend: previous` → merges into `010` when included) |
| 020 | gpt/codex | PRD (inventory-backed UI structure) |
| 030–040 | gpt/codex | Prisma schema + seed (mock extraction + `seed-coverage.md`) |
| 050–070 | gpt/codex | Backend design / implement / API tests |
| 080–090 | codex | Web↔backend integrate (seed-coverage gate); API docs |
| 100 | gpt/codex | Author `web-e2e/workflows.md` |
| 110–120 | codex | Route smoke + workflow Playwright (120 also merges TUC/NTC/ECS/TODO) |
| 150 | codex | Final coverage audit (inventory + seed + e2e) |

Branch selection: `sopwin` picks `default` / `zh_CN` / `ja` from `LANG` (or
`LC_ALL` / `LC_MESSAGES`). Override with `-S/--sop-dir`, or choose suite with
`-s/--suite` (default `worldman`). GUI **View → Language** switches branches
within the current suite.

## vrischika

Guided **game vertical slice** construction from Vrischika templates (`vsk`).

```text
suite/vrischika/
    default/
    zh_CN/
    catalog.md
```

Design artifacts land in the game project under `sop/GDD.md`,
`sop/GDD-zh_CN.md`, `sop/TDD.md`, `sop/ADD.md`.

