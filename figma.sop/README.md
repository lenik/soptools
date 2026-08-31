# Project Workflow SOP

This archive contains the project workflow split into independently reusable prompt/command files.

Naming convention:

```text
<seq><role>.<title>.md
```

Examples:

```text
000sh.refactor_figma.md
010codex.refactor_web.md
020gpt.create_prd.md
```

Roles:

- `sh` — shell/project operation
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
