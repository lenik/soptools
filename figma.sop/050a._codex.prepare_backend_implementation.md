# Prepare repository-grounded backend implementation

This is the Codex fallback for the GPT step that would normally generate a backend implementation prompt.

Because you are already operating inside the repository, do not generate a prompt intended to be pasted back into Codex.

Instead, inspect the repository and prepare a concrete implementation plan, then leave the repository ready for the backend implementation step.

Review at minimum:

```text
docs/PRD.md
prisma/schema.prisma
prisma/seed.ts
web/
package.json
pnpm-workspace.yaml
existing backend/
```

Also inspect:

- web routes
- frontend data/service abstractions
- mock data
- existing request/response shapes
- domain enums and state transitions
- search/filter/sort behavior
- bulk operations
- cross-entity navigation
- features that appear reusable beyond the current UI

Create:

```text
docs/backend-implementation-plan.md
```

## Backend assumptions

Target:

```text
pnpm
Node.js
TypeScript
```

API prefix:

```text
/api/v1/
```

The backend must expose useful domain capabilities, not merely the minimum endpoints needed by the current web UI.

For every domain resource, evaluate whether it meaningfully needs:

```text
CRUD
list
detail
search
lookup
autocomplete
filtering
sorting
pagination
batch retrieval
bulk create/update/delete
relation traversal
tree traversal
state transitions
history
version access
validation
statistics
aggregation
import
export
media operations
external reference resolution
```

Do not mechanically prescribe every capability for every model.

The plan should define:

```text
backend architecture
domain/service boundaries
route groups
validation strategy
error model
serialization conventions
pagination conventions
filtering/sorting conventions
resource capabilities
domain-specific operations
bulk-operation semantics
testing strategy
integration concerns with web/
implementation order
```

Prefer separation between:

```text
route/controller layer
validation
service/domain logic
data access
shared errors
API serialization
```

Do not put domain logic directly in route handlers.

## Repository-grounded gap handling

If `docs/PRD.md`, `schema.prisma`, seed data, and the web application disagree:

- record the contradiction in `docs/backend-implementation-plan.md`
- identify which source appears authoritative and why
- choose the smallest reversible implementation assumption
- do not silently redesign the schema or product

If `prisma/request-for-refactor.md` exists, review it and incorporate relevant blocking items into the implementation plan, but do not automatically change the schema unless the later implementation step explicitly authorizes it.

Do not implement the backend in this step unless the workflow invoking this file explicitly asks you to combine planning and implementation.
