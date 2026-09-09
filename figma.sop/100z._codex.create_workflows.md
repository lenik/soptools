# Create complete testing workflows from repository evidence

Create:

```text
web-e2e/workflows.md
```

You do not have the original GPT conversation context.

Reconstruct the intended behavioral workflows from the current repository and existing specifications.

Before writing workflows, inspect:

```text
docs/PRD.md
prisma/schema.prisma
prisma/seed.ts
docs/api.md              # if already present
docs/backend-implementation-plan.md   # if present
backend/
web/
existing backend tests
existing web-e2e tests
```

Also inspect the web application for:

- route table
- navigation
- interactive controls
- forms
- validation
- dialogs/sheets/drawers
- menus/context menus
- search/filter/sort
- pagination
- bulk selection/actions
- state transitions
- media controls
- import/export
- empty/error states
- destructive confirmations

## Source priority

When determining intended behavior, prefer:

1. `docs/PRD.md`
2. explicit domain rules in `schema.prisma`
3. stable backend/API behavior
4. implemented user-visible behavior
5. seed scenarios and fixtures
6. isolated UI artifacts or dead Figma-generated controls

Do not rewrite intended workflows merely to accommodate an obvious implementation bug.

Do not invent workflows for unsupported product features.

If an important behavior is unresolved, include a short:

```text
Unresolved behavior
```

note in the relevant workflow or in a final unresolved section rather than fabricating a rule.

## Workflow goals

These workflows are behavioral acceptance specifications for Playwright.

They are not Playwright source code.

Use readable workflow-style pseudocode.

Cover the product with finite but broad functional coverage.

Include, where applicable:

```text
route access
navigation
create flows
edit flows
delete/archive flows
detail views
search
filter
sort
pagination
tabs
dialogs
sheets
menus
context menus
bulk actions
state transitions
validation failures
cancel paths
empty states
confirmation flows
media interactions
import/export
cross-entity navigation
important error paths
important boundary cases
```

Prefer realistic end-to-end workflows over one workflow per button.

Use seed data where it makes scenarios deterministic and reproducible.

## Branch syntax

Branches should describe the real condition.

Avoid generic branches such as:

```text
{correct/error}
```

unless those terms are actually meaningful in the domain.

Prefer:

```text
Login
    enter phone
    enter password

    if password matches:
        submit
        dashboard opens

    if password does not match:
        submit
        login remains open
        error message appears
```

Workflows may use:

```text
precondition
sequence
selection
branch
repeat
expected visible result
```

Keep them practical, readable, and directly implementable in Playwright.

Write the completed artifact directly to:

```text
web-e2e/workflows.md
```
