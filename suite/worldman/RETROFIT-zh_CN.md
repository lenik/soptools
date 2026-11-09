# WorldMan SOP 对照改进清单（已完工项目）

面向：**已经跑过一轮 WorldMan SOP、产品已落地的项目组**。  
对照当前流程包版本：`suite/worldman/VERSION` = **1.5.0**（`major.minor.patch`）。  
变更明细：[`CHANGELOG.md`](./CHANGELOG.md)。

本文不是完整 SOP 重跑指南，而是把近期流程收紧点收成**可勾选的自查/整改项**。按优先级处理；每项注明对应步骤与期望产物。

英文步骤原文：`suite/worldman/default/`；中文：`suite/worldman/zh_CN/`。

---

## 怎么用

1. 若项目已有 `sop/template-version`，先读其中的 `version=`，再对照 `CHANGELOG.md` 里**高于该版本**的条目，只补缺项。
2. 若没有 `sop/template-version`，按本清单全量自查；整改时补写该文件（钉住当前包版本）。
3. 打开本文件，打印或导入任务看板。
4. 对每个已交付项目勾选「现状」；未达标的开整改任务，改完勾「已整改」。
5. 整改提交建议带简短说明（不必强行 `[sopNNN]`，除非你们仍按 SOP 节奏推进）。
6. 全部关键项通过后，在项目 `sop/TODO.md` 记一行：`WorldMan retrofit vs 1.5.0 done`，并确保 `sop/template-version` 已更新。

图例：🔴 高（用户可见/数据污染）　🟡 中（可维护性）　🟢 低（规范对齐）

---

## 0. 模板版本钉 🟡

| # | 检查项 | 对应步骤 | 期望 |
|---|--------|----------|------|
| 0.1 | 是否有 `sop/template-version`（`suite=worldman` + `version=…`）？ | `010` | 与当初/当前使用的包版本一致；升级后按 CHANGELOG 迭代时更新 |

---

## 1. 设计保真与清单 🔴

| # | 检查项 | 对应步骤 | 期望 |
|---|--------|----------|------|
| 1.1 | 是否有 `sop/design-inventory.md`，且在大改路由/拆文件**之前**就写过？ | `010` | 含：Routes、页签/表格面、主要界面外壳、对话框、Mock 来源、**Typography/字体** |
| 1.2 | 清单里 `kept`/`moved` 的路由、页签是否都还在、且不是空白壳？ | `010`→`150` | 丢弃必须在清单里写 `dropped:<理由>`；禁止「先简化再说」 |
| 1.3 | 是否还有 `sop/figma-archive/`（或等价的导出脚手架存档）？ | `000` | 供 inventory / seed 回溯；产品源码仍在 `web/` |
| 1.4 | 最终审计是否对照过同一份 inventory？ | `150` | 缺页、被掏空的表格/页签要恢复，而不是删面“过审计” |
| 1.5 | 浏览器标签 / `<title>` 是否已不是 `Figma Make App`？ | `010` | 格式 **`族 \| 代码 \| 标题`**（例：`ClassMate \| CM-Ballet \| ClassMate Ballet`）；`index.html` 与 Vite `figma:title` / site 配置一致；标题优先取自 `worldman.json` `"title"` |

**常见差距：** 早期项目没有 inventory，后续“清理”掉了 Figma 里仍在的页签；标签页仍显示 `Figma Make App`。

---

## 2. 主题与字体（看起来不像 Figma） 🔴

| # | 检查项 | 对应步骤 | 期望 |
|---|--------|----------|------|
| 2.1 | 启动主题是否为 **`default`**，且颜色来自 Figma/导出 CSS？ | `011`（并入 `010`） | 现成色板（`innocent`、国家包等）只能当切换器选项，不能当首屏默认 |
| 2.2 | 是否有 `sop/theme-default.md`（token → 提取色、字体栈说明）？ | `011` | 有则便于复核 |
| 2.3 | 字族是否仍是 Figma 原字族（未换成 Inter / system-ui / 主题包字体）？ | `010`/`011` | 主题化**只动颜色**，不动字号阶梯/字距/字族 |
| 2.4 | 字体文件是否在 `web/assets/fonts/`，CSS 不再热链 Google Fonts？ | `010` | Google / gstatic **必须**经 `http://localhost:8118`；同名字体可复用 `$HOME/.cache/fonts/` 或 `/usr/share/fonts`（复制进项目）；下载的原文件同时放入 `web/assets/fonts/` 与 `$HOME/.cache/fonts/{truetype\|opentype\|woff}/<Family>/` |
| 2.5 | 图上是否只有 `010` 一步、没有单独的 `011` 节点？ | sopwin / `extend` | `011` 用 `extend: previous` 并入 `010`；不要拆成独立 seq |

**常见差距：** 主题步骤把首屏换成库存色板；CDN 不通就换字族；字体只下到项目未进缓存导致重复失败。

---

## 3. Seed / Demo 数据一致性 🔴

| # | 检查项 | 对应步骤 | 期望 |
|---|--------|----------|------|
| 3.1 | 是否有 `sop/seed-coverage.md`？ | `040` | 表：mock 源 → 实体 → 样例 → seed 行 → **status** → **media↔text** |
| 3.2 | status 是否使用：`matched` / `seed-enriched` / `figma-suspect` / `schema-blocked` / `missing`？ | `040` | 非 `matched` 必须有 **差异记录（Discrepancy log）** |
| 3.3 | 图文是否仍配对（同一张图配原来的标题/文案）？ | `040`/`080` | 禁止 Figma 图配到另一套发明文案 |
| 3.4 | 集成前是否按 seed-coverage 门禁再删 mock？ | `080` | `missing` / 未解决 `schema-blocked` 不能直接删 mock 留空表 |
| 3.5 | Figma 数据可疑时是否标了 `figma-suspect`，而不是静默“修好”？ | `040` | seed 可以 enrich，但 Figma 行应保留或写明原因 |

**常见差距：** seed 整表重写成 Alice Demo；列表空了；海报与标题错配。

---

## 4. 根目录脚本约定 🟡

| # | 检查项 | 对应步骤 | 期望 |
|---|--------|----------|------|
| 4.1 | 根上是否有 `pnpm-workspace.yaml` + 根 `package.json`？ | `010` | workspace 至少含 `.`、`web`、`backend`、`prisma`、`web-e2e` |
| 4.2 | 是否具备：`dev`、`dev:web`、`dev:backend`、`db:generate`、`db:migrate`、`db:migrate:dev`、`db:seed`、`e2e`？ | `010` | 运维从**仓库根**执行 |
| 4.3 | 根上是否已去掉与 `db:generate` 重复的 `prisma:generate`？ | `010` | 根统一 `db:*` |
| 4.4 | `db:migrate` 与 `db:migrate:dev` 是否分成两个脚本？ | `010` | **deploy** vs **dev**，不是别名；不要再加只重复 deploy 的 `db:deploy` |
| 4.5 | `prisma/package.json` 短名是否与根转发一致？ | `060` | `generate`、`migrate`(=deploy)、`migrate:dev`、`seed`、`studio`… |
| 4.6 | 是否已从 git **去掉** `pnpm-lock.yaml`（及 `package-lock.json` / `yarn.lock`）？ | `010` | `.gitignore` 忽略；`git rm --cached`；WorldMan **不要** lock 文件入库 |

**对照：**

| 名称 | 含义 | 处理 |
|------|------|------|
| `prisma:generate` ≈ `db:generate` | 相同 | 只保留 `db:generate` |
| `db:migrate` | `prisma migrate deploy` | 保留 |
| `db:migrate:dev` | `prisma migrate dev` | 保留（不同） |
| `pnpm-lock.yaml` 等 | lock 文件 | 不入库；ignore + 取消跟踪 |

---

## 5. 后端提示产物路径 🟡

| # | 检查项 | 对应步骤 | 期望 |
|---|--------|----------|------|
| 5.1 | 是否存在 `sop/Backend implementation prompt.md`？ | `050` → `060` | chat 轨 `Save-As`；code 轨也要写同路径，供 `060` Checkout |
| 5.2 | code 轨是否另有 `docs/backend-implementation-plan.md`？ | `050z` | 人读详细计划 + 可执行 prompt 两份 |

---

## 6. 测试数据卫生（共享库污染） 🔴

| # | 检查项 | 对应步骤 | 期望 |
|---|--------|----------|------|
| 6.1 | 后端 API 测试插入共享库后，是否在 `afterEach`/`afterAll` 删除？ | `070` | 失败也要删；用前缀/标签避免误伤 seed |
| 6.2 | Playwright 创建的领域行是否有 teardown？ | `110`/`120` | 优先用 seed ID；创建则必须清 |
| 6.3 | 整套 E2E 跑完后是否扫过残留测试行？ | `140` | 绿了但库里堆着上百条 “Ava …” 等测试实体 = 未完成 |
| 6.4 | 最终审计是否把「测试污染」列进缺口？ | `150` | 发现则清理并加固拆卸 |

**整改建议：** 为测试实体统一前缀（如 `e2e-` / `test-`），提供 `scripts/cleanup-test-data.ts`（按标记删除，禁止对 demo 表整表 `TRUNCATE`）。

---

## 7. SOP 驱动提交（可选对齐） 🟢

| # | 检查项 | 期望 |
|---|--------|------|
| 7.1 | 若仍按 SOP 节奏改产品，提交是否带 `[sopNNN]`？ | 见 `CHECKPOINT.md`；历史已推送的不要 amend |
| 7.2 | 大步是否拆成多笔小提交？ | 避免一步一个大 dump |

---

## 建议整改顺序

1. **字体 + `default` 主题**（立刻改善「不像 Figma」）  
2. **共享库测试污染清理**（立刻改善数据可信度）  
3. **补 `design-inventory` / `seed-coverage` + 差异记录**（挡住后续静默丢页/丢数）  
4. **根 `package.json` `db:*`/`dev:*` 对齐；去掉 git 中的 lock 文件**  
5. **补 `Backend implementation prompt.md` / theme-default 等文档产物**

---

## 快速命令备忘

```bash
# 字体：Google 类 CDN 必须走代理
export http_proxy=http://localhost:8118 https_proxy=http://localhost:8118

# 缓存目录约定（与 /usr/share/fonts 类似）
#   $HOME/.cache/fonts/truetype/<Family>/…
#   $HOME/.cache/fonts/opentype/<Family>/…
#   $HOME/.cache/fonts/woff/<Family>/…

# 根脚本（达标后）
pnpm dev
pnpm db:generate
pnpm db:migrate          # deploy
pnpm db:migrate:dev      # 本地写迁移
pnpm db:seed
pnpm e2e
```

---

## 变更背景（给负责人）

相对早期 WorldMan 包，**1.2.x–1.5.0** 主要收紧了：

1. Figma 保真：inventory → 主题 `default` → seed 差异可追溯；文档标题 `族 \| 代码 \| 标题`（去掉 `Figma Make App`）  
2. 字体可复现：代理 + 同名复用 + `~/.cache/fonts`  
3. 运维入口：根 `db:*` / `dev:*`，去掉重复 `prisma:generate`；**不入库** `pnpm-lock.yaml` 等 lock 文件  
4. 测试卫生：创建即拆卸，`140` 跑完再清残留  
5. 流程版本：`VERSION` + `CHANGELOG.md`（`major.minor.patch`，改流程必同步更新）；项目钉 `sop/template-version`  

完整步骤正文仍以 `default/` / `zh_CN/` 为准；本文仅作**已完工项目的对照表**。明细见 [`CHANGELOG.md`](./CHANGELOG.md)。
