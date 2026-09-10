# soptools

`soptools` is a guided **Standard Operating Procedure (SOP)** runner for project
construction. It loads a directory of step files, walks you through shell
scripts, AI prompt capture (`.get`), and Codex implementation prompts, and
tracks branch choices and progress in a wxWidgets GUI (`sopwin`) or a console
session.

The default SOP suite builds a **WorldMan** module (web + backend + optional
mobile) from a Figma prototype export. Localized branches ship for English
(`default`), Simplified Chinese (`zh_CN`), and Japanese (`ja`).

## Features

- **SOP suites** — ordered Markdown / shell / GPT steps under
  `suite/<name>/<branch>/` with branching (`a` / `z`) and roles
  (`shell`, `gpt`, `codex`)
- **GUI (`sopwin`)** — workflow graph, step preview, Back / Next / Run / Pause,
  Execute·Get·Copy, detachable graph & log panes, status bar feedback
- **Console mode** — same engine without a display
- **GPT `.get` flow** — copy prompt, Paste Response dialog, attachment download,
  optional multi-part render / select
- **Shell steps** — run with project cwd, enriched `PATH`, progress FIFO for the
  gauge
- **Persistence** — workflow location and exclusions under project `sop/status`
- **Language / branch** — `LANG` selects suite branch; **View → Language**
  switches within the current suite

## SOP suites

Packs live under `suite/`. See [`suite/README.md`](suite/README.md).

### worldman

Guided WorldMan module construction from a Figma export.

```text
suite/worldman/
    default/    # English
    zh_CN/      # Simplified Chinese
    ja/         # Japanese
    themes/     # catalog/ + minor|vibe|country palettes → sop/themes/ in 000
```

Approximate content (same step ids in every branch): Figma layout refactor +
seed `sop/themes/` (000), web refactor + asset localization (010), semantic
theming from grouped web palettes + switcher (011), PRD (020), Prisma schema/seed
(030–040), backend design/implement/tests (050–070), web integration + API docs
(080–090), E2E workflows (100), smoke/Playwright (110–120; **120 merges**
scattered `TUC`/`NTC`/`ECS`/`TODO` and revises workflows), final audit (150).

## Requirements

- C++17 toolchain, Meson, Ninja
- wxWidgets 3.2 (GTK) development packages
- pkg-config, gettext, asciidoctor
- Linux with a display for GUI (or use `--console`)

## Build

```bash
sudo apt install meson ninja-build g++ pkg-config asciidoctor \
  wx3.2-headers libwxgtk3.2-dev gettext

meson setup build
meson compile -C build
meson test -C build
```

The GUI binary is `build/sopwin` (also installed as `soptools` / `sopwin`
depending on packaging).

Stage-install for packaging checks:

```bash
DESTDIR=$PWD/stage meson install -C build
```

### Live development symlinks

Point the install prefix at this tree without packaging:

```bash
ninja -C build install-symlinks
# …
ninja -C build uninstall-symlinks
```

Links `sopwin`, man pages, bash completion, `suite/`, and `extension/` under
the configured prefix (default `/usr`) back to the source / build tree. Useful
when editing SOP packs and re-running `sopwin` immediately.

## Usage

```bash
sopwin [OPTIONS] [PROJECTDIR]
# or, when installed as soptools:
soptools [OPTIONS] [PROJECTDIR]
```

| Option | Meaning |
|--------|---------|
| `-s, --suite NAME` | Suite under `suite/` (default: `worldman`) |
| `-S, --sop-dir DIR` | Explicit SOP directory (overrides suite / LANG) |
| `-g, --gui` | Force GUI |
| `-c, --console` | Console only |
| `-C, --chdir DIR` | Project directory (walks up for `.git`) |
| `-v` / `-q` | More / less logging |
| `-h` / `--version` | Help / version |

Branch (`default` / `zh_CN` / `ja`) comes from `LANG` / `LC_ALL` /
`LC_MESSAGES` unless `-S` is set.

Examples:

```bash
# Suite worldman; branch from LANG (e.g. en_US → default)
./build/sopwin -C ~/src/my-app

# Force Simplified Chinese branch
LANG=zh_CN.UTF-8 ./build/sopwin -s worldman -C ~/src/my-app

# Explicit directory
./build/sopwin -S suite/worldman/ja -c -C ~/src/my-app
```

In the GUI, **View → Language** switches `default` / `zh_CN` / `ja` within the
current suite (reloads steps; project config is keyed as
`<suite>-<branch>`).

### Keyboard (GUI)

| Key | Action |
|-----|--------|
| `Alt+F` / `Alt+P` / `Alt+V` / `Alt+H` | Open File / Procedure / View / Help |
| `Ctrl+O` / `Ctrl+S` | Open project / Save status |
| `Ctrl+U` | Load SOP directory |
| `PgUp` / `PgDn` | Back / Next |
| `F5` / `F8` | Run·Resume / Pause auto-run |
| `Ctrl+Enter` | Execute / Get / Copy current step |
| `F2` / `Ctrl+L` | Toggle graph / loggings |
| `F1` | Shortcuts dialog |
| Graph: wheel | Pan vertically |
| Graph: `Ctrl+wheel` | Zoom |

## SOP file naming

```text
<seq><variant>.<_padded_role>.<title>.(md|sh|get)
```

Examples: `000a._shell.refactor_figma.sh`, `020a.___gpt.create_prd.get`,
`060a._codex.implement_backend.md`.

- **seq** — three-digit order key shared by branch alternatives
- **variant** — `a` default, `z` optional alternative
- **role** — leading underscores are padding; effective role is `shell` / `gpt` /
  `codex`

See each branch’s `README.md` under `suite/worldman/{default,zh_CN,ja}/` for
WorldMan architecture, GPT headers (`Save-As`, `Parse`, `Interaction`, …), and
shell progress helpers.

## Project layout under the target app

While running, sopwin uses / writes:

```text
<project>/
  sop/
    status          # location=, exclusions, branch choices (auto-saved)
    <seq>/          # GPT responses, parts, downloads
    TODO.md …       # working docs created by later SOP steps
  …                 # backend/, web/, prisma/, … as the SOP creates them
```

Legacy status files under `.config/sopwin/` are still readable.

## Shell step environment

Shell steps typically start with `. sop-script`. sopwin runs them with:

- cwd = project directory
- `PATH` including `<prefix>/share/soptools/extension/bash` (or source
  `extension/bash`)
- `SOP_PROJECT_DIR`, `SOP_DIR`, `SOP_STEP_ID`, `SOP_LOGLEVEL`, `LOGLEVEL`,
  `SOP_PROGRESS_FIFO`

Scripts report **their own** progress with `set_progress 10%` (not overall SOP
progress). The GUI gauge follows that FIFO.

### Downloading blocked assets (SOP 010)

Figma hotlinks (Google Fonts, Facebook CDNs, …) may be unreachable from
mainland China. SOP 010 instructs agents to fetch via HTTP proxy
`http://localhost:8118` (`http_proxy` / `https_proxy` or curl `--proxy`) and to
keep real assets under `web/assets/` — not placeholders.

## Repository layout

| Path | Role |
|------|------|
| `src/` | Engine, model, util, GUI (`gui/`), console (`ui/`) |
| `suite/` | SOP suites (`worldman/{default,zh_CN,ja}/`, …) |
| `extension/bash/` | PATH helpers for SOP scripts (`sop-script`) |
| `docs/` | AsciiDoc man pages (+ locale scaffolds) |
| `po/` | gettext catalogs |
| `tests/` | Engine / project tests |
| `debian/`, `packaging/` | Distro metadata |

## Packaging / release notes

- Version comes from `debian/changelog` (pre-commit writes `VERSION`). Prefer
  `dch` for changelog bullets; do not edit solidified tagged stanzas.
- Release (`zfr lint` / `zfr publish`) only when explicitly requested.

## License

Copyright (C) 2026 Lenik <soptools@bodz.net>

Licensed under **AGPL-3.0-or-later**. See `LICENSE`.
