# Project Workflow SOP

This archive contains the project workflow split into independently reusable prompt/command files.

Naming convention:

```text
<seq><variant>.<_padded_role>.<title>.md
```

Examples:

```text
000a._shell.refactor_figma.md
010a._codex.refactor_web.md
020a.___gpt.create_prd.md
020z._codex.create_prd.md
```

Sequence (`seq`):

- Three-digit order key shared by branch alternatives (for example `020`).

Variant:

- `a` — default / preferred branch choice
- `z` — optional / alternative branch choice

Roles (leading underscores are visual padding for `ls` alignment and are ignored):

- `shell` — shell/project operation
- `gpt` — GPT specification/artifact generation step
- `codex` — Codex repository implementation step

The sequence intentionally allows the GPT specification track and the Codex implementation track to remain logically independent.

Important design choices:

- `schema.prisma` and `seed.ts` are generated separately to avoid output-length pressure and to let seed generation act as a domain-model pressure test.
- Seed generation must not silently rewrite the schema.
- If seed generation reveals a schema problem, it may additionally produce `prisma/request-for-refactor.md`, with severity `blocking` or `recommended`.
- `web-e2e/workflows.md` is generated late in the GPT reasoning sequence so it can synthesize PRD, schema, seed scenarios, and backend design.
- `docs/api.md` is written after backend implementation/integration and therefore serves as the as-built API contract.
- Playwright coverage proceeds from route smoke tests to workflow coverage to interactive-control coverage and finally a cross-artifact coverage audit.
