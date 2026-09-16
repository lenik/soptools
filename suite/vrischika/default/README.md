# Vrischika game workflow SOP

Build a playable **demo stage** (`some_puff1`) from a Vrischika template using
`vsk`, with storyline-first design docs under `sop/`.

## Naming convention

Same as worldman:

```text
<seq><variant>.<_padded_role>.<title>.(md|sh|get)
```

## Architecture notes

- Template id: `[engine]-[genre][dimension](_[view/style])`
- Design pack in every template/project: `sop/GDD.md`, `sop/GDD-zh_CN.md`,
  `sop/TDD.md`, `sop/ADD.md`
- GDD prioritizes **storyline** and a vivid place; avoid generic genre essays
- Demo must be concrete content, not an empty genre scaffold
