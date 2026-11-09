# SOP 驱动的提交（WorldMan）

在 WorldMan 步骤运行期间，**随工作演进**提交——在每个连贯的
进展单元之后——而不是等到步骤结束才提交一次。

每个完成的步骤只做一个提交**太粗**：单个 `[sop010]` 倾倒既改写清单、
又拆分页面、还本地化资源，无法解释是哪次编辑丢掉了某个页签。同一
步骤内应偏好若干较小提交。

## 主题前缀

每个 **SOP 驱动**的提交（针对活动步骤的代理/shell 工作）必须以
`[sopNNN]` 开头，其中 `NNN` 是当前步骤序号（`000`、`010`、`080`、
…）。例：活动步骤 id 为 `200` 时用 `[sop200]`。

```text
[sop010] freeze design inventory before refactor
[sop010] extract EventsBrowser tabs into features/events
[sop040] seed Episode rows from web mock catalog
[sop080] wire RolesBrowser list to GET /api/v1/roles
```

SOP 之外的用户/人工提交不必使用此前缀。不要事后给它们改标签。

## 规则

1. 步骤进行中，只要完成有意义的切片就**尽早、经常提交**（写出产物、
   抽出一条路由、替换一个 mock、子集测试转绿，…）。
2. 当树经历了多种不同变换时，**绝不**把整个步骤批成步骤末尾的一次
   提交。
3. **绝不 amend** 更早步骤（或你已推送的更早切片）的提交。用新的
   `[sopNNN]` 提交向前修复。
4. 除非用户明确要求，**绝不** `--no-verify` / `--no-gpg-sign`。
5. 正文说明*为什么* / 保真例外 / 清单更新——不只列文件名。
6. 出现密钥或巨大二进制时，偏好有意路径而非盲目 `git add -A`；不要
   提交 `.env`、凭证或仅本地垃圾。
7. 若工作树干净则跳过。仅写在 `sop/<seq>/` 下的 Chat 工作，在更新了
   受跟踪交付物（`sop/PRD.md`、`prisma/seed.ts`、`sop/design-inventory.md`
   等）时仍应提交。

## 命令模板

```bash
git add <paths-for-this-slice>
git status
git commit -m "$(cat <<'EOF'
[sopNNN] short summary of this slice

- notable decisions, fidelity exceptions, inventory updates
EOF
)"
```

当包含步骤 `011` 时，sopwin 通过 `extend: previous` 将其并入 `010`。
该合并运行中的提交使用 `[sop010]`（图上只有一个活动步骤 id）。

## 必需的伴生产物

| 产物 | 由谁写出 / 更新 | 用途 |
|------|-----------------|------|
| `sop/design-inventory.md` | `010`（重构前）；UI 结构有意变更时更新 | 基线路由、页签、chrome、字体、mock 来源 |
| `sop/figma-archive/` | `000` | 保留的 Figma 导出元数据（非产品源码） |
| `sop/theme-default.md` | `011`（被包含 / 并入 `010` 时） | Figma 颜色作为主题 `default` + 未改动字体栈 |
| `sop/seed-coverage.md` | `040`；由 `080` 复核 | 删除 mock 前的 mock/demo → seed 对等 + discrepancy log |
