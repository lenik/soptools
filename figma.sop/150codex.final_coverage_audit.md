# Final coverage audit

Perform a final product coverage audit across:

```text
docs/PRD.md
prisma/schema.prisma
prisma/seed.ts
backend/
docs/api.md
web/
web-e2e/workflows.md
web-e2e/
```

Check for:

```text
PRD features without implementation
implemented domain features missing from documentation
UI controls with no real behavior
dead buttons
dead menus
dead routes
obsolete mock data
obsolete local persistence
API endpoints without backend tests
important backend features with no E2E path
workflow steps without Playwright coverage
interactive controls without coverage
stale API documentation
schema fields never handled by backend where they should be
frontend assumptions inconsistent with backend behavior
```

Fix genuine gaps.

Do not force every backend API capability to have a GUI if the API is intentionally reusable beyond the current web application.

Finally run:

```text
backend tests
web build/typecheck
Playwright E2E tests
```

All required tests must pass.
