# WorldMan 项目流程 SOP

本归档是构建 **WorldMan** 模块的默认 SOP（含 `backend/`、`web/`、可选 `mobile/` 与 e2e 测试）。模块通常从 **Figma 原型导出** 开始；步骤 `000` 必须先把该导出重构进标准布局。各步骤为可独立复用的提示词/命令文件。

本包为简体中文版（`suite/worldman/zh_CN`）；英文源真相见 `suite/worldman/default/`。

## 命名规范

```text
<seq><variant>.<_padded_role>.<title>.(md|sh|get)
```

示例：

```text
000a._shell.refactor_figma.sh
010a.__code.refactor_web.md
020a.__chat.create_prd.get
020z.__code.create_prd.md
050z.__code.prepare_backend_implementation.md
```

序号（`seq`）：三位顺序键，同一序号下的文件构成分支备选（例如 `020`）。

变体（variant）：

- `a` — 默认 / 优先分支
- `z` — 可选 / 备选分支

角色（role；前导下划线仅用于 `ls` 对齐，解析时忽略）：

- `shell` — shell / 项目操作（优先可执行 `*.sh` 步骤）
- `chat` — Chat 规格/产物生成步骤（扩展名 `.get`）
- `code` — 代码仓库实现步骤

Shell 步骤使用扩展名 `.sh`（以代码为主，说明写在注释里）。脚本通常以以下内容开头：

```bash
. sop-script
```

`sopwin` 以项目目录为 `cwd` 运行，并导出 `PATH`（含 `<pkgdatadir>/extension/bash`）、`SOP_PROJECT_DIR`、`SOP_DIR`、`SOP_STEP_ID`、`SOP_LOGLEVEL`、`LOGLEVEL`、`SOP_PROGRESS_FIFO`。运行中的脚本用 `set_progress 10%`（或 `30.78%`）报告**本脚本自身**进度，不是整个 SOP 会话进度；sopwin 从 FIFO 读取并更新该脚本的进度条。Code 提示词步骤仍为 Markdown（`.md`）。

Markdown / `.get` 步骤可选用 YAML frontmatter：

```text
---
extend: previous
---
```

- `extend: previous` / `extend: next` — 将本步骤并入前/后一个**同类型**步骤（`role` + 扩展名，如均为 `code` + `.md`）。类型不一致则忽略。图上只显示合并后的锚点节点；提示词正文拼接且去掉 YAML 头。例：当包含 `011` 时，`extend: previous` 会将其并入 `010`，成为单一步骤。

Chat 步骤使用扩展名 `.get`。可选 RFC822 风格文件头（名称不分大小写），空行后是提示词正文。编辑器 modeline 等用 `Discard:` 写出，sopwin 忽略。

```text
Save-As: sop/PRD.md
File-Link: inode
Parse: multi-parts
Interaction: none
Append-To: sop/TUC.md from TUC*.md (delete)
Discard: -*- mode: markdown -*-
Discard: vim: set ft=markdown :

# 提示词标题
...
```

- `Save-As` — 将保存的回答同时写入/链接到该项目相对路径（自动创建父目录）。若粘贴回答为空，则用已下载附件充当 Save-As（多个附件时 `file.ext` → `file1.ext`、`file2.ext`…）。
- `File-Link` — `default`/`auto`（ext2/3/4 上用 inode，否则复制）、`none`（复制）、`inode`（硬链接）或 `sym`（符号链接）
- `Parse` — 逗号分隔的格式；当前 `multi-parts` 会把多部分回答拆到 `sop/<seq>/` 下的 `partNN` 文件
- `Interaction` — `none`（默认：仅保存）或 `select`（保存后弹出渲染/分片选择窗）
- `Append-To` — `dest from Pattern [(delete)]`；拖放/暂存附件若匹配 `Pattern`（如 `TUC*.md`），Accept 时合并到 `dest`；`(delete)` 删除源文件
- 正文标记 — `<select title="…">` / `<option>…`（运行前择一）；`<optional title="…">…</optional>`（运行前是否纳入）；`\response` 展开为最近一次 Chat 回答

- `Discard` — sopwin 忽略（编辑器 modeline、备注等）
运行 chat 步骤时，sopwin 调用 `sop/AGENTS` 中的 `chat` 代理
（默认：`sopwin sopdlg --paste`）。该代理是伪代理对话框：复制
提示、采集回复，Accept 时应用 `Save-As` / `Append-To`，并把回答存到
`sop/<seq>/`（`seq` 去掉前导 0）。正文可在代理运行前使用 `\response`
与 `<select>` 交互。

## WorldMan 架构

WorldMan 模块类似轻量微服务：

- 每个应用既可**完全独立运行**，也可与对等应用**集成**。
- 独立模式：应用拥有共享基础数据的**简化本地定义**（用户、联系人等）。普通业务应用通常只需联系人姓名+电话的小表。
- 集成模式：同一逻辑实体变为**不透明引用**（cuid2 id），通过对等应用 API 解析——不是跨模块库表外键。专用联系人应用可建模更完整的联系人；消费方仍只保存所需引用。
- 主数据主键使用 **`cuid(2)`**，以便在独立与集成部署间保持 id 稳定。

标准包布局：

```text
prisma/            # schema、migrations、seed（cuid2 ids）
backend/           # Fastify + TypeScript，/api/v1
web/               # Vite + React
web-e2e/           # Playwright
mobile/            # 可选 Expo / mobile-web
mobile-e2e/        # 可选
sop/               # 构建/重构期间的工作 PRD、TODO、TUC/NTC/ECS
docs/              # 建成态文档（api.md 等）
docker/            # 基于 zephyr-docker 定制的镜像
i-local/           # 内置单节点 docker 本地实例 + 韵味快捷方式
i-medium/          # 内置中小规模 docker compose 集群 + 韵味快捷方式
worldman.json      # name、portBase、masterData、references、…
```

### 项目 `sop/` 工作区

在复杂的构建/重构工作中，于 `<projectdir>/sop/` 下维护活的工作文档：

- `status` — 当前流程状态；`location=` 为活动步骤 id（自动更新；打开项目时从此恢复）
- `TODO.md` — 任务清单；随构建/重构推进同步条目状态
- `PRD.md` — 产品需求（施工期权威）
- `TUC.md` — 测试用例参考集
- `NTC.md` — 反用例 / 负面测试用例
- `ECS.md` — 边界场景
- `design-inventory.md` — Figma/UI 基线（路由、页签、chrome、mock 来源）；在 `010` **结构重构之前**写出；后续步骤不得静默丢弃条目
- `seed-coverage.md` — mock/demo → `prisma/seed.ts` 对等矩阵；在 `040` 写出，在 `080` 门控
- `figma-archive/` — 由 `000` 保留的轻量导出脚手架（`.figma`、`src/imports`、agent 文档）。**权威 Figma 原件**放在本产品树之外以便并行对照；`000` 删除 `.git`，使代理无法 git-revert 回原始导出。

当用户给出复杂提示时，尽快把候选测试提取或构造进 `TUC.md`、`NTC.md`、`ECS.md`。后续 Playwright e2e 以及工具类/支撑类抽取应以这些 `sop/` 文档为输入。

### 设计保真

Figma 导出是**视觉与导航基线**，不是可随意丢弃的脚手架。

`000` 之后所有步骤的硬性规则：

1. **先建清单** — 步骤 `010` 必须在拆分文件或移动路由**之前**写出 `sop/design-inventory.md`。列出每条路由、每个页签/表格面、主要 chrome、对话框/sheet，以及 mock/demo 数据来源与大致行数/实体数。
2. **禁止静默丢弃** — 删除或合并已列出的路由、页签、表格列集或对话框，必须更新清单并写明明确理由。“清理”“简化”或“PRD 里还没有”本身不够。
3. **PRD 服从 UI 结构** — 对导航结构而言，`sop/design-inventory.md` 优先于臆测的“导出噪音”过滤。仅当清单本身将控件标为 `dead`/`scaffold` 时，才可降级死脚手架控件。
4. **mock 保留至 seed 对等** — 不要用空 API 响应替换 demo 数据。步骤 `040` 优先从 `web/` mock 机械提取；步骤 `080` 在删除 mock 前必须复核 `sop/seed-coverage.md`。
5. **主题仅改颜色** — 步骤 `011` 把颜色映射到 token；不得为了迁就库存色板而改布局、字号阶梯、间距或导航。

### `web/src` 目录职责

`web/src/` 目标布局（Vite + React — **不是** Next.js 文件路由）：

| 目录 | 职责 |
|------|------|
| `pages/` | **路由边界** — 每个可导航屏面对应一个模块，供路由器与路由冒烟测试使用 |
| `features/` | 某条路由或产品功能的页面专属 UI、状态与领域分块 |
| `components/` | 跨路由/功能共享的可复用 UI |
| `hooks/`、`services/`、`lib/` | 共享的非视觉支撑 |

仅当真实页面主体位于 `features/`（或等价功能模块）下时，才允许薄的 `pages/*` 再导出。**不要**把完整页面实现堆在 `components/` 下，却让 `pages/` 只剩空再导出。

### SOP 驱动的提交

见 [`CHECKPOINT.md`](./CHECKPOINT.md)。在步骤**进行过程中**随工作演进提交
（不要在步骤结束时一次性倾倒）。SOP 驱动的主题以 `[sopNNN]` 开头（当前活动
步骤序号，例如 `[sop010]`、`[sop200]`）。

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

- 从 Figma 原型开始：步骤 `000` 必须先把导出结果放到 `web/`，再做规格或后端工作。轻量导出脚手架可落到 `sop/figma-archive/`；**权威 Figma 原件保留在本树之外**以便并行对照。
- 步骤 `000` **故意删除 `.git`**，使代理无法 checkout / reset 回原始 Figma 导出。产品历史从 WorldMan 布局提交开始。
- 设计保真优先于机会主义清理：清单（`010`）→ 在 PRD/schema/seed/集成中尊重 → 最终审计（`150`）对照同一清单。优先用清单 + 路由/页签冒烟，而不是昂贵的视觉回归/截图门控（token 成本）。
- SOP 驱动的 git 提交（`CHECKPOINT.md`）在每个步骤中持续进行，前缀 `[sopNNN]`，使演进可 bisect。
- 步骤 `011`（主题）是带 `extend: previous` 的可选内容：被纳入时，**sopwin 将其并入 `010` 成为图上的单一步骤**。不要去掉 `extend` 去发明独立节点——可见的 **SOP 步骤数是产品契约的一部分**，完善包时不得随意增减 seq 编号。
- 同一序号下的 Chat（`*.get`）与 Code（`*.md`）轨是**备选**：运行时只选**其一**。有对话上下文时用 Chat；若项目已由其他路径推进、早期步骤无法重放且 Chat 上下文已丢失，则改用 Code。同一次构建中不要对同一序号跑两条轨。
- `schema.prisma` 与 `seed.ts` 分开生成，避免输出过长，并用 seed 对领域模型做压力测试。
- Seed 优先**提取** Figma/web demo 数据，而非发明新 demo 行；seed 生成不得静默改写 schema。
- Seed 可在后端提示之后运行时可选地请求修订（`050`）：`prisma/request-for-refactor.md` 和/或 `sop/Backend amends.md`（经 `<optional>`）。步骤 `060` 合并二者并删除请求文件。
- 跨模块引用是标量 cuid2 字段 + API 查询，不是跨库 FK（除非明确要求）。
- 施工期工作文档放在 `sop/`（`PRD.md`、`TODO.md`、`TUC.md`、`NTC.md`、`ECS.md`、`design-inventory.md`、`seed-coverage.md`）；构建/重构期间持续更新。
- 本地化 demo 资源：Google / Google Fonts / gstatic / Facebook CDN 等**必须**经本地 HTTP 代理 `http://localhost:8118`。
- `web-e2e/workflows.md` 靠后生成，以便综合 `sop/` 下的 PRD/TUC/NTC/ECS、schema、seed 场景与后端设计。
- `docs/api.md` 在后端实现/集成之后编写，作为建成态 API 合同。
- Playwright 覆盖顺序：路由冒烟 → 工作流 → 交互控件审计 → 跨产物覆盖审计；路由冒烟必须覆盖 `design-inventory.md` 中每个未丢弃的路由。
