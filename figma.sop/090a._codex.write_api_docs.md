# Write final API documentation

After backend implementation and integration stabilize, write complete:

```text
docs/api.md
```

This document describes the API as actually implemented.

It is the as-built API contract.

Document:

```text
API base path
common conventions
authentication if applicable
request format
response format
error format
pagination
filtering
sorting
search
bulk operation conventions
resource endpoints
state-transition endpoints
relation endpoints
domain-specific endpoints
examples
important validation rules
important conflict behavior
```

For every endpoint include enough information for another application to use the API without reading backend source code.

Do not document nonexistent endpoints.

Do not omit useful implemented endpoints merely because the current web application does not call them.
