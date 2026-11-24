# Rogue 5.4.5 JEC

**Rogue 5.4.5 JEC 057**

Rogueは、1980年頃に誕生した、文字や記号で描かれた迷宮を探索するコンピューターRPGです。プレイヤーは怪物と戦い、さまざまな道具を使いながら、迷宮の奥に隠された「イェンダーの魔除け」を探します。１文字で表現されたキャラクターやアイテムが特徴で、いま遊んでも楽しめるものだと思います。いわゆる「ローグライクゲーム」の元祖になるものです。

**Rogue 5.4**は、1985年にリリースされたRogueの最終バージョンです。

ここで配布する**Rogue 5.4.5 JEC** は、Rogue 5.4.5のソースコードに日本語・英語の切り替え、カラー表示、現代のOSへの対応などを加えた修正版です。ゲームの内容はRogue 5.4そのものです。

*Rogue is a computer role-playing game that originated around 1980. Players explore dungeons drawn with characters and symbols, fighting monsters and using various items in search of the Amulet of Yendor hidden deep within the dungeon. Its characters and items, each represented by a single character, are a distinctive feature, and I think the game is still enjoyable today. It is the original game behind what are now called “roguelikes.”*

***Rogue 5.4*** *is the final version of Rogue, released in 1985.*

*The **Rogue 5.4.5 JEC** distributed here is a modified version of the Rogue 5.4.5 source code, adding Japanese/English language switching, color display, support for modern operating systems, and other improvements. The gameplay itself is that of Rogue 5.4.*

## 対応環境とREADME / Platforms and documentation

macOS、Linux（Raspberry Pi OSを含む）、Windows 10 / 11に対応しています。ビルド方法、起動方法、オプションやセーブファイルの扱いについては、OS・言語別のREADMEをご覧ください。

*For build instructions, launching, options, and save-file locations, choose the README for your operating system and language.*

| 対象OS / Platform | 日本語 | English |
| --- | --- | --- |
| macOS / Linux / Raspberry Pi OS | [README（日本語）](README_mac_linux_ja.md) | [README (English)](README_mac_linux_en.md) |
| Windows 10 / 11 | [README（日本語）](README_windows_ja.md) | [README (English)](README_windows_en.md) |

macOS / Linuxでは通常 `make`、WindowsではMSYS2 UCRT64環境からビルドできます。必要な準備や詳しい手順は各READMEを参照してください。

*On macOS/Linux, the standard build uses `make`. Windows builds use MSYS2 UCRT64; see the platform-specific README for prerequisites and details.*

## 【運命の洞窟】への案内書 / A Guide to the Dungeons of Doom

ゲームの操作、画面表示、アイテム、オプションなどは、日英の案内書にまとめています。HTML版と固定幅のテキスト版を同梱しています。

*The Japanese and English dungeon guides describe the game, commands, items, and options. Both HTML and plain-text editions are included.*

| 言語 / Language | HTML | TXT |
| --- | --- | --- |
| 日本語 | [rogue_ja.html](rogue_ja.html) | [rogue_ja.txt](rogue_ja.txt) |
| English | [rogue_en.html](rogue_en.html) | [rogue_en.txt](rogue_en.txt) |

ブラウザで読む場合は、[公開サイトの案内書](https://leopard-gecko.github.io/jrogue/guide.html?version=54)も利用できます。

*The [online dungeon guide](https://leopard-gecko.github.io/jrogue/guide.html?version=54) is also available.*

## 著作権・ライセンス / Copyright and license

オリジナルRogueおよび既存のコードの著作権・ライセンス、JEC版での変更と謝辞については、[LICENSE.TXT](LICENSE.TXT) をご覧ください。

*See [LICENSE.TXT](LICENSE.TXT) for copyright notices, license terms, JEC modifications, and acknowledgments.*
