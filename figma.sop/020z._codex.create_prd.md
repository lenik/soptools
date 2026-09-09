# Create PRD from repository evidence

Create or replace:

```text
docs/PRD.md
```

You do not have the original GPT product-discussion context. Reconstruct the product specification from the repository itself.

Before writing the PRD, inspect the repository systematically.

At minimum review:

```text
web/
docs/
prisma/
README*
package.json
worldman.json
```

if they exist.

Inspect the web application for:

- routes and pages
- navigation structure
- page titles and labels
- forms and fields
- tables and columns
- dialogs, sheets, drawers and menus
- search/filter/sort controls
- CRUD actions
- bulk actions
- state/status controls
- media usage
- mock data
- fixtures
- constants/enums
- local-storage/session-storage usage
- validation messages
- empty states
- error states
- comments/TODOs that reveal intended behavior

Do not treat every accidental Figma-generated detail as a product requirement.

Infer product intent from repeated, semantically meaningful evidence.

## Evidence priority

When sources disagree, prefer in this order:

1. existing explicit product documentation under `docs/`
2. existing Prisma/domain definitions
3. implemented user-visible behavior
4. stable mock/fixture data structures
5. isolated labels, placeholder copy, or dead Figma-generated controls

Do not invent requirements merely to make the PRD look complete.

If an important requirement cannot be determined from repository evidence, record it under:

```text
Open Questions / Unresolved Requirements
```

Do not block the task by asking for clarification.

## PRD requirements

The PRD should be suitable for implementation and later acceptance testing.

Cover, where supported by evidence:

```text
Product overview
Goals
Scope
Out of scope
Terminology
User roles
Domain concepts
Core entities
Entity relationships
Business rules
Lifecycle/state transitions
CRUD behavior
Search
Filter
Sort
Pagination where relevant
Bulk operations
Import/export where relevant
Media
History/audit behavior where relevant
External references
Validation rules
Deletion/archive behavior
Empty states
Error behavior
Important edge cases
Main UI workflows
Non-functional expectations
Open Questions / Unresolved Requirements
```

For each material inferred requirement, make it clear enough that another developer can implement or test it.

Do not redesign the existing product while documenting it.

Write the completed artifact directly to:

```text
docs/PRD.md
```
