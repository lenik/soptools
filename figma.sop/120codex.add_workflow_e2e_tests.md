# Implement workflow-based Playwright tests

Read:

```text
web-e2e/workflows.md
```

Implement Playwright tests covering all workflows.

Prefer workflow-oriented tests rather than one-test-per-control.

Tests should exercise real user behavior.

Use stable selectors.

Prefer, in order:

```text
accessible role/name
label
placeholder where appropriate
stable semantic test ID
```

Avoid brittle selectors based on:

```text
deep CSS hierarchy
nth-child
visual position
generated class names
```

Add `data-testid` only where semantic selectors are insufficient.

Do not alter visible UI merely to make Playwright easier.
