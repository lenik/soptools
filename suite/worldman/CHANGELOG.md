# WorldMan SOP changelog

Process pack version: see [`VERSION`](./VERSION) (`major.minor.patch`).  
Shipped projects pin the template they ran in `sop/template-version` (written
by step `010`). Use this file + [`RETROFIT-zh_CN.md`](./RETROFIT-zh_CN.md) to
self-iterate when the pack moves ahead.

## 1.3.0 — 2026-11-09

- Require suite `CHANGELOG.md` alongside `VERSION`; document bump policy in
  `.cursor/rules/suite-sop.mdc`.
- Step `010`: write `sop/template-version` (`suite=worldman`, `version=…`) so
  later pack upgrades can be diffed against this changelog / retrofit list.

## 1.2.1 — 2026-11-09

- Add `RETROFIT-zh_CN.md` hand-out checklist for already-shipped projects.

## 1.2.0 — 2026-11-09

- Fonts: Google Fonts / gstatic via `http://localhost:8118`; reuse same-name
  faces from `$HOME/.cache/fonts` or `/usr/share/fonts`; otherwise download
  originals into `web/assets/fonts/` and
  `$HOME/.cache/fonts/{truetype|opentype|woff}/…`.
- E2E / API tests must tear down created rows; step `140` cleans residual
  shared-DB test pollution after the suite.
- Suite `VERSION` format fixed to `major.minor.patch`.

## 1.1.0 — 2026-11-09

- Design fidelity: `sop/design-inventory.md` before refactor; `figma-archive/`;
  evolving `[sopNNN]` commits (`CHECKPOINT.md`).
- Theme hard contract: Figma colors as theme id `default` (boot default);
  stock packs switcher-only; typography out of scope; `sop/theme-default.md`.
- Seed: extraction preferred; `sop/seed-coverage.md` with status / media↔text /
  Discrepancy log (`figma-suspect`, …); `080` gate before deleting mocks.
- Root `package.json` / `pnpm-workspace`: `dev:*`, `db:*`, `e2e`; keep
  `db:generate` only (no root `prisma:generate`); `db:migrate` (deploy) ≠
  `db:migrate:dev`.
- Step `050`: document output `sop/Backend implementation prompt.md` for
  chat and code tracks (`060` Checkout).
- sopwin: suite badge `[icon] name version` (suite icons under
  `assets/icons/suite/`).
- zh_CN: fluent retranslation of all step prose.

## 1.0.0 — earlier

- Initial WorldMan multi-locale SOP layout under `suite/worldman/` with
  Figma→`web/` flow, chat/code tracks, Prisma/seed/backend/e2e sequence, and
  optional `011` theming via `extend: previous`.
