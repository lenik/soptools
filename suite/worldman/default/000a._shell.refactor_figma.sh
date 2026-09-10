#!/bin/bash
# Refactor a Figma prototype export into the WorldMan monorepo layout.
#
# WorldMan webapps usually start from a Figma-made prototype. This required
# first step cleans throwaway export tooling, moves the UI under web/, and
# creates the standard package directories. Run with cwd = project root
# (sopwin sets SOP_PROJECT_DIR and PATH).

. sop-script

set -euo pipefail

set_progress 5%

# Seed project sop/themes/ from the WorldMan suite attachment (web .theme files).
# Attachment lives at suite/worldman/themes/ (sibling of language branches).
seed_sop_themes() {
    local src=""
    if [ -n "${SOP_DIR:-}" ]; then
        if [ -f "${SOP_DIR}/../themes/catalog.tsv" ] || [ -f "${SOP_DIR}/../themes/minor/web-palettes.mjs" ]; then
            src="${SOP_DIR}/../themes"
        elif [ -f "${SOP_DIR}/themes/catalog.tsv" ] || [ -f "${SOP_DIR}/themes/minor/web-palettes.mjs" ]; then
            src="${SOP_DIR}/themes"
        fi
    fi
    if [ -z "$src" ]; then
        sop_log 1 "Theme attachment not found beside SOP pack; skip sop/themes seed."
        return 0
    fi
    mkdir -p sop/themes
    cp -a "$src"/. sop/themes/
    sop_log 1 "Seeded project sop/themes/ from WorldMan theme attachment."
}

# Skip if a prior run already produced the WorldMan layout. Re-running would
# move backend/prisma/sop/… into web/ and destroy the tree.
if [ -d web ] && [ -d backend ] && [ -d prisma ] && [ -d sop ] && [ -d docs ] && {
       [ -f web/package.json ] || [ -d web/src ] ||
           [ -f web/vite.config.ts ] || [ -f web/vite.config.js ] ||
           [ -f web/vite.config.mts ] || [ -f web/index.html ]
   }; then
    seed_sop_themes
    set_progress 100%
    sop_log 1 "WorldMan layout already present under web/; skipping Figma refactor."
    exit 0
fi

# Remove Figma / agent scaffolding that should not stay in the product tree.
rm -rf AGENT.md CLAUDE.md src/imports .figma .git
set_progress 15%

mkdir -p web
set_progress 25%

# Move everything except web/ into web/ (the exported app becomes the web package).
shopt -s dotglob extglob
mv !(web) web
set_progress 45%

# Keep a root .gitignore if the export had one under web/.
[ -f web/.gitignore ] && mv web/.gitignore .
set_progress 55%

# Standard WorldMan layout around the relocated UI.
# sop/ holds working PRD/TODO/TUC/NTC/ECS during later build/refactor steps.
# sop/themes/ is a copy of the suite theme attachment (for step 011 theming).
mkdir -p sop docs prisma backend web-e2e mobile mobile-e2e docker i-local i-medium
seed_sop_themes
set_progress 70%

git init
set_progress 80%

git add .
set_progress 90%

git commit -m "init"
set_progress 100%

# Expected layout after this step:
#   project/{sop,prisma,backend,web,web-e2e,mobile,mobile-e2e,docs,docker,i-local,i-medium,...}
# The Figma-exported application must live only under web/.
# Do not move repository-level files back into web/ after this point.

sop_log 1 "Figma export refactored under web/; WorldMan layout ready."
