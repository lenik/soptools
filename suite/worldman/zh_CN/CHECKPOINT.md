# SOP 驱动的提交（WorldMan）

WorldMan 步骤运行期间，**随工作演进**提交——每完成一块连贯进展就提交，而不是等步骤结束再一次性提交。

一步一提交**太粗**：一个 `[sop010]` 大提交若同时改写清单、拆页面、本地化资源，就无法说明是哪次编辑丢掉了某个页签。同一步骤内宜拆成多次较小提交。

## 主题前缀

每条 **SOP 驱动**的提交（当前步骤的代理/shell 工作）必须以 `[sopNNN]` 开头，其中 `NNN` 是当前步骤序号（`000`、`010`、`080`…）。例：活动步骤 id 为 `200` 时用 `[sop200]`。

```text
[sop010] freeze design inventory before refactor
[sop010] extract EventsBrowser tabs into features/events
[sop040] seed Episode rows from web mock catalog
[sop080] wire RolesBrowser list to GET /api/v1/roles
```

SOP 之外的用户/人工提交不必加此前缀，也不要事后给它们补标。

## 规则

1. 步骤进行中，只要完成有意义的切片就**尽早、多次**提交（产物已写出、某条路由已抽出、某个 mock 已替换、某子集测试已绿等）。
2. 若工作树经历了多种不同变换，**绝不**把整步工作塞进步骤末尾的一次提交。
3. **绝不 amend** 更早步骤（或你已推送过的更早切片）的提交。向前修，用新的 `[sopNNN]` 提交。
4. 除非用户明确要求，**绝不** `--no-verify` / `--no-gpg-sign`。
5. 正文说明*为何* / 保真例外 / 清单更新——不要只列文件名。
6. 出现密钥或巨大二进制时，优先有意选择路径，避免盲目 `git add -A`；不要提交 `.env`、凭证或仅本地杂物。
7. 工作树干净则跳过。仅写在 `sop/<seq>/` 下的 Chat 工作，若更新了受跟踪交付物（`sop/PRD.md`、`prisma/seed.ts`、`sop/design-inventory.md` 等），仍应提交。

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

纳入步骤 `011` 时，sopwin 经 `extend: previous` 将其并入 `010`。合并运行时的提交使用 `[sop010]`（图上只有一个活动步骤 id）。

## 必伴随产物

| 产物 | 写出 / 更新方 | 用途 |
|------|---------------|------|
| `sop/template-version` | `010` | 钉住 `suite=worldman` + 流程包 `version=`，便于对照 CHANGELOG / 改造清单 |
| `sop/design-inventory.md` | `010`（重构前）；UI 结构有意变更时更新 | 基线路由、页签、界面外壳、字体、mock 来源 |
| `sop/figma-archive/` | `000` | 保留的 Figma 导出元数据（不是产品源码） |
| `sop/theme-default.md` | `011`（纳入 / 并入 `010` 时） | Figma 颜色作为主题 `default` + 未改动的字体栈 |
| `sop/seed-coverage.md` | `040`；`080` 复核 | 删除 mock 前的 mock/demo → seed 对等 + 差异记录 |
