# soptools

`soptools` 通过标准作业程序（SOP）引导项目构建。它从 Markdown 步骤文件（`<seq><variant>.<_role>.<title>.md`）加载流程，执行 shell 步骤、收集 AI 输出，并在 wxWidgets 图形界面或控制台模式下管理分支选择。

## 用法

```bash
soptools [选项] [项目目录]
```

常用选项：

- `-s, --sop SOPDIR` — SOP 目录（默认内置 `worldman.sop`；另提供 `worldman-zh_CN.sop`、`worldman-ja.sop`）
- `-g, --gui` / `-c, --console` — 强制图形或控制台模式
- `-C, --chdir DIR` — 项目目录（向上查找 `.git`）
- `-v, --verbose` / `-q, --quiet` — 日志级别

## 构建

```bash
sudo apt install meson ninja-build g++ pkg-config asciidoctor wx3.2-headers libwxgtk3.2-dev gettext
meson setup builddir
meson compile -C builddir
meson test -C builddir
```

本地安装：

```bash
DESTDIR=$PWD/stage meson install -C builddir
```

## 目录结构

- `src/` — 应用源码（`soptools.cpp`、SOP 引擎、GUI）
- `worldman.sop/` — 默认英文 SOP 步骤
- `worldman-zh_CN.sop/` — 简体中文 SOP
- `worldman-ja.sop/` — 日文 SOP
- `extension/bash/` — SOP 脚本 PATH 辅助（`sop-script`）
- `docs/` — AsciiDoc 手册页（英文及翻译）
- `po/` — gettext 翻译目录
- `debian/`、`packaging/` — 打包元数据

## 许可证

Copyright (C) 2026 Lenik <soptools@bodz.net>

采用 **AGPL-3.0-or-later**。完整文本见 `LICENSE`。
