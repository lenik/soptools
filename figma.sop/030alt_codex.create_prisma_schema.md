# Create Prisma schema from repository and PRD

Create:

```text
prisma/schema.prisma
prisma/comments.sql
```

Target PostgreSQL.

You do not have the original GPT discussion context. Derive the data model from repository evidence.

Before designing the schema, inspect:

```text
docs/PRD.md
web/
existing prisma/ files
mock data
fixtures
types/interfaces
API/service definitions if present
```

Treat `docs/PRD.md` as the primary product specification if it exists, but verify it against the current application for obvious contradictions.

## Modeling rules

Requirements:

- use `cuid(2)` for ID columns
- use `snake_case` for database table names
- use `snake_case` for database column names
- include meaningful Prisma documentation comments
- convert relevant Prisma comments into PostgreSQL `COMMENT ON` statements
- use PostgreSQL schema:

```text
<appname>
```

Model the complete domain that is supported by repository evidence, not merely the fields immediately visible on one screen.

Consider:

- entity identity
- lifecycle/state
- parent-child relationships
- many-to-many relationships
- ordering
- version/history needs
- media
- external references
- search/filter patterns
- uniqueness rules
- archival/deletion semantics
- timestamps
- extensibility where actually required

Use useful indexes for common lookup and search paths.

Add unique constraints only when the domain evidence supports them.

Cross-application identifiers should normally be scalar cuid2-compatible fields without database foreign-key constraints unless existing repository requirements clearly demand a relation.

Avoid JSON when a stable relational structure is evident.

Use JSON only when the structure is genuinely dynamic, extension-oriented, or otherwise unsuitable for stable relational modeling.

## Uncertainty handling

Do not invent domain structures merely to make the schema more elaborate.

If repository evidence is insufficient for an important modeling decision:

- choose the least destructive, most reversible representation
- document the uncertainty in Prisma comments where useful
- do not silently assert unsupported business rules

Do not generate `seed.ts` in this step.

Write the completed files directly to:

```text
prisma/schema.prisma
prisma/comments.sql
```
