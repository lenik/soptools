#!/bin/bash
# 将 Figma 原型导出重构为 WorldMan monorepo 布局。
#
# WorldMan webapp 通常从 Figma 制作的原型起步。本必需的
# 第一步清理一次性导出工具、把 UI 移到 web/ 下，并
# 创建标准包目录。以 cwd = 项目根运行
# （sopwin 设置 SOP_PROJECT_DIR 与 PATH）。

. sop-script

set -euo pipefail

set_progress 5%

# 从 WorldMan suite 附件播种项目 sop/themes/。
# 附件位于 suite/worldman/themes/（语言分支的兄弟目录）。
# 拷贝是项目级 fork（色板 + scripts/generate-theme-css.mjs），
# 可随产品演进；011 据此生成应用 CSS。
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

# 保留轻量 Figma / agent 脚手架，供清单线索使用。
# 权威 Figma 原件放在本产品树之外以便并行对照；
# 下方删除 .git 可防止代理通过 git 回退。
# 产品树根目录不应继续保留这些路径。
archive_figma_scaffolding() {
    local dest="sop/figma-archive"
    local moved=0
    mkdir -p "$dest"
    for path in .figma src/imports AGENT.md CLAUDE.md; do
        if [ -e "$path" ]; then
            mkdir -p "$dest/$(dirname "$path")"
            # 在全新导出树上重跑时替换先前归档副本。
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

# 若先前运行已产生 WorldMan 布局则跳过。重跑会把
# backend/prisma/sop/… 移进 web/ 并毁掉树。
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

# 故意丢弃任何既有 .git。
# 产品历史必须从本 WorldMan 布局提交开始，使代理无法
# `git checkout` / reset 回原始 Figma 导出。Figma 原件
# 保留在别处（本产品树之外）供人工/代理并行对照——
# 不要依赖项目内 git 历史做这件事。
# sop/figma-archive/ 下的脚手架只是轻量导出元数据。
rm -rf .git
set_progress 20%

mkdir -p web
set_progress 25%

# 将其余顶层导出内容移入 web/。WorldMan 根目录
# （sop/ 已持有 figma-archive/）保留在仓库根。
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

# 若导出在 web/ 下有根 .gitignore，则提升到仓库根。
[ -f web/.gitignore ] && mv web/.gitignore .
set_progress 55%

# sop/themes/ 是 suite 主题附件的 fork（色板 + 生成
# 脚本），供步骤 011 主题使用。
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

# 本步骤后的预期布局：
#   project/{sop,prisma,backend,web,web-e2e,mobile,mobile-e2e,docs,docker,i-local,i-medium,...}
# Figma 导出的应用必须只位于 web/ 下。
# 此后不要把仓库级文件再移回 web/。

sop_log 1 "Figma export refactored under web/; WorldMan layout ready."
