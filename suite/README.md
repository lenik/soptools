# SOP suites

Shipped SOP packs live under `suite/<name>/<branch>/`.

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
| 000 | shell | Refactor Figma export into WorldMan layout; seed `sop/themes/` |
| 010 | codex | Refactor web; localize demo assets |
| 011 | codex | Theme web from `web-palettes.mjs` + switcher |
| 020 | gpt/codex | PRD |
| 030–040 | gpt/codex | Prisma schema + seed |
| 050–070 | gpt/codex | Backend design / implement / API tests |
| 080–090 | codex | Web↔backend integrate; API docs |
| 100 | gpt/codex | Author `web-e2e/workflows.md` |
| 110–120 | codex | Route smoke + workflow Playwright (120 also merges TUC/NTC/ECS/TODO) |
| 150 | codex | Final coverage audit |

Branch selection: `sopwin` picks `default` / `zh_CN` / `ja` from `LANG` (or
`LC_ALL` / `LC_MESSAGES`). Override with `-S/--sop-dir`, or choose suite with
`-s/--suite` (default `worldman`). GUI **View → Language** switches branches
within the current suite.
