#!/bin/bash
# Figma プロトタイプのエクスポートを WorldMan monorepo レイアウトへ再構成する。
#
# WorldMan Web アプリは通常 Figma 製プロトタイプのエクスポートから始まる。
# 必須の最初の手順として、捨てる足場を掃除し UI を web/ へ移し、標準パッケージ
# ディレクトリを作る。cwd はプロジェクト根（sopwin が SOP_PROJECT_DIR と PATH を設定）。

. sop-script

set -euo pipefail

set_progress 5%

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
mkdir -p sop docs prisma backend web-e2e mobile mobile-e2e docker i-local i-medium
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
