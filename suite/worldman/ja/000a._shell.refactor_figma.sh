#!/bin/bash
# Figma プロトタイプのエクスポートを WorldMan monorepo レイアウトへ再構成する。
#
# WorldMan Web アプリは通常 Figma 製プロトタイプのエクスポートから始まる。
# 必須の最初の手順として、捨てる足場を掃除し UI を web/ へ移し、標準パッケージ
# ディレクトリを作る。cwd はプロジェクト根（sopwin が SOP_PROJECT_DIR と PATH を設定）。

. sop-script

set -euo pipefail

set_progress 5%

# WorldMan スイートのテーマ添付をプロジェクトの sop/themes/ へ複製する
# （プロジェクト fork：パレット + scripts/generate-theme-css.mjs。製品に合わせて
# 進化させてよい。011 はここからアプリ CSS を生成する）。
# 添付は suite/worldman/themes/（言語ブランチの兄弟）。
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
        sop_log 1 "SOP パック横にテーマ添付が見つかりません；sop/themes の投入をスキップします。"
        return 0
    fi
    mkdir -p sop/themes
    cp -a "$src"/. sop/themes/
    sop_log 1 "プロジェクトの sop/themes/ fork（パレット + 生成スクリプト）を投入しました。"
}

# 既に WorldMan レイアウトがある場合はスキップ。再実行すると backend/prisma/sop/… が
# web/ 配下へ移り、ツリーが壊れる。
if [ -d web ] && [ -d backend ] && [ -d prisma ] && [ -d sop ] && [ -d docs ] && {
       [ -f web/package.json ] || [ -d web/src ] ||
           [ -f web/vite.config.ts ] || [ -f web/vite.config.js ] ||
           [ -f web/vite.config.mts ] || [ -f web/index.html ]
   }; then
    seed_sop_themes
    set_progress 100%
    sop_log 1 "WorldMan レイアウトは既にあります（web/ と標準パッケージ）；Figma 再構成をスキップします。"
    exit 0
fi

# 製品ツリーに残すべきでない Figma / agent 足場を削除する。
rm -rf AGENT.md CLAUDE.md src/imports .figma .git
set_progress 15%

mkdir -p web
set_progress 25%

# web/ 以外をすべて web/ へ移す（エクスポートアプリが web パッケージになる）。
shopt -s dotglob extglob
mv !(web) web
set_progress 45%

# web/ 配下に .gitignore があればリポジトリ直下へ上げる。
[ -f web/.gitignore ] && mv web/.gitignore .
set_progress 55%

# 移した UI の周りに標準 WorldMan レイアウトを作る。
# sop/ は後続の構築／再構成手順で作業中の PRD/TODO/TUC/NTC/ECS を置く。
# sop/themes/ はスイート添付のプロジェクト fork（scripts/ 生成スクリプト含む；011 用）。
mkdir -p sop docs prisma backend web-e2e mobile mobile-e2e docker i-local i-medium
seed_sop_themes
set_progress 70%

git init
set_progress 80%

git add .
set_progress 90%

git commit -m "init"
set_progress 100%

# 完了後の期待構成:
#   project/{sop,prisma,backend,web,web-e2e,mobile,mobile-e2e,docs,docker,i-local,i-medium,...}
# Figma 由来のアプリは web/ のみ。
# この後、リポジトリ直下のファイルを再び web/ へ戻さない。

sop_log 1 "Figma エクスポートを web/ へ再構成し、WorldMan レイアウトを用意しました。"
