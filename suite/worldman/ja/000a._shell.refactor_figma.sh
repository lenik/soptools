#!/bin/bash
# Figma プロトタイプのエクスポートを WorldMan モノレポレイアウトへ再構成する。
#
# WorldMan Web アプリは通常 Figma 製プロトタイプから始まる。この必須の
# 最初の手順は使い捨てエクスポートツールを掃除し、UI を web/ へ移し、
# 標準パッケージディレクトリを作る。cwd = プロジェクトルートで実行
# （sopwin が SOP_PROJECT_DIR と PATH を設定）。

. sop-script

set -euo pipefail

set_progress 5%

# WorldMan スイート付属からプロジェクト sop/themes/ をシードする。
# 付属は suite/worldman/themes/（言語ブランチの兄弟）にある。
# コピーはプロジェクト側フォーク（パレット + scripts/generate-theme-css.mjs）で、
# 製品とともに進化してよい。011 はこれからアプリ CSS を生成する。
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
    sop_log 1 "Seeded project sop/themes/ fork (palettes + generate script) from suite attachment."
}

# インベントリのヒント用に軽量な Figma / エージェント足場を保全する。
# 正本の Figma オリジナルはこの製品ツリー外に置き、並行参照する。
# 下で .git を削除することで、エージェントが git 経由で戻るのを防ぐ。
# 製品ツリーはルートにこれらのパスを残すべきではない。
archive_figma_scaffolding() {
    local dest="sop/figma-archive"
    local moved=0
    mkdir -p "$dest"
    for path in .figma src/imports AGENT.md CLAUDE.md; do
        if [ -e "$path" ]; then
            mkdir -p "$dest/$(dirname "$path")"
            # 新しいエクスポートツリーでの再実行時は以前のアーカイブコピーを置き換える。
            rm -rf "$dest/$path"
            mv "$path" "$dest/$path"
            moved=1
            sop_log 1 "Archived $path → $dest/$path"
        fi
    done
    if [ "$moved" -eq 0 ]; then
        sop_log 2 "No Figma scaffolding (.figma / src/imports / AGENT.md / CLAUDE.md) to archive."
    else
        printf '%s\n' \
            '# Figma export archive' \
            '' \
            'Preserved by WorldMan step `000` (lightweight scaffolding only).' \
            'The canonical Figma original is kept outside this product tree for' \
            'parallel reference. Step `000` deletes `.git` so agents cannot' \
            'git-revert to the raw export. Use this archive plus the external' \
            'original when writing `sop/design-inventory.md` (010) or recovering' \
            'demo/mock hints for seed (040).' \
            '' \
            "Archived at: $(date -u +%Y-%m-%dT%H:%M:%SZ)" \
            >"$dest/README.md"
    fi
}

ensure_git_init_commit() {
    local msg="$1"
    if [ ! -d .git ]; then
        git init
        sop_log 1 "Initialized git repository."
    fi
    git add -A
    if git diff --cached --quiet; then
        sop_log 1 "Nothing to commit; skip [sop000]."
        return 0
    fi
    git commit -m "$msg"
    sop_log 1 "[sop000] commit recorded."
}

# 以前の実行が既に WorldMan レイアウトを作っている場合はスキップ。
# 再実行すると backend/prisma/sop/… が web/ に入り、ツリーが壊れる。
if [ -d web ] && [ -d backend ] && [ -d prisma ] && [ -d sop ] && [ -d docs ] && {
       [ -f web/package.json ] || [ -d web/src ] ||
           [ -f web/vite.config.ts ] || [ -f web/vite.config.js ] ||
           [ -f web/vite.config.mts ] || [ -f web/index.html ]
   }; then
    seed_sop_themes
    set_progress 80%
    ensure_git_init_commit "$(cat <<'EOF'
[sop000] ensure layout and themes

- WorldMan layout already present; re-seeded sop/themes/ if needed
- Ensure git history exists for later [sopNNN] commits
EOF
)"
    set_progress 100%
    sop_log 1 "WorldMan layout already present under web/; skipping Figma refactor."
    exit 0
fi

mkdir -p sop docs prisma backend web-e2e mobile mobile-e2e docker i-local i-medium
set_progress 10%

archive_figma_scaffolding
set_progress 15%

# 以前の .git を意図的に削除する。
# 製品履歴はこの WorldMan レイアウトコミットから始まり、エージェントが
# 生の Figma エクスポートへ `git checkout` / reset できないようにする。
# Figma オリジナルは別場所（この製品ツリー外）に置き、人間／エージェントの
# 並行参照に使う——そのためにプロジェクト内の git 履歴に頼らない。
# sop/figma-archive/ 下の足場は軽量なエクスポートメタデータのみ。
rm -rf .git
set_progress 20%

mkdir -p web
set_progress 25%

# 残りのトップレベルエクスポート内容を web/ へ移す。WorldMan ルート dir
# （sop/ は既に figma-archive/ を持つ）はリポジトリルートに残す。
shopt -s dotglob nullglob
keep_at_root='|web|sop|docs|prisma|backend|web-e2e|mobile|mobile-e2e|docker|i-local|i-medium|'
for entry in * .[!.]* ..?*; do
    [ -e "$entry" ] || continue
    case "$keep_at_root" in
        *"|$entry|"*) continue ;;
    esac
    mv "$entry" "web/"
done
set_progress 45%

# エクスポートが web/ 下に .gitignore を持っていたらルートへ戻す。
[ -f web/.gitignore ] && mv web/.gitignore .
set_progress 55%

# sop/themes/ はスイートのテーマ付属（パレット + 生成スクリプト）のフォークで、
# 手順 011 のテーミング用。
seed_sop_themes
set_progress 70%

ensure_git_init_commit "$(cat <<'EOF'
[sop000] init from Figma export layout

- Move exported UI under web/; create standard WorldMan package dirs
- Archive Figma scaffolding under sop/figma-archive/ for inventory/seed recovery
- Seed sop/themes/ fork for later theming
EOF
)"
set_progress 100%

# この手順後の想定レイアウト：
#   project/{sop,prisma,backend,web,web-e2e,mobile,mobile-e2e,docs,docker,i-local,i-medium,...}
# Figma エクスポートされたアプリは web/ 下のみに置く。
# この時点以降、リポジトリレベルのファイルを web/ へ戻さない。

sop_log 1 "Figma export refactored under web/; WorldMan layout ready."
