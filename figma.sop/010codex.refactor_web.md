# Refactor the Figma-generated web application

Audit and refactor the Figma-generated application under `web/`.

First inspect the current application and identify:

- routes and pages
- large TSX files
- page-level components
- shared components
- dialogs, sheets, drawers and popovers
- forms
- tables and lists
- search, filter and sort behavior
- menus and context menus
- CRUD actions
- local/mock data sources
- local persistence
- interactive controls
- incomplete or dead controls

Then refactor the structure where useful.

## Page extraction

Extract route-level pages from large or monolithic TSX files when appropriate.

Prefer structure based on responsibility:

```text
src/
    pages/
    features/
    components/
    hooks/
    services/
    lib/
```

Do not split files merely because they are long.

Split according to:

- route boundary
- feature boundary
- state ownership
- reusable UI responsibility
- domain operation

Preserve the existing visual design and interaction model unless a structural change is necessary.

## UI testability refactor

Refactor UI logic to make automated testing and maintenance easier.

Business logic must not live directly inside anonymous JSX callbacks.

Anonymous callbacks may only:

- forward arguments
- extract event values
- adapt an event into a named handler call

Bad:

```tsx
<button
    onClick={async () => {
        setLoading(true)
        await api.deleteItem(item.id)
        refresh()
        setLoading(false)
    }}
>
```

Preferred:

```tsx
async function performDeleteItem(id: string) {
    ...
}

function handleDeleteItemClick(id: string) {
    void performDeleteItem(id)
}
```

For event-based behavior, separate event extraction from event-independent behavior when practical.

Example:

```tsx
function performSeek(timestamp: number) {
    ...
}

function handleTimelineClick(event: MouseEvent) {
    const timestamp = extractTimestamp(event)
    performSeek(timestamp)
}
```

Use clear naming such as:

```text
performCreateItem
performDeleteItem
performSave
performSearch
handleSaveClick
handleFilterChange
handleDialogOpen
handleDialogClose
```

Avoid names tied unnecessarily to visual position such as `button1_click` unless there is no meaningful semantic name.

Do not change product behavior during this stage unless fixing an obvious Figma-generated defect.
