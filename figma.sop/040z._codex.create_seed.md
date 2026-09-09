# Create seed data from repository state

Create:

```text
prisma/seed.ts
```

Use the existing repository as the source of truth.

Before writing the seed, inspect:

```text
prisma/schema.prisma
docs/PRD.md
web/
existing fixtures
mock data
demo data
tests
```

Reuse stable demo concepts, names, categories, media references, and representative states already present in the project where appropriate.

Do not assume access to the original GPT discussion.

## Seed objectives

The seed should do more than populate rows. It should exercise the domain sufficiently for:

- local development
- UI integration
- API testing
- Playwright workflows
- search/filter/sort demonstrations
- state-transition testing

Include, where applicable:

- multiple representative entities
- multiple categories/types
- realistic relationships
- different lifecycle states
- active/inactive examples
- draft/published/completed/cancelled examples
- parent/child relations
- many-to-many relations
- history/version examples
- empty or optional-field examples
- boundary examples useful for testing
- search/filter-distinguishable values
- date/time variations
- demo media URLs
- image URLs
- audio/video URLs where relevant
- files/attachments where relevant

Prefer deterministic, readable demo values over random data.

Use stable identifiers or stable lookup keys where tests may need predictable records.

## Schema pressure test

While designing realistic seed scenarios, actively evaluate whether the current schema can represent the documented product.

Do not modify or silently redesign:

```text
prisma/schema.prisma
```

during this step.

If seed design reveals that the schema cannot adequately represent an important product requirement, relationship, lifecycle state, constraint, historical case, or realistic test scenario, also create:

```text
prisma/request-for-refactor.md
```

For every issue include:

```text
Affected models
Observed repository/product evidence
Concrete seed scenario that exposed the problem
Why the current schema is insufficient
Severity: blocking | recommended
Smallest recommended schema change
Impact on existing data
Impact on seed logic
Impact on backend/API
Impact on automated tests
```

Do not apply the proposed schema changes automatically.

If no schema change is needed, do not create `request-for-refactor.md`.

The final deliverables for this step are:

```text
prisma/seed.ts
prisma/request-for-refactor.md   # optional
```
