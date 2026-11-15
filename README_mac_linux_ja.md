# Rogue 5.4.5 JEC 056 — macOS / Linux（日本語）

この文書では、**macOS、Linux、Raspberry Pi OS** でのビルド、起動、インストール、セーブ・スコアの保存先について説明します。ゲームの紹介と各OS向け文書の一覧は[リポジトリのREADME](https://github.com/leopard-gecko/rogue_5.4.5_jec)、操作方法やアイテムの説明は[「【運命の洞窟】への案内書」](https://leopard-gecko.github.io/jrogue/guide.html?version=54)をご覧ください。

English version: [README_mac_linux_en.md](README_mac_linux_en.md)

## 1. 開発環境の準備

ソースコードを展開し、ターミナルでそのディレクトリを開きます。

**macOS：** Appleのコマンドラインツールをインストールします。

```sh
xcode-select --install
```

**Ubuntu / Debian / Raspberry Pi OS：**

```sh
sudo apt update
sudo apt install build-essential pkg-config libncurses-dev
```

**Fedora：** `sudo dnf install gcc make pkgconf-pkg-config ncurses-devel`  
**Arch Linux：** `sudo pacman -S --needed base-devel pkgconf ncurses`  
**Alpine Linux：** `sudo apk add build-base pkgconf ncurses-dev`

Linuxでは、ビルド前に次のように実行して使用環境を設定することを推奨します。

```sh
./configure
```

macOSは一般に `make` のみでもビルドできます。curses関連で問題があれば `./configure` の `--with-curses-cflags` や `--with-curses-libs` を利用して調整してください。

## 2. ビルド

通常ビルドは次のとおりです。

```sh
make clean
make
```

成功すると **`rogue`** が生成されます。通常の `make` は遊びやすい初期設定を採用し、`make original` は元のRogue 5.4に近い初期設定と英語表示で起動する実行ファイルを作ります。ゲーム中の言語切り替え機能はどちらにも残ります。

次の表にある `color`、`passgo` などはゲームのオプション名です。`on` は有効、`off` は無効を表します。これらの設定は、起動時に環境変数 **`ROGUEOPTS`** で変更することもできます。

| 設定 | 通常ビルド `make` | `make original` |
|---|---|---|
| 初期言語 | OSの設定から自動選択 | 英語 |
| `color`（カラー表示） | on | off |
| `passgo`（通路の角を自動で曲がる） | on | off |
| `idscrl`（識別の巻き物を1種類にする） | on | off |
| `cursor`（カーソル表示） | off | on |

「識別の巻き物」は、持ち物の正体を調べる道具です。オリジナルのRogue 5.4では調べる道具の種類に応じて5種類ありますが、`idscrl` を有効にすると、どの種類の道具にも使える1種類にまとめられます。この設定はゲーム開始後には変更できないため、変更したい場合は起動前に `ROGUEOPTS` で `idscrl`（1種類）または `noidscrl`（5種類）を指定します。

言語やその他の変更可能なオプションは、ゲーム中のオプション画面でも変更できます。詳しい説明は、「【運命の洞窟】への案内書」をご覧ください。

| 目的 | コマンド |
|---|---|
| 通常ビルド | `make` |
| オリジナル寄りの初期設定・英語 | `make original` |
| ウィザード機能を組み込む | `make wizard` |
| 実行ファイル名を `jrogue` にする | `./configure --program=jrogue` → `make clean` → `make` |

ビルド方式や実行ファイル名を変更する場合は、一度 `make clean` してから作り直してください。`configure` を使った場合は設定が `config.mk` に保存されます。**`make clean` と `make distclean` は、セーブ・スコアファイルを削除しません。** `make clean` はビルド生成物を、`make distclean` はそれらに加えて `config.mk`・`config.log` を削除します。

## 3. 起動

```sh
./rogue
```

通常起動では新しいゲームが始まります。ターミナルは **80列×24行以上** を用意してください。

起動時に次のオプションも指定できます。

| コマンド | 内容 |
|---|---|
| `./rogue -r` | 既定のセーブファイルからゲームを再開する |
| `./rogue -s` | スコア一覧を表示する |
| `./rogue -d` | 墓標を表示し、スコアへの登録を試みる |

詳しい説明は「【運命の洞窟】への案内書」をご覧ください。

## 4. インストール（任意）

ビルドしたフォルダで遊ぶ場合はインストール不要です。通常は次のコマンドで `/usr/local` 以下にインストールします。

```sh
sudo make install
```

インストール先を変更する場合は、事前に `./configure --prefix=DIR` を指定できます。実行ファイル名は `--program=NAME` で指定できます。英語・日本語のmanページもインストールされ、`--ja-man-style=auto|mac|linux|all|none` で日本語manページのインストール先を選べます。`make uninstall` でアンインストールできます。

## 5. セーブ・スコアなどの保存先

指定しなければ、セーブ **`rogue.save`** とスコア **`rogue.scr`** は、ゲーム起動時の作業ディレクトリに置かれます。実行ファイル名を変更すると、通常はこれらの既定ファイル名も連動して変わります。

スコア更新時は `rogue.scr` 自体をロックするため、専用のロックファイルは作成しません。旧版が作成した `rogue.lck` は、旧版のゲームが終了していることを確認してから手動で削除できます。

**2種類のファイルを1か所にまとめる例：**

```sh
mkdir -p "$HOME/.local/share/rogue545"
./configure --file-dir="$HOME/.local/share/rogue545"
make clean
make
sudo make install
```

保存先はゲームを実行するユーザーが書き込める必要があるため、この例では先に `mkdir -p` で作成しています。`--file-dir` で指定したディレクトリは `make install` の際に必要に応じて作成されますが、`sudo` で新規作成すると書き込み権限に注意が必要です。なお、`make clean` は既存のセーブ・スコアファイルを削除しません。

個別の保存ファイル名は `./configure --save-file=FILE`、`--score-file=FILE` で指定できます。`configure` を使わず、`make PROGRAM=NAME FILEDIR=DIR`（または `PROG=NAME`）などで指定する方法もあります。個別指定は `FILEDIR` より優先されます。設定変更後は再ビルドしてください。

## 6. ファイルの説明と配布

| ファイル | 用途 |
|---|---|
| `rogue` | ビルド後の実行ファイル |
| `rogue.save` / `rogue.scr` | セーブ／スコア |
| `configure` / `Makefile` | ビルドとインストール |
| `config.mk` / `config.log` | `configure`が生成する設定・確認ログ |
| `rogue.6.en.in` / `rogue.6.ja.in` | 日英のmanページの原稿 |
| `*.c` / `*.h` | ソースコード |

案内書やほかのOS向けREADMEの一覧は、[リポジトリのREADME](https://github.com/leopard-gecko/rogue_5.4.5_jec)をご覧ください。

`make dist.src` でソースアーカイブを生成できます（通常は `rogue-5.4.5jec.056-src.tar.gz`）。

## 7. 著作権・ライセンス

オリジナルRogueと既存のコードに関する著作権・ライセンス条件、JEC版での日本語化・機能追加・Windows用アイコンなどの独自の貢献、およびRogue 5.4.5pのY.Oz Voxさんへの謝辞は、すべて`LICENSE.TXT`に記載しています。
