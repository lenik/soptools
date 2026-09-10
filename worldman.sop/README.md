# WorldMan project workflow SOP

This archive is the default SOP for building a **WorldMan** module (webapp with `backend/`, `web/`, optional `mobile/`, and e2e tests). Modules usually begin as a **Figma prototype export**; step `000` refactoring that export into the standard layout is required. Steps are independently reusable prompt/command files.

## Naming convention

```text
<seq><variant>.<_padded_role>.<title>.(md|sh|get)
```

Examples:

```text
000a._shell.refactor_figma.sh
010a._codex.refactor_web.md
020a.___gpt.create_prd.get
020z._codex.create_prd.md
050z._codex.prepare_backend_implementation.md
```

Sequence (`seq`): three-digit order key shared by branch alternatives (for example `020`).

Variant:

- `a` — default / preferred branch choice
- `z` — optional / alternative branch choice

Roles (leading underscores are visual padding for `ls` alignment and are ignored):

- `shell` — shell/project operation (prefer executable `*.sh` steps)
- `gpt` — GPT specification/artifact generation step (extension `.get`)
- `codex` — Codex repository implementation step

Shell steps use extension `.sh` (code-first, documentation in comments). They usually begin with:

```bash
. sop-script
```

`sopwin` runs them with `cwd` = project directory and exports `PATH` (including `<pkgdatadir>/extension/bash`), `SOP_PROJECT_DIR`, `SOP_DIR`, `SOP_STEP_ID`, `SOP_LOGLEVEL`, `LOGLEVEL`, and `SOP_PROGRESS_FIFO`. A running script reports **its own** progress with `set_progress 10%` (or `30.78%`) — not the overall SOP session progress. sopwin reads the FIFO and updates the gauge for that script. Codex prompt steps remain Markdown (`.md`).

GPT steps use extension `.get`. They begin with optional RFC822-style headers (names case-insensitive), then a blank line, then the prompt body. Use `Discard:` for editor modelines and other ignored lines.

```text
Save-As: sop/PRD.md
File-Link: inode
Parse: multi-parts
Interaction: none
Discard: -*- mode: markdown -*-
Discard: vim: set ft=markdown :

# prompt title
...
```

- `Save-As` — also write/link the saved response to this project-relative path (parent dirs created automatically). If the pasted response is empty, Save-As uses downloaded attachments instead (`file.ext` → `file1.ext`, `file2.ext`, … when multiple).
- `File-Link` — `default`/`auto` (inode on ext2/3/4, otherwise copy), `none` (copy), `inode` (hard link), or `sym` (symlink)
- `Parse` — comma-separated formats; currently `multi-parts` splits a multi-part response into `partNN` files under `sop/<seq>/`
- `Interaction` — `none` (default: save only) or `select` (show rendered response / part picker after save)
- `Discard` — ignored by sopwin (editor modelines, notes, …)
When a GPT step runs, sopwin copies the prompt, then opens a Paste Response dialog for the model reply and attachment downloads. Responses are stored under `sop/<seq>/` (leading zeros stripped from `seq`).

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

When the user supplies a complex prompt, extract or construct candidate tests promptly into `TUC.md`, `NTC.md`, and `ECS.md`. Later Playwright e2e work and tool/support-class extraction should use these `sop/` documents as input.

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

- Start from the Figma prototype: step `000` must refactor the export under `web/` before specification or backend work.
- The GPT specification track and the Codex implementation track stay logically independent (`a` vs `z` branches).
- `schema.prisma` and `seed.ts` are generated separately to avoid output-length pressure and to let seed generation pressure-test the domain model.
- Seed generation must not silently rewrite the schema.
- If seed generation reveals a schema problem, it may additionally produce `prisma/request-for-refactor.md` (`blocking` or `recommended`).
- Cross-module references are scalar cuid2 fields + API lookups, not cross-database FKs, unless explicitly required.
- Working construction docs live under `sop/` (`PRD.md`, `TODO.md`, `TUC.md`, `NTC.md`, `ECS.md`); keep them updated during build/refactor.
- `web-e2e/workflows.md` is generated late so it can synthesize `sop/` PRD/TUC/NTC/ECS, schema, seed scenarios, and backend design.
- `docs/api.md` is written after backend implementation/integration as the as-built API contract.
- Playwright coverage proceeds from route smoke → workflow → interactive-control audit → cross-artifact coverage audit.
