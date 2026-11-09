# WorldMan project workflow SOP

This archive is the default SOP for building a **WorldMan** module (webapp with `backend/`, `web/`, optional `mobile/`, and e2e tests). Modules usually begin as a **Figma prototype export**; step `000` refactoring that export into the standard layout is required. Steps are independently reusable prompt/command files.

## Naming convention

```text
<seq><variant>.<_padded_role>.<title>.(md|sh|get)
```

Examples:

```text
000a._shell.refactor_figma.sh
010a.__code.refactor_web.md
020a.__chat.create_prd.get
020z.__code.create_prd.md
050z.__code.prepare_backend_implementation.md
```

Sequence (`seq`): three-digit order key shared by branch alternatives (for example `020`).

Variant:

- `a` — default / preferred branch choice
- `z` — optional / alternative branch choice

Roles (leading underscores are visual padding for `ls` alignment and are ignored):

- `shell` — shell/project operation (prefer executable `*.sh` steps)
- `chat` — Chat specification/artifact generation step (extension `.get`)
- `code` — Code repository implementation step

Shell steps use extension `.sh` (code-first, documentation in comments). They usually begin with:

```bash
. sop-script
```

`sopwin` runs them with `cwd` = project directory and exports `PATH` (including `<pkgdatadir>/extension/bash`), `SOP_PROJECT_DIR`, `SOP_DIR`, `SOP_STEP_ID`, `SOP_LOGLEVEL`, `LOGLEVEL`, and `SOP_PROGRESS_FIFO`. A running script reports **its own** progress with `set_progress 10%` (or `30.78%`) — not the overall SOP session progress. sopwin reads the FIFO and updates the gauge for that script. Code prompt steps remain Markdown (`.md`).

Markdown / `.get` steps may start with an optional YAML frontmatter block:

```text
---
extend: previous
---
```

- `extend: previous` / `extend: next` — absorb this step into the previous/next step of the **same type** (`role` + extension, e.g. both `code` + `.md`). Type mismatch is ignored. The graph shows one merged node (the anchor); prompt bodies are concatenated with the YAML header removed. Example: when `011` is included, `extend: previous` merges it into `010` as a single step.

Chat steps use extension `.get`. They begin with optional RFC822-style headers (names case-insensitive), then a blank line, then the prompt body. Use `Discard:` for editor modelines and other ignored lines.

```text
Save-As: sop/PRD.md
File-Link: inode
Parse: multi-parts
Interaction: none
Append-To: sop/TUC.md from TUC*.md (delete)
Discard: -*- mode: markdown -*-
Discard: vim: set ft=markdown :

# prompt title
...
```

- `Save-As` — also write/link the saved response to this project-relative path (parent dirs created automatically). If the pasted response is empty, Save-As uses downloaded attachments instead (`file.ext` → `file1.ext`, `file2.ext`, … when multiple).
- `File-Link` — `default`/`auto` (inode on ext2/3/4, otherwise copy), `none` (copy), `inode` (hard link), or `sym` (symlink)
- `Parse` — comma-separated formats; currently `multi-parts` splits a multi-part response into `partNN` files under `sop/<seq>/`
- `Interaction` — `none` (default: save only) or `select` (show rendered response / part picker after save)
- `Append-To` — `dest from Pattern [(delete)]`; when a dropped/staged attachment matches `Pattern` (e.g. `TUC*.md`), merge it into `dest` on Accept; `(delete)` removes the source file
- Body markup — `<select title="…">` / `<option>…` (choose one before run); `<optional title="…">…</optional>` (yes/no include before run); `\response` expands to the nearest prior chat response

- `Discard` — ignored by sopwin (editor modelines, notes, …)
When a chat step runs, sopwin invokes the `chat` agent from `sop/AGENTS`
(default: `sopwin sopdlg --paste`). The agent is a pseudo-agent dialog: it copies
the prompt, captures the reply, applies `Save-As` / `Append-To` on Accept, and
stores responses under `sop/<seq>/` (leading zeros stripped from `seq`). Bodies
may use `\response` and `<select>` interactions before the agent runs.

## WorldMan architecture

WorldMan modules behave like lightweight microservices:

- Each app can run **fully independently**, or **integrate** with peer apps.
- Standalone mode: the app owns a **simple local definition** of shared basics (user, contact, …). A general business app usually only needs contact name + phone in its own small table.
- Integrated mode: the same logical entity becomes an **opaque reference** (cuid2 id) resolved through the peer app’s API — not a database foreign key into another module’s schema. A dedicated contact app may model contacts far more completely; consumers still only store the reference they need.
- Master-data primary keys use **`cuid(2)`** so ids remain stable across standalone and integrated deployments.

Standard package layout:

```text
prisma/            # schema, migrations, seed (cuid2 ids)
backend/           # Fastify + TypeScript, /api/v1
web/               # Vite + React
web-e2e/           # Playwright
mobile/            # optional Expo / mobile-web
mobile-e2e/        # optional
sop/               # working PRD, TODO, TUC/NTC/ECS during build/refactor
docs/              # as-built docs (api.md, …)
docker/            # images customized from zephyr-docker
i-local/           # built-in single-node docker local instance + flavor shortcuts
i-medium/          # built-in small/medium docker compose cluster + flavor shortcuts
worldman.json      # name, portBase, masterData, references, …
```

### Project `sop/` workspace

During complex build/refactor work, keep living working documents under `<projectdir>/sop/`:

- `status` — current workflow state; `location=` is the active step id (auto-updated; resumed on open)
- `TODO.md` — task list; sync item status as build/refactor progresses
- `PRD.md` — product requirements (authoritative during construction)
- `TUC.md` — test use-case reference set
- `NTC.md` — negative / anti test cases
- `ECS.md` — edge-case scenarios
- `design-inventory.md` — Figma/UI baseline (routes, tabs, chrome, mock sources); written in `010` **before** structural refactor; later steps must not silently drop entries
- `seed-coverage.md` — mock/demo → `prisma/seed.ts` parity matrix + discrepancy log; written in `040`, gated in `080`
- `theme-default.md` — Figma-extracted theme `default` token map + unchanged fonts; written when `011` is included
- `figma-archive/` — lightweight export scaffolding preserved by `000` (`.figma`, `src/imports`, agent docs). The **canonical Figma original** lives outside this product tree for parallel reference; `000` deletes `.git` so agents cannot git-revert to the raw export.

When the user supplies a complex prompt, extract or construct candidate tests promptly into `TUC.md`, `NTC.md`, and `ECS.md`. Later Playwright e2e work and tool/support-class extraction should use these `sop/` documents as input.

### Design fidelity

The Figma export is the **visual and navigational baseline**, not disposable scaffolding.

Hard rules for all steps after `000`:

1. **Inventory first** — step `010` must write `sop/design-inventory.md` *before* splitting files or moving routes. List every route, every tabbed/tabular surface, primary chrome, dialogs/sheets, mock/demo data sources with approximate row/entity counts, and **typography / font stacks**.
2. **No silent drops** — removing or merging a listed route, tab, table column set, or dialog requires updating the inventory with an explicit reason. “Cleanup”, “simplify”, or “not in PRD yet” is not enough by itself.
3. **PRD follows UI structure** — `sop/design-inventory.md` outranks speculative “export noise” filtering for navigational structure. Dead scaffold controls may be demoted only when the inventory itself marks them `dead`/`scaffold`.
4. **Mocks survive until seed parity** — do not replace demo data with empty API responses. Step `040` prefers mechanical extraction from `web/` mocks; write a seed **Discrepancy log** when seed and Figma diverge (including `figma-suspect`); step `080` must recheck `sop/seed-coverage.md` (including media↔text pairs) before deleting mocks.
5. **Theming is color-only; `default` = Figma** — step `011` maps the export’s live colors into a theme id named `default` and boots that theme. Stock packs are switcher-only. Do not change fonts, type ramp, layout, spacing, or navigation to match a stock palette; document the mapping in `sop/theme-default.md`.

### `web/src` directory roles

Target layout under `web/src/` (Vite + React — **not** Next.js file routing):

| Directory | Role |
|-----------|------|
| `pages/` | **Route boundary** — one module per navigable screen for the router and route-smoke tests |
| `features/` | Page-specific UI, state, and domain chunks for a route or product feature |
| `components/` | Reusable UI shared across routes/features |
| `hooks/`, `services/`, `lib/` | Shared non-visual support |

Thin `pages/*` re-exports are allowed **only** when the real page body lives under `features/` (or an equivalent feature module). Do **not** leave `pages/` as empty re-exports of full page implementations dumped under `components/`.

### SOP-driven commits

See [`CHECKPOINT.md`](./CHECKPOINT.md). Commit **as work evolves** inside a step
(not one dump at the end). SOP-driven subjects start with `[sopNNN]` (the active
step sequence, e.g. `[sop010]`, `[sop200]`).

Deploy options: run processes directly, reverse-proxy with Apache/Nginx, or containerize (bridge or published ports).

## Port allocation

Compute the module’s base port `N` with:

```bash
wm port -n <name>
```

(`wm port` runs `naac -pwm <name>`; equivalent: `worldman port -n <name>`.)

`N` is in `0..1999` (default `sha1(repo|directory|name) % 2000`). Override via `wm port NUM`, `-p`, or `worldman.json` → `"portBase"`.

Rule: for every HTTP listen port `P`, the matching HTTPS port is `P+1`, except the classic bridge web pair `80`/`443`.

| Service    | Direct HTTP | Direct HTTPS | Apache / published HTTP | Apache / published HTTPS | Bridge (container internal) |
|------------|-------------|--------------|-------------------------|--------------------------|-----------------------------|
| backend    | `2000+N`    | `2001+N`     | `6000+N`                | `6001+N`                 | `3000` / `3001`             |
| web        | `4000+N`    | `4001+N`     | `8000+N`                | `8001+N`                 | `80` / `443`                |
| mobile-web | `14000+N`   | `14001+N`    | `18000+N`               | `18001+N`                | `9000` / `9001`             |

Do not leave unrelated default listen ports (`5173`, …) in committed source.

## Design choices in this SOP

- Start from the Figma prototype: step `000` must refactor the export under `web/` before specification or backend work. Soft export scaffolding may land under `sop/figma-archive/`; the **canonical Figma original is kept outside** this tree for parallel reference.
- Step `000` **deletes `.git` on purpose** so agents cannot check out / reset back to the raw Figma export. Product history starts at the WorldMan layout commit.
- Design fidelity beats opportunistic cleanup: inventory (`010`) → respect in PRD/schema/seed/integrate → final audit (`150`) against the same inventory. Prefer inventory + route/tab smoke over expensive visual-regression / screenshot gates (token cost).
- SOP-driven git commits (`CHECKPOINT.md`) happen continuously during each step, prefixed `[sopNNN]`, so evolution stays bisectable.
- Step `011` (theming) is optional content attached with `extend: previous`: when included, **sopwin merges it into `010` as one graph step**. Do not drop `extend` to invent a separate node — the visible **SOP step count is part of the product contract** and must not change casually (no ad-hoc add/remove of seq numbers when refining the pack).
- Chat (`*.get`) vs Code (`*.md`) tracks for the same seq are **alternatives**: at runtime pick **one**. Use Chat while conversation context is available; switch to Code when the project was advanced by another path, earlier steps cannot be replayed, and Chat context is gone. Do not run both tracks for the same seq in one build.
- `schema.prisma` and `seed.ts` are generated separately to avoid output-length pressure and to let seed generation pressure-test the domain model.
- Seed prefers **extracting** Figma/web demo data over inventing new demo rows; keep media↔text pairs; log discrepancies (`matched` / `seed-enriched` / `figma-suspect` / …) instead of silently “fixing” or replacing the demo set; seed generation must not silently rewrite the schema.
- Step `011` must ship Figma colors as theme `default` (boot default); optional stock packs never become first paint.
- Seed may optionally request amends when it can run after the backend prompt (`050`): `prisma/request-for-refactor.md` and/or `sop/Backend amends.md` (via `<optional>`). Step `060` merges both and deletes the request files.
- Cross-module references are scalar cuid2 fields + API lookups, not cross-database FKs, unless explicitly required.
- Working construction docs live under `sop/` (`PRD.md`, `TODO.md`, `TUC.md`, `NTC.md`, `ECS.md`, `design-inventory.md`, `seed-coverage.md`, `theme-default.md`); keep them updated during build/refactor.
- Root `package.json` (from step `010`) owns operator scripts: `dev` / `dev:*`, `db:generate`, `db:migrate` (= deploy), `db:migrate:dev`, `db:seed`, `e2e`. Prefer `db:*` over duplicate `prisma:*` root aliases.
- Localizing demo assets: Google / Google Fonts / gstatic / Facebook CDNs and similar **must** go through the local HTTP proxy `http://localhost:8118`. Fonts: reuse same-name faces from `$HOME/.cache/fonts` or `/usr/share/fonts` when present; otherwise download the original file into both `web/assets/fonts/` and `$HOME/.cache/fonts/{truetype|opentype|woff}/…` — never substitute another typeface because the CDN failed.
- Backend API tests and Playwright E2E must tear down rows they create; step `140` also cleans residual test pollution from the shared DB after the suite run.
- `web-e2e/workflows.md` is generated late so it can synthesize `sop/` PRD/TUC/NTC/ECS, schema, seed scenarios, and backend design.
- `docs/api.md` is written after backend implementation/integration as the as-built API contract.
- Playwright coverage proceeds from route smoke → workflow → interactive-control audit → cross-artifact coverage audit; route smoke must cover every non-dropped route in `design-inventory.md`.
