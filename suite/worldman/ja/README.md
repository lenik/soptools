# WorldMan プロジェクトワークフロー SOP

このアーカイブは **WorldMan** モジュール構築用のデフォルト SOP です（`backend/`、`web/`、任意の `mobile/`、および e2e テスト付き Web アプリ）。モジュールは通常 **Figma プロトタイプのエクスポート** から始まります。手順 `000` でそのエクスポートを標準レイアウトへ再構成することが必須です。各手順は独立して再利用可能なプロンプト／コマンドファイルです。

本パッケージは日本語版です（`suite/worldman/ja`）。英語版（正本）は `suite/worldman/default/` を参照してください。

## 命名規則

```text
<seq><variant>.<_padded_role>.<title>.(md|sh|get)
```

例：

```text
000a._shell.refactor_figma.sh
010a.__code.refactor_web.md
020a.__chat.create_prd.get
020z.__code.create_prd.md
050z.__code.prepare_backend_implementation.md
```

シーケンス（`seq`）：3 桁の順序キー。同一番号のファイルが分岐候補になります（例：`020`）。

バリアント：

- `a` — デフォルト／優先分岐
- `z` — 任意／代替分岐

ロール（先頭の `_` は `ls` 整列用の視覚パディングで、解析時は無視）：

- `shell` — shell／プロジェクト操作（実行可能な `*.sh` 手順を推奨）
- `chat` — Chat 仕様／成果物生成手順（拡張子 `.get`）
- `code` — コードリポジトリ実装手順

shell 手順は拡張子 `.sh`（コード主体、説明はコメント）。スクリプトは通常次で始まります：

```bash
. sop-script
```

`sopwin` はプロジェクトディレクトリを `cwd` として実行し、`PATH`（`<pkgdatadir>/extension/bash` を含む）、`SOP_PROJECT_DIR`、`SOP_DIR`、`SOP_STEP_ID`、`SOP_LOGLEVEL`、`LOGLEVEL`、`SOP_PROGRESS_FIFO` をエクスポートします。実行中のスクリプトは `set_progress 10%`（または `30.78%`）で**当該スクリプト自身**の進捗を報告します（SOP セッション全体の進捗ではありません）。sopwin は FIFO を読み、そのスクリプト用ゲージを更新します。コードプロンプト手順は Markdown（`.md`）のままです。

Markdown / `.get` 手順は任意の YAML frontmatter ブロックを持てます：

```text
---
extend: previous
---
```

- `extend: previous` / `extend: next` — 本手順を前後の**同型**手順（`role` + 拡張子、例: どちらも `code` + `.md`）に吸収します。型が一致しない場合は無視。グラフは結合後のアンカーノードのみ表示し、プロンプト本文は YAML ヘッダを除いて連結します。例: `011` が含まれる場合、`extend: previous` により `010` に結合され、単一の手順になります。

Chat 手順は拡張子 `.get` です。任意の RFC822 風ヘッダ（名前は大文字小文字を区別しない）、空行のあとにプロンプト本文。エディタ modeline など無視すべき行には `Discard:` を使います。

```text
Save-As: sop/PRD.md
File-Link: inode
Parse: multi-parts
Interaction: none
Append-To: sop/TUC.md from TUC*.md (delete)
Discard: -*- mode: markdown -*-
Discard: vim: set ft=markdown :

# プロンプトタイトル
...
```

- `Save-As` — 保存した応答をこのプロジェクト相対パスにも書き込み／リンクする（親ディレクトリは自動作成）。貼り付け応答が空のときはダウンロード済み添付を Save-As に使う（複数時は `file.ext` → `file1.ext`、`file2.ext`…）。
- `File-Link` — `default`/`auto`（ext2/3/4 では inode、それ以外はコピー）、`none`（コピー）、`inode`（ハードリンク）、`sym`（シンボリックリンク）
- `Parse` — カンマ区切りの形式。現在の `multi-parts` は複数パート応答を `sop/<seq>/` 下の `partNN` に分割する
- `Interaction` — `none`（デフォルト：保存のみ）または `select`（保存後にレンダ／パート選択ダイアログ）
- `Append-To` — `宛先 from パターン [(delete)]`；ドロップ／ステージされた添付がパターン（例 `TUC*.md`）に一致したら Accept で宛先へマージ；`(delete)` は元ファイルを削除
- 本文マークアップ — `<select title="…">` / `<option>…`（実行前に1つ選択）；`<optional title="…">…</optional>`（実行前に含めるかどうか）；`\response` は直近の先行 chat 応答に展開

- `Discard` — sopwin が無視（エディタ modeline、メモなど）
chat 手順実行時、sopwin は `sop/AGENTS` の `chat` エージェント（既定：`sopwin sopdlg --paste`）を呼び出します。この疑似エージェントはプロンプトをコピーし応答を取り込み、Accept で `Save-As` / `Append-To` を適用し、応答を `sop/<seq>/` に保存します（`seq` の先頭ゼロは除去）。本文はエージェント実行前に `\response` や `<select>` インタラクションを使えます。

## WorldMan アーキテクチャ

WorldMan モジュールは軽量マイクロサービスに近い振る舞いをします。

- 各アプリは**完全に独立**して動かすことも、対等アプリと**統合**することもできます。
- 独立モード：共有基礎データ（ユーザー、連絡先など）の**簡素なローカル定義**をアプリが持ちます。一般業務アプリは連絡先の氏名＋電話だけの小さな表で足りることが多いです。
- 統合モード：同じ論理エンティティは**不透明な参照**（cuid2 id）になり、対等アプリの API で解決します——他モジュールのスキーマへのデータベース外部キーではありません。専用の連絡先アプリはより完全なモデルを持てますが、利用側は必要な参照だけを保持します。
- マスタデータの主キーは **`cuid(2)`** とし、独立／統合のどちらでも id を安定に保ちます。

標準パッケージレイアウト：

```text
prisma/            # schema、migrations、seed（cuid2 ids）
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

- `status` — 現在のワークフロー状態。`location=` が活性ステップ id（自動更新；開くときに再開）
- `TODO.md` — タスク一覧。構築／再構成の進行に合わせて項目状態を同期する
- `PRD.md` — 製品要件（施工中の権威）
- `TUC.md` — テストユースケース参照集
- `NTC.md` — 反用例・ネガティブケース
- `ECS.md` — エッジケースシナリオ
- `design-inventory.md` — Figma/UI ベースライン（ルート、タブ、chrome、モック元）。`010` で**構造再構成の前に**書く。後続手順はエントリを黙って落としてはならない
- `seed-coverage.md` — モック／デモ → `prisma/seed.ts` の対応マトリクス。`040` で書き、`080` でゲートする
- `figma-archive/` — `000` が保全する軽量エクスポート足場（`.figma`、`src/imports`、エージェント文書）。**正本の Figma オリジナル**はこの製品ツリー外に置き、並行参照する。`000` は `.git` を削除し、エージェントが生エクスポートへ git-revert できないようにする。

ユーザーが複雑なプロンプトを渡したら、候補テストを速やかに抽出または構成して `TUC.md`、`NTC.md`、`ECS.md` に入れる。後続の Playwright e2e およびツール／サポートクラス抽出は、これらの `sop/` 文書を入力とする。

### デザイン忠実性

Figma エクスポートは**視覚およびナビのベースライン**であり、使い捨て足場ではない。

`000` 以降の全手順の厳格ルール：

1. **まずインベントリ** — 手順 `010` はファイル分割やルート移動の**前に** `sop/design-inventory.md` を書かなければならない。全ルート、全タブ／表形式サーフェス、主要 chrome、ダイアログ／シート、モック／デモデータ元とおおよその行／エンティティ数を列挙する。
2. **黙って落とさない** — 列挙済みのルート、タブ、表カラム集合、ダイアログを削除またはマージする場合は、明示的な理由とともにインベントリを更新する。「クリーンアップ」「簡略化」「まだ PRD にない」だけでは足りない。
3. **PRD は UI 構造に従う** — ナビ構造については、推測的な「エクスポートノイズ」フィルタリングより `sop/design-inventory.md` が優先する。死んだ足場コントロールは、インベントリ自身が `dead`/`scaffold` とマークした場合に限り降格してよい。
4. **モックは seed パリティまで残す** — デモデータを空の API 応答で置き換えない。手順 `040` は `web/` モックからの機械的抽出を優先し、手順 `080` はモック削除前に `sop/seed-coverage.md` を再確認しなければならない。
5. **テーミングは色のみ** — 手順 `011` は色をトークンに写す。ストックパレットに合わせてレイアウト、タイポグラフィ尺度、余白、ナビを再スタイリングしてはならない。

### `web/src` ディレクトリの役割

`web/src/` 下の目標レイアウト（Vite + React — **Next.js のファイルルーティングではない**）：

| ディレクトリ | 役割 |
|-----------|------|
| `pages/` | **ルート境界** — ルーターと route-smoke テスト向けの、ナビ可能な画面ごとのモジュール |
| `features/` | あるルートまたは製品機能向けのページ固有 UI・状態・ドメイン塊 |
| `components/` | ルート／機能横断で共有する再利用 UI |
| `hooks/`、`services/`、`lib/` | 共有の非視覚サポート |

薄い `pages/*` 再エクスポートは、実ページ本体が `features/`（または同等の機能モジュール）にある場合**のみ**許可する。完全なページ実装を `components/` に置き、`pages/` をその空の再エクスポートにしては**ならない**。

### SOP 駆動コミット

[`CHECKPOINT.md`](./CHECKPOINT.md) を参照。手順の**末尾に一括**ではなく、手順内で作業が進むにつれてコミットする。SOP 駆動の件名は `[sopNNN]` で始まる（活性ステップのシーケンス、例：`[sop010]`、`[sop200]`）。

デプロイ選択肢：直接プロセス実行、Apache/Nginx リバースプロキシ、またはコンテナ化（bridge または公開ポート）。

## ポート割当

モジュールの基準ポート `N` を次で計算します。

```bash
wm port -n <name>
```

（`wm port` は `naac -pwm <name>` を実行します。同等：`worldman port -n <name>`。）

`N` は `0..1999`（デフォルト `sha1(repo|directory|name) % 2000`）。`wm port NUM`、`-p`、または `worldman.json` の `"portBase"` で上書きできます。

規則：各 HTTP 待受ポート `P` に対する HTTPS は `P+1`。**例外**は bridge 上の web の古典的ペア `80`/`443`。

| サービス | Direct HTTP | Direct HTTPS | Apache / 公開 HTTP | Apache / 公開 HTTPS | Bridge（コンテナ内） |
|------------|-------------|--------------|-------------------------|--------------------------|-----------------------------|
| backend    | `2000+N`    | `2001+N`     | `6000+N`                | `6001+N`                 | `3000` / `3001`             |
| web        | `4000+N`    | `4001+N`     | `8000+N`                | `8001+N`                 | `80` / `443`                |
| mobile-web | `14000+N`   | `14001+N`    | `18000+N`               | `18001+N`                | `9000` / `9001`             |

コミットするソースに無関係なデフォルト待受（`5173` など）を残さないでください。

## 本 SOP の設計方針

- Figma プロトタイプから開始：手順 `000` で必ずエクスポートを `web/` へ再構成してから仕様・バックエンドへ進む。ソフトなエクスポート足場は `sop/figma-archive/` に置いてよいが、**正本の Figma オリジナルはこのツリー外**に置き、並行参照する。
- 手順 `000` は意図的に **`.git` を削除**し、エージェントが生の Figma エクスポートへ checkout / reset できないようにする。製品履歴は WorldMan レイアウトコミットから始まる。
- デザイン忠実性は便宜的なクリーンアップに勝る：インベントリ（`010`）→ PRD／schema／seed／統合での尊重 → 最終監査（`150`）で同一インベントリと照合。高価な視覚回帰／スクリーンショットゲートより、インベントリ＋ルート／タブ smoke を優先する（トークンコスト）。
- SOP 駆動 git コミット（`CHECKPOINT.md`）は各手順中に継続的に行い、件名は `[sopNNN]` で始める。進化を bisect 可能に保つ。
- 手順 `011`（テーミング）は任意コンテンツで `extend: previous` により付随する：含まれる場合、**sopwin はそれを `010` にマージしてグラフ上は1手順**とする。`extend` を外して別ノードを作らない——表示される **SOP 手順数は製品契約の一部**であり、パック洗練時に seq 番号を安易に増減してはならない。
- 同一 `seq` の Chat（`*.get`）と Code（`*.md`）トラックは**代替**：実行時に**どちらか一方**を選ぶ。会話コンテキストがある間は Chat を使い、別経路でプロジェクトが進み、先行手順を再生できず Chat コンテキストが消えたら Code に切り替える。同一ビルドで同じ `seq` の両トラックを走らせない。
- `schema.prisma` と `seed.ts` は分けて生成し、出力長圧力を避け、seed 生成でドメインモデルを圧迫検証できるようにする。
- Seed は新しいデモ行を発明するより、Figma／web デモデータの**抽出**を優先する。seed 生成は schema を黙って書き換えてはならない。
- Seed はバックエンドプロンプト（`050`）の後に走れる場合、任意で修正要求を出してよい：`prisma/request-for-refactor.md` および／または `sop/Backend amends.md`（`<optional>` 経由）。手順 `060` は両方をマージし、要求ファイルを削除する。
- モジュール横断参照はスカラー cuid2 フィールド＋ API 照会であり、明示要求がなければ横断 DB FK にしない。
- 施工中の作業文書は `sop/`（`PRD.md`、`TODO.md`、`TUC.md`、`NTC.md`、`ECS.md`、`design-inventory.md`、`seed-coverage.md`）に置き、構築／再構成中に更新し続ける。
- デモアセットのローカライズ：Google／Google Fonts／gstatic／Facebook CDN などは**必ず**ローカル HTTP プロキシ `http://localhost:8118` 経由にする。
- `web-e2e/workflows.md` は後半で生成し、`sop/` の PRD/TUC/NTC/ECS・schema・seed シナリオ・バックエンド設計を統合する。
- `docs/api.md` は実装／統合後に書き、as-built API 契約とする。
- Playwright カバレッジは route smoke → workflow → interactive-control 監査 → 横断成果物カバレッジ監査の順。route smoke は `design-inventory.md` の未ドロップルートすべてをカバーしなければならない。
