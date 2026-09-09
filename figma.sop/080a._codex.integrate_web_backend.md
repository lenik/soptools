# Integrate web with backend

Integrate:

```text
web/
```

with:

```text
backend/
```

Replace Figma/demo/mock/local data sources with real backend APIs where appropriate.

Preserve the approved UI structure and visual design.

Do not redesign the application merely because backend integration is taking place.

Add a clear frontend API/service layer.

Avoid spreading raw `fetch()` calls throughout page components.

Prefer:

```text
web/src/services/
```

or an equivalent API client layer.

Ensure UI behavior correctly handles:

```text
loading
empty state
server error
validation error
not found
conflict
save success
delete/archive confirmation
refresh
reload persistence
search
filter
sort
pagination
```

Use server-side behavior where appropriate for large or reusable datasets.

If integration exposes a genuine backend architectural problem, refactor the backend as necessary.

When backend behavior changes:

- update backend tests
- keep API behavior consistent
- rerun backend tests
