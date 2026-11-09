# Suite icons

SVG sources are Tabler Icons (outline), copied from
`/usr/share/tabler-icons/svg/outline/`:

| Suite | Tabler SVG | Asset |
|-------|------------|-------|
| worldman | `world.svg` | `worldman.svg` |
| vrischika | `device-gamepad.svg` | `vrischika.svg` |

Meson runs `rsvg-convert -w 32 -h 32` to produce installable PNGs under
`$prefix/share/soptools/icons/suite/`.