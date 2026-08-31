# Add Playwright route smoke tests

Under:

```text
web-e2e/
```

add Playwright E2E tests.

First implement route smoke coverage.

Enumerate the application's route table.

For every reachable application route:

- open the route
- verify the page renders
- verify it does not crash
- verify there is no fatal page error
- verify there is no unexpected uncaught exception
- verify the page is not blank

Dynamic routes should use valid seed-data IDs or create required test data.

Do not consider route smoke tests sufficient functional coverage.
