#!/bin/bash
# 将 Figma 原型导出重构为 WorldMan monorepo 布局。
#
# WorldMan Web 应用通常从 Figma 原型导出开始。本必做第一步清理导出脚手架，
# 把 UI 移到 web/，并创建标准包目录。cwd 为项目根（sopwin 设置 SOP_PROJECT_DIR 与 PATH）。

. sop-script

set -euo pipefail

set_progress 5%

# 将 WorldMan 套件主题附件（web .theme 文件）复制到项目 sop/themes/。
# 附件位于 suite/worldman/themes/（与语言分支同级）。
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
        sop_log 1 "未在 SOP 包旁找到主题附件；跳过 sop/themes 播种。"
        return 0
    fi
    mkdir -p sop/themes
    cp -a "$src"/. sop/themes/
    sop_log 1 "已将 WorldMan 主题附件播种到项目 sop/themes/。"
}

# 若先前已跑出 WorldMan 布局则跳过。重复执行会把 backend/prisma/sop/… 再移进 web/，破坏目录树。
if [ -d web ] && [ -d backend ] && [ -d prisma ] && [ -d sop ] && [ -d docs ] && {
       [ -f web/package.json ] || [ -d web/src ] ||
           [ -f web/vite.config.ts ] || [ -f web/vite.config.js ] ||
           [ -f web/vite.config.mts ] || [ -f web/index.html ]
   }; then
    seed_sop_themes
    set_progress 100%
    sop_log 1 "已存在 WorldMan 布局（web/ 及标准包目录）；跳过 Figma 重构。"
    exit 0
fi

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
# sop/themes/ 为套件主题附件副本（供 011 主题化使用）。
mkdir -p sop docs prisma backend web-e2e mobile mobile-e2e docker i-local i-medium
seed_sop_themes
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
