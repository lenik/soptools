# soptools

`soptools` 是面向项目构建的**标准作业程序（SOP）**运行器。它加载一组步骤文件，
引导你完成 shell 脚本、AI 提示采集（`.get`）以及 Codex 实现提示，并在
wxWidgets 图形界面（`sopwin`）或控制台中跟踪分支选择与进度。

默认 SOP 包用于从 Figma 原型导出构建 **WorldMan** 模块（web + backend + 可选
mobile）。另提供简体中文与日文语言包。

## 功能

- **SOP 包** — 有序的 Markdown / shell / GPT 步骤，支持分支（`a` 首选 / `z` 备选）
  与角色（`shell`、`gpt`、`codex`）
- **图形界面（`sopwin`）** — 流程图、步骤预览、Back / Next / Run / Pause、
  Execute·Get·Copy、可拆出的图与日志面板、状态栏反馈
- **控制台模式** — 同一引擎，无需显示器
- **GPT `.get` 流程** — 复制提示、粘贴响应对话框、附件下载、可选多段渲染/选择
- **Shell 步骤** — 在项目目录执行，扩展 `PATH`，经进度 FIFO 驱动进度条
- **持久化** — 位置与排除项写入项目 `sop/status`
- **语言包** — **查看 → 语言** 切换 WorldMan SOP 语言

## 依赖

- C++17、Meson、Ninja
- wxWidgets 3.2（GTK）开发包
- pkg-config、gettext、asciidoctor
- GUI 需要图形显示（否则用 `--console`）

## 构建

```bash
sudo apt install meson ninja-build g++ pkg-config asciidoctor \
  wx3.2-headers libwxgtk3.2-dev gettext

meson setup build
meson compile -C build
meson test -C build
```

GUI 二进制为 `build/sopwin`（打包后也可能叫 `soptools` / `sopwin`）。

打包检查用暂存安装：

```bash
DESTDIR=$PWD/stage meson install -C build
```

### 开发用符号链接

不打包也可把安装前缀指回本树：

```bash
ninja -C build install-symlinks
# …
ninja -C build uninstall-symlinks
```

会将 `sopwin`、手册、bash 补全、WorldMan SOP 包与 `extension/` 链接到配置的
前缀（默认 `/usr`），便于改 SOP 后立刻再跑。

## 用法

```bash
sopwin [选项] [项目目录]
# 安装为 soptools 时：
soptools [选项] [项目目录]
```

| 选项 | 含义 |
|------|------|
| `-s, --sop SOPDIR` | SOP 目录（默认内置 `worldman.sop`） |
| `-g, --gui` | 强制 GUI |
| `-c, --console` | 仅控制台 |
| `-C, --chdir DIR` | 项目目录（向上查找 `.git`） |
| `-v` / `-q` | 增加 / 减少日志 |
| `-h` / `--version` | 帮助 / 版本 |

示例：

```bash
./build/sopwin -C ~/src/my-app
./build/sopwin -s worldman-zh_CN.sop -C ~/src/my-app
./build/sopwin -s worldman-ja.sop -c -C ~/src/my-app
```

GUI 中也可通过 **查看 → 语言** 在 `worldman.sop` /
`worldman-zh_CN.sop` / `worldman-ja.sop` 之间切换（会重新加载步骤；项目
`sop/status` 按包名区分）。

### 快捷键（GUI）

| 按键 | 作用 |
|------|------|
| `Alt+F` / `Alt+P` / `Alt+V` / `Alt+H` | 打开 文件 / 流程 / 查看 / 帮助 |
| `Ctrl+O` / `Ctrl+S` | 打开项目 / 保存状态 |
| `Ctrl+U` | 加载 SOP 目录 |
| `PgUp` / `PgDn` | 上一步 / 下一步 |
| `F5` / `F8` | 运行·继续 / 暂停自动跑 |
| `Ctrl+Enter` | 执行 / Get / 复制当前步 |
| `F2` / `Ctrl+L` | 切换图 / 日志 |
| `F1` | 快捷键说明 |
| 图：滚轮 | 纵向平移 |
| 图：`Ctrl+滚轮` | 缩放 |

## SOP 命名

```text
<seq><variant>.<_padded_role>.<title>.(md|sh|get)
```

例：`000a._shell.refactor_figma.sh`、`020a.___gpt.create_prd.get`、
`060a._codex.implement_backend.md`。

- **seq** — 三位序号，分支共享
- **variant** — `a` 默认，`z` 备选
- **role** — 前导下划线仅为对齐；有效角色为 `shell` / `gpt` / `codex`

各语言包内的 `README.md` 有 WorldMan 架构、GPT 头字段（`Save-As`、`Parse`、
`Interaction` 等）与 shell 进度说明。

## 目标项目中的布局

运行时读写：

```text
<project>/
  sop/
    status          # location=、排除、分支（自动保存）
    <seq>/          # GPT 响应、分段、下载
    TODO.md …       # 后续步骤产生的工作文档
  …                 # backend/、web/、prisma/ 等由 SOP 创建
```

仍可读 `.config/sopwin/` 下的旧状态文件。

## Shell 步骤环境

Shell 步骤通常以 `. sop-script` 开头。sopwin 提供：

- 工作目录 = 项目目录
- `PATH` 含 `<prefix>/share/soptools/extension/bash`（或源码 `extension/bash`）
- `SOP_PROJECT_DIR`、`SOP_DIR`、`SOP_STEP_ID`、`SOP_LOGLEVEL`、`LOGLEVEL`、
  `SOP_PROGRESS_FIFO`

脚本用 `set_progress 10%` 报告**自身**进度（非整条 SOP）。GUI 进度条跟该 FIFO。

### 下载被墙资源（SOP 010）

Figma 外链（Google Fonts、Facebook CDN 等）在中国大陆可能不可达。SOP 010 要求
经 HTTP 代理 `http://localhost:8118`（`http_proxy` / `https_proxy` 或 curl
`--proxy`）下载，并把真实资源放进 `web/assets/`，不要用假图替代。

## 仓库结构

| 路径 | 作用 |
|------|------|
| `src/` | 引擎、模型、工具、GUI（`gui/`）、控制台（`ui/`） |
| `worldman.sop/` | 默认英文 WorldMan SOP |
| `worldman-zh_CN.sop/` | 简体中文包 |
| `worldman-ja.sop/` | 日文包 |
| `extension/bash/` | SOP 脚本 PATH 辅助（`sop-script`） |
| `docs/` | AsciiDoc 手册（含各语言脚手架） |
| `po/` | gettext 目录 |
| `tests/` | 引擎 / 项目测试 |
| `debian/`、`packaging/` | 发行元数据 |

## 打包 / 版本

- 版本来自 `debian/changelog`（pre-commit 写入 `VERSION`）。changelog 用 `dch`；
  已打 tag 的固化段落勿改。
- 仅在明确要求时做 Release（`zfr lint` / `zfr publish`）。

## 许可证

Copyright (C) 2026 Lenik <soptools@bodz.net>

采用 **AGPL-3.0-or-later**。全文见 `LICENSE`。
