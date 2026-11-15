# Rogue 5.4.5 JEC 056 — Windows（日本語）

この文書では、**Windows 10 / 11** 向けのビルド、起動、環境変数、セーブ・スコアの保存先について説明します。ゲームの紹介と各OS向け文書の一覧は[リポジトリのREADME](https://github.com/leopard-gecko/rogue_5.4.5_jec)、操作方法やアイテムの説明は[「【運命の洞窟】への案内書」](https://leopard-gecko.github.io/jrogue/guide.html?version=54)をご覧ください。

English version: [README_windows_en.md](README_windows_en.md)

## 1. 開発環境の準備

ソースからWindows版を作る場合は **MSYS2 UCRT64** を使います。<https://www.msys2.org/> からMSYS2をインストールし、スタートメニューの「MSYS2 UCRT64」を開いてください。

```sh
pacman -Syu
```

更新のため再起動が必要と表示されたらUCRT64を開き直して更新し、続いて開発ツールとPDCursesModをインストールします。

```sh
pacman -S --needed make mingw-w64-ucrt-x86_64-gcc mingw-w64-ucrt-x86_64-binutils mingw-w64-ucrt-x86_64-pdcurses
```

ソースのフォルダへ移動してください。

## 2. ビルド

```sh
make clean
make
```

成功すると **`rogue.exe`** が作成されます。

| 目的 | コマンド |
|---|---|
| 通常ビルド | `make` |
| 実行ファイル名を `jrogue.exe` にする | `make PROGRAM=jrogue`（または `make PROG=jrogue`） |
| 英語・オリジナル寄りの初期設定 | `make original` |
| 別名でオリジナル寄りの実行ファイルを作る | `make PROG=rogue_original original` |
| ウィザード機能を組み込む | `make wizard` |

通常の `make` は遊びやすい初期設定を採用し、`make original` は元のRogue 5.4に近い初期設定と英語表示で起動する実行ファイルを作ります。次の表にある `color`、`passgo` などはゲームのオプション名です。`on` は有効、`off` は無効を表します。これらの設定は、起動時に環境変数 **`ROGUEOPTS`** で変更することもできます。

| 設定 | 通常ビルド `make` | `make original` |
|---|---|---|
| 初期言語 | Windowsの表示言語から自動選択 | 英語 |
| `color`（カラー表示） | on | off |
| `passgo`（通路の角を自動で曲がる） | on | off |
| `idscrl`（識別の巻き物を1種類にする） | on | off |
| `cursor`（カーソル表示） | off | on |

「識別の巻き物」は、持ち物の正体を調べる道具です。オリジナルのRogue 5.4では調べる道具の種類に応じて5種類ありますが、`idscrl` を有効にすると、どの種類の道具にも使える1種類にまとめられます。この設定はゲーム開始後には変更できないため、変更したい場合は起動前に `ROGUEOPTS` で `idscrl`（1種類）または `noidscrl`（5種類）を指定します。

言語やその他の変更可能なオプションは、ゲーム中のオプション画面でも変更できます。詳しい説明は、「【運命の洞窟】への案内書」をご覧ください。ビルド方式を変更するときは `make clean` してからビルドしてください。

Windowsの描画方式は **PDCursesModのVTバックエンドに統一**しています。通常の `make` と `make original` で異なるのは初期オプションと言語設定だけです。従来のWinConバックエンドへの切り替え機能はありません。表示色や絵文字の見え方は、ターミナルとフォントにも依存します。

## 3. 起動と終了

**`rogue.exe`をダブルクリック**するとゲームが始まります。既定のセーブファイルがある場合は、起動時に `[R/Enter]` セーブした冒険を再開する、`[N]` 新しい冒険を始める、`[F]` 別のセーブファイルを選ぶ、`[Q]` 終了する、のいずれかを選べます。セーブファイルがなければ、選択画面を出さずに新規ゲームを開始します。セーブファイルを `rogue.exe` にドラッグ＆ドロップして、指定したファイルから直接再開することもできます。日本語や空白を含むファイルパスにも対応しています。PowerShell、コマンドプロンプト、MSYS2 UCRT64からも実行できます。

```powershell
.\rogue.exe
```

既存の端末からの通常起動は新規ゲームです。端末は最低 **80列×24行** 必要です。直接ダブルクリックで起動した場合は、Windows TerminalまたはConsole Hostのウィンドウも80×24に調整するよう試みます。既存のシェルから起動した場合は、その端末のサイズを変更しません。端末の種類や制限によっては調整できない場合があります。

PowerShellなどの端末からは、次のオプションも指定できます。

| コマンド | 内容 |
|---|---|
| `rogue.exe -r` | 既定のセーブファイルからゲームを再開する |
| `rogue.exe -s` | スコア一覧を表示する |
| `rogue.exe -d` | 墓標を表示し、スコアへの登録を試みる |

ゲーム中に `!` を入力すると、一時的にWindowsのコマンドプロンプトを開けます。シェルを終了するには `exit` を入力してください。終了するとRogueに戻ります。起動するシェルは環境変数 `COMSPEC` に従い、未設定の場合は `cmd.exe` を使います。

PowerShellでは、実行ファイル名の先頭に `.\` を付けます（例：`.\rogue.exe -s`）。詳しい説明は「【運命の洞窟】への案内書」をご覧ください。

## 4. Windowsでの環境変数の設定

Rogueの起動時設定は、Windowsの環境変数 **`ROGUEOPTS`** で指定できます。ここではWindowsでの登録方法だけを説明します。設定できる項目とその意味については、[「【運命の洞窟】への案内書」](https://leopard-gecko.github.io/jrogue/guide.html?version=54)の「オプション」をご覧ください。

**一時的に指定する方法**：現在開いている端末から起動する場合は、次のように指定します。`language=ja` は設定文字列の一例です。

PowerShell：

```powershell
$env:ROGUEOPTS = 'language=ja'
.\rogue.exe
```

コマンドプロンプト：

```cmd
set "ROGUEOPTS=language=ja"
rogue.exe
```

MSYS2 UCRT64：

```sh
export ROGUEOPTS='language=ja'
./rogue.exe
```

これらの指定は、その端末とそこから起動したプログラムにだけ適用されます。別の端末から起動する場合や、エクスプローラーからダブルクリックする場合には引き継がれません。

**Windowsに登録して継続して使う方法**：

1. `Win + R` キーで「ファイル名を指定して実行」を開き、`sysdm.cpl` と入力します。
2. 「詳細設定」タブから「環境変数」を開きます。
3. 「ユーザー環境変数」で「新規」を選び、変数名に `ROGUEOPTS`、変数値に設定文字列（例：`language=ja`）を入力します。すでに存在する場合は「編集」を使用します。
4. 「OK」で画面を閉じます。新しく開いた端末から使用し、反映されない場合はWindowsから一度サインアウトしてサインインし直してください。

不要になった場合はユーザー環境変数から `ROGUEOPTS` を削除します。

## 5. セーブ・スコアなどの保存先

既定ではセーブ **`rogue.save`** とスコア **`rogue.scr`** をゲームの作業ディレクトリに保存します。Windowsのバイナリ配布では、展開先フォルダで直接使う想定です。別のフォルダを使う場合はソースからの**ビルド時**に指定します。

スコア更新時は `rogue.scr` 自体をロックするため、専用のロックファイルは作成しません。旧版が作成した `rogue.lck` は、旧版のゲームが終了していることを確認してから手動で削除できます。

MSYS2 UCRT64で、既に作成した専用フォルダに保存する例：

```sh
mkdir -p rogue-data
make clean
make FILEDIR=rogue-data
```

絶対パスを使用する場合はWindows形式の `/` 区切りパスを指定できます。

```sh
make clean
make FILEDIR='C:/Games/Rogue545'
```

通常の `FILEDIR` 指定ではディレクトリは自動作成されません。**先に作成して書き込み権限を確保してください。** または `make prepare-filedir FILEDIR='C:/Games/Rogue545'` でも作成できます。

**Windowsの「保存したゲーム」を利用する場合（個人用ビルド）：**

```sh
make clean
make FILEDIR=saved-games
```

Windowsの「保存したゲーム」フォルダの下に `Rogue` フォルダを作り、そこへ保存します（必要なら自動作成）。**この指定ではビルドしたユーザーの絶対パスが実行ファイルへ埋め込まれます。別のPCへ配布するEXEには使用しないでください。**

ファイル名は `SAVEFILE=FILE`、`SCOREFILE=FILE` などのビルド変数で変更できます。個別指定は `FILEDIR` より優先されます。ほかのPCへの配布用は `FILEDIR` を指定せずにビルドするのが適切です。

**通常版とオリジナル設定版で保存ファイル名を共通にする場合（バイナリ配布用）：**

```sh
make clean
make PROGRAM=rogue SAVEFILE=rogue.save SCOREFILE=rogue.scr
make original PROGRAM=rogue_original SAVEFILE=rogue.save SCOREFILE=rogue.scr
```

これで `rogue.exe` と `rogue_original.exe` の両方が同じ `rogue.save` と `rogue.scr` を使用します。両方ともVT版になります。スコアの同時更新はロックで保護されますが、同じセーブファイルを複数のゲームで操作することは避けてください。

相対パスで保存先を指定した場合は、起動時の作業ディレクトリを基準とします。

## 6. 配布ファイルと説明書

MSYS2 UCRT64でも、ソース一式の配布用アーカイブを作成できます。

```bash
make dist.src
```

`rogue-5.4.5jec.056-src.tar.gz` が作成されます。

| ファイル | 用途 |
|---|---|
| `rogue.exe` | 通常ビルドの実行ファイル |
| `rogue_original.exe` | オリジナル寄りの初期設定で作成した実行ファイル（配布パッケージに含まれる場合） |
| `rogue.save` / `rogue.scr` | セーブ／スコア |
| `Makefile` / `Makefile.win` | ソースからのビルド |
| `windows/rogue.ico` / `windows/rogue-icon.rc` | Windows版のアイコンリソース |
| `windows/windows-saved-games.ps1` | 「保存したゲーム」保存先の取得・作成 |

案内書やほかのOS向けREADMEの一覧は、[リポジトリのREADME](https://github.com/leopard-gecko/rogue_5.4.5_jec)をご覧ください。

WindowsでEXEを他のPCにコピーする場合は、必要な実行時DLLの有無を確認してください。ゲームの遊び方は「【運命の洞窟】への案内書」を参照してください。

## 7. 著作権・ライセンス

オリジナルRogueと既存のコードに関する著作権・ライセンス条件、JEC版での日本語化・機能追加・Windows用アイコンなどの独自の貢献、およびRogue 5.4.5pのY.Oz Voxさんへの謝辞は、すべて`LICENSE.TXT`に記載しています。
