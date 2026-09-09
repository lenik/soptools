#!/bin/bash
# 将 Figma 原型导出重构为 WorldMan monorepo 布局。
#
# WorldMan Web 应用通常从 Figma 原型导出开始。本必做第一步清理导出脚手架，
# 把 UI 移到 web/，并创建标准包目录。cwd 为项目根（sopwin 设置 SOP_PROJECT_DIR 与 PATH）。

. sop-script

set -euo pipefail

set_progress 5%

# 删除不应留在产品树中的 Figma / agent 脚手架。
rm -rf AGENT.md CLAUDE.md src/imports .figma .git
set_progress 15%

mkdir -p web
set_progress 25%

# 除 web/ 外全部移入 web/（导出应用成为 web 包）。
shopt -s dotglob extglob
mv !(web) web
set_progress 45%

# 若导出在 web/ 下带有 .gitignore，提升到仓库根。
[ -f web/.gitignore ] && mv web/.gitignore .
set_progress 55%

# 在已迁移的 UI 周围建立标准 WorldMan 布局。
# sop/ 在后续构建/重构步骤中存放工作中的 PRD/TODO/TUC/NTC/ECS。
mkdir -p sop docs prisma backend web-e2e mobile mobile-e2e docker i-local i-medium
set_progress 70%

git init
set_progress 80%

git add .
set_progress 90%

git commit -m "init"
set_progress 100%

# 完成后期望布局：
#   project/{sop,prisma,backend,web,web-e2e,mobile,mobile-e2e,docs,docker,i-local,i-medium,...}
# Figma 导出的应用只能位于 web/。
# 此后不要再把仓库根级文件移回 web/。

sop_log 1 "已将 Figma 导出重构到 web/，WorldMan 布局就绪。"
