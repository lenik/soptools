# soptools

`soptools` guides project construction through Standard Operating Procedure (SOP) workflows. It loads step definitions from Markdown files (`<seq><role>.<name>.md`), runs shell steps, captures AI output, and tracks branch selection in a wxWidgets GUI or console mode.

## Usage

```bash
soptools [OPTIONS] [PROJECTDIR]
```

Common options:

- `-s, --sop SOPDIR` — SOP directory (default: builtin `figma.sop`)
- `-g, --gui` / `-c, --console` — force GUI or console mode
- `-C, --chdir DIR` — project directory (walks up to find `.git`)
- `-v, --verbose` / `-q, --quiet` — logging level

## Build

```bash
sudo apt install meson ninja-build g++ pkg-config asciidoctor wx3.2-headers libwxgtk3.2-dev gettext
meson setup builddir
meson compile -C builddir
meson test -C builddir
```

Install locally:

```bash
DESTDIR=$PWD/stage meson install -C builddir
```

## Layout

- `src/` — application (`soptools.cpp`, SOP engine, GUI)
- `figma.sop/` — default SOP step definitions
- `docs/` — AsciiDoc man page sources (English + translations)
- `po/` — gettext catalogs
- `debian/`, `packaging/` — distribution metadata

## License

Copyright (C) 2026 Lenik <soptools@bodz.net>

Licensed under **AGPL-3.0-or-later**. See `LICENSE` for the full text.
