#!/usr/bin/env bash
# Bootstrap from a Vrischika template using vsk.
. sop-script 2>/dev/null || true
set -euo pipefail
: "${SOP_PROJECT_DIR:=$PWD}"
cd "$SOP_PROJECT_DIR"

TEMPLATE="${VRISCHIKA_TEMPLATE:-}"
if [ -z "$TEMPLATE" ]; then
  echo "Set VRISCHIKA_TEMPLATE to a template id (see suite/vrischika/catalog.md)" >&2
  exit 2
fi

if command -v vsk >/dev/null 2>&1; then
  if [ ! -f VERSION ]; then
    name=$(basename "$SOP_PROJECT_DIR")
    vsk create -l "$TEMPLATE" "/tmp/_vsk_$name" || true
    # If already a project tree, skip create
  fi
fi

mkdir -p sop stages
echo "bootstrap ok for template=$TEMPLATE"
