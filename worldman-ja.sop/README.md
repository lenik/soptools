# WorldMan プロジェクトワークフロー SOP（日本語）

このディレクトリは **WorldMan** モジュール構築用のデフォルト SOP です（`backend/`、`web/`、任意の `mobile/` と e2e）。モジュールは通常 **Figma プロトタイプのエクスポート** から始まります。手順 `000` でそのエクスポートを標準レイアウトへ再構成することが必須です。各手順は再利用可能なプロンプト／コマンドファイルです。

本パッケージは日本語版です。英語版は `worldman.sop/` を参照してください。

## 命名規則

```text
<seq><variant>.<_padded_role>.<title>.(md|sh|get)
```

例：

```text
000a._shell.refactor_figma.sh
010a._codex.refactor_web.md
020a.___gpt.create_prd.get
020z._codex.create_prd.md
050z._codex.prepare_backend_implementation.md
```

シーケンス（`seq`）：3 桁の順序キー。同一番号のファイルが分岐候補になります（例：`020`）。

バリアント：

- `a` — デフォルト／優先分岐
- `z` — 任意／代替分岐

ロール（先頭の `_` は `ls` 整列用で、解析時は無視）：

- `shell` — shell／プロジェクト操作
- `gpt` — GPT 仕様／成果物生成（拡張子 `.get`）
- `codex` — Codex リポジトリ実装

shell 手順は拡張子 `.sh`（コード主体、説明はコメント）。スクリプトは通常次で始まります：

```bash
. sop-script
```

`sopwin` はプロジェクトディレクトリを `cwd` として実行し、`PATH`（`<pkgdatadir>/extension/bash` を含む）、`SOP_PROJECT_DIR`、`SOP_DIR`、`SOP_STEP_ID`、`SOP_LOGLEVEL`、`LOGLEVEL`、`SOP_PROGRESS_FIFO` をエクスポートします。スクリプトは `set_progress 10%`（または `30.78%`）で**当該スクリプト自身**の進捗を報告します（SOP セッション全体の進捗ではありません）。sopwin は FIFO を読み、そのスクリプト用ゲージを更新します。Codex プロンプト手順は Markdown（`.md`）のままです。

GPT 手順は拡張子 `.get` です。任意の RFC822 風ヘッダ（名前は大文字小文字を区別しない）、空行のあとにプロンプト本文。エディタ modeline などは `Discard:` で書き、sopwin は無視します。

```text
Save-As: sop/PRD.md
File-Link: inode
Parse: multi-parts
Interaction: none
Discard: -*- mode: markdown -*-
Discard: vim: set ft=markdown :

# プロンプトタイトル
...
```

- `Save-As` — 保存した応答をこのプロジェクト相対パスにも書き込み／リンクする（親ディレクトリは自動作成）。貼り付け応答が空のときはダウンロード済み添付を Save-As に使う（複数時は `file.ext` → `file1.ext`、`file2.ext`…）。
- `File-Link` — `default`/`auto`（ext2/3/4 では inode、それ以外はコピー）、`none`（コピー）、`inode`（ハードリンク）、`sym`（シンボリックリンク）
- `Parse` — カンマ区切りの形式。現在の `multi-parts` は複数パート応答を `sop/<seq>/` 下の `partNN` に分割する
- `Interaction` — `none`（デフォルト：保存のみ）または `select`（保存後にレンダ／パート選択ダイアログ）
- `Discard` — sopwin が無視（エディタ modeline、メモなど）

GPT 手順実行時、sopwin はプロンプトをコピーしたあと Paste Response ダイアログを開き、モデル応答の貼り付けと添付ダウンロードを受け付けます。応答は `sop/<seq>/` に保存します（`seq` の先頭ゼロは除去）。


## WorldMan アーキテクチャ

WorldMan モジュールは軽量マイクロサービスに近い振る舞いをします。

- 各アプリは**完全に独立**して動かすことも、対等アプリと**統合**することもできます。
- 独立モード：共有基礎データ（ユーザー、連絡先など）の**簡素なローカル定義**をアプリが持ちます。一般業務アプリは連絡先の氏名＋電話だけの小さな表で足りることが多いです。
- 統合モード：同じ論理エンティティは**不透明な参照**（cuid2 id）になり、対等アプリの API で解決します——他モジュール DB への外部キーではありません。専用の連絡先アプリはより完全なモデルを持てますが、利用側は必要な参照だけを保持します。
- マスタデータの主キーは **`cuid(2)`** とし、独立／統合のどちらでも id を安定に保ちます。

標準レイアウト：

```text
prisma/            # schema、migrations、seed（cuid2）
backend/           # Fastify + TypeScript、/api/v1
web/               # Vite + React
web-e2e/           # Playwright
mobile/            # 任意 Expo / mobile-web
mobile-e2e/        # 任意
sop/               # 構築／再構成中の作業文書：PRD、TODO、TUC/NTC/ECS
docs/              # as-built 文書（api.md など）
docker/            # zephyr-docker ベースのカスタムイメージ
i-local/           # 組み込み単ノード docker ローカルインスタンス＋フレーバー短縮コマンド
i-medium/          # 組み込み中小規模 docker compose クラスタ＋フレーバー短縮コマンド
worldman.json      # name、portBase、masterData、references、…
```

### プロジェクト `sop/` ワークスペース

複雑な構築／再構成作業では、`<projectdir>/sop/` 配下に生きた作業文書を置く：

- `TODO.md` — タスク一覧。構築／再構成の進行に合わせて項目状態を同期する
- `PRD.md` — 製品要件（施工中の権威）
- `TUC.md` — テストユースケース参照集（Test Use-Case）
- `NTC.md` — 反用例・ネガティブケース（Negative Test Cases）
- `ECS.md` — エッジケースシナリオ（Edge-Case Scenarios）

ユーザーが複雑なプロンプトを渡したら、候補テストを速やかに抽出または構成して `TUC.md`、`NTC.md`、`ECS.md` に入れる。後続の Playwright e2e およびツール／サポートクラス抽出は、これらの `sop/` 文書を入力とする。

デプロイ：直接プロセス、Apache/Nginx リバースプロキシ、またはコンテナ（bridge／公開ポート）。

## ポート割当

モジュールの基準ポート `N` を次で計算します。

```bash
wm port -n <name>
```

（`wm port` は実際には `naac -pwm <name>` を実行します。同等：`worldman port -n <name>`。）

`N` は `0..1999`（デフォルト `sha1(repo|ディレクトリ|name) % 2000`）。`wm port NUM`、`-p`、または `worldman.json` の `"portBase"` で上書きできます。

規則：各 HTTP 待受ポート `P` に対する HTTPS は `P+1`。**例外**は bridge 上の web の古典的ペア `80`/`443`。

| サービス | Direct HTTP | Direct HTTPS | Apache/公開 HTTP | Apache/公開 HTTPS | Bridge（コンテナ内） |
|----------|-------------|--------------|------------------|-------------------|----------------------|
| backend | `2000+N` | `2001+N` | `6000+N` | `6001+N` | `3000` / `3001` |
| web | `4000+N` | `4001+N` | `8000+N` | `8001+N` | `80` / `443` |
| mobile-web | `14000+N` | `14001+N` | `18000+N` | `18001+N` | `9000` / `9001` |

コミットするソースに無関係なデフォルト待受（`5173` など）を残さないでください。

## 本 SOP の設計方針

- Figma プロトタイプから開始：手順 `000` で必ずエクスポートを `web/` へ再構成してから仕様・バックエンドへ進む。
- GPT 仕様トラックと Codex 実装トラックは論理的に独立（`a` / `z`）。
- `schema.prisma` と `seed.ts` は分けて生成し、出力長を抑えつつ seed でドメインを検証する。
- seed は schema を黙って書き換えない。
- seed が schema 問題を露出したら `prisma/request-for-refactor.md` を追加してよい（`blocking` または `recommended`）。
- モジュール横断参照はスカラー cuid2 ＋ API 照会であり、明示要求がなければ横断 FK にしない。
- 施工中の作業文書は `sop/`（`PRD.md`、`TODO.md`、`TUC.md`、`NTC.md`、`ECS.md`）に置き、構築／再構成中に更新し続ける。
- `web-e2e/workflows.md` は後半で生成し、`sop/` の PRD/TUC/NTC/ECS・schema・seed シナリオ・バックエンド設計を統合する。
- `docs/api.md` は実装／統合後に書き、as-built API 契約とする。
- Playwright は route smoke → workflow → interactive control → 横断カバレッジ監査の順。
