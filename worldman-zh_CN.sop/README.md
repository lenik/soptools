# WorldMan 项目流程 SOP（简体中文）

本目录是构建 **WorldMan** 模块的默认 SOP（含 `backend/`、`web/`、可选 `mobile/` 与 e2e）。模块通常从 **Figma 原型导出** 开始；步骤 `000` 必须先把导出结果重构进标准布局。各步骤为可独立复用的提示词/命令文件。

本包为简体中文版；英文版见 `worldman.sop/`。

## 命名规范

```text
<seq><variant>.<_padded_role>.<title>.(md|sh|get)
```

示例：

```text
000a._shell.refactor_figma.sh
010a._codex.refactor_web.md
020a.___gpt.create_prd.get
020z._codex.create_prd.md
050z._codex.prepare_backend_implementation.md
```

序号（`seq`）：三位顺序键，同一序号下的文件构成分支备选（例如 `020`）。

变体（variant）：

- `a` — 默认 / 优先分支
- `z` — 可选 / 备选分支

角色（role；前导下划线仅用于 `ls` 对齐，解析时忽略）：

- `shell` — shell / 项目操作
- `gpt` — GPT 规格/产物生成（扩展名 `.get`）
- `codex` — Codex 仓库实现

Shell 步骤使用扩展名 `.sh`（以代码为主，说明写在注释里）。脚本通常以以下内容开头：

```bash
. sop-script
```

`sopwin` 以项目目录为 `cwd` 运行，并导出 `PATH`（含 `<pkgdatadir>/sopenv/bash`）、`SOP_PROJECT_DIR`、`SOP_DIR`、`SOP_STEP_ID`、`SOP_LOGLEVEL`、`LOGLEVEL`、`SOP_PROGRESS_FIFO`。脚本用 `set_progress 10%`（或 `30.78%`）报告**本脚本自身**进度，不是整个 SOP 会话进度；sopwin 从 FIFO 读取并更新该脚本的进度条。Codex 提示词步骤仍为 Markdown（`.md`）。

GPT 步骤使用扩展名 `.get`。可选 RFC822 风格文件头（名称不分大小写），空行后是提示词正文。编辑器 modeline 等用 `Discard:` 写出，sopwin 忽略。

```text
Save-As: sop/PRD.md
File-Link: inode
Parse: multi-parts
Interaction: none
Discard: -*- mode: markdown -*-
Discard: vim: set ft=markdown :

# 提示词标题
...
```

- `Save-As` — 将保存的回答同时写入/链接到该项目相对路径（自动创建父目录）。若粘贴回答为空，则用已下载附件充当 Save-As（多个附件时 `file.ext` → `file1.ext`、`file2.ext`…）。
- `File-Link` — `default`/`auto`（ext2/3/4 上用 inode，否则复制）、`none`（复制）、`inode`（硬链接）或 `sym`（符号链接）
- `Parse` — 逗号分隔的格式；当前 `multi-parts` 会把多部分回答拆到 `sop/<seq>/` 下的 `partNN` 文件
- `Interaction` — `none`（默认：仅保存）或 `select`（保存后弹出渲染/分片选择窗）
- `Discard` — sopwin 忽略（编辑器 modeline、备注等）

运行 GPT 步骤时，sopwin 先复制提示词，再打开 Paste Response 对话框以粘贴模型回答并下载附件。回答保存在 `sop/<seq>/`（`seq` 去掉前导 0）。

## WorldMan 架构

WorldMan 模块类似轻量微服务：

- 每个应用既可**完全独立运行**，也可与对等应用**集成**。
- 独立模式：应用拥有基础数据的**简化本地定义**（用户、联系人等）。普通业务应用通常只需联系人姓名+电话的小表。
- 集成模式：同一逻辑实体变为**不透明引用**（cuid2 id），通过对等应用 API 解析——不是跨模块库表外键。专用联系人应用可建模更完整的联系人；消费方仍只保存所需引用。
- 主数据主键使用 **`cuid(2)`**，以便在独立与集成部署间保持 id 稳定。

标准包布局：

```text
prisma/            # schema、migrations、seed（cuid2）
backend/           # Fastify + TypeScript，/api/v1
web/               # Vite + React
web-e2e/           # Playwright
mobile/            # 可选 Expo / mobile-web
mobile-e2e/        # 可选
sop/               # 构建/重构期间的工作文档：PRD、TODO、TUC/NTC/ECS
docs/              # 建成态文档（api.md 等）
docker/            # 基于 zephyr-docker 定制的镜像
i-local/           # 内置单节点 docker 本地实例，并提供快捷韵味命令
i-medium/          # 内置中小规模 docker compose 集群，并提供快捷韵味命令
worldman.json      # name、portBase、masterData、references、…
```

### 项目 `sop/` 工作区

在复杂的构建/重构工作中，于 `<projectdir>/sop/` 下维护活的工作文档：

- `TODO.md` — 任务清单；随构建/重构推进同步条目状态
- `PRD.md` — 产品需求（施工期权威）
- `TUC.md` — 测试用例参考集（Test Use-Case）
- `NTC.md` — 反用例 / 负面测试用例（Negative Test Cases）
- `ECS.md` — 边界场景（Edge-Case Scenarios）

当用户给出复杂提示时，尽快把候选测试提取或构造进 `TUC.md`、`NTC.md`、`ECS.md`。后续 Playwright e2e 以及工具类/支撑类抽取应以这些 `sop/` 文档为输入。

部署方式：直接跑进程、Apache/Nginx 反代、或容器化（bridge / 发布端口）。

## 端口分配

用以下命令计算模块基准端口 `N`：

```bash
wm port -n <name>
```

（`wm port` 实际执行 `naac -pwm <name>`；等价于 `worldman port -n <name>`。）

`N` 范围为 `0..1999`（默认 `sha1(repo|目录名|name) % 2000`）。可用 `wm port NUM`、`-p` 或 `worldman.json` 的 `"portBase"` 覆盖。

规则：对每个 HTTP 监听端口 `P`，对应 HTTPS 端口为 `P+1`；**例外**是 bridge 下 web 的经典端口对 `80`/`443`。

| 服务 | 直接 HTTP | 直接 HTTPS | Apache/发布 HTTP | Apache/发布 HTTPS | Bridge（容器内） |
|------|-----------|------------|------------------|-------------------|------------------|
| backend | `2000+N` | `2001+N` | `6000+N` | `6001+N` | `3000` / `3001` |
| web | `4000+N` | `4001+N` | `8000+N` | `8001+N` | `80` / `443` |
| mobile-web | `14000+N` | `14001+N` | `18000+N` | `18001+N` | `9000` / `9001` |

提交的源码中不要留下无关默认监听口（如 `5173` 等）。

## 本 SOP 的设计取舍

- 从 Figma 原型开始：步骤 `000` 必须先把导出结果放到 `web/`，再做规格或后端工作。
- GPT 规格轨与 Codex 实现轨逻辑独立（`a` / `z` 分支）。
- `schema.prisma` 与 `seed.ts` 分开生成，避免输出过长，并用 seed 对领域模型做压力测试。
- seed 不得静默改写 schema。
- 若 seed 暴露 schema 问题，可额外产出 `prisma/request-for-refactor.md`（`blocking` 或 `recommended`）。
- 跨模块引用是标量 cuid2 字段 + API 查询，不是跨库 FK（除非明确要求）。
- 施工期工作文档放在 `sop/`（`PRD.md`、`TODO.md`、`TUC.md`、`NTC.md`、`ECS.md`）；构建/重构期间持续更新。
- `web-e2e/workflows.md` 靠后生成，以便综合 `sop/` 下的 PRD/TUC/NTC/ECS、schema、seed 场景与后端设计。
- `docs/api.md` 在后端实现/集成之后编写，作为建成态 API 合同。
- Playwright 覆盖顺序：路由冒烟 → 工作流 → 交互控件审计 → 跨产物覆盖审计。
