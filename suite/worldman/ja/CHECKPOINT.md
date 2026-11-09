# SOP 駆動コミット（WorldMan）

WorldMan 手順の実行中は、手順末尾に一度ではなく、**作業が進むにつれて**——
まとまった進捗単位のあとに——コミットする。

完成した手順ごとの1コミットは**粗すぎる**：インベントリ書き換え、ページ分割、
アセットのローカライズをまとめた単一の `[sop010]` ダンプでは、どの編集が
タブを落としたのか説明できない。同一手順内で複数の小さなコミットを好む。

## 件名前置

すべての **SOP 駆動**コミット（活性手順向けのエージェント／shell 作業）は
`[sopNNN]` で始めなければならない。`NNN` は現在の手順シーケンス（`000`、
`010`、`080`、…）。例：活性ステップ id が `200` のとき `[sop200]`。

```text
[sop010] freeze design inventory before refactor
[sop010] extract EventsBrowser tabs into features/events
[sop040] seed Episode rows from web mock catalog
[sop080] wire RolesBrowser list to GET /api/v1/roles
```

SOP 外のユーザー／人手コミットはこの前置を使う必要はない。それらを
付け替えない。

## ルール

1. 意味のあるスライスが終わるたび（成果物の書き出し、1ルート抽出、1モック置換、
   一部テストが green、…）、手順中に**早めに頻繁に**コミットする。
2. ツリーが複数の明確な変換を経たとき、手順全体を末尾の単一コミットに
   **まとめてはならない**。
3. 先行手順（または既にプッシュした先行スライス）のコミットを
   **amend してはならない**。新しい `[sopNNN]` コミットで前へ直す。
4. ユーザーが明示要求しない限り、`--no-verify` / `--no-gpg-sign` を
   **使ってはならない**。
5. 本文はファイル名だけでなく、*なぜ*／忠実性の例外／インベントリ更新を説明する。
6. シークレットや巨大バイナリが出たときは盲目的な `git add -A` より意図的なパスを好む。
   `.env`、資格情報、ローカル専用のゴミをコミットしない。
7. ツリーがクリーンならスキップ。`sop/<seq>/` 下だけを書く chat 作業でも、
   追跡される成果物（`sop/PRD.md`、`prisma/seed.ts`、`sop/design-inventory.md`、…）
   を更新したらコミットする。

## コマンドテンプレート

```bash
git add <paths-for-this-slice>
git status
git commit -m "$(cat <<'EOF'
[sopNNN] short summary of this slice

- notable decisions, fidelity exceptions, inventory updates
EOF
)"
```

手順 `011` が含まれる場合、sopwin は `extend: previous` によりそれを `010` にマージする。
そのマージ実行でのコミットは `[sop010]` を使う（グラフ上の活性ステップ id は1つ）。

## 必須の付随成果物

| 成果物 | 書込／更新担当 | 目的 |
|----------|----------------------|---------|
| `sop/design-inventory.md` | `010`（再構成前）；UI 構造を意図的に変えるときに更新 | ベースラインのルート、タブ、chrome、フォント、モック元 |
| `sop/figma-archive/` | `000` | 保全された Figma エクスポートメタデータ（製品ソースではない） |
| `sop/theme-default.md` | `011`（含まれる／`010` にマージされるとき） | Figma 色をテーマ `default` として＋未変更のフォントスタック |
| `sop/seed-coverage.md` | `040`；`080` が再確認 | モック削除前のモック／デモ → seed パリティ＋ discrepancy log |
