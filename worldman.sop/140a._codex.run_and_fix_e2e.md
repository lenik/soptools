# Run and repair E2E tests

Run the complete Playwright test suite.

For failures:

1. determine whether the failure is:
   - product implementation bug
   - backend bug
   - integration bug
   - test bug
   - unstable selector
   - incorrect test data assumption

2. fix the correct layer.

Do not weaken tests merely to make them pass.

Do not change the intended workflow unless the workflow contradicts the established product specification.

Repeat until all defined workflows are working.
