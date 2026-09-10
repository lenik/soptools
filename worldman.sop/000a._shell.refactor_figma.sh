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

# Skip if a prior run already produced the WorldMan layout. Re-running would
# move backend/prisma/sop/… into web/ and destroy the tree.
if [ -d web ] && [ -d backend ] && [ -d prisma ] && [ -d sop ] && [ -d docs ] && {
       [ -f web/package.json ] || [ -d web/src ] ||
           [ -f web/vite.config.ts ] || [ -f web/vite.config.js ] ||
           [ -f web/vite.config.mts ] || [ -f web/index.html ]
   }; then
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
mkdir -p sop docs prisma backend web-e2e mobile mobile-e2e docker i-local i-medium
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
