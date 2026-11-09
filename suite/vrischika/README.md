# vrischika suite

SOP pack for building a **game vertical slice** from a Vrischika engine template
(`godot-*`, `unity-*`, `unreal-*`, `bevy-*`).

```text
suite/vrischika/
    VERSION      # process semver major.minor.patch (e.g. 1.0.0)
    default/     # English steps
    zh_CN/       # Simplified Chinese steps
```

Bump `VERSION` in the same commit whenever this suite’s process files change
(see `suite/README.md`).

CLI for templates: **`vsk`** (project meta-repo `vrischika`).

Typical flow: pick template → copy into project → author/confirm `sop/GDD.md`
(storyline) → implement `some_puff1` → art pass → playtest.

See branch `README.md` files for step naming (same convention as worldman).
