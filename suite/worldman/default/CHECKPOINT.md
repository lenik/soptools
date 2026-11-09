# SOP-driven commits (WorldMan)

While a WorldMan step runs, commit **as the work evolves** — after each coherent
unit of progress — not once at the end of the step.

One commit per finished step is **too coarse**: a single `[sop010]` dump that
rewrites inventory, splits pages, and localizes assets cannot explain which
edit dropped a tab. Prefer several smaller commits inside the same step.

## Subject prefix

Every **SOP-driven** commit (agent/shell work for the active step) must start
with `[sopNNN]` where `NNN` is the current step sequence (`000`, `010`, `080`,
…). Example: `[sop200]` when the active step id is `200`.

```text
[sop010] freeze design inventory before refactor
[sop010] extract EventsBrowser tabs into features/events
[sop040] seed Episode rows from web mock catalog
[sop080] wire RolesBrowser list to GET /api/v1/roles
```

User / human commits outside the SOP need not use this prefix. Do not relabel
them.

## Rules

1. **Commit early and often** during the step whenever a meaningful slice is
   done (artifact written, one route extracted, one mock replaced, tests green
   for a subset, …).
2. **Never** batch the whole step into a single end-of-step commit when the
   tree went through multiple distinct transformations.
3. **Never amend** commits from an earlier step (or earlier slices you already
   pushed). Fix forward with a new `[sopNNN]` commit.
4. **Never** `--no-verify` / `--no-gpg-sign` unless the user explicitly requires it.
5. Body explains *why* / fidelity exceptions / inventory updates — not only
   file names.
6. Prefer intentional paths over blind `git add -A` when secrets or huge
   binaries appeared; do not commit `.env`, credentials, or local-only junk.
7. If the tree is clean, skip. Chat-only work that only writes under
   `sop/<seq>/` should still commit when it updates tracked deliverables
   (`sop/PRD.md`, `prisma/seed.ts`, `sop/design-inventory.md`, …).

## Command template

```bash
git add <paths-for-this-slice>
git status
git commit -m "$(cat <<'EOF'
[sopNNN] short summary of this slice

- notable decisions, fidelity exceptions, inventory updates
EOF
)"
```

When step `011` is included, sopwin merges it into `010` via `extend: previous`.
Commits in that merged run use `[sop010]` (one active step id on the graph).

## Required companion artifacts

| Artifact | Written / updated by | Purpose |
|----------|----------------------|---------|
| `sop/design-inventory.md` | `010` (before refactor); updated when UI structure intentionally changes | Baseline routes, tabs, chrome, fonts, mock sources |
| `sop/figma-archive/` | `000` | Preserved Figma export metadata (not product source) |
| `sop/theme-default.md` | `011` (when included / merged into `010`) | Figma colors as theme `default` + unchanged font stacks |
| `sop/seed-coverage.md` | `040`; rechecked by `080` | Mock/demo → seed parity + discrepancy log before mock removal |
